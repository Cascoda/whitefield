/*
 * Copyright (C) 2017 Rahul Jadhav <nyrahul@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU
 * General Public License v2. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     airline
 * @{
 *
 * @file
 * @brief       Interface Handler for NS3
 *
 * @author      Rahul Jadhav <nyrahul@gmail.com>
 *
 * @}
 */

#include <ns3/single-model-spectrum-channel.h>
#include <ns3/mobility-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/spectrum-value.h>

#include <PropagationModel.h>
#include <common.h>
#include <Nodeinfo.h>
#include <Config.h>
#include <IfaceHandler.h>
#include <inttypes.h>

static AirlineManager* airlineMgr;

enum
{
	kFcfPanidCompression = 1 << 6,
};

enum
{
	kFcfSrcAddrNone  = 0 << 14,
	kFcfSrcAddrShort = 2 << 14,
	kFcfSrcAddrExt   = 3 << 14,
	kFcfSrcAddrMask  = 3 << 14,
};

enum
{
	kFcfDstAddrNone  = 0 << 10,
	kFcfDstAddrShort = 2 << 10,
	kFcfDstAddrExt   = 3 << 10,
	kFcfDstAddrMask  = 3 << 10,
};

enum
{
	kFcfFrameAck      = 2 << 0,
	kFcfFrameTypeMask = 7 << 0,
};

enum
{
	kFcfSize = sizeof(uint16_t),
	kDsnSize = sizeof(uint8_t),
};

enum
{
	LQI_MIN = 0,
	LQI_MAX = 255,
	RSSI_MIN = -100,
	RSSI_MAX = -50,
};

typedef uint16_t PanId;
typedef uint16_t ShortAddress;
typedef uint64_t ExtAddress;

static double round_number(double d)
{
	return floor(d + 0.5);
}

static double get_rssi_from_lqi(double lqi_in)
{
	double slope = 1.0 * (RSSI_MAX - RSSI_MIN) / (LQI_MAX - LQI_MIN);
	double rssi = RSSI_MIN + round_number(slope * (lqi_in - LQI_MIN));
	return rssi;
}

static enum OtErrors ot_status_convert(LrWpanMcpsDataConfirmStatus confirmStatus)
{
	switch(confirmStatus)
	{
	case IEEE_802_15_4_SUCCESS:
		return OT_ERROR_NONE;
	case IEEE_802_15_4_CHANNEL_ACCESS_FAILURE:
		return OT_ERROR_CHANNEL_ACCESS_FAILURE;
	default:
		return OT_ERROR_ABORT;
	}
}

static Mac16Address id16toaddr(const uint16_t id)
{
    Mac16Address mac;

    uint8_t idstr[2];
    uint8_t *ptr = (uint8_t*)&id;

    idstr[1] = ptr[0];
    idstr[0] = ptr[1];

    mac.CopyFrom(idstr);

    return mac;
};

static Mac64Address id64toaddr(const uint64_t id)
{
    Mac64Address mac;

    uint8_t idstr[8];
    uint8_t *ptr = (uint8_t*)&id;

    idstr[7] = ptr[0];
    idstr[6] = ptr[1];
    idstr[5] = ptr[2];
    idstr[4] = ptr[3];
    idstr[3] = ptr[4];
    idstr[2] = ptr[5];
    idstr[1] = ptr[6];
    idstr[0] = ptr[7];

    mac.CopyFrom(idstr);

    return mac;
};

static uint16_t addr16toid(const Mac16Address addr)
{
    uint16_t id = 0;
    uint8_t str[2];
    uint8_t *ptr = (uint8_t *)&id;

    addr.CopyTo(str);

    ptr[1] = str[0];
    ptr[0] = str[1];

    return id;
}

static uint64_t addr64toid(const Mac64Address addr)
{
    uint64_t id = 0;
    uint8_t str[8];
    uint8_t *ptr = (uint8_t *)&id;

    addr.CopyTo(str);

    ptr[7] = str[0];
    ptr[6] = str[1];
    ptr[5] = str[2];
    ptr[4] = str[3];
    ptr[3] = str[4];
    ptr[2] = str[5];
    ptr[1] = str[6];
    ptr[0] = str[7];

    return id;
}

