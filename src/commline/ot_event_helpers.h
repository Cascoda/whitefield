#ifndef _OT_EVENT_HELPERS_H
#define _OT_EVENT_HELPERS_H

#include <stdint.h>
#include "commline.h"

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_END __attribute__((packed))

extern uint64_t g_nodes_cur_time[1024]; //Stores the current virtual time of each node.
extern uint16_t g_nodes_short_addr[1024]; //Maps each node's id to its short address.
extern uint64_t g_nodes_ext_addr[1024]; //Maps each node's id to its extended address.

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
	OT_EVENT_METADATA_SIZE  = 17, // mDelay, mNodeId, mEventType, mParam1, mParam2, mDataLength
	OT_EVENT_DATA_MAX_SIZE  = 1024, //mData
	OT_RADIO_FRAME_MAX_SIZE =  127 //max psdu size for radio events
};

enum
{
	InvalidShortAddr   = 0xfffe,
	BroadcastShortAddr = 0xffff,
	InvalidExtendedAddr = (uint64_t)-1,
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

OT_TOOL_PACKED_BEGIN
struct RadioMessage
{
	uint8_t channel;
	uint8_t psdu[OT_RADIO_FRAME_MAX_SIZE];
} OT_TOOL_PACKED_BEGIN;

// Convenience function for printing event names
const char *getEventTypeName(enum EventTypes evtType);

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

void initAddressMaps(void);

void setNodeShortAddr(uint32_t nodeId, uint16_t short_addr);
uint16_t getShortAddrFromNodeId(uint32_t nodeId);
uint32_t getNodeIdFromShortAddr(uint16_t short_addr);

void setNodeExtendedAddr(uint32_t nodeId, uint64_t ext_addr);
uint64_t getExtendedAddrFromNodeId(uint32_t nodeId);
uint32_t getNodeIdFromExtendedAddr(uint64_t short_addr);

// Handles events received from OT nodes.
void handleReceivedEvent(struct Event *evt);

//// Process a received event.
//void processEvent(const struct Event *evt);

#endif //_OT_EVENT_HELPERS_H
