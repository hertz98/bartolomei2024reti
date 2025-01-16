#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>

int main(int argc, char ** argv)
{
    uint32_t host = 41,
    network, host2;

    host = htonl(host);
    host2 = ntohl(host);

    printf("%d -> %d\n", host, host2);
}