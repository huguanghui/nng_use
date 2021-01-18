#include <stdio.h>
#include <stdlib.h>

#include "nng/nng.h"
#include "nng/protocol//reqrep0/req.h"
#include "nng/supplemental/util/platform.h"

#define REST_URL "http://127.0.0.1:%u/api/rest/rot13"

void fatal(const char what, int rv)
{
    fprintf(stderr, "%s: %s\n", what, nng_strerror(rv));
    exit(1);
}

// 1. 支持 http 的API交互
// 2. 进程内部交互

/*! \enum job_state
 *
 *  Detailed description
 */
typedef enum {
    SEND_REQ, /*!< send REQ request */
    RECV_REQ, /*!< receive REP replay */
} job_state;

/*! \struct rest_job
 *  \brief Brief struct description
 *
 *  Detailed description
 */
typedef struct rest_job {
    nng_aio* http_aio;
    nng_http_res* http_res;
    job_state state;
    nng_msg* msg;
    nng_aio* aio;
    nng_ctx ctx;
    struct rest_job* next;
} rest_job;

nng_socket req_sock;

nng_mtx* job_lock;
rest_job* job_freelist;

void rest_start(uint16_t port)
{
    char rest_addr[128];
    nng_url* url;
    int rv;

    rv = nng_mtx_alloc(&job_lock);
    if (rv != 0) {
        fatal("nng_mtx_alloc", rv);
    }

    job_freelist = NULL;
    snprintf(rest_addr, sizeof(rest_addr), REST_URL, port);
    rv = nng_url_parse(&url, rest_addr);
    if (rv != 0) {
        fatal("nng_url_parse", rv);
    }
#if 0
    rv = nng_req0_open(&req_sock);
    if (rv != 0) {
        fatal("nng_req0_open", rv);
    }
#endif
}

int main(int argc, char* argv[])
{
    printf("HGH-TEST[%s %d]\n", __FUNCTION__, __LINE__);
    return 0;
}
