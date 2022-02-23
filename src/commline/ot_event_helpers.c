#define _OT_EVENT_HELPERS_C

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#include "ot_event_helpers.h"
#include "cl_usock.h"

int numOfAliveNodes = 0;
uint64_t g_nodes_cur_time[1024] = {0}; //Stores the current virtual time of each node.
uint16_t g_nodes_short_addr[1024] = {0}; //Maps each node's id to its short address.
uint64_t g_nodes_ext_addr[1024] = {0}; //Maps each node's id to its extended address.

const char *getEventTypeName(enum EventTypes evtType)
{
	switch (evtType)
	{
		case OT_EVENT_TYPE_ALARM_FIRED:
			return "OT_EVENT_TYPE_ALARM_FIRED";
		case OT_EVENT_TYPE_RADIO_FRAME_TO_NODE:
			return "OT_EVENT_TYPE_RADIO_FRAME_TO_NODE";
		case OT_EVENT_TYPE_UART_WRITE :
			return "OT_EVENT_TYPE_UART_WRITE";
		case OT_EVENT_TYPE_STATUS_PUSH:
			return "OT_EVENT_TYPE_STATUS_PUSH";
		case OT_EVENT_TYPE_RADIO_TX_DONE:
			return "OT_EVENT_TYPE_RADIO_TX_DONE";
		case OT_EVENT_TYPE_RADIO_FRAME_TO_SIM:
			return "OT_EVENT_TYPE_RADIO_FRAME_TO_SIM";
		case OT_EVENT_TYPE_RADIO_FRAME_ACK_TO_SIM:
			return "OT_EVENT_TYPE_RADIO_FRAME_ACK_TO_SIM";
		default:
			return "INVALID EVENT TYPE";
	}
}

void printEvent(const struct Event *evt)
{
	INFO("printEvent\n");
    fprintf(stderr, "evt->mDelay: %"PRIu64"\n", evt->mDelay);
    fprintf(stderr, "evt->mNodeId: %d\n", evt->mNodeId);
    fprintf(stderr, "evt->mEventType: %s\n", getEventTypeName((enum EventTypes)evt->mEventType));
    fprintf(stderr, "evt->mParam1: %d\n", evt->mParam1);
    fprintf(stderr, "evt->mParam2: %d\n", evt->mParam2);
    fprintf(stderr, "evt->mDataLength: %d\n", evt->mDataLength);
    fprintf(stderr, "evt->mData: 0x");
    for(uint16_t i = 0; i < evt->mDataLength; i++)
    {
    	fprintf(stderr, "%02x", evt->mData[i]);
    }
    fprintf(stderr, "\n");
}

void wfBufToOtEvent(struct Event *evt_out, const struct msg_buf_extended *mbuf_in)
{
    evt_out->mTimestamp = mbuf_in->evt.mTimestamp;
	evt_out->mDelay = mbuf_in->evt.mDelay;
	evt_out->mNodeId = mbuf_in->evt.mNodeId;
	evt_out->mEventType = mbuf_in->evt.mEventType;
	evt_out->mParam1 = mbuf_in->evt.mParam1;
	evt_out->mParam2 = mbuf_in->evt.mParam2;
	evt_out->mDataLength = mbuf_in->evt.mDataLength;
    memcpy(evt_out->mData, mbuf_in->evt.mData, evt_out->mDataLength);
}

void OtEventToWfBuf(struct msg_buf_extended *mbuf_out, const struct Event *evt_in)
{
	mbuf_out->evt.mTimestamp = evt_in->mTimestamp;
	mbuf_out->evt.mDelay = evt_in->mDelay;
	mbuf_out->evt.mNodeId = evt_in->mNodeId;
	mbuf_out->evt.mEventType = evt_in->mEventType;
	mbuf_out->evt.mParam1 = evt_in->mParam1;
	mbuf_out->evt.mParam2 = evt_in->mParam2;
	mbuf_out->evt.mDataLength = evt_in->mDataLength;
    memcpy(mbuf_out->evt.mData, evt_in->mData, evt_in->mDataLength);
}

void serializeEvent(char *msg_out, const struct Event *evt_in)
{
    size_t idx;

    PUTLE64(evt_in->mDelay, (uint8_t *)msg_out);
    idx = sizeof(evt_in->mDelay);

    PUTLE32(evt_in->mNodeId, (uint8_t *)&msg_out[idx]);
    idx += sizeof(evt_in->mNodeId);

    msg_out[idx++] = evt_in->mEventType;
    msg_out[idx++] = evt_in->mParam1;
    msg_out[idx++] = evt_in->mParam2;

    PUTLE16(evt_in->mDataLength, (uint8_t *)&msg_out[idx]);
    idx += sizeof(evt_in->mDataLength);

    memcpy(&msg_out[idx], evt_in->mData, evt_in->mDataLength);
}

