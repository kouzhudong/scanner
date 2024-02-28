#include "test.h"
#include "udp.h"
#include "icmp.h"


int test()
{
    IN_ADDR DestinationAddress;
    DestinationAddress.S_un.S_addr = inet_addr("104.193.88.77");
    Icmpv4Scan(&DestinationAddress);

    IN6_ADDR sin6_addr;
    InetPtonA(AF_INET6, "fe80::36ca:81ff:fe23:b43", &sin6_addr); //局域网内的一个链路本地地址。
    //InetPtonA(AF_INET6, "240e:390:8a0:f940:a09a:948a:3480:4479", &sin6_addr);//局域网内的一个全局IPv6.
    //InetPtonA(AF_INET6, "2603:1030:20e:3::23c", &sin6_addr);//谨记谷歌的IPv6不翻墙会失败。
    Icmpv6Scan(&sin6_addr);

    return ERROR_SUCCESS;
}
