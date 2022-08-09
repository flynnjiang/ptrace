#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "perfetto.h"
#include "ptrace_msg.h"

enum ptrace_msg_id_e {
    PTRACE_MSG_ID_CONNECT = 0,
    PTRACE_MSG_ID_DISCONNECT,
    PTRACE_MSG_ID_ECHO,
    PTRACE_MSG_ID_TIME,
    PTRACE_MSG_ID_START_TRACING,
    PTRACE_MSG_ID_STOP_TRACING,
    PTRACE_MSG_ID_FILE_BEGIN,
    PTRACE_MSG_ID_FILE_CONTENT,
    PTRACE_MSG_ID_FILE_END,
};

struct ptrace_msg_time_data {
    uint64_t boot_time;     /* nanosecond */
    uint64_t trans_delay;   /* nanosecond */
};

enum ptrace_working_mode_e {
    PTRACE_WORKING_MODE_ALONE = 0,
    PTRACE_WORKING_MODE_CLIENT,
    PTRACE_WORKING_MODE_SERVER,
};

#define PTRACE_FAILURE  0
#define PTRACE_SUCCESS  1

static const char *g_working_mode_name[] = {
    "alone",
    "client",
    "server",
};

struct ptrace_config {
    struct sockaddr_in addr;    /* server address */
    uint8_t work_mode;          /* working mode: PTRACE_WORKING_MODE_XXX */
    uint8_t no_wait;            /* don't wait client, tracking immediately */
    char cfg_file[128];         /* config file */
    char out_file[128];         /* output tracking file */
    char clnt_file[128];        /* client's tracking file */
    char comb_file[128];        /* combined tracking file */
};

static struct ptrace_config g_cfg;

static const char *g_program_name;

static int g_running;

static pid_t g_tracing_pid;

static char g_send_buff[8192];

extern int combine_file(const char *f1, const char *f2,
                        const char *fc, int64_t time_diff_ns);


static int simple_request(struct sockaddr_in *dest, uint8_t msg_id,
                          void *data, int data_len, uint32_t timeout_ms)
{
    int32_t success = 0;
    struct ptrace_msg *rsp = NULL;

    if (0 == ptrace_msg_request(dest, msg_id,
                                data, data_len, &rsp, timeout_ms)) {
        success = PTRACE_MSG_DATA(rsp, int32_t);
        ptrace_msg_free(rsp);
    }

    return success ? 0 : -1;
}

static int simple_response(struct ptrace_msg *req, int32_t success)
{
    return ptrace_msg_response(req, &success, sizeof(success));
}

static int start_tracing(void)
{
    pid_t pid;

    if (g_tracing_pid)
        return 0;

    printf("start tracing...\n");

    pid = fork();

    if (0 == pid) { /* children */
        execlp("perfetto",
               "perfetto",
               "--txt",
               "-c", g_cfg.cfg_file,
               "-o", g_cfg.out_file,
               NULL);

        /* should never reach here */
        fprintf(stderr, "failed to start perfetto, please check whether "
                "perfetto is installed or the configuraton file is correct\n");
        exit(EXIT_FAILURE);
    }

    /* parent */
    if (pid < 0) {
        fprintf(stderr, "create tracing process failed, errno = %d\n", errno);
        return -1;
    }

    usleep(50*1000);
    if (waitpid(pid, NULL, WNOHANG) > 0) {
        fprintf(stderr, "tracing process exit abnormally\n");
        return -1;
    }

    printf("tracing process running: %d\n", pid);
    g_tracing_pid = pid;

    return 0;
}

static int stop_tracing(void)
{
    int rc;
    pid_t pid;
    int wstatus;

    if (!g_tracing_pid)
        return 0;

    printf("stop tracing...\n");

    rc = kill(g_tracing_pid, SIGTERM);
    if (rc < 0) {
        if (ESRCH == errno) {
            fprintf(stderr, "tracing process %d not found\n", g_tracing_pid);
            g_tracing_pid = 0;
            return 0;
        }

        fprintf(stderr, "sig to tracing process failed, errno = %d\n", errno);
        return -1;
    }

    pid = waitpid(g_tracing_pid, &wstatus, 0);
    if (pid != g_tracing_pid) {
        fprintf(stderr, "wait tracing process failed, errno = %d\n", errno);
        return -1;
    }

    g_tracing_pid = 0;

    return 0;
}

static int connect_server(struct sockaddr_in *dest)
{
    printf("connect to server(%s:%d)\n",
           inet_ntoa(dest->sin_addr), ntohs(dest->sin_port));

    return simple_request(dest, PTRACE_MSG_ID_CONNECT, NULL, 0, 3000);
}