static uint16_t getFrameControlField(msg_buf_extended *msg_ext)
{
	struct RadioMessage *radio_msg = (struct RadioMessage *)msg_ext->evt.mData;
	uint16_t fcf = GETLE16(radio_msg->psdu);
	return fcf;
}

static LrWpanAddressMode getSourceAddressMode(msg_buf_extended *msg_ext)
{
	uint16_t srcAddrMode = getFrameControlField(msg_ext) & kFcfSrcAddrMask;
	if(srcAddrMode == kFcfSrcAddrNone)
	{
		return NO_PANID_ADDR;
	}
	else if(srcAddrMode == kFcfSrcAddrShort)
	{
		return SHORT_ADDR;
	}
	else
	{
		return EXT_ADDR;
	}
}

static LrWpanAddressMode getDestinationAddressMode(msg_buf_extended *msg_ext)
{
	uint16_t dstAddrMode = getFrameControlField(msg_ext) & kFcfDstAddrMask;
	if(dstAddrMode == kFcfDstAddrNone)
	{
		return NO_PANID_ADDR;
	}
	else if(dstAddrMode == kFcfDstAddrShort)
	{
		return SHORT_ADDR;
	}
	else
	{
		return EXT_ADDR;
	}
}

static bool isDstAddrPresent(uint16_t aFcf)
{
	return (aFcf & kFcfDstAddrMask) != kFcfDstAddrNone;
}

static bool isDstPanIdPresent(uint16_t aFcf)
{
	//TODO: Assuming no header ie support
	return isDstAddrPresent(aFcf);
}

static bool isSrcPanIdPresent(uint16_t aFcf)
{
	//TODO: Assuming no header ie support
	bool srcPanIdPresent = false;

	if((aFcf & kFcfSrcAddrMask) != kFcfSrcAddrNone && (aFcf & kFcfPanidCompression) == 0)
	{
		srcPanIdPresent = true;
	}

	return srcPanIdPresent;
}

static uint8_t findDstAddrIndex(msg_buf_extended *msg_ext)
{
	return kFcfSize + kDsnSize + (isDstPanIdPresent(getFrameControlField(msg_ext)) ? sizeof(PanId) : 0);
}

static uint8_t findSrcAddrIndex(msg_buf_extended *msg_ext)
{
	uint8_t index = 0;
	uint16_t fcf = getFrameControlField(msg_ext);

	index += kFcfSize + kDsnSize;

	if(isDstPanIdPresent(fcf))
	{
		index += sizeof(PanId);
	}

	switch (fcf & kFcfDstAddrMask)
	{
	case kFcfDstAddrShort:
		index += sizeof(ShortAddress);
		break;
	case kFcfDstAddrExt:
		index += sizeof(ExtAddress);
		break;
	}

	if(isSrcPanIdPresent(fcf))
	{
		index += sizeof(PanId);
	}

	return index;
}

static Mac64Address extendedAddr(uint64_t addr)
{
	return id64toaddr(addr);
}

static Mac16Address shortAddr(uint16_t addr)
{
	return id16toaddr(addr);
}

static void getDestinationExtendedAddress(msg_buf_extended *msg_ext, Mac64Address *addr)
{
	struct RadioMessage *radio_msg = (struct RadioMessage *)msg_ext->evt.mData;

	uint8_t index = findDstAddrIndex(msg_ext);

	switch (getFrameControlField(msg_ext) & kFcfDstAddrMask)
	{
		case kFcfDstAddrShort:
			fprintf(stderr, "WRONG FUNCTION CALL, DESTINATION ADDRESS IS A SHORT ADDRESS...\n");
			break;
		case kFcfDstAddrExt:
			*addr = extendedAddr(GETLE64(&radio_msg->psdu[index]));
			break;
		default:
			fprintf(stderr, "PARSING OF ADDRESS FAILED.\n");
			break;
	}
}

