#include <netinet/in.h>
#include <pthread.h>

#include "../commline/ot_event_helpers.h"

#define _OT_HANDLER_C_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include "commline/commline.h"
#include "utils/forker_common.h"
#include "commline/cl_usock.h"

#define RADIO_SERVER_PORT 9000

//extern struct pollfd g_udpsock_fd[MAX_CHILD_PROCESSES];
int gRadioFD = -1;

int start_radio_server(int portno)
{
    struct sockaddr_in myaddr; /* our address */
    int                fd;     /* our socket */

    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        ERROR("cannot create socket %m\n");
        goto failure;
    }

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family      = AF_INET;
//    inet_pton(AF_INET, "127.0.0.1", &myaddr.sin_addr);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port        = htons(portno);

    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        ERROR("bind failed portno=%d %m\n", portno);
        goto failure;
    }
    return fd;
failure:
    if (fd > 0) {
        close(fd);
    }
    return FAILURE;
}

//void *radio_thread(void *arg)
//{
//	int poll_ret;
//	int timeout = 3 * 1000; //3 seconds
//
//	for (int i = 0; i < MAX_CHILD_PROCESSES; i++)
//	{
//		g_udpsock_fd[i].fd = -1;
//		g_udpsock_fd[i].events = POLLIN;
//		g_udpsock_fd[i].revents = 0;
//	}
//
//	sleep(5);
//	while (1)
//	{
//		INFO("fd[0]: %d; fd[1]: %d; fd[2]: %d\n", g_udpsock_fd[0].fd, g_udpsock_fd[1].fd, g_udpsock_fd[2].fd);
//		errno = 0;
//		poll_ret = poll(g_udpsock_fd, MAX_CHILD_PROCESSES, timeout);
//
//		if(poll_ret < 0) // poll call failed
//		{
//			ERROR("poll() failed\n");
//			perror("poll");
//			return NULL;
//		}
//
//		if(poll_ret == 0) // timer expired
//		{
//			ERROR("poll() timeout\n");
//			continue;
//		}
//
//		// Determines which filed descriptors are readable
//		for(int i = 0; i < MAX_CHILD_PROCESSES; i++)
//		{
//			if(g_udpsock_fd[i].revents != 0)
//			{
//				struct Event evt;
//				int ret = OT_RECVFROM(i, &evt);
//				if(ret <= 0)
//				{
//					continue;
//				}
//
//				ERROR("RECEIVED EVENT!\n");
//				ERROR("evt.mEventType: %d\n", evt.mEventType);
//			}
//
//		}
//	}
//}

void *radio_thread(void *arg)
{
    struct sockaddr_in remaddr;                /* remote address */
    socklen_t          alen = sizeof(remaddr); /* length of addresses */
    char               cmd[1041];               /* receive buffer */
    int                n;                      /* # bytes received */

    while (1) {
        n = recvfrom(gRadioFD, (void *)cmd, sizeof(cmd), 0, (struct sockaddr *)&remaddr, &alen);
        if (n <= 0) {
            continue;
        }
    else if (n < OT_EVENT_METADATA_SIZE)
    {
        ERROR("UDP RECV NOT ENOUGH BYTES FROM OT NODE ID\n");
        return NULL;
    }

        fprintf(stderr, "gRadioFD RECEIVED\n");

        struct Event evtt;
        struct Event *evt = &evtt;
    deserializeMessage(evt, cmd);
    fprintf(stderr, "evt->mDelay: %"PRIu64"\n", evt->mDelay);
    fprintf(stderr, "evt->mNodeId: %d\n", evt->mNodeId);
    fprintf(stderr, "evt->mEventType: %d\n", evt->mEventType);
    fprintf(stderr, "evt->mParam1: %d\n", evt->mParam1);
    fprintf(stderr, "evt->mParam2: %d\n", evt->mParam2);
    fprintf(stderr, "evt->mDataLength: %d\n", evt->mDataLength);
    fprintf(stderr, "evt->mData: 0x");
    for(uint8_t i = 0; i < evt->mDataLength; i++)
    {
    	fprintf(stderr, "%x", evt->mData[i]);
    }
    fprintf(stderr, "\n");
//        sendto(gRadioFD, rsp, n, 0, (struct sockaddr *)&remaddr, alen);
//        sendto(gRadioFD, END_OF_RSP, sizeof(END_OF_RSP), 0, (struct sockaddr *)&remaddr, alen);
        usleep(1);
    }
    //Unreachable!
    close(gRadioFD);
    return NULL;
}

int start_radio_thread(void)
{
    pthread_t tid;

    gRadioFD = start_radio_server(RADIO_SERVER_PORT);
    if (gRadioFD < 0) {
        ERROR("Failure starting UDP server\n");
        return FAILURE;
    }

    if (pthread_create(&tid, NULL, radio_thread, NULL) < 0) {
        ERROR("failure creating radio thread %m\n");
        return FAILURE;
    }
    pthread_detach(tid);
    return SUCCESS;
}