static void disconnect_server(struct sockaddr_in *dest)
{
    printf("disconnect server\n");
    simple_request(dest, PTRACE_MSG_ID_DISCONNECT, NULL, 0, 3000);
}

static uint64_t calc_avg(uint64_t *data, unsigned int cnt)
{
    unsigned int i;
    uint64_t avg = 0;

    for (i = 0; i < cnt; i++)
        avg += data[i] / cnt;

    return avg;
}

static uint64_t calc_max_diff(uint64_t *data, unsigned int cnt)
{
    uint64_t min = 0, max = 0;
    unsigned int i;

    max = min = data[0];

    for (i = 0; i < cnt; i++) {
        if (data[i] < min)
            min = data[i];
        if (data[i] > max)
            max = data[i];
    }

    return max - min;
}

#define MEASURE_TIMES   3
static int measure_delay(struct sockaddr_in *dest, uint64_t *delay_out)
{
    int i, rc;
    uint64_t time, delay, delay_avg, delay_max_diff;
    uint64_t delays[MEASURE_TIMES];
    unsigned int cnt;

    printf("measuring the transmission delay...\n");

    cnt = 0;
    memset(delays, 0, sizeof(delays));
    for (i = 0; i < 100; i++) {
        time = ::perfetto::base::GetBootTimeNs().count();
        rc = ptrace_msg_request(dest, PTRACE_MSG_ID_ECHO, NULL, 0, NULL, 3000);
        if (0 == rc) {
            delay = (::perfetto::base::GetBootTimeNs().count() - time) / 2;
            delays[cnt++ % MEASURE_TIMES] = delay;

            if (cnt < MEASURE_TIMES) {
                printf("delay[%d]: %luns\n", i, delay);
            } else {
                delay_avg = calc_avg(delays, MEASURE_TIMES);
                delay_max_diff = calc_max_diff(delays, MEASURE_TIMES);
                printf("delay[%d]: %luns, avg(3) = %luns, maxdiff = %luns\n",
                       i, delay, delay_avg, delay_max_diff);
                if (delay_max_diff <= 10000) {
                    *delay_out = delay_avg;
                    return 0;
                }
            }
        } else {
            printf("delay[%d]: timeout\n", i);
        }

        usleep(10*1000);
    }

    return -1;
}

static int send_time(struct sockaddr_in *dest, uint64_t delay)
{
    struct ptrace_msg_time_data data;

    printf("send boot/delay time to server\n");
    data.trans_delay = delay;
    data.boot_time = ::perfetto::base::GetBootTimeNs().count();

    return simple_request(dest, PTRACE_MSG_ID_TIME, &data, sizeof(data), 3000);
}

static int start_server_tracing(struct sockaddr_in *dest)
{
    printf("start server tracing\n");
    return simple_request(dest, PTRACE_MSG_ID_START_TRACING, NULL, 0, 3000);
}

static int stop_server_tracing(struct sockaddr_in *dest)
{
    printf("stop server tracing\n");
    return simple_request(dest, PTRACE_MSG_ID_STOP_TRACING, NULL, 0, 3000);
}

static int send_file(struct sockaddr_in *dest, const char *path)
{
    int fd;
    ssize_t n, send_bytes;
    struct stat st;
    struct ptrace_msg *msg = (struct ptrace_msg *)g_send_buff;

    if (NULL == dest || NULL == path || '\0' == path[0])
        return -1;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open %s failed, errno = %d\n", path, errno);
        return -1;
    }

    fstat(fd, &st);
    printf("send %s to %s:%u, bytes = %ld(%.1fMB)\n",
            path, inet_ntoa(dest->sin_addr), ntohs(dest->sin_port),
            st.st_size, (double)(st.st_size) / 1024.0 / 1024.0);

    if (simple_request(dest, PTRACE_MSG_ID_FILE_BEGIN, NULL, 0, 3000) < 0) {
        close(fd);
        return -1;
    }

    send_bytes = 0;
    msg->id = PTRACE_MSG_ID_FILE_CONTENT;
    memcpy(&msg->dest, dest, sizeof(msg->dest));
    while ((n = read(fd, msg->data, sizeof(g_send_buff)-sizeof(*msg))) > 0) {
        msg->data_len = n;
        ptrace_msg_reqeust2(msg, NULL, 3000);
        send_bytes += n;
        printf("\b\b\b\b%ld%%", send_bytes * 100 / st.st_size);
        fflush(stdout);
    }
    printf("\nsend complete\n");

    simple_request(dest, PTRACE_MSG_ID_FILE_END, NULL, 0, 3000);

    close(fd);

    return 0;
}

