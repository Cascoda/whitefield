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
 * @brief       Exported interfaces for commline
 *
 * @author      Rahul Jadhav <nyrahul@gmail.com>
 *
 * @}
 */

#define _COMMLINE_C_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <commline.h>
#include <errno.h>

#ifdef USE_UNIX_SOCKETS
#include <cl_usock.h>
#else
#include <cl_msgq.h>
#endif

int cl_init(const long my_mtype, const uint8_t flags)
{
    return CL_INIT(my_mtype, flags);
}

int ot_init(const uint16_t nodeId)
{
	return OT_INIT(nodeId);
}

int cl_bind(const long my_mtype)
{
#ifdef USE_UNIX_SOCKETS
    return CL_INIT(my_mtype, 0);
#else
    return SUCCESS;
#endif
}

void cl_cleanup(void)
{
    CL_CLEANUP();
}

int cl_sendto_q(const long mtype, msg_buf_t *mbuf, uint16_t len)
{
    if (!mbuf || !len) {
        ERROR("sendto invalid parameters passed buf:%p, buflen:%d\n", mbuf, len);
        return FAILURE;
    }

    int line = GET_LINE(mtype);

    if(line == STACKLINE){
    	struct Event evt;
    	struct Event *evt_p = &evt;

    	wfBufToOtEvent(evt_p, (struct msg_buf_extended *)mbuf);
    	return OT_SENDTO(GET_ID(mtype), evt_p);
    } else {
    	return CL_SENDTO(mtype, mbuf, len);
    }
}

int cl_recvfrom_q(const long mtype, msg_buf_t *mbuf, uint16_t len, uint16_t flags)
{
    if (!mbuf || !len) {
        ERROR("invalid parameters passed buf:%p, buflen:%d\n", mbuf, len);
        return FAILURE;
    }

    memset(mbuf, 0, sizeof(msg_buf_t));
    return CL_RECVFROM(mtype, mbuf, len, flags);
}

int cl_get_descriptor(const long mtype)
{
#ifdef USE_UNIX_SOCKETS
    return CL_GET_DESCRIPTOR(mtype);
#else
    ERROR("Get descriptor not supported for used IPC\n");
    return FAILURE;
#endif
}
