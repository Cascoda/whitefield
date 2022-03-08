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
 * @brief       NS3 specific airline module for managing virtual node set.
 *
 * @author      Rahul Jadhav <nyrahul@gmail.com>
 *
 * @}
 */

#define	_AIRLINEMANAGER_CC_

#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <csignal>

#include <inttypes.h>

#include <ns3/single-model-spectrum-channel.h>
#include <ns3/mobility-module.h>
#include <ns3/spectrum-value.h>

#include "AirlineManager.h"
#include "Airline.h"
#include "Command.h"
#include "mac_stats.h"
#include "IfaceHandler.h"
extern "C" {
#include "commline/ot_event_helpers.h"
}

#define OPENTHREAD_RESOLUTION Time::Unit::US

ifaceCtx_t g_ifctx;
bool g_cfg_sent[1024] = {false};
bool g_should_send_ping[1024] = {false};
bool g_first_alarm_sent[1024] = {false};

int getNodeConfigVal(int id, char *key, char *val, int vallen)
{
	wf::Nodeinfo *ni=NULL;
    ni=WF_config.get_node_info(id);
    if (!ni) {
        snprintf(val, vallen, "cudnot get nodeinfo id=%d", id);
        CERROR << "Unable to get node config\n";
        return 0;
    }
    if (!strcmp(key, "nodeexec")) {
        return snprintf(val, vallen, "%s", 
                (char *)ni->getNodeExecutable().c_str());
    }
    snprintf(val, vallen, "unknown key [%s]", key);
    CERROR << "Unknown key " << key << "\n";
    return 0;
}

int AirlineManager::cmd_node_exec(uint16_t id, char *buf, int buflen)
{
    return getNodeConfigVal(id, (char *)"nodeexec", buf, buflen);
}

int AirlineManager::cmd_node_position(uint16_t id, char *buf, int buflen)
{
	int n=0;
	ofstream of;
	NodeContainer const & nodes = NodeContainer::GetGlobal (); 
	if(buf[0]) {
		of.open(buf);
		if(!of.is_open()) {
            char tmpbuf[256];
            snprintf(tmpbuf, sizeof(tmpbuf), "%s", buf);
			return snprintf(buf, buflen, "could not open file %s", tmpbuf);
		} else {
			n = snprintf(buf, buflen, "SUCCESS");
		}
	}
	for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i) 
	{ 
		Ptr<Node> node = *i; 
		Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
		if (! mob) continue;

		Vector pos = mob->GetPosition (); 
		if(id == 0xffff || id == node->GetId()) {
			if(of.is_open()) {
				of << "Node " << node->GetId() << " Location= "
                   << pos.x << " " << pos.y << " " << pos.z << "\n"; 
			} else {
				n += snprintf(buf+n, buflen-n, "%d loc= %.2f %.2f %.2f\n",
                        node->GetId(), pos.x, pos.y, pos.z);
				if(n > (buflen-50)) {
					n += snprintf(buf+n, buflen-n, "[TRUNC]");
					break;
				}
			}
		}
	}
	of.close();
	return n;
}

void AirlineManager::setPositionAllocator(NodeContainer & nodes)
{
	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	//TODO: In the future this could support different types of mobility models

	if(CFG("topologyType") == "grid") {
		int gw=stoi(CFG("gridWidth"));
		CINFO << "Using GridPositionAllocator\n";
		mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
				"MinX", DoubleValue(.0),
				"MinY", DoubleValue(.0),
				"DeltaX", DoubleValue(stod(CFG("fieldX"))/gw),
				"DeltaY", DoubleValue(stod(CFG("fieldY"))/gw),
				"GridWidth", UintegerValue(gw),
				"LayoutType", StringValue("RowFirst"));
	} else if(CFG("topologyType") == "randrect") {
		char x_buf[128], y_buf[128];
		snprintf(x_buf, sizeof(x_buf),
                "ns3::UniformRandomVariable[Min=0.0|Max=%s]",
                CFG("fieldX").c_str());
		snprintf(y_buf, sizeof(y_buf),
                "ns3::UniformRandomVariable[Min=0.0|Max=%s]",
                CFG("fieldY").c_str());
		CINFO << "Using RandomRectanglePositionAllocator\n";
		mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
				"X", StringValue(x_buf),
				"Y", StringValue(y_buf));
	} else {
		CERROR << "Unknown topologyType: " 
              << CFG("topologyType") << " in cfg\n";
		throw FAILURE;
	}
	mobility.Install (nodes);
}

