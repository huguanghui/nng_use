#include "nng/nng.h"
#include "nng/protocol/bus0/bus.h"
#include <stdio.h>

/*
 * 创建3个节点, bu0 - server bus1 - client bus2 - client
 */
const char* addr = "inproc://test";

#define APPENDSTR(m, s) nng_msg_append(m, s, strlen(s))

int main(int argc, char* argv[])
{
    int rc = 0;
    nng_socket bus0;
    nng_socket bus1;
    nng_socket bus2;
    nng_socket bus3;
    nng_duration rtimeo;

    nng_bus0_open(&bus0);
    nng_bus0_open(&bus1);
    nng_bus0_open(&bus2);

    rc = nng_listen(bus0, addr, NULL, 0);
    nng_dial(bus1, addr, NULL, 0);
    nng_dial(bus2, addr, NULL, 0);
    printf("HGH-TEST[%s %d] rc: %d\n", __FUNCTION__, __LINE__, rc);

    rtimeo = 50;
    nng_setopt_ms(bus0, NNG_OPT_RECVTIMEO, rtimeo);
    nng_setopt_ms(bus1, NNG_OPT_RECVTIMEO, rtimeo);
    nng_setopt_ms(bus2, NNG_OPT_RECVTIMEO, rtimeo);

    /* 测试接收 */
    nng_msg* msg;
    rc = nng_recvmsg(bus0, &msg, 0);
    printf("HGH-TEST[%s %d] rc: %d\n", __FUNCTION__, __LINE__, rc);
    nng_msg_alloc(&msg, 0);
    APPENDSTR(msg, "test999");
    nng_sendmsg(bus1, msg, 0);
    rc = nng_recvmsg(bus0, &msg, 0);
    printf("HGH-TEST[%s %d] rc: %d content: %s\n", __FUNCTION__, __LINE__, rc,
        (char*)nng_msg_body(msg));
    nng_msg_alloc(&msg, 0);
    APPENDSTR(msg, "onthe");
    nng_sendmsg(bus0, msg, 0);
    rc = nng_recvmsg(bus1, &msg, 0);
    printf("HGH-TEST[%s %d] rc: %d content: %s\n", __FUNCTION__, __LINE__, rc,
        (char*)nng_msg_body(msg));
    rc = nng_recvmsg(bus2, &msg, 0);
    printf("HGH-TEST[%s %d] rc: %d content: %s\n", __FUNCTION__, __LINE__, rc,
        (char*)nng_msg_body(msg));
    nng_close(bus2);
    nng_close(bus1);
    nng_close(bus0);
    return 0;
}