static void getDestinationShortAddress(msg_buf_extended *msg_ext, Mac16Address *addr)
{
	struct RadioMessage *radio_msg = (struct RadioMessage *)msg_ext->evt.mData;

	uint8_t index = findDstAddrIndex(msg_ext);

	switch (getFrameControlField(msg_ext) & kFcfDstAddrMask)
	{
		case kFcfDstAddrShort:
			*addr = shortAddr(GETLE16(&radio_msg->psdu[index]));
			break;
		case kFcfDstAddrExt:
			fprintf(stderr, "WRONG FUNCTION CALL, SOURCE ADDRESS IS AN EXTENDED ADDRESS...\n");
			break;
		default:
			fprintf(stderr, "PARSING OF ADDRESS FAILED.\n");
			break;
	}
}

static void getSourceExtendedAddress(msg_buf_extended *msg_ext, Mac64Address *addr)
{
	struct RadioMessage *radio_msg = (struct RadioMessage *)msg_ext->evt.mData;

	uint8_t index = findSrcAddrIndex(msg_ext);

	switch (getFrameControlField(msg_ext) & kFcfSrcAddrMask)
	{
		case kFcfSrcAddrShort:
			fprintf(stderr, "WRONG FUNCTION CALL, SOURCE ADDRESS IS A SHORT ADDRESS...\n");
			break;
		case kFcfSrcAddrExt:
			*addr = extendedAddr(GETLE64(&radio_msg->psdu[index]));
			break;
		default:
			fprintf(stderr, "PARSING OF ADDRESS FAILED.\n");
			break;
	}
}

static void getSourceShortAddress(msg_buf_extended *msg_ext, Mac16Address *addr)
{
	struct RadioMessage *radio_msg = (struct RadioMessage *)msg_ext->evt.mData;

	uint8_t index = findSrcAddrIndex(msg_ext);

	switch (getFrameControlField(msg_ext) & kFcfSrcAddrMask)
	{
		case kFcfSrcAddrShort:
			*addr = shortAddr(GETLE16(&radio_msg->psdu[index]));
			break;
		case kFcfSrcAddrExt:
			fprintf(stderr, "WRONG FUNCTION CALL, SOURCE ADDRESS IS AN EXTENDED ADDRESS...\n");
			break;
		default:
			fprintf(stderr, "PARSING OF ADDRESS FAILED.\n");
			break;
	}
}

static uint8_t getType(msg_buf_extended *msg_ext)
{
	struct RadioMessage *radio_msg = (struct RadioMessage *)msg_ext->evt.mData;

	return radio_msg->psdu[0] & kFcfFrameTypeMask;
}

static bool isAck(msg_buf_extended *msg_ext)
{
	return getType(msg_ext) == kFcfFrameAck;
}

static Ptr<LrWpanNetDevice> getDev(ifaceCtx_t *ctx, int id)
{
    Ptr<Node> node = ctx->nodes.Get(id); 
    Ptr<LrWpanNetDevice> dev = NULL;

    if (node) {
        dev = node->GetDevice(0)->GetObject<LrWpanNetDevice>();
    }
    return dev;
}

static uint8_t wf_ack_status(LrWpanMcpsDataConfirmStatus status)
{
    switch(status) {
        case IEEE_802_15_4_SUCCESS:
            return WF_STATUS_ACK_OK;
        case IEEE_802_15_4_NO_ACK:
            return WF_STATUS_NO_ACK;
        case IEEE_802_15_4_TRANSACTION_OVERFLOW:
        case IEEE_802_15_4_TRANSACTION_EXPIRED:
        case IEEE_802_15_4_CHANNEL_ACCESS_FAILURE:
            return WF_STATUS_ERR;	//can retry later
        case IEEE_802_15_4_INVALID_GTS:
        case IEEE_802_15_4_COUNTER_ERROR:
        case IEEE_802_15_4_FRAME_TOO_LONG:
        case IEEE_802_15_4_UNAVAILABLE_KEY:
        case IEEE_802_15_4_UNSUPPORTED_SECURITY:
        case IEEE_802_15_4_INVALID_PARAMETER:
        default:
            return WF_STATUS_FATAL;
    }
}

