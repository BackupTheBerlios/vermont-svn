/*
 * Vermont PCAP Exporter
 * Copyright (C) 2009 Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _PCAP_EXPORTER_FIFO_H_
#define _PCAP_EXPORTER_FIFO_H_

#include "core/Module.h"

#include <common/msg.h>

#include <string>
#include <pcap.h>
#include <ctime>
#include "PCAPExporterBase.h"
#include <common/SignalInterface.h>

class Packet;

class PCAPExporterPipe : public Module, public Destination<Packet *>, public Source<Packet *>, public PCAPExporterBase, public SignalInterface
{
public:
	PCAPExporterPipe(const std::string& file);
	~PCAPExporterPipe();
    void setPipeReaderCmd(const std::string& cmd);
    void setSigKillTimeout(int s);
	virtual void handleSigChld(int i);
	virtual void handleSigPipe(int sig);
	void setAppendDate(bool b);
	bool isRunning(int pid);
    int execCmd(std::string& cmd);
    void kill_all(int ppid);
    void kill_pid(int ppid);
    void startProcess();
    bool checkint(const char *my_string);

//    virtual void postReconfiguration();
//    virtual void preReconfiguration();
//    virtual void onReconfiguration1();
//    virtual void onReconfiguration2();
	virtual void receive(Packet* packet);
	virtual void performStart();
	virtual void performShutdown();

private:
	static void* pcapExporterSink(void* data);

	std::string logFileName;
	std::string fifoReaderCmd;
	bool appenddate;
    int fifoReaderPid;
	pcap_t* dummy;
    int sigKillTimeout;
    int fd[2];
	int child_parent_pipe[2];
	int counter;
	time_t last_check;
	volatile bool onRestart;
};


#endif
