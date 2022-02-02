#define _OT_EVENT_HELPERS_C

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ot_event_helpers.h"

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

//TODO: Do meaningful translation (extract actual information from buffer into event)
void wfBufToOtEvent(struct Event *evt_out, const msg_buf_t *mbuf_in, uint32_t dst_id)
{
	//Dummy values for now...
	(void)mbuf_in;

	uint32_t ot_dst_id = dst_id + 1;

	static uint8_t data[4] = {0xca, 0x5c, 0x0d, 0xa0};
	evt_out->mDelay = 5;
	evt_out->mNodeId = ot_dst_id;
	evt_out->mEventType = OT_EVENT_TYPE_UART_WRITE;
	evt_out->mParam1 = 1;
	evt_out->mParam2 = 2;
	evt_out->mDataLength = sizeof(data);
    memcpy(evt_out->mData, data, evt_out->mDataLength);
}

//TODO: Do meaningful translation
void OtEventToWfBuf(const msg_buf_t *mbuf_out, struct Event *evt_in)
{
	//Dummy values for now...
	(void)evt_in;
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