static void OTSendTxDone(struct msg_buf_extended *msg_buf_ext, uint32_t OtSrcNodeId)
{
	fprintf(stderr, "In OTSendTxDone..., sim time: %ld\n", Simulator::Now().GetTimeStep());

	msg_buf_ext->evt.mEventType = OT_EVENT_TYPE_RADIO_TX_DONE;
	msg_buf_ext->evt.mNodeId = OtSrcNodeId;
    msg_buf_ext->evt.mDelay = Simulator::Now().GetTimeStep() - getNodeCurTime(msg_buf_ext->evt.mNodeId);
    printEvent(&msg_buf_ext->evt);

	fprintf(stderr, "SENDING TX_DONE TO NODE %d...\n", OtSrcNodeId);
	cl_sendto_q(MTYPE(STACKLINE, msg_buf_ext->evt.mNodeId - 1), (msg_buf_t *)msg_buf_ext, sizeof(struct msg_buf_extended));

	// Update node's status
	setAliveNode(msg_buf_ext->evt.mNodeId);
	uint64_t node_cur_time = getNodeCurTime(msg_buf_ext->evt.mNodeId);
	setNodeCurTime(msg_buf_ext->evt.mNodeId, node_cur_time + msg_buf_ext->evt.mDelay);

	airlineMgr->SetSkipListen(false);
	airlineMgr->ScheduleCommlineRX();
}

static void OTSendFrameToNode(struct msg_buf_extended *msg_buf_ext, uint32_t OtDstNodeId)
{
	fprintf(stderr, "In OTSendFrameToNode..., sim time: %ld\n", Simulator::Now().GetTimeStep());

	msg_buf_ext->evt.mEventType = OT_EVENT_TYPE_RADIO_FRAME_TO_NODE;
    msg_buf_ext->evt.mNodeId = OtDstNodeId;
    msg_buf_ext->evt.mDelay = Simulator::Now().GetTimeStep() - getNodeCurTime(msg_buf_ext->evt.mNodeId);
    printEvent(&msg_buf_ext->evt);

	fprintf(stderr, "SENDING FRAME TO NODE %d...\n", OtDstNodeId);
	cl_sendto_q(MTYPE(STACKLINE, msg_buf_ext->evt.mNodeId - 1), (msg_buf_t *)msg_buf_ext, sizeof(struct msg_buf_extended));

	// Update node's status
	setAliveNode(msg_buf_ext->evt.mNodeId);
	uint64_t node_cur_time = getNodeCurTime(msg_buf_ext->evt.mNodeId);
	setNodeCurTime(msg_buf_ext->evt.mNodeId, node_cur_time + msg_buf_ext->evt.mDelay);
}

static void DataConfirm (int id, McpsDataConfirmParams params)
{
	fprintf(stderr, "In DataConfirm..., sim time: %ld\n", Simulator::Now().GetTimeStep());
	fprintf(stderr, "Confirm status: %d\n", params.m_status);

	fprintf(stderr, "\tmsdu_handle: %d\n", params.m_msduHandle);
//	fprintf(stderr, "\tretries: %d\n", params.m_retries);
//	fprintf(stderr, "\tdstAddrMode: %d\n", params.m_dstAddrMode);
//	fprintf(stderr, "\tpktSz: %d\n", params.m_pktSz);
//	fprintf(stderr, "\taddrShortDstAddr: %d\n", addr16toid(params.m_addrShortDstAddr));

    DEFINE_MBUF(mbuf);

    struct msg_buf_extended *mbuf_ext = (struct msg_buf_extended *)mbuf;
    struct Event evt;

    //Insert data confirm status into data of event
    evt.mDataLength = 1;
    evt.mData[0] = ot_status_convert(params.m_status);
    evt.mNodeId = params.m_msduHandle; //HACK: see lrwpanSendPacket
    OtEventToWfBuf(mbuf_ext, &evt);

    fprintf(stderr, "Passing confirm, back to node %d\n", evt.mNodeId);
	OTSendTxDone(mbuf_ext, evt.mNodeId);

#if 0
    INFO << "Sending ACK status"
         << " src=" << id << " dst=" << dst_id
         << " status=" << params.m_status
         << " retries=" << (int)params.m_retries
         << " pktSize(inc mac-hdr)=" << params.m_pktSz << "\n";
    fflush(stdout);
#endif
}

