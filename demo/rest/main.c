#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nng/nng.h"
#include "nng/protocol//reqrep0/req.h"
#include "nng/supplemental/http/http.h"
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
    RECV_REP, /*!< receive REP replay */
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

static void rest_job_cb(void* arg);

static void rest_recycle_job(rest_job* job)
{
    if (job->http_res != NULL) {
        nng_http_res_free(job->http_res);
        job->http_res = NULL;
    }
    if (job->msg != NULL) {
        nng_msg_free(job->msg);
        job->msg = NULL;
    }
    if (nng_ctx_id(job->ctx) != 0) {
        nng_ctx_close(job->ctx);
    }
    nng_mtx_lock(job_lock);
    job->next = job_freelist;
    job_freelist = job;
    nng_mtx_unlock(job_lock);
}

static rest_job* rest_get_job()
{
    rest_job* job;

    nng_mtx_lock(job_lock);
    if ((job = job_freelist) != NULL) {
        job_freelist = job->next;
        nng_mtx_unlock(job_lock);
        job->next = NULL;
        return job;
    }
    nng_mtx_unlock(job_lock);
    if ((job = calloc(1, sizeof(*job))) == NULL) {
        return NULL;
    }
    if (nng_aio_alloc(&job->aio, rest_job_cb, job) != 0) {
        free(job);
        return NULL;
    }
    return job;
}

static void rest_http_fatal(rest_job* job, const char* fmt, int rv)
{
    char buf[128];
    nng_aio* aio = job->http_aio;
    nng_http_res* res = job->http_res;

    job->http_res = NULL;
    job->http_aio = NULL;
    snprintf(buf, sizeof(buf), fmt, nng_strerror(rv));
    nng_http_res_set_status(res, NNG_HTTP_STATUS_INTERNAL_SERVER_ERROR);
    nng_http_res_set_reason(res, buf);
    nng_aio_set_output(aio, 0, res);
    nng_aio_finish(aio, 0);
    rest_recycle_job(job);
}

static void rest_job_cb(void* arg)
{
    rest_job* job = arg;
    nng_aio* aio = job->aio;
    int rv;

    switch (job->state) {
    case SEND_REQ:
        if ((rv = nng_aio_result(aio)) != 0) {
            rest_http_fatal(job, "send REQ failed: %s", rv);
            return;
        }
        job->msg = NULL;
        nng_aio_set_msg(aio, NULL);
        job->state = RECV_REP;
        nng_ctx_recv(job->ctx, aio);
        break;
    case RECV_REP:
        if ((rv = nng_aio_result(aio)) != 0) {
            rest_http_fatal(job, "recv reply failed: %s", rv);
            return;
        }
        job->msg = nng_aio_get_msg(aio);
        rv = nng_http_res_copy_data(job->http_res, nng_msg_body(job->msg), nng_msg_len(job->msg));
        if (rv != 0) {
            rest_http_fatal(job, "nng_http_res_copy_data: %s", rv);
            return;
        }
        nng_aio_set_output(job->http_aio, 0, job->http_res);
        nng_aio_finish(job->http_aio, 0);
        job->http_aio = NULL;
        job->http_res = NULL;
        rest_recycle_job(job);
        return;
    default:
        fatal("bad case", NNG_ESTATE);
        break;
    }
}

#if 0
void rest_handle(nng_aio* aio)
{
    struct rest_job* job;
    nng_http_req* req = nng_aio_get_input(aio, 0);
    nng_http_conn* conn = nng_aio_get_input(aio, 2);
    size_t sz;
    int rv;
    void* data;

    if ((job = rest_get_job()) == NULL) {
        nng_aio_finish(aio, NNG_ENOMEM);
        return;
    }
    if (((rv = nng_http_res_alloc(&job->http_res)) != 0) || ((rv = nng_ctx_open(&job->ctx, req_sock)) != 0)) {
        rest_recycle_job(job);
        nng_aio_finish(aio, rv);
        return;
    }

    nng_http_req_get_data(req, &data, &sz);
    job->http_aio = aio;

    if ((rv = nng_msg_alloc(&job->msg, sz)) != 0) {
        rest_http_fatal(job, "nng_msg_alloc: %s", rv);
        return;
    }

    memcpy(nng_msg_body(job->msg), data, sz);
    nng_aio_set_msg(job->aio, job->msg);
    job->state = SEND_REQ;
    nng_ctx_send(job->ctx, job->aio);
}
#endif

void rest_handle(nng_aio *aio)
{
    nng_http_req *req = nng_aio_get_input(aio, 0);
    nng_http_res *res;
    size_t sz;
    int rv;
    void*data;

    nng_http_req_get_data(req, &data, &sz);
    if (( (rv = nng_http_res_alloc(&res)) != 0 ) ||
            ((rv = nng_http_res_copy_data(res, data, sz))!=0)||
            ((rv = nng_http_res_set_header(res, "Content-Type", "test/plain")) != 0) ||
            ((rv = nng_http_res_set_status(aio, NNG_HTTP_STATUS_OK)) != 0)) {
            nng_http_res_free(res);
            nng_aio_finish(aio, rv);
            return;
    }
    nng_aio_set_output(aio, 0, res);
    nng_aio_finish(aio, 0);
}

void rest_start(uint16_t port)
{
    nng_http_server* server;
    nng_http_handler* handler;
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
    rv = nng_http_server_hold(&server, url);
    if (rv != 0) {
        fatal("nng_http_server_hold", rv);
    }
    rv = nng_http_handler_alloc(&handler, url->u_path, rest_handle);
    if (rv != 0) {
        fatal("nng_http_handler_alloc", rv);
    }
    rv = nng_http_handler_set_method(handler, "POST");
    if (rv != 0) {
        fatal("nng_http_handler_set_method", rv);
    }
    rv = nng_http_handler_collect_body(handler, true, 1024 * 128);
    if (rv != 0) {
        fatal("nng_http_handler_collect_body", rv);
    }
    rv = nng_http_server_add_handler(server, handler);
    if (rv != 0) {
        fatal("nng_http_server_add_handler", rv);
    }
    rv = nng_http_server_start(server);
    if (rv != 0) {
        fatal("nng_http_server_start", rv);
    }
    nng_url_free(url);
}

int main(int argc, char* argv[])
{
    rest_start(8888);
    while (1) {
        sleep(3600);
    }
    return 0;
}