int AirlineManager::cmd_802154_set_ext_addr(uint16_t id, char *buf, int buflen)
{
    int ret = ifaceSetAddress(&g_ifctx, id, buf, buflen);
    if (ret == SUCCESS) {
        return snprintf(buf, buflen, "SUCCESS");
    }
    return snprintf(buf, buflen, "FAILURE");
}

int AirlineManager::cmd_set_node_position(uint16_t id, char *buf, int buflen)
{
	char *ptr, *saveptr;
	double x, y, z=0;
	NodeContainer const & nodes = NodeContainer::GetGlobal (); 
	int numNodes = stoi(CFG("numOfNodes"));

	if(!IN_RANGE(id, 0, numNodes)) {
		return snprintf(buf, buflen,
                "NodeID mandatory for setting node pos id=%d", id);
	}
	ptr = strtok_r(buf, " ", &saveptr);
	if(!ptr) return snprintf(buf, buflen, "invalid loc format! No x pos!");
	x=stod(ptr);
	ptr = strtok_r(NULL, " ", &saveptr);
	if(!ptr) return snprintf(buf, buflen, "invalid loc format! No y pos!");
	y=stod(ptr);
	ptr = strtok_r(NULL, " ", &saveptr);
	if(ptr) z = stod(ptr);

	Ptr<MobilityModel> mob = nodes.Get(id)->GetObject<MobilityModel>();
	Vector m_position = mob->GetPosition();
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
	mob->SetPosition(m_position);
	return snprintf(buf, buflen, "SUCCESS");
}

void AirlineManager::otSendConfigUart(const uint16_t nodeID, const string ot_config)
{
	fprintf(stderr, "In otSendConfigUart()\n");
	struct Event evt;
	struct msg_buf_extended mbuf;
	struct msg_buf_extended *mbuf_ext = &mbuf;

    evt.mDelay = 0;
    evt.mNodeId = nodeID;
    evt.mEventType = OT_EVENT_TYPE_UART_WRITE;
    evt.mDataLength = ot_config.length();
    strcpy((char *)evt.mData, ot_config.c_str());

    OtEventToWfBuf(mbuf_ext, &evt);

	cl_sendto_q(MTYPE(STACKLINE, mbuf_ext->evt.mNodeId - 1), (msg_buf_t *)mbuf_ext, sizeof(struct msg_buf_extended));
}

void AirlineManager::otSendPingUart(const uint16_t nodeID, const string ot_ping_cmd)
{
	fprintf(stderr, "In otSendPingUart()\n");
	struct Event evt;
	struct msg_buf_extended mbuf;
	struct msg_buf_extended *mbuf_ext = &mbuf;

    evt.mDelay = 0;
    evt.mNodeId = nodeID;
    evt.mEventType = OT_EVENT_TYPE_UART_WRITE;
    evt.mDataLength = ot_ping_cmd.length();
    strcpy((char *)evt.mData, ot_ping_cmd.c_str());

    OtEventToWfBuf(mbuf_ext, &evt);

	cl_sendto_q(MTYPE(STACKLINE, mbuf_ext->evt.mNodeId - 1), (msg_buf_t *)mbuf_ext, sizeof(struct msg_buf_extended));
}

void AirlineManager::OTSendAlarm(struct msg_buf_extended *mbuf_ext)
{
	fprintf(stderr, "In OTSendAlarm, sim time: %ld\n", Simulator::Now().GetTimeStep());

	// Update delay based on node's current time and send event to OT node
	if(getNodeCurTime(mbuf_ext->evt.mNodeId) > Simulator::Now().GetTimeStep())
		ERROR("ASFASEOFIJAOEJFOSEJF PROOOOOOOOOBLEM!!!!!! ========ASD=FASD=F-ASD=F-\n");

	mbuf_ext->evt.mDelay = Simulator::Now().GetTimeStep() - getNodeCurTime(mbuf_ext->evt.mNodeId);
	printEvent(&mbuf_ext->evt);

	cl_sendto_q(MTYPE(STACKLINE, mbuf_ext->evt.mNodeId - 1), (msg_buf_t *)mbuf_ext, sizeof(struct msg_buf_extended));

	// Update node's status
	setAliveNode(mbuf_ext->evt.mNodeId);
	uint64_t node_cur_time = getNodeCurTime(mbuf_ext->evt.mNodeId);
	setNodeCurTime(mbuf_ext->evt.mNodeId, node_cur_time + mbuf_ext->evt.mDelay);

	// Decide whether to process next event or listen for new incoming events
	if(Simulator::IsNextEventNow())
	{
		// Don't do anything special, cause will be processed automatically.
		INFO("PROCESS NEXT EVENT...\n");
		SetSkipListen(true);
	}
	else
	{
		// We don't want to process the next event quite yet. Instead,
		// want to go back to listening...
		INFO("BACK TO LISTENING...\n");
		SetSkipListen(false);
		ScheduleCommlineRX();
	}

	delete mbuf_ext;
}