static void DataIndication (int id, McpsDataIndicationParams params,
                            Ptr<Packet> p)
{
	INFO("DataIndication called!, sim time: %ld\n", Simulator::Now().GetTimeStep());
	fprintf(stderr, "LQI: %d\n", params.m_mpduLinkQuality);

//	fprintf(stderr, "\tsrcAddrMode: %d\n", params.m_srcAddrMode);
//	fprintf(stderr, "\tsrcPanId: %d\n", params.m_srcPanId);
//	fprintf(stderr, "\tsrcAddr: %d\n", addr2id(params.m_srcAddr));
//	fprintf(stderr, "\tmdstAddrMode: %d\n", params.m_dstAddrMode);
//	fprintf(stderr, "\tdstPanId: 0x%x\n", params.m_dstPanId);
//	fprintf(stderr, "\tdstAddr: %d\n", addr2id(params.m_dstAddr));
//	fprintf(stderr, "\tmpduLinkQuality: %d\n", params.m_mpduLinkQuality);
//	fprintf(stderr, "\tDSN: %d\n", params.m_dsn);

    DEFINE_MBUF(mbuf);

    struct msg_buf_extended *mbuf_ext = (struct msg_buf_extended *)mbuf;
    struct Event evt;

    uint32_t dstNodeId;
    uint32_t OtDstNodeId;
    uint32_t srcNodeId;
    uint32_t OtSrcNodeId;
    bool dispatchedByDstAddr = false;
    bool isAcknowledgement = false;

    if (p->GetSize() >= COMMLINE_MAX_BUF) {
        CERROR << "Pkt len" << p->GetSize() << " bigger than\n";
        return;
    }

    struct RadioMessage *radio_msg = (struct RadioMessage *)evt.mData;

    //Extract packet
    evt.mDataLength = p->GetSize() + sizeof(radio_msg->channel);
    radio_msg->channel = 11;
    p->CopyData(radio_msg->psdu, evt.mDataLength);

    //Add RSSI
    evt.mParam1 = (int8_t)get_rssi_from_lqi((double) params.m_mpduLinkQuality);
    OtEventToWfBuf(mbuf_ext, &evt);

    //Determine whether this is an acknowledgement or not
    isAcknowledgement = isAck(mbuf_ext);

    if(!isAcknowledgement)
    {
		fprintf(stderr, "This is not an acknowledgement\n");

		//Extract the source address
		if(getSourceAddressMode(mbuf_ext) == EXT_ADDR)
		{
			Mac64Address src_addr;
			uint64_t src_extended;
			//Extract the source address
			getSourceExtendedAddress(mbuf_ext, &src_addr);

			//Save it as a uint64_t
			src_extended = addr64toid(src_addr);

			//Deduce the nodeId of the source node
			srcNodeId = getNodeIdFromExtendedAddr(src_extended);
			OtSrcNodeId = srcNodeId + 1;
			fprintf(stderr, "EXTENDED SOURCE ADDRESS: 0x%" PRIx64 ", corresponding to node %d\n", src_extended, OtSrcNodeId);
		}
		else if(getSourceAddressMode(mbuf_ext) == SHORT_ADDR)
		{
			Mac16Address src_addr;
			uint16_t src_short;
			//Extract the source address
			getSourceShortAddress(mbuf_ext, &src_addr);

			//Save it as a uint64_t
			src_short = addr16toid(src_addr);

			//Deduce the nodeId of the source node
			srcNodeId = getNodeIdFromShortAddr(src_short);
			OtSrcNodeId = srcNodeId + 1;
			fprintf(stderr, "SHORT SOURCE ADDRESS: 0x%x, corresponding to node %d\n", src_short, OtSrcNodeId);
		}
		else
		{
			fprintf(stderr, "PROBLEM WITH SOURCE ADDRESS....???\n");
		}

		if(getDestinationAddressMode(mbuf_ext) == EXT_ADDR)
		{
			//The message should only be dispatched to the target node with the extaddr
			Mac64Address dst_addr;
			uint64_t dst_extended;

			//Extract the destination address
			getDestinationExtendedAddress(mbuf_ext, &dst_addr);

			//Save it as a uint64_t
			dst_extended = addr64toid(dst_addr);

			//Deduce the nodeId of the destination node
			dstNodeId = getNodeIdFromExtendedAddr(dst_extended);
			OtDstNodeId = dstNodeId + 1;

			fprintf(stderr, "dstAddrMode: EXT_ADDR, dst_extended: 0x%" PRIx64 ", dst_node_id: %d\n", dst_extended, OtDstNodeId);
			OTSendFrameToNode(mbuf_ext, OtDstNodeId);

			dispatchedByDstAddr = true;
		}
		else if(getDestinationAddressMode(mbuf_ext) == SHORT_ADDR)
		{
			Mac16Address dst_addr;
			uint16_t dst_short;

			//Extract the destination address
			getDestinationShortAddress(mbuf_ext, &dst_addr);

			//Save it as a uint16_t
			dst_short = addr16toid(dst_addr);

			if(dst_short != BroadcastShortAddr)
			{
				//Unicast message should only be dispatched to target node with the short address
				//Deduce the nodeId of the destination node
				dstNodeId = getNodeIdFromShortAddr(dst_short);
				OtDstNodeId = dstNodeId + 1;

				//TODO: Add ability to handle there being multiple nodes with the same short address
				fprintf(stderr, "dstAddrMod: SHORT_ADDR, dst_short: 0x%x, dst_node_id: %d\n", dst_short, OtDstNodeId);
				OTSendFrameToNode(mbuf_ext, OtDstNodeId);

				dispatchedByDstAddr = true;
			}
		}
		else
		{
			fprintf(stderr, "PROBLEM WITH DESTINATION ADDRESS PARSING...\n");
		}
    }

    if(!dispatchedByDstAddr)
    {
    	//Send to every node
    	if(isAcknowledgement)
    	{
			fprintf(stderr, "ACK: Sending to every node that is not the source node (%d in total)\n", getSpawnedNodes() - 1);
    	}
    	else
    	{
			fprintf(stderr, "BROADCAST: Sending to every node that is not the source node (%d in total)\n", getSpawnedNodes() - 1);
    	}

    	for(uint32_t i = 0; i < (uint32_t)getSpawnedNodes(); i++)
    	{
    		if(i == srcNodeId)
    		{
    			fprintf(stderr, "Skipping node %d because source node\n", srcNodeId + 1);
    			continue;
    		}
    		OTSendFrameToNode(mbuf_ext, i + 1); //+1 because node 0 in Whitefield corresponds to node 1 in OT.
    	}
    }

	airlineMgr->SetSkipListen(false);
	airlineMgr->ScheduleCommlineRX();
}

