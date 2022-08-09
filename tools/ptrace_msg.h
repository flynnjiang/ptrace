#ifndef __PTRACE_MSG_H__
#define __PTRACE_MSG_H__

#include <stdint.h>
#include <netinet/in.h>

enum ptrace_msg_type_e {
    PTRACE_MSG_TYPE_NTF = 0,    /* notification */
    PTRACE_MSG_TYPE_REQ,        /* reqeuest */
    PTRACE_MSG_TYPE_RSP,        /* response */
};

struct ptrace_msg {
    struct sockaddr_in dest;
    struct sockaddr_in src;
    uint8_t type;           /* type */
    uint8_t sid;            /* session id */
    uint8_t id;             /* message id */
    uint32_t data_len;
    char data[0];
};

#define PTRACE_MSG_DATA_PTR(msg, type)  ((type *)(msg->data))
#define PTRACE_MSG_DATA(msg, type)      (*PTRACE_MSG_DATA_PTR(msg, type))

extern struct ptrace_msg *ptrace_msg_alloc(uint32_t data_len);
extern struct ptrace_msg *ptrace_msg_dup(const struct ptrace_msg *msg);
extern void ptrace_msg_free(struct ptrace_msg *msg);
extern uint32_t ptrace_msg_size(const struct ptrace_msg *msg);

extern int ptrace_msg_send(struct ptrace_msg *msg);
extern struct ptrace_msg *ptrace_msg_recv(uint32_t timeout_ms);

extern int ptrace_msg_notify(struct sockaddr_in *dest,
        uint8_t msg_id, void *data, int data_len);

extern int ptrace_msg_request(struct sockaddr_in *dest,
        int msg_id, void *data, int data_len,
        struct ptrace_msg **rsp_msg, uint32_t timeout_ms);
extern int ptrace_msg_reqeust2(struct ptrace_msg *req,
        struct ptrace_msg **rsp_msg, uint32_t timeout_ms);

extern int ptrace_msg_response(struct ptrace_msg *req,
        void *data, int data_len);

extern int ptrace_msg_init(struct sockaddr_in *local_addr);
extern void ptrace_msg_destroy(void);

#endif /* __PTRACE_MSG_H__ */