void AirlineManager::OTFrameToSim(struct msg_buf_extended *mbuf_ext)
{
	INFO("IN OTFrameToSim\n");
	uint32_t wfNodeId = mbuf_ext->evt.mNodeId - 1; //Nodes start at 0 in Whitefield but 1 in OpenThread

	printEvent(&mbuf_ext->evt);

	//PROCESS THIS EVENT...
    ifaceSendPacket(&g_ifctx, wfNodeId, (msg_buf_t *)mbuf_ext);
    SetSkipListen(true);
}

void AirlineManager::OTProcessStatusPush(struct msg_buf_extended *mbuf_ext)
{
	INFO("In OTProcessStatusPush\n");

	char *   token;
	uint16_t rloc16;
	uint64_t ext_addr;

	uint16_t len = mbuf_ext->evt.mDataLength;
	mbuf_ext->evt.mData[len] = '\0'; //So that strtok can be used

	token = strtok((char *)mbuf_ext->evt.mData, ";");

	while(token != NULL)
	{
		fprintf(stderr, " %s\n", token);

		int ret1 = sscanf(token, "rloc16=%hd", &rloc16);
		int ret2 = sscanf(token, "extaddr=%lx", &ext_addr);

		if(ret1 == 1)
		{
			fprintf(stderr, "Extracted rloc16\n");
			setNodeShortAddr(mbuf_ext->evt.mNodeId, rloc16);
		}
		if(ret2 == 1)
		{
			fprintf(stderr, "Extracted extaddr\n");
			setNodeExtendedAddr(mbuf_ext->evt.mNodeId, ext_addr);
		}
		if(ret1 != 1 && ret2 != 1)
		{
			fprintf(stderr, "Didn't extract any information\n");
		}

		token = strtok(NULL, ";");
	}
}

