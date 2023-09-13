#include "InetAddress.h"
#include <strings.h>
#include <string.h>


InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); //inet_addr同时完成主机字节序到网络字节序的转换和asic到二进制，c_str将string化为char*
}
std::string InetAddress::toIp() const
{
    char buf[64];
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}
std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    snprintf(buf + end, sizeof(buf) - end,":%u", port);
    return buf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}



// #include <iostream>
// int main()
// {
//     InetAddress addr(8000);
//     std::cout << addr.toPort() << std::endl;
//     return 0;
// }