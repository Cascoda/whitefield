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

void *radio_thread(void *arg)
{
    char               msg[1041];               /* receive buffer */
    int                n;                      /* bytes received */
    struct Event evt;
    struct Event *evt_p = &evt;

    while (1) {
    	if(numOfAliveNodes > 0)
    	{
    		n = recvfrom(gRadioFD, (void *)msg, sizeof(msg), 0, NULL, 0);

    		if (n <= 0) {
    			usleep(1);
   	 	        continue;
		    }
		    else if (n < OT_EVENT_METADATA_SIZE)
		    {
	 	       	ERROR("UDP RECV NOT ENOUGH BYTES FROM OT NODE ID\n");
   	 	        return NULL;
		    }
		    INFO("gRadioFD RECEIVED\n");

		    deserializeMessage(evt_p, msg);
		    printEvent(evt_p);

		    handleReceivedEvent(evt_p);
    	}
    	else
    	{
    		usleep(100);
    	}

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