void AirlineManager::OTmsgrecvCallback(msg_buf_t *mbuf)
{
	fprintf(stderr, "In OTmsgrecvCallback, sim time: %ld, aliveNodes: %d\n", Simulator::Now().GetTimeStep(), getAliveNodes());
	Time delay;

	struct msg_buf_extended *mbuf_ext = new struct msg_buf_extended;

	memcpy(mbuf_ext, mbuf, sizeof(struct msg_buf_extended));

	// Declutter logs from STATUS PUSH events
	if(mbuf_ext->evt.mEventType != OT_EVENT_TYPE_STATUS_PUSH)
	{
		printEvent(&mbuf_ext->evt);
	}

	// Configure OT node. Only happens once at the start
	if(!g_cfg_sent[mbuf_ext->evt.mNodeId - 1]) // If node not configured yet
	{
		string cfg = CFG("nodeConfig");
		otSendConfigUart(mbuf_ext->evt.mNodeId, cfg);
		g_cfg_sent[mbuf_ext->evt.mNodeId - 1] = true;
	}
	else if(g_should_send_ping[mbuf_ext->evt.mNodeId - 1] && Simulator::Now().GetTimeStep() > Seconds(10).GetTimeStep())
	{
		wf::Nodeinfo *ni = WF_config.get_node_info(mbuf_ext->evt.mNodeId - 1);
		uint8_t is_set;
		string ping_cmd = ni->getNodePing(is_set);
		otSendPingUart(mbuf_ext->evt.mNodeId, ping_cmd);
		g_should_send_ping[mbuf_ext->evt.mNodeId - 1] = false;
	}
	else
	{
		delay = MicroSeconds(mbuf_ext->evt.mDelay);

		switch (mbuf_ext->evt.mEventType) {
			case OT_EVENT_TYPE_STATUS_PUSH:
				// Deal with STATUS PUSH events immediately, no scheduling needed.
				INFO("%s RECEIVED...\n", getEventTypeName((enum EventTypes)mbuf_ext->evt.mEventType));
				AirlineManager::OTProcessStatusPush(mbuf_ext);
				break;
			case OT_EVENT_TYPE_ALARM_FIRED:
				if(!g_first_alarm_sent[mbuf_ext->evt.mNodeId - 1]) //Ignore the first ever alarm event
				{
					g_first_alarm_sent[mbuf_ext->evt.mNodeId - 1] = true;
					break;
				}
				INFO("%s SCHEDULED...\n", getEventTypeName((enum EventTypes)mbuf_ext->evt.mEventType));
				Simulator::Schedule(delay, &AirlineManager::OTSendAlarm, this, mbuf_ext);
				setAsleepNode(mbuf_ext->evt.mNodeId);
				break;
			case OT_EVENT_TYPE_RADIO_FRAME_ACK_TO_SIM:
			case OT_EVENT_TYPE_RADIO_FRAME_TO_SIM:
				INFO("%s SCHEDULED...\n", getEventTypeName((enum EventTypes)mbuf_ext->evt.mEventType));
				Simulator::Schedule(delay, &AirlineManager::OTFrameToSim, this, mbuf_ext);
				break;
			default:
				INFO("PROCESSING OF EVENT %s NOT IMPLEMENTED YET...\n", getEventTypeName((enum EventTypes)mbuf_ext->evt.mEventType));
		}
	}

	if(getAliveNodes() == 0)
	{
		INFO("PROCESSING TIME...\n");
		SetSkipListen(true);
	}
	else
	{
		fprintf(stderr, "ScheduleCommlineRx() OTmsgrecvCallback\n");
		SetSkipListen(false);
		ScheduleCommlineRX();
	}

}

void AirlineManager::msgrecvCallback(msg_buf_t *mbuf)
{
	NodeContainer const & n = NodeContainer::GetGlobal (); 
	int numNodes = stoi(CFG("numOfNodes"));

	if(mbuf->flags & MBUF_IS_CMD) {
        if(0) {}
		HANDLE_CMD(mbuf, cmd_node_exec)
		HANDLE_CMD(mbuf, cmd_node_position)
		HANDLE_CMD(mbuf, cmd_set_node_position)
		HANDLE_CMD(mbuf, cmd_802154_set_ext_addr)	
		else {
			al_handle_cmd(mbuf);
		}
        if(!(mbuf->flags & MBUF_DO_NOT_RESPOND)) {
            cl_sendto_q(MTYPE(MONITOR, CL_MGR_ID), mbuf,
                    mbuf->len+sizeof(msg_buf_t));
        }
		return;
	}
	if(!IN_RANGE(mbuf->src_id, 0, numNodes)) {
        CERROR << "rcvd src id=" << mbuf->src_id << " out of range!!\n";
		return;
	}
    if(mbuf->dst_id == CL_DSTID_MACHDR_PRESENT) {
        if(CFG_INT("macHeaderAdd", 1)) {
            CERROR << "rcvd a packet from stackline with DSTID_MACHDR_PRESENT \
                set but config file does not have macHeaderAdd=0\n";
            CERROR << "If you are using openthread, please set macHeaderAdd=0 \
                to prevent Airline from adding its own mac hdr\n";
            return;
        }
    }
    ifaceSendPacket(&g_ifctx, mbuf->src_id, mbuf);
    wf::Macstats::set_stats(AL_TX, mbuf);
}

void AirlineManager::nodePos(NodeContainer const & nodes, 
        uint16_t id, double & x, double & y, double & z)
{
	MobilityHelper mob;
	Ptr<ListPositionAllocator> positionAlloc = 
        CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (x, y, z));
	mob.SetPositionAllocator (positionAlloc);
	mob.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mob.Install(nodes.Get(id));
}

void AirlineManager::setAirlineManagerPtr(void)
{
	ifaceSaveAirlinePtr(&g_ifctx, this);
}

