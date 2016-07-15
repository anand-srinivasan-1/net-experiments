#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> /* close() */

#include <sys/socket.h>
#include <arpa/inet.h>

#define ERR_CHECK(condition, returncode) if(condition){return (returncode);}
#define RETCODE_CHECK(condition, string) if(condition){puts(string);return -1;}
#define SOCKET_CREATE_FAIL -1
#define PORT_NOT_OPEN -2
#define PORT_OPEN 1

#define STR_EQ(str1, str2) (strcmp((str1), (str2)) == 0)

int32_t is_tcp_port_open(char * ip, uint16_t port)
{
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ERR_CHECK(s == -1, SOCKET_CREATE_FAIL);

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    int ret = connect(s, (struct sockaddr*) &server, sizeof(server));
    if(ret < 0)
    {
        close(s);
        return PORT_NOT_OPEN;
    }

    return PORT_OPEN;
}

int main(int argc, char * argv[])
{
    char * ip = argv[2]; /* add support for host lookup later */
    
    if(argc < 3)
    {
        printf("usage: %s [port number|all] <ip>\n", argv[0]);
        return -1;
    }

    if(STR_EQ(argv[1], "all"))
    {
        /*
        use uint32_t to avoid infinite loop;
        port <= 65535 is always true for uint16_t
        */
        for(uint32_t port = 1; port < 65536; ++port)
        {
            int32_t code = is_tcp_port_open(ip, (uint16_t)port);
            /* dont check for SOCKET_CREATE_FAIL */
            if(code == PORT_OPEN)
            {
                printf("%d\n", port);
            }
        }
        
        return 0;
    }

    uint16_t port = atoi(argv[1]);
    RETCODE_CHECK(port == 0, "invalid port: 0");
    int32_t code = is_tcp_port_open(ip, port);
    RETCODE_CHECK(code == SOCKET_CREATE_FAIL, "could not create socket");
    if(code == PORT_NOT_OPEN)
    {
        printf("port %d not open\n", port);
        return 1;
    }
    else if(code == PORT_OPEN)
    {
        printf("port %d open\n", port);
        return 0;
    }
    else
    {
        /* invalid code - should never happen */
        return -1;
    }
}
