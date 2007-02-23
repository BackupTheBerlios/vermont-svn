/**************************************************************************/
/*    Copyright (C) 2005-2007 Lothar Braun <mail@lobraun.de>              */
/*                                                                        */
/*    This library is free software; you can redistribute it and/or       */
/*    modify it under the terms of the GNU Lesser General Public          */
/*    License as published by the Free Software Foundation; either        */
/*    version 2.1 of the License, or (at your option) any later version.  */
/*                                                                        */
/*    This library is distributed in the hope that it will be useful,     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/*    Lesser General Public License for more details.                     */
/*                                                                        */
/*    You should have received a copy of the GNU Lesser General Public    */
/*    License along with this library; if not, write to the Free Software  */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA    */
/**************************************************************************/

#include "manager.h"
#include "detectmodexporter.h"


#include <commonutils/exceptions.h>
#include <concentrator/msg.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>


/* static variables */
ModuleContainer Manager::runningModules;
DetectModExporter* Manager::exporter = 0;
bool Manager::restartOnCrash = false;
bool Manager::shutdown = false;


Manager::Manager(DetectModExporter* exporter)
        : killTime(config_space::DEFAULT_KILL_TIME)
{       
        this->exporter = exporter;
        /* install signal handlers */
        if (SIG_ERR == signal(SIGALRM, sigAlarm)) {
                msg(MSG_ERROR, "Manager: Couldn't install signal handler for SIGALRM.\n"
		               "Failure in detection modules won't be detected");
        }
        if (SIG_ERR == signal(SIGCHLD, sigChild)) {
                msg(MSG_ERROR, "Manager: Couldn't install signal handler for SIGCHLD.\n"
		    "Crashing detection modules won't be detected");
        }

	mutex.lock();
}


Manager::~Manager()
{
#ifdef IDMEF_SUPPORT_ENABLED
        msg(MSG_DEBUG, "Disconnecting from xmlBlaster servers");
        for (unsigned i = 0; i != commObjs.size(); ++i) {
                commObjs[i]->disconnect();
                delete commObjs[i];
        }
#endif
}

void Manager::addDetectionModule(const std::string& modulePath,
				 const std::string& configFile,
				 std::vector<std::string>& arguments,
				 ModuleState s)
{
        if (modulePath.size() == 0) {
                msg(MSG_ERROR, "Manager: Got empty path to detection module");
        }
        
        /* cancle action when file doesn't exist */
        struct stat buffer;
        if (-1 == lstat(modulePath.c_str(), &buffer)) {
                msg(MSG_ERROR, "Can't stat %s: %s", modulePath.c_str(), strerror(errno));
                return;
        }

	arguments.insert(arguments.begin(), 1, configFile);
	availableModules[modulePath] = arguments;

	if (s == start) {
		runningModules.createModule(modulePath, arguments);
	}
}

void Manager::startModules()
{
#ifdef IDMEF_SUPPORT_ENABLED
        if (topasID.empty()) {
                throw std::runtime_error("TOPAS id is empty. Cannot start modules!");
        }
        detectionModules.topasID = config_space::TOPAS + "-" + topasID;

        /* connect to all xmlBlaster servers */
        msg(MSG_INFO, "Connecting to xmlBlaster servers");
        for (unsigned i = 0; i != xmlBlasters.size(); ++i) {
                try {
                        XmlBlasterCommObject* comm = new XmlBlasterCommObject(*xmlBlasters[i].getElement());
                        comm->connect();
                        commObjs.push_back(comm);
                } catch (const XmlBlasterException &e) {
                        msg(MSG_FATAL, "Cannot connect to xmlBlaster: ", e.what());
                        throw std::runtime_error("Make sure, the xmlBlaster server is up and running.");
                }
        }
        /* send <Heartbeat> message to all xmlBlaster servers and subscribe for update messages */
        IdmefMessage* currentMessage = new IdmefMessage(config_space::TOPAS, topasID, "", IdmefMessage::HEARTBEAT);
        for (unsigned i = 0; i != commObjs.size(); ++i) {
		std::string managerID = (*xmlBlasters[i].getElement()).getProperty().getProperty(config_space::MANAGER_ID);
                if (managerID == "") {
                        msg(MSG_INFO, ("Using default " + config_space::MANAGER_ID + " \""
                                       + config_space::DEFAULT_MANAGER_ID + "\"").c_str());
                        managerID = config_space::DEFAULT_MANAGER_ID;
                }
                currentMessage->publish(*commObjs[i], managerID);
                commObjs[i]->subscribe(config_space::TOPAS + "-" + topasID, XmlBlasterCommObject::MESSAGE);
        }
        delete currentMessage;
#endif

        runningModules.startModules(exporter);
}


