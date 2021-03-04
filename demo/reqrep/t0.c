#include "nng/nng.h"
#include "nng/protocol/reqrep0/rep.h"
#include "nuts.h"
#include <stdio.h>

static void test_rep_send_nonblock()
{
    nng_socket rep;
    nng_socket req;
    int rv = 0;

    NUTS_PASS(nng_req0_open(&req));
    NUTS_PASS(nng_rep0_open(&rep));
    NUTS_PASS(nng_setopt_ms(req, NNG_OPT_SENDTIMEO, 1000));
    NUTS_PASS(nng_setopt_ms(rep, NNG_OPT_RECVTIMEO, 1000));
    NUTS_PASS(nng_setopt_ms(rep, NNG_OPT_SENDTIMEO, 1000));
    NUTS_MARRY(rep, req);

    NUTS_SEND(req, "SEND");
    NUTS_RECV(rep, "SEND");

    rv = nng_send(rep, "RECV", 5, NNG_FLAG_NONBLOCK);
    NUTS_PASS(rv);
    NUTS_RECV(req, "RECV");

    NUTS_CLOSE(rep);
    NUTS_CLOSE(req);

    return;
}

static void test_rep_double_recive()
{
    nng_socket s1;
    nng_aio* aio1;
    nng_aio* aio2;

    NUTS_PASS(nng_rep0_open(&s1));
    NUTS_PASS(nng_aio_alloc(&aio1, NULL, NULL));
    NUTS_PASS(nng_aio_alloc(&aio2, NULL, NULL));

    nng_recv_aio(s1, aio1);
    nng_recv_aio(s1, aio2);

    nng_aio_wait(aio2);
    NUTS_FAIL(nng_aio_result(aio2), NNG_ESTATE);

    NUTS_CLOSE(s1);
    NUTS_FAIL(nng_aio_result(aio1), NNG_ECLOSED);
    nng_aio_free(aio2);
    nng_aio_free(aio1);

    return;
}

NUTS_TESTS = {
    { "rep send nonblock", test_rep_send_nonblock },
    { "rep double recive", test_rep_double_recive },
    { NULL, NULL },
};
