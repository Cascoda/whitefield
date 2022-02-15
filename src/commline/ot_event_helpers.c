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
uint64_t g_nodes_cur_time[MAX_CHILD_PROCESSES]; //Stores the current virtual time of each node.

/**
 * Extract a 16-bit value from a little-endian octet array
 */
static inline uint16_t GETLE16(const uint8_t *in)
{
	return ((in[1] << 8) & 0xff00) | (in[0] & 0x00ff);
}

/**
 * Extract a 32-bit value from a little-endian octet array
 */
static inline uint32_t GETLE32(const uint8_t *in)
{
	return (((uint32_t)in[3] << 24) + ((uint32_t)in[2] << 16) + ((uint32_t)in[1] << 8) + (uint32_t)in[0]);
}

/**
 * Extract a 64-bit value from a little-endian octet array
 */
static inline uint64_t GETLE64(const uint8_t *in)
{
	return (((uint64_t)in[7] << 56) + ((uint64_t)in[6] << 48) + ((uint64_t)in[5] << 40) + ((uint64_t)in[4] << 32) +
			((uint64_t)in[3] << 24) + ((uint64_t)in[2] << 16) + ((uint64_t)in[1] << 8) + (uint64_t)in[0]);
}

/**
 * Put a 16-bit value into a little-endian octet array
 */
static inline void PUTLE16(uint16_t in, uint8_t *out)
{
	out[0] = in & 0xff;
	out[1] = (in >> 8) & 0xff;
}
/**
 * Put a 32-bit value into a little-endian octet array
 */
static inline void PUTLE32(uint32_t in, uint8_t *out)
{
	out[0] = in & 0xff;
	out[1] = (in >> 8) & 0xff;
	out[2] = (in >> 16) & 0xff;
	out[3] = (in >> 24) & 0xff;
}

/**
 * Put a 64-bit value into a little-endian octet array
 */
static inline void PUTLE64(uint64_t in, uint8_t *out)
{
	out[0] = in & 0xff;
	out[1] = (in >> 8) & 0xff;
	out[2] = (in >> 16) & 0xff;
	out[3] = (in >> 24) & 0xff;
	out[4] = (in >> 32) & 0xff;
	out[5] = (in >> 40) & 0xff;
	out[6] = (in >> 48) & 0xff;
	out[7] = (in >> 56) & 0xff;
}

static const char *getEventTypeName(enum EventTypes evtType)
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
    	fprintf(stderr, "%x", evt->mData[i]);
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
	g_nodes_cur_time[nodeId] = time;
}

uint64_t getNodeCurTime(uint32_t nodeId)
{
	return g_nodes_cur_time[nodeId];
}

void handleReceivedEvent(struct Event *evt)
{
    struct msg_buf_extended msg;
    struct msg_buf_extended *msg_p = &msg;

	evt->mTimestamp = getNodeCurTime(evt->mNodeId) + evt->mDelay;

    OtEventToWfBuf(msg_p, evt);

	fprintf(stderr, "RECEIVED EVENT (node %d curTime: %"PRIu64") + "
			"mDelay: %"PRIu64" = timestamp %"PRIu64"\n", evt->mNodeId,
			getNodeCurTime(evt->mNodeId), evt->mDelay, evt->mTimestamp);

	switch(evt->mEventType)
	{
	case OT_EVENT_TYPE_STATUS_PUSH:
		INFO("%s events ignored...\n", getEventTypeName(evt->mEventType));
		break;
	case OT_EVENT_TYPE_UART_WRITE:
		INFO("%s event processing not implemented yet...\n", getEventTypeName(evt->mEventType));
		break;
	case OT_EVENT_TYPE_ALARM_FIRED:
			INFO("Handling %s event...\n", getEventTypeName(evt->mEventType));
			if(CL_SUCCESS != cl_sendto_q(MTYPE(AIRLINE, CL_MGR_ID), (msg_buf_t *)msg_p, sizeof(struct msg_buf_extended))) {
	//				mac_call_sent_callback(sent, ptr, MAC_TX_ERR_FATAL, 3);
				ERROR("FAILURE SENDING TO AIRLINE!!\n");
			}
	}
}

//void processEvent(const struct Event *evt)
//{
//	if(evt->mEventType == OT_EVENT_TYPE_STATUS_PUSH)
//	{
//		INFO("%s events ignored...\n", getEventTypeName(evt->mEventType));
//		return;
//	}
//	else
//	{
//		INFO("Processing %s event...\n", getEventTypeName(evt->mEventType));
//
//		// time keeping: infer abs time this event should happen, from the delta Delay given.
//		evt->mTimestamp = node.CurTime + evt->mDelay;
//		//NOTE: AND ALSO NEXT STEP FOR MONDAY... node's curr time should be set to Whitefield's current time at the point where it is instantiated.
//		//When does the node's curr time get updated? Look that up and implement.
//
////		switch (evt->mEventType)
////		{
////			case OT_EVENT_TYPE_ALARM_FIRED:
////				//1. assert that delay is greater than 0
////				//2. indicate that this node isn't alive anymore (array of bools?)
////				//3.
////				break;
////		}
//	}
//}