static void setShortAddress(Ptr<LrWpanNetDevice> dev, uint16_t id)
{
    Mac16Address address;
    uint8_t idBuf[2];

    idBuf[0] = (id >> 8) & 0xff;
    idBuf[1] = (id >> 0) & 0xff;
    address.CopyFrom (idBuf);
    dev->GetMac()->SetShortAddress (address);
};

static int setAllNodesParam(NodeContainer & nodes)
{
    Ptr<SingleModelSpectrumChannel> channel;
    string loss_model = CFG("lossModel");
    string del_model = CFG("delayModel");
    bool macAdd = CFG_INT("macHeaderAdd", 1);
    LrWpanSpectrumValueHelper svh;

    if (!loss_model.empty() || !del_model.empty()) {
        channel = CreateObject<SingleModelSpectrumChannel> ();
        if (!channel) {
            return FAILURE;
        }
        if (!loss_model.empty()) {
            static Ptr <PropagationLossModel> plm;
            string loss_model_param = CFG("lossModelParam");
            plm = getLossModel(loss_model, loss_model_param);
            if (!plm) {
                return FAILURE;
            }
            channel->AddPropagationLossModel(plm);
        }
        if (!del_model.empty()) {
            static Ptr <PropagationDelayModel> pdm;
            string del_model_param = CFG("delayModelParam");
            pdm = getDelayModel(del_model, del_model_param);
            if (!pdm) {
                return FAILURE;
            }
            channel->SetPropagationDelayModel(pdm);
        }
    }

	for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i) 
	{ 
		Ptr<Node> node = *i; 
		Ptr<LrWpanNetDevice> dev = node->GetDevice(0)->GetObject<LrWpanNetDevice>();
        if (!dev) {
            CERROR << "Coud not get device\n";
            continue;
        }
		dev->GetMac()->SetMacMaxFrameRetries(CFG_INT("macMaxRetry", 3));

        /* Set Callbacks */
		dev->GetMac()->SetMcpsDataConfirmCallback(
                MakeBoundCallback(DataConfirm, node->GetId()));
		dev->GetMac()->SetMcpsDataIndicationCallback(
                MakeBoundCallback (DataIndication, node->GetId()));
        setShortAddress(dev, (uint16_t)node->GetId());

        if(!macAdd) {
            dev->GetMac()->SetMacHeaderAdd(macAdd);
            //In case where stackline itself add mac header, the airline needs
            //to be set in promiscuous mode so that all the packets with
            //headers are transmitted as is to the stackline on reception
            //dev->GetMac()->SetPromiscuousMode(1);
        }
        if (!loss_model.empty() || !del_model.empty()) {
            dev->SetChannel (channel);
        }
	}
    return SUCCESS;
}

