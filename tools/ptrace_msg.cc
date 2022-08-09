#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "ptrace_msg.h"

static uint32_t g_session_id;
static int g_sockfd = -1;
static char g_recv_buff[8192];

uint32_t ptrace_msg_size(const struct ptrace_msg *msg)
{
    return msg ? (sizeof(*msg) + msg->data_len) : 0;
}

struct ptrace_msg *ptrace_msg_alloc(uint32_t data_len)
{
    struct ptrace_msg *msg;

    msg = (struct ptrace_msg *)malloc(sizeof(struct ptrace_msg) + data_len);
    if (!msg) {
        fprintf(stderr, "alloc msg failed, data_len = %d\n", data_len);
        return NULL;
    }

    msg->data_len = data_len;

    return msg;
}

struct ptrace_msg *ptrace_msg_dup(const struct ptrace_msg *msg)
{
    struct ptrace_msg *dup = ptrace_msg_alloc(msg->data_len);

    if (dup)
        memcpy(dup, msg, ptrace_msg_size(msg));

    return dup;
}

void ptrace_msg_free(struct ptrace_msg *msg)
{
    if (msg) {
        free(msg);
    }
}

int ptrace_msg_send(struct ptrace_msg *msg)
{
    ssize_t sz;

#if 0
    fprintf(stderr, "Msg[%d] > %s:%d\n", msg->id,
            inet_ntoa(msg->dest.sin_addr), ntohs(msg->dest.sin_port));
#endif

    sz = sendto(g_sockfd, msg, ptrace_msg_size(msg), 0,
               (struct sockaddr *)&msg->dest, sizeof(msg->dest));
    if (sz != ptrace_msg_size(msg)) {
        fprintf(stderr, "send msg failed, errno = %d\n", errno);
        return -1;
    }

    return 0;
}

struct ptrace_msg *ptrace_msg_recv(uint32_t timeout_ms)
{
    int rc;
    socklen_t addrlen;
    fd_set rfds;
    struct timeval tv;
    struct ptrace_msg *msg = (struct ptrace_msg *)g_recv_buff;

    FD_ZERO(&rfds);
    FD_SET(g_sockfd, &rfds);

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    rc = select(g_sockfd + 1, &rfds, NULL, NULL, &tv);
    if (rc <= 0) {
        if (rc < 0)
            fprintf(stderr, "select faield, rc = %d, errno = %d\n", rc, errno);
        return NULL;
    }

    addrlen = sizeof(msg->src);
    rc = recvfrom(g_sockfd, g_recv_buff, sizeof(g_recv_buff), 0,
                 (struct sockaddr *)&msg->src, &addrlen);
    if (rc <= 0 || (uint32_t)rc < ptrace_msg_size(msg)) {
        fprintf(stderr, "recv msg faield, rc = %d, errno = %d\n", rc, errno);
        return NULL;
    }

#if 0
    fprintf(stderr, "Msg[%d] < %s:%d\n", msg->id,
            inet_ntoa(msg->dest.sin_addr), ntohs(msg->dest.sin_port));
#endif

    return ptrace_msg_dup(msg);
}

int ptrace_msg_notify(struct sockaddr_in *dest,
        uint8_t msg_id, void *data, int data_len)
{
    int rc = -1;
    struct ptrace_msg *msg;

    msg = ptrace_msg_alloc(data_len);
    if (msg) {
        msg->id = msg_id;
        msg->type = PTRACE_MSG_TYPE_NTF;
        memcpy(&msg->dest, dest, sizeof(msg->dest));
        if (data)
            memcpy(msg->data, data, data_len);

        rc = ptrace_msg_send(msg);

        ptrace_msg_free(msg);
    }

    return rc;
}

int ptrace_msg_request(struct sockaddr_in *dest,
        int msg_id, void *data, int data_len,
        struct ptrace_msg **rsp_msg, uint32_t timeout_ms)
{
    int rc = -1;
    struct ptrace_msg *req, *rsp;

    req = ptrace_msg_alloc(data_len);
    if (!req)
        return -1;

    req->id = msg_id;
    req->type = PTRACE_MSG_TYPE_REQ;
    req->sid = g_session_id++;
    memcpy(&req->dest, dest, sizeof(req->dest));
    if (data)
        memcpy(req->data, data, data_len);

    ptrace_msg_send(req);

    rsp = ptrace_msg_recv(timeout_ms);
    if (rsp && PTRACE_MSG_TYPE_RSP == rsp->type && req->sid == rsp->sid) {
        rc = 0;
        if (rsp_msg) {
            *rsp_msg = rsp;
            rsp = NULL;
        }
    }

    ptrace_msg_free(req);
    if (rsp)
        ptrace_msg_free(rsp);

    return rc;
}

int ptrace_msg_reqeust2(struct ptrace_msg *req,
                        struct ptrace_msg **rsp_msg, uint32_t timeout_ms)
{
    int rc = -1;
    struct ptrace_msg *rsp;
    
    req->type = PTRACE_MSG_TYPE_REQ;
    req->sid = g_session_id++;
    ptrace_msg_send(req);

    rsp = ptrace_msg_recv(timeout_ms);
    if (rsp && PTRACE_MSG_TYPE_RSP == rsp->type && req->sid == rsp->sid) {
        rc = 0;
        if (rsp_msg) {
            *rsp_msg = rsp;
            rsp = NULL;
        }
    }

    if (rsp)
        ptrace_msg_free(rsp);

    return rc;
}

int ptrace_msg_response(struct ptrace_msg *req, void *data, int data_len)
{
    int rc = -1;
    struct ptrace_msg *msg;

    msg = ptrace_msg_alloc(data_len);
    if (msg) {
        msg->type = PTRACE_MSG_TYPE_RSP;
        msg->id = req->id;
        msg->sid = req->sid;
        memcpy(&msg->dest, &req->src, sizeof(msg->dest));
        if (data)
            memcpy(msg->data, data, data_len);

        rc = ptrace_msg_send(msg);

        ptrace_msg_free(msg);
    }

    return rc;
}

int ptrace_msg_init(struct sockaddr_in *local_addr)
{
    int rc;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "create socket failed, errno = %d\n", errno);
        return -1;
    }

    if (local_addr) {
        rc = bind(sockfd, (struct sockaddr *)local_addr, sizeof(*local_addr));
        if (rc < 0) {
            fprintf(stderr, "bind socket failed, errno = %d\n", errno);
            close(sockfd);
            return -1;
        }
    }

    g_sockfd = sockfd;

    return 0;
}

void ptrace_msg_destroy(void)
{
    if (g_sockfd > 0) {
        close(g_sockfd);
        g_sockfd = -1;
    }
}
