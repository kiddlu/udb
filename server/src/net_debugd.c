#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "rlog.h"
#include "udb_debug.h"

#define SERV_PORT      (5164)
#define MAX_MSG_LENGTH (1470)

static pthread_t thread_id;

static int net_debugd_entry_thread(void)
{
    pthread_setname_np(pthread_self(), "net debugd");

    int socket_fd; /* file description into transport */
    int length;    /* length of address structure */

    int  readc; /* the number of read **/
    char buf[MAX_MSG_LENGTH];
    char result[MAX_MSG_LENGTH];
    int  result_len = 0;

    struct sockaddr_in myaddr;      /* address of this service */
    struct sockaddr_in client_addr; /* address of client */
    /*
     *	Get a socket into UDP/IP
     */

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("udp socket failed");
        exit(EXIT_FAILURE);
    }

    /*
     *    Set up our address
     */
    bzero((char *)&myaddr, sizeof(myaddr));
    myaddr.sin_family      = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port        = htons(SERV_PORT);

    /*
     *	Bind to the address to which the service will be offered
     */
    if (bind(socket_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
        perror("udp bind failed\n");
        exit(1);
    }

    /*
     * Loop continuously, waiting for datagrams
     * and response a message
     */
    length = sizeof(client_addr);

    printf("Net Debugd init\n");

    while (1)
    {
        if ((readc = recvfrom(socket_fd, &buf, MAX_MSG_LENGTH, 0,
                              (struct sockaddr *)&client_addr,
                              (socklen_t *)&length)) < 0)
        {
            memset(buf, 0, MAX_MSG_LENGTH);
            memset(result, 0, MAX_MSG_LENGTH);
            perror("could not read datagram!!");
            continue;
        }

        if (readc > 1 && readc < 64)
        {
            memset(buf, 0, MAX_MSG_LENGTH);
            memset(result, 0, MAX_MSG_LENGTH);
            continue;
        }

        if (buf[0] == 'u' && buf[1] == 'd' && buf[2] == 'b')
        {
            // printf("Net Debugd recv cmd\n");
            udb_debug_bridge(UDB_LAYER_NET, buf, result, &result_len);
        }
        else
        {
            printf("net debugd unknow cmd\n");
            result_len = strlen("unknow cmd");
            memcpy(result, "unknow cmd", result_len);
        }

        /* return to client */
        if (sendto(socket_fd, &result, result_len, 0,
                   (struct sockaddr *)&client_addr, length) < 0)
        {
            memset(buf, 0, MAX_MSG_LENGTH);
            memset(result, 0, MAX_MSG_LENGTH);
            result_len = 0;
            perror("Could not send datagram!!\n");
            continue;
        }

        memset(buf, 0, MAX_MSG_LENGTH);
        memset(result, 0, MAX_MSG_LENGTH);
        result_len = 0;
    }
}

void net_debugd_entry(void)
{
    pthread_create(&thread_id, NULL, (void *)net_debugd_entry_thread, NULL);
}