static int server_run(void)
{
    int rc;
    int fd = -1;
    uint64_t file_size;
    int64_t diff_time;
    struct ptrace_msg *msg;
    time_t start_time = 0;
    struct stat st;

    if (g_cfg.no_wait) {
        start_tracing();
        start_time = time(NULL);
    }

    while (g_running) {
        if (g_tracing_pid && 0 == stat(g_cfg.out_file, &st)) {
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b> %.1lfMB, %lds",
                   (double)(st.st_size) / 1024.0 / 1024.0,
                   time(NULL) - start_time);
            fflush(stdout);
        }

        msg = ptrace_msg_recv(1000);
        if (!msg)
            continue;

        switch (msg->id) {
            case PTRACE_MSG_ID_CONNECT:
                simple_response(msg, PTRACE_SUCCESS);
                printf("client connected: %s:%d\n",
                       inet_ntoa(msg->src.sin_addr), ntohs(msg->src.sin_port));
                break;

            case PTRACE_MSG_ID_DISCONNECT:
                simple_response(msg, PTRACE_SUCCESS);
                printf("client disconnected\n");
                return 0;
                break;

            case PTRACE_MSG_ID_ECHO:
                /* run 2 times */
                ::perfetto::base::GetBootTimeNs().count();
                ::perfetto::base::GetBootTimeNs().count();
                ptrace_msg_response(msg, NULL, 0);
                break;

            case PTRACE_MSG_ID_TIME:
            {
                uint64_t bootime = ::perfetto::base::GetBootTimeNs().count();
                struct ptrace_msg_time_data *data = \
                        (struct ptrace_msg_time_data *)(msg->data);

                diff_time = bootime - data->boot_time - data->trans_delay;
                simple_response(msg, PTRACE_SUCCESS);

                printf("server time: %lu ns\n"
                       "client time: %lu ns\n"
                       "trans delay: %lu ns\n"
                       "diff time  : %ld ns\n",
                       bootime, data->boot_time,
                       data->trans_delay, diff_time);

                break;
            }

            case PTRACE_MSG_ID_START_TRACING:
                rc = start_tracing();
                if (0 == rc)
                    start_time = time(NULL);
                simple_response(msg, 0 == rc ? PTRACE_SUCCESS : PTRACE_FAILURE);
                break;

            case PTRACE_MSG_ID_STOP_TRACING:
                printf("\n");
                rc = stop_tracing();
                simple_response(msg, 0 == rc ? PTRACE_SUCCESS : PTRACE_FAILURE);
                break;

            case PTRACE_MSG_ID_FILE_BEGIN:
                printf("receiving client file, save as %s\n", g_cfg.clnt_file);
                file_size = 0;
                fd = open(g_cfg.clnt_file, O_CREAT | O_WRONLY, 0664);
                if (fd < 0)
                    fprintf(stderr, "create %s failed, errno = %d\n",
                            g_cfg.clnt_file, errno);
                simple_response(msg, fd >= 0 ? PTRACE_SUCCESS : PTRACE_FAILURE);
                break;

            case PTRACE_MSG_ID_FILE_CONTENT:
                if (fd >= 0) {
                    rc = write(fd, msg->data, msg->data_len);
                    file_size += msg->data_len;
                    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b>%lu Bytes",
                           file_size);
                    fflush(stdout);
                }
                simple_response(msg, fd >= 0 ? PTRACE_SUCCESS : PTRACE_FAILURE);
                break;

            case PTRACE_MSG_ID_FILE_END:
                if (fd >= 0) {
                    close(fd);
                    fd = -1;
                    simple_response(msg, PTRACE_SUCCESS);
                    printf("\nreceive complete\n");

                    printf("combining file:\n  + %s\n  + %s\n  = %s\n",
                           g_cfg.out_file, g_cfg.clnt_file, g_cfg.comb_file);
                    combine_file(g_cfg.out_file, g_cfg.clnt_file,
                                 g_cfg.comb_file, diff_time);
                } else {
                    simple_response(msg, PTRACE_FAILURE);
                }
                break;

            default:
                fprintf(stderr, "unknown msg: id = %u\n", msg->id);
                break;
        }

        ptrace_msg_free(msg);
    }

    if (fd >= 0)
        close(fd);

    return 0;
}


static int server_main(void)
{
    ptrace_msg_init(&g_cfg.addr);

    printf("server started at: %s:%d\n",
            inet_ntoa(g_cfg.addr.sin_addr), ntohs(g_cfg.addr.sin_port));

    server_run();

    ptrace_msg_destroy();

    printf("server exit\n");

    return 0;
}

