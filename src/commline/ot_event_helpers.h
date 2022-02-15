#ifndef _OT_EVENT_HELPERS_H
#define _OT_EVENT_HELPERS_H

#include <stdint.h>
#include "commline.h"

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_END __attribute__((packed))

enum EventTypes
{
    OT_EVENT_TYPE_ALARM_FIRED            = 0,
    OT_EVENT_TYPE_RADIO_FRAME_TO_NODE    = 1,
    OT_EVENT_TYPE_UART_WRITE             = 2,
    OT_EVENT_TYPE_STATUS_PUSH            = 5,
    OT_EVENT_TYPE_RADIO_TX_DONE          = 6,
    OT_EVENT_TYPE_RADIO_FRAME_TO_SIM     = 8,
    OT_EVENT_TYPE_RADIO_FRAME_ACK_TO_SIM = 9,
};

enum
{
	OT_EVENT_METADATA_SIZE = 17, // mDelay, mNodeId, mEventType, mParam1, mParam2, mDataLength
	OT_EVENT_DATA_MAX_SIZE = 1024, //mData
};

OT_TOOL_PACKED_BEGIN
struct Event
{
	uint64_t mTimestamp; //Not sent, only used internally
    uint64_t mDelay;
    uint32_t mNodeId;
    uint8_t  mEventType;
    int8_t   mParam1;
    int8_t   mParam2;
    uint16_t mDataLength;
    uint8_t  mData[OT_EVENT_DATA_MAX_SIZE];
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
struct msg_buf_extended
{
	msg_buf_t msg;
	struct Event evt;
} OT_TOOL_PACKED_BEGIN;

// Print event.
void printEvent(const struct Event *evt);

// Translate from msg_buf_extended to Event type.
void wfBufToOtEvent(struct Event *evt_out, const struct msg_buf_extended *mbuf_in);

// Translate from Event to msg_buf_extended.
void OtEventToWfBuf(struct msg_buf_extended *mbuf_out, const struct Event *evt_in);

// Serialize OT event for sending over UDP.
void serializeEvent(char *msg_out, const struct Event *evt_in);

// Deserialize buffer received from OT node over UDP and extract into Event struct.
void deserializeMessage(struct Event *evt_out, const char *msg_in);

void setAliveNode();
void setAsleepNode();
int getAliveNodes();

void setNodeCurTime(uint32_t nodeId, uint64_t time);
uint64_t getNodeCurTime(uint32_t nodeId);

// Handles events received from OT nodes.
void handleReceivedEvent(struct Event *evt);

//// Process a received event.
//void processEvent(const struct Event *evt);

#endif //_OT_EVENT_HELPERS_H
