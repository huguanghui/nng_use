#include "nng/nng.h"
#include "nuts.h"

static void test_pairs1_send_buffer()
{
    nng_socket s;
    int v;
    bool b;
    size_t sz;

    NUTS_PASS(nng_pair1_open(&s));
    NUTS_PASS(nng_getopt_int(s, NNG_OPT_SENDBUF, &v));
    NUTS_TRUE(v == 0);
    NUTS_FAIL(nng_getopt_bool(s, NNG_OPT_SENDBUF, &b), NNG_EBADTYPE);
    sz = 1;
    NUTS_FAIL(nng_getopt(s, NNG_OPT_SENDBUF, &b, &sz), NNG_EINVAL);

    NUTS_CLOSE(s);

    return;
}

NUTS_TESTS = {
    { "pairs1 send buffer", test_pairs1_send_buffer },

    { NULL, NULL },
};
