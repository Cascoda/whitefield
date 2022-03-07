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
 * @brief       Node specific configuration and stats
 *
 * @author      Rahul Jadhav <nyrahul@gmail.com>
 *
 * @}
 */

#ifndef _NODEINFO_H_
#define _NODEINFO_H_

#include <common.h>
#include <mac_stats.h>
#include <cstring>

namespace wf {
class Nodeinfo : public Macstats {
private:
    string  nodeExec;
    string  nodePing;
    string  capFile;
    double  X, Y, Z;
    uint8_t pos_set;
    uint8_t promis_mode;
    uint8_t ping_set;

    map<string, string, ci_less> keyval;

public:
    string getkv(string key, string def = std::string())
    {
        if (keyval.find(key) == keyval.end())
            return def;
        return keyval[key];
    };
    void setkv(string key, string val)
    {
        keyval[key] = val;
    };
    string getNodePing(uint8_t &is_set, string def = std::string())
    {
        is_set = pos_set;
        if (!is_set)
            return def;

        return nodePing;
    };
    void setNodePing(const string ping_str)
    {
        nodePing = ping_str;

        for(size_t i = 0; i < nodePing.length(); i++)
        {
        	if(nodePing[i] == ';')
        	{
        		nodePing[i] = '\n';
        	}
        }

        ping_set = 1;
    };
    string getNodeExecutable(void)
    {
        return nodeExec;
    };
    void setNodeExecutable(const string path)
    {
        nodeExec = path;
    };
    void setNodeCaptureFile(const string path)
    {
        capFile = path;
    };
    void setPromisMode(int val)
    {
        promis_mode = !!val;
    };
    int getPromisMode(void)
    {
        return promis_mode;
    };
    void setNodePosition(const double x_pos, const double y_pos, const double z_pos)
    {
        X       = x_pos;
        Y       = y_pos;
        Z       = z_pos;
        pos_set = 1;
    };
    void getNodePosition(uint8_t &is_set, double &x_pos, double &y_pos, double &z_pos)
    {
        is_set = pos_set;
        if (!is_set)
            return;
        x_pos = X;
        y_pos = Y;
        z_pos = Z;
    };
    Nodeinfo()
    {
        pos_set     = 0;
        promis_mode = 0;
        ping_set = 0;
    };
};
} //namespace wf

#endif // _NODEINFO_H_
