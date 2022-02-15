/*
 * Copyright (C) 2017 Rahul Jadhav <nyrahul@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU
 * General Public License v2. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     commline
 * @{
 *
 * @file
 * @brief       Abstract Unix domain sockets for commline
 *
 * @author      Rahul Jadhav <nyrahul@gmail.com>
 *
 * @}
 */

#include "ot_event_helpers.h"

#ifndef _CL_USOCK_H_
#define _CL_USOCK_H_

#define MAX_CHILD_PROCESSES 1024

int  usock_init(const long my_mtype, const uint8_t flags);
void usock_cleanup(void);
int  usock_recvfrom(const long mtype, msg_buf_t *mbuf, uint16_t len, uint16_t flags);
int  usock_sendto(const long mtype, msg_buf_t *mbuf, uint16_t len);
int  usock_get_descriptor(const long mtype);

// The following UDP functions are used for communicating with OpenThread nodes
int udp_sock_init(const uint16_t nodeId);
int udp_sock_sendto(const uint16_t nodeId, struct Event *evt);

#define CL_INIT           usock_init
#define CL_CLEANUP        usock_cleanup
#define CL_RECVFROM       usock_recvfrom
#define CL_SENDTO         usock_sendto
#define CL_GET_DESCRIPTOR usock_get_descriptor
#define OT_INIT 		  udp_sock_init
#define OT_SENDTO         udp_sock_sendto

#endif //_CL_MSGQ_H_