static int lrwpanSetup(ifaceCtx_t *ctx)
{
    INFO("setting up lrwpan\n");
    static LrWpanHelper lrWpanHelper;
    static NetDeviceContainer devContainer = lrWpanHelper.Install(ctx->nodes);
    lrWpanHelper.AssociateToPan (devContainer, CFG_PANID);

    INFO("Using lr-wpan as PHY\n");
    string ns3_capfile = CFG("NS3_captureFile");
    if(!ns3_capfile.empty()) {
        INFO("NS3 Capture File:%s\n", ns3_capfile.c_str());
        lrWpanHelper.EnablePcapAll (ns3_capfile, false /*promiscuous*/);
    }
    setAllNodesParam(ctx->nodes);
    return SUCCESS;
}

static int lrwpanSetTxPower(ifaceCtx_t *ctx, int id, double txpow)
{
    Ptr<LrWpanNetDevice> dev = getDev(ctx, id);
    LrWpanSpectrumValueHelper svh;
    Ptr<SpectrumValue> psd = svh.CreateTxPowerSpectralDensity (txpow, 11);

    if (!dev || !psd) {
        CERROR << "set tx power failed for lrwpan\n";
        return FAILURE;
    }
    INFO("Node:%d txpower:%f\n", id, txpow);
    dev->GetPhy()->SetTxPowerSpectralDensity(psd);
    return SUCCESS;
}

static int lrwpanSetPromiscuous(ifaceCtx_t *ctx, int id)
{
    Ptr<LrWpanNetDevice> dev = getDev(ctx, id);

    if (!dev) {
        CERROR << "get dev failed for lrwpan\n";
        return FAILURE;
    }
    INFO("Set promis mode for lr-wpan iface node:%d\n", id);
    dev->GetMac()->SetPromiscuousMode(1);
    return SUCCESS;
}