void AirlineManager::setNodeSpecificParam(NodeContainer & nodes)
{
	uint8_t is_set=0;
	double x, y, z;
	wf::Nodeinfo *ni=NULL;
    string txpower, deftxpower = CFG("txPower");

	for(int i=0;i<(int)nodes.GetN();i++) {
		ni = WF_config.get_node_info(i);
		if(!ni) {
			CERROR << "GetN doesnt match nodes stored in config!!\n";
			return;
		}
		ni->getNodePosition(is_set, x, y, z);
		if(is_set) {
    		nodePos(nodes, i, x, y, z);
        }
		if(ni->getPromisMode()) {
            ifaceSetPromiscuous(&g_ifctx, i);
        }
		ni->getNodePing(is_set);
		if(is_set) {
			g_should_send_ping[i] = true;
		}
        txpower = ni->getkv("txPower");
        if (txpower.empty())
            txpower = deftxpower;
        if (!txpower.empty()) {
            ifaceSetTxPower(&g_ifctx, i, stod(txpower));
        }
	}
}

void AirlineManager::setSimulationEndTime()
{
    uint64_t sim_end_time = CFG_INT("simulationEndTime", 1);
    m_simEndTimeUs = Minutes(sim_end_time);
    fprintf(stderr, "simulation due to stop in %ld us\n", m_simEndTimeUs.GetTimeStep());
}

int AirlineManager::startNetwork(wf::Config & cfg)
{
	try {
		GlobalValue::Bind ("ChecksumEnabled", 
            BooleanValue (CFG_INT("macChecksumEnabled", 1)));
		GlobalValue::Bind ("SimulatorImplementationType", 
		   StringValue ("ns3::DefaultSimulatorImpl"));

		Time::SetResolution(OPENTHREAD_RESOLUTION);

		wf::Macstats::clear();

		g_ifctx.nodes.Create (cfg.getNumberOfNodes());

		CINFO << "Creating " << cfg.getNumberOfNodes() << " nodes..\n";
		SeedManager::SetSeed(stoi(CFG("randSeed", "0xbabe"), nullptr, 0));

		setPositionAllocator(g_ifctx.nodes);

        if (ifaceInstall(&g_ifctx) != SUCCESS) {
            return FAILURE;
        }

		setNodeSpecificParam(g_ifctx.nodes);
		setAirlineManagerPtr();
		setSimulationEndTime();
		initAddressMaps();

		AirlineHelper airlineApp;
		ApplicationContainer apps = airlineApp.Install(g_ifctx.nodes);
		apps.Start(Seconds(0.0));

		ScheduleSimulationEnd();
		ScheduleCommlineRX();
		CINFO << "NS3 Simulator::Run initiated...\n";
        fflush(stdout);
        Simulator::Run();
		pause();
		Simulator::Destroy ();
	} catch (int e) {
		CERROR << "Configuration failed\n";
		return FAILURE;
	}
	return SUCCESS;
}

void AirlineManager::ScheduleCommlineRX(void)
{
	m_sendEvent = Simulator::ScheduleNow (&AirlineManager::msgReader, this);
}

void AirlineManager::SetSkipListen(bool shouldSkip)
{
	m_skip_msgrecv_listen = shouldSkip;
}

bool AirlineManager::GetSkipListen(void)
{
	return m_skip_msgrecv_listen;
}

void AirlineManager::ScheduleSimulationEnd(void)
{
	Simulator::Schedule(m_simEndTimeUs, &AirlineManager::KillSimulation, this);
}

void AirlineManager::msgReader(void)
{
	DEFINE_MBUF(mbuf);
	while(1) {
		int rcvLen = cl_recvfrom_q(MTYPE(AIRLINE,CL_MGR_ID),
                mbuf, sizeof(mbuf_buf), CL_FLAG_NOWAIT);
		if(rcvLen >= (int)sizeof(msg_buf_extended)) {
			OTmsgrecvCallback(mbuf);
			usleep(1);
		} else {
			break;
		}
	}
	if(!GetSkipListen())
	{
		ScheduleCommlineRX();
	}
}

void AirlineManager::KillSimulation(void)
{
	fprintf(stderr, "SIMULATION ENDED\n");
	raise(SIGINT);
}

AirlineManager::AirlineManager(wf::Config & cfg)
{
	m_sendEvent = EventId ();
	m_skip_msgrecv_listen = false;
	startNetwork(cfg);
	CINFO << "AirlineManager started" << endl;
}

AirlineManager::~AirlineManager() 
{
	Simulator::Cancel (m_sendEvent);
}