static void signal_exit(int signum)
{
    fprintf(stderr, "\ncatch signal %d\n", signum);
    g_running = 0;
} 

static void signal_init(void)
{
    signal(SIGINT,  signal_exit);
    signal(SIGTERM, signal_exit);
    signal(SIGABRT, signal_exit);
    signal(SIGQUIT, signal_exit);
}

static void show_version(void)
{
    printf("ptrace %s Copyright (c) 2021 Flynn\n", PTRACE_VERSION);
}

static void usage(void)
{
    show_version();

    printf("Usage: %s [options]\n\
\n\
Options:\n\
  -h                        display this help and exit\n\
  -v                        output version information and exit\n\
  -c <config file>          configuration file passed to perfetto\n\
  -e <event category>       tracking events category, similar to option '-c',\n\
                            which can be 'app', 'sys' or 'all', means the\n\
                            <install dir>/etc/XXX.cfg configuration file\n\
                            will be used.\n\
                              app - only tracing applications events\n\
                              sys - only tracing system/kernel events\n\
                              all - tracing applications and system events\n\
                            if both '-c' and '-e' options are omitted, the\n\
                            <install dir>/etc/app.cfg file is used by default\n\
  -m <working mode>         working mode, which can be 'server', 'client' or '\n\
                            'alone' (default if omitted)\n\
  -o <output file>          output path\n\
  -s <IP address>           server's IP address, only used in 'server' and \n\
                            'client' mode\n\
  -p <port>                 server's UDP port, only used in 'server' and\n\
                            'client' mode\n\
  -n                        do not wait for the client's messages, start\n\
                            tracking directly after running. only used in\n\
                            'server' mode\n",
        g_program_name);
}

static int parse_args(int argc, char *argv[])
{
    int opt;
    int tmp;
    time_t now;
    struct tm tm;
    char prefix[32];

    memset(&g_cfg, 0, sizeof(g_cfg));

    while ((opt = getopt(argc, argv, "hve:c:m:o:s:p:n")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(EXIT_SUCCESS);
                break;

            case 'v':
                show_version();
                exit(EXIT_SUCCESS);
                break;

            case 'e':
                if ('\0' != g_cfg.cfg_file[0]) {
                    fprintf(stderr, "can not specify '-e' and '-c' options at"
                            " the same time\n");
                    return -1;
                }

                if (0 == strcmp(optarg, "all")) {
                    snprintf(g_cfg.cfg_file, sizeof(g_cfg.cfg_file),
                            "%s/%s", PTRACE_CONFIG_PATH, "all.cfg");
                } else if (0 == strcmp(optarg, "sys")) {
                    snprintf(g_cfg.cfg_file, sizeof(g_cfg.cfg_file),
                            "%s/%s", PTRACE_CONFIG_PATH, "sys.cfg");
                } else if (0 == strcmp(optarg, "app")) {
                    snprintf(g_cfg.cfg_file, sizeof(g_cfg.cfg_file),
                            "%s/%s", PTRACE_CONFIG_PATH, "app.cfg");
                } else {
                    fprintf(stderr, "invalid event category: %s\n", optarg);
                    return -1;
                }

                break;

            case 'c':
                if ('\0' != g_cfg.cfg_file[0]) {
                    fprintf(stderr, "can not specify '-e' and '-c' options at"
                            " the same time\n");
                    return -1;
                }

                if (!access(optarg, R_OK)) {
                    fprintf(stderr,
                            "file does not exist or permission denied: %s\n",
                            optarg);
                    return -1;
                }

                snprintf(g_cfg.cfg_file, sizeof(g_cfg.cfg_file), "%s", optarg);

                break;

            case 'm':
                if (0 == strcmp(optarg, "alone")) {
                    g_cfg.work_mode = PTRACE_WORKING_MODE_ALONE;
                } else if (0 == strcmp(optarg, "server")) {
                    g_cfg.work_mode = PTRACE_WORKING_MODE_SERVER;
                } else if (0 == strcmp(optarg, "client")) {
                    g_cfg.work_mode = PTRACE_WORKING_MODE_CLIENT;
                } else {
                    fprintf(stderr, "invalid working mode: %s\n", optarg);
                    return -1;
                }
                break;

            case 'o':
                snprintf(g_cfg.out_file, sizeof(g_cfg.out_file), "%s", optarg);
                break;

            case 's':
                if (!inet_aton(optarg, &g_cfg.addr.sin_addr)) {
                    fprintf(stderr, "invalid IP address: %s\n", optarg);
                    return -1;
                }
                g_cfg.work_mode = PTRACE_WORKING_MODE_CLIENT;
                break;

            case 'p':
                if ((tmp = atoi(optarg)) <= 0) {
                    fprintf(stderr, "invalid port number: %s\n", optarg);
                    return -1;
                }
                g_cfg.addr.sin_port = htons(tmp);
                break;

            case 'n':
                g_cfg.no_wait = 1;
                break;

            default:
                return -1;
                break;
        }
    }

    /* default configuration */
    if (0 == g_cfg.addr.sin_port)
        g_cfg.addr.sin_port = htons(6000);

    if ('\0' == g_cfg.cfg_file[0])
        snprintf(g_cfg.cfg_file, sizeof(g_cfg.cfg_file), "%s/%s",
                 PTRACE_CONFIG_PATH, "app.cfg");

    now = time(NULL);
    localtime_r(&now, &tm);
    snprintf(prefix, sizeof(prefix), "%04d%02d%02d-%02d%02d%02d",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);

    if ('\0' == g_cfg.out_file[0])
        snprintf(g_cfg.out_file, sizeof(g_cfg.out_file), "%s_%s.ptrace",
                prefix, g_working_mode_name[g_cfg.work_mode]);

    if (PTRACE_WORKING_MODE_SERVER == g_cfg.work_mode) {
        snprintf(g_cfg.clnt_file, sizeof(g_cfg.out_file), "%s_client.ptrace", prefix);
        snprintf(g_cfg.comb_file, sizeof(g_cfg.out_file), "%s_comb.ptrace", prefix);
    }

    /* dump configuration */
    printf("\033[32m\
=====================================================================\n\
Working Mode        : %s\n\
Config File         : %s\n\
Trace File          : %s\n",
        g_working_mode_name[g_cfg.work_mode],
        g_cfg.cfg_file, g_cfg.out_file);

    if (PTRACE_WORKING_MODE_ALONE != g_cfg.work_mode) {
        printf("\
Server Address      : %s:%d\n",
            inet_ntoa(g_cfg.addr.sin_addr),
            ntohs(g_cfg.addr.sin_port));
    }

    if (PTRACE_WORKING_MODE_SERVER == g_cfg.work_mode) {
        printf("\
Client Trace File   : %s\n\
Combined Trace File : %s\n",
            g_cfg.clnt_file, g_cfg.comb_file);
    }

        printf("\
=====================================================================\033[0m\n");

    return 0;
}