void Manager::sigAlarm(int) 
{
        /* do nothing if collector announced system shutdown */
        if (shutdown)
                return;

        msg(MSG_ERROR, "Manager: One or more detection modules seem to parse their files to slowly.");
	runningModules.findAndKillSlowModule();
}

void Manager::sigChild(int sig) 
{
        if (shutdown)
                return;

        msg(MSG_ERROR, "Manager: A detection module exited.");
        int status;
        pid_t pid = wait(&status);
        if (pid == -1)
                msg(MSG_ERROR, "Manager: Can't determine exit state from "
		    "detecton module: %s", strerror(errno));

        if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                        msg(MSG_ERROR, "Manager: Detection module with pid %i"
			    "terminated with exist state 0. Not restarting module", pid);
			runningModules.setState(pid, DetectMod::NotRunning);
			return;
                }else {
                        msg(MSG_ERROR, "Manager: Detection module with pid %i "
			    "terminated abnormally. Return value was: %i", pid, status);
			runningModules.setState(pid, DetectMod::Crashed);
                }
        } else {
                if (WIFSIGNALED(status)) {
                        msg(MSG_ERROR, "Manager: Detection module with pid %i was "
			    "terminated by signal %i", pid, WTERMSIG(status));
			runningModules.setState(pid, DetectMod::Crashed);
                }
        }
        
        if (restartOnCrash) {
                msg(MSG_INFO, "Manager: Restarting crashed module");
                runningModules.restartCrashedModule(pid, exporter);
        } else {
                msg(MSG_INFO, "Manager: Not restarting crashed module");
                runningModules.deleteModule(pid);
        }
}

void* Manager::run(void* data) 
{
        Manager* man = (Manager*)data;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
        
	while (!shutdown) {
		man->lockMutex();
		
		alarm(man->killTime);
		runningModules.notifyAll(exporter);
		exporter->clearSink();
		alarm(0);
 
#ifdef IDMEF_SUPPORT_ENABLED
                for (unsigned i = 0; i != man->commObjs.size(); ++i) {
			std::string ret = man->commObjs[i]->getUpdateMessage();
                        if (ret != "") {
                                XMLConfObj* confObj = new XMLConfObj(ret, XMLConfObj::XML_STRING);
                                if (NULL != confObj) {
                                        man->update(confObj);
                                }
				delete confObj;
                        }
                }
#endif               
	}
        return NULL;
}

void Manager::newPacket() 
{
        unlockMutex();
}

void Manager::lockMutex() 
{
	mutex.lock();
}

void Manager::unlockMutex() 
{
	mutex.unlock();
}

void Manager::prepareShutdown()
{
        shutdown = true;
}

void Manager::killModules()
{
	runningModules.killDetectionModules();
}

#ifdef IDMEF_SUPPORT_ENABLED
void Manager::update(XMLConfObj* xmlObj)
{
	std::cout << "Update for topas received!" << std::endl;
        if (xmlObj->nodeExists("start")) {
		std::cout << "-> starting module..." << std::endl;
        } else if (xmlObj->nodeExists("stop")) {
		std::cout << "-> stoping module..." << std::endl;
        } else { // add your commands here
		std::cout << "-> unknown operation" << std::endl;
        }
}
#endif
