#include "nng/nng.h"
#include "nng/protocol/pipeline0/pull.h"
#include "nng/protocol/pipeline0/push.h"
#include <stdio.h>

#define APPEND_STR(m, s) nng_msg_append(m, s, strlen(s))
#define NUTS_SEND(sock, string) nng_send(sock, string, strlen(string) + 1, 0)

int main(int argc, char* argv[])
{
    int rc = 0;
    char buf[1024];
    int buflen = 1024;
    nng_socket pull;
    nng_socket push;
    nng_msg* msg;
    const char* addr = "inproc://pipeline1_mono_faithful";

    nng_pull0_open(&pull);
    nng_pull0_open(&push);
    printf("HGH-TEST[%s %d]\n", __FUNCTION__, __LINE__);

    nng_socket_set_ms(pull, NNG_OPT_RECVTIMEO, 1000);
    nng_socket_set_ms(push, NNG_OPT_SENDTIMEO, 1000);

    nng_listen(pull, addr, NULL, 0);
    nng_dial(push, addr, NULL, 0);

    nng_msg_alloc(&msg, 0);
    APPEND_STR(msg, "ONE");
    nng_sendmsg(push, msg, 0);
    rc = nng_recvmsg(pull, &msg, 0);
    printf("HGH-TEST[%s %d] rc: %d\n", __FUNCTION__, __LINE__, rc);
    printf(
        "HGH-TEST[%s %d] buf: %x\n", __FUNCTION__, __LINE__, nng_msg_body(msg));

    nng_close(push);
    nng_close(pull);
    return 0;
}