static int lrwpanSetAddress(ifaceCtx_t *ctx, int id, const char *buf, int sz)
{
    Ptr<LrWpanNetDevice> dev = getDev(ctx, id);

    if (!dev) {
        CERROR << "get dev failed for lrwpan\n";
        return FAILURE;
    }
    if (sz == 8) {
		Mac64Address address(buf);
        INFO("Setting Ext Addr:%s\n", buf);
		dev->GetMac()->SetExtendedAddress (address);
    }
    return SUCCESS;
}

static void lrwpanCleanup(ifaceCtx_t *ctx)
{
}

static int lrwpanSendPacket(ifaceCtx_t *ctx, int id, msg_buf_t *mbuf)
{
	fprintf(stderr, "In lrwpanSendPacket()\n");
	struct msg_buf_extended *mbuf_ext = (struct msg_buf_extended *)mbuf;

    McpsDataRequestParams params;
    Ptr<LrWpanNetDevice> dev = getDev(ctx, id);
    Ptr<Packet> p0;

    if (!dev) {
        CERROR << "get dev failed for lrwpan\n";
        return FAILURE;
    }

    struct RadioMessage *radio_msg = (struct RadioMessage *)mbuf_ext->evt.mData;
    p0 = Create<Packet> (radio_msg->psdu, (uint32_t)mbuf_ext->evt.mDataLength-sizeof(radio_msg->channel));

//    params.m_srcAddrMode = getSourceAddressMode(mbuf_ext);
//    params.m_dstAddrMode = getDestinationAddressMode(mbuf_ext);
//    params.m_dstPanId    = CFG_PANID;
//    params.m_dstAddr     = getDestinationAddress(mbuf_ext);
//    fprintf(stderr, "srcAddrMode: %d\n", params.m_srcAddrMode);
//    fprintf(stderr, "dstAddrMode: %d\n", params.m_dstAddrMode);
//    fprintf(stderr, "dstPanId: 0x%x\n", params.m_dstPanId);
//
//    Address dst_addr;
//    getDestinationAddress(mbuf_ext, &dst_addr);
//    if(dst_addr != NULL)
//    {
//    	params.m_dstExtAddr = dst_addr;
//    	PRINT THE ADDRESS TO VERIFY!!!!!
//    }

    //TODO: HACK which allows the DataConfirm to know which Node sent the data request.
    //Maybe figure out a better way to do this, eventually?
    params.m_msduHandle  = mbuf_ext->evt.mNodeId;

    //TODO: HACK which allows ns3 to know that this is an ACK, so it can skip the CCA.
    params.m_txOptions = isAck(mbuf_ext);
    if(params.m_txOptions)
    {
		fprintf(stderr, "mcpsdatarequest, ACKNOWLEDGEMENT\n");
    }

    delete mbuf;

//    params.m_txOptions   = TX_OPTION_NONE;
//    if(mbuf->dst_id != 0xffff) {
//        params.m_txOptions = TX_OPTION_ACK;
//    }

#if 0
    INFO << "TX DATA: "
         << " src_id=" << id
         << " dst_id=" << params.m_dstAddr
         << " pktlen=" << (int)mbuf->len
         << "\n";
    fflush(stdout);
#endif

    INFO("In lrwpanSendPacket\n");

    Simulator::ScheduleNow (&LrWpanMac::McpsDataRequest,
            dev->GetMac(), params, p0);

    return SUCCESS;
}

static int lrwpanSaveAirlineMgr (ifaceCtx_t *ctx, AirlineManager* mgr_p)
{
	fprintf(stderr, "In lrwpanSaveAirlineMgr()\n");
	airlineMgr = mgr_p;
	return 0;
}

ifaceApi_t lrwpanIface = {
    setup          : lrwpanSetup,
    setTxPower     : lrwpanSetTxPower,
    setPromiscuous : lrwpanSetPromiscuous,
    setAddress     : lrwpanSetAddress,
    sendPacket     : lrwpanSendPacket,
    cleanup        : lrwpanCleanup,
	savePtr        : lrwpanSaveAirlineMgr,
};

