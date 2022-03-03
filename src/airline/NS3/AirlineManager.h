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

#ifndef _AIRLINEMANAGER_H_
#define _AIRLINEMANAGER_H_

#include <common.h>
#include <Nodeinfo.h>
#include <Config.h>

#include <ns3/node-container.h>
#include <ns3/core-module.h>
extern "C" {
#include "commline/ot_event_helpers.h"
}

using namespace ns3;

class AirlineManager {
public:
    void    ScheduleCommlineRX(void);
    void    SetSkipListen(bool shouldSkip);
    bool 	GetSkipListen(void);
private:
    void    msgrecvCallback(msg_buf_t *mbuf);
    void    OTmsgrecvCallback(msg_buf_t *mbuf);
    void    OTSendAlarm(struct msg_buf_extended *mbuf);
    void    OTFrameToSim(struct msg_buf_extended *mbuf_ext);
    void    OTProcessStatusPush(struct msg_buf_extended *mbuf_ext);
    int     phyInstall(NodeContainer &nodes);
    int     startNetwork(wf::Config &cfg);
    void    nodePos(NodeContainer const &nodes, uint16_t id, double &x, double &y, double &z);
    int     cmd_node_exec(uint16_t id, char *buf, int buflen);
    int     cmd_node_position(uint16_t id, char *buf, int buflen);
    int     cmd_set_node_position(uint16_t id, char *buf, int buflen);
    int     cmd_802154_set_short_addr(uint16_t id, char *buf, int buflen);
    int     cmd_802154_set_ext_addr(uint16_t id, char *buf, int buflen);
    int     cmd_802154_set_panid(uint16_t id, char *buf, int buflen);
    void    setPositionAllocator(NodeContainer &nodes);
    void    setNodeSpecificParam(NodeContainer &nodes);
    void    setAirlineManagerPtr(void);
    int     setAllNodesParam(NodeContainer &nodes);
    void    setSimulationEndTime(void);
    void    msgReader(void);
    void    KillSimulation(void);
    void    ScheduleSimulationEnd(void);
    void    otSendConfigUart(const uint16_t nodeID, const string ot_config);
    EventId m_sendEvent;
    Time    m_simEndTimeUs;
    bool 	m_skip_msgrecv_listen;

public:
    AirlineManager(wf::Config &cfg);
    ~AirlineManager();
};

#endif //_AIRLINEMANAGER_H_