void deserializeMessage(struct Event *evt_out, const char *msg_in)
{
    size_t idx;

    evt_out->mDelay = GETLE64((uint8_t *)msg_in);
    idx = sizeof(evt_out->mDelay);

    evt_out->mNodeId = GETLE32((uint8_t *)&msg_in[idx]);
    idx += sizeof(evt_out->mNodeId);

    evt_out->mEventType = msg_in[idx++];
    evt_out->mParam1 = msg_in[idx++];
    evt_out->mParam2 = msg_in[idx++];

    evt_out->mDataLength = GETLE16((uint8_t *)&msg_in[idx]);
    idx += sizeof(evt_out->mDataLength);

    memcpy(evt_out->mData, &msg_in[idx], evt_out->mDataLength);
}

void setAliveNode()
{
	numOfAliveNodes++;
    fprintf(stderr, "numOfAliveNodes++: %d\n", getAliveNodes());
}

void setAsleepNode()
{
	numOfAliveNodes--;
    fprintf(stderr, "numOfAliveNodes--: %d\n", getAliveNodes());
}

int getAliveNodes()
{
	return numOfAliveNodes;
}

void setNodeCurTime(uint32_t nodeId, uint64_t time)
{
	g_nodes_cur_time[nodeId - 1] = time;
}

uint64_t getNodeCurTime(uint32_t nodeId)
{
	return g_nodes_cur_time[nodeId - 1];
}

static void initShortAddrMap(void)
{
	for(int i = 0; i < sizeof(g_nodes_short_addr)/sizeof(g_nodes_short_addr[0]); i++)
	{
		g_nodes_short_addr[i] = InvalidShortAddr;
	}
}

static void initExtendedAddrMap(void)
{
	for(int i = 0; i < sizeof(g_nodes_ext_addr)/sizeof(g_nodes_ext_addr[0]); i++)
	{
		g_nodes_ext_addr[i] = InvalidExtendedAddr;
	}
}

void initAddressMaps(void)
{
	initShortAddrMap();
	initExtendedAddrMap();
}

void setNodeShortAddr(uint32_t nodeId, uint16_t short_addr)
{
	fprintf(stderr, "Setting node %d rloc: 0x%x -> 0x%x\n", nodeId, getShortAddrFromNodeId(nodeId), short_addr);
	g_nodes_short_addr[nodeId - 1] = short_addr;
}

uint16_t getShortAddrFromNodeId(uint32_t nodeId)
{
	return g_nodes_short_addr[nodeId - 1];
}

uint32_t getNodeIdFromShortAddr(uint16_t short_addr)
{
	for(int i = 0; i < sizeof(g_nodes_short_addr)/sizeof(g_nodes_short_addr[0]); i++)
	{
		if(g_nodes_short_addr[i] == short_addr)
			return (uint32_t)i;
	}
	fprintf(stderr, "SHORT ADDRESS NOT REGISTERED...\n");
	return (uint32_t)-1;
}

void setNodeExtendedAddr(uint32_t nodeId, uint64_t ext_addr)
{
	fprintf(stderr, "Setting node %d ext_addr: 0x%"PRIx64" -> 0x%"PRIx64"\n", nodeId, getExtendedAddrFromNodeId(nodeId), ext_addr);
	g_nodes_ext_addr[nodeId - 1] = ext_addr;
}

uint64_t getExtendedAddrFromNodeId(uint32_t nodeId)
{
	return g_nodes_ext_addr[nodeId - 1];
}

uint32_t getNodeIdFromExtendedAddr(uint64_t ext_addr)
{
	for(int i = 0; i < sizeof(g_nodes_ext_addr)/sizeof(g_nodes_ext_addr[0]); i++)
	{
		if(g_nodes_ext_addr[i] == ext_addr)
			return (uint32_t)i;
	}
	fprintf(stderr, "SHORT ADDRESS NOT REGISTERED...\n");
	return (uint32_t)-1;
}

void handleReceivedEvent(struct Event *evt)
{
    struct msg_buf_extended msg;
    struct msg_buf_extended *msg_p = &msg;

    OtEventToWfBuf(msg_p, evt);

	switch(evt->mEventType)
	{
	case OT_EVENT_TYPE_UART_WRITE:
//		INFO("Log message from OpenThread:\n");
//		for(size_t i = 0; i < evt->mDataLength; i++)
//		{
//			fprintf(stderr, "%c", evt->mData[i]);
//		}
//		fprintf(stderr, "\n");
		break;
	case OT_EVENT_TYPE_STATUS_PUSH:
	case OT_EVENT_TYPE_ALARM_FIRED:
	case OT_EVENT_TYPE_RADIO_FRAME_TO_SIM:
	case OT_EVENT_TYPE_RADIO_FRAME_TO_NODE:
	case OT_EVENT_TYPE_RADIO_FRAME_ACK_TO_SIM:
	case OT_EVENT_TYPE_RADIO_TX_DONE:
		INFO("Handling %s event...\n", getEventTypeName(evt->mEventType));
		if(CL_SUCCESS != cl_sendto_q(MTYPE(AIRLINE, CL_MGR_ID), (msg_buf_t *)msg_p, sizeof(struct msg_buf_extended))) {
	//				mac_call_sent_callback(sent, ptr, MAC_TX_ERR_FATAL, 3);
			ERROR("FAILURE SENDING TO AIRLINE!!\n");
		}
		break;
	default:
		INFO("%s events not implemented yet...\n", getEventTypeName(evt->mEventType));
	}
}