int ptrace_main(int argc, char *argv[])
{
    int rc;
    uint64_t delay;
    time_t start_time;
    struct stat st;

    g_program_name = argv[0];

    rc = parse_args(argc, argv);
    if (rc < 0) {
        usage();
        exit(EXIT_FAILURE);
    }

    g_running = 1;

    signal_init();

    if (PTRACE_WORKING_MODE_SERVER == g_cfg.work_mode)
        return server_main();

    if (PTRACE_WORKING_MODE_CLIENT == g_cfg.work_mode) {
        ptrace_msg_init(NULL);

        rc = connect_server(&g_cfg.addr);
        if (rc < 0) {
            fprintf(stderr, "connect server failed\n");
            goto _exit;
        }

        rc = measure_delay(&g_cfg.addr, &delay);
        if (rc < 0) {
            fprintf(stderr, "warning: the delay time may have a large error\n");
            goto _exit;
        }

        send_time(&g_cfg.addr, delay);

        rc = start_server_tracing(&g_cfg.addr);
        if (rc < 0) {
            fprintf(stderr, "server start tracing failed\n");
            goto _exit;
        }
    }

    rc = start_tracing();
    if (rc < 0) {
        fprintf(stderr, "start tracing failed\n");
        goto _exit;
    }

    start_time = time(NULL);
    printf("press CTRL-C to stop tracing\n");

    while (g_running) {
        rc = stat(g_cfg.out_file, &st);
        if (0 == rc) {
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b> %.1lfMB, %lds",
                   (double)(st.st_size) / 1024.0 / 1024.0,
                   time(NULL) - start_time);
            fflush(stdout);
        }

        sleep(1);
    }

    printf("\n\n");

_exit:
    stop_tracing();

    if (PTRACE_WORKING_MODE_CLIENT == g_cfg.work_mode) {
        stop_server_tracing(&g_cfg.addr);
        send_file(&g_cfg.addr, g_cfg.out_file);
        disconnect_server(&g_cfg.addr);
    }

    if (PTRACE_WORKING_MODE_CLIENT == g_cfg.work_mode)
        ptrace_msg_destroy();

    printf("exit\n");

    return 0;
}
