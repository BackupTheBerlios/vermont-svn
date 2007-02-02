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

#include "modulecontainer.h"
#include "detectmodexporter.h"


#include <concentrator/msg.h>


ModuleContainer::ModuleContainer()
{

}

ModuleContainer::~ModuleContainer()
{
	for (std::vector<DetectMod*>::iterator i = detectionModules.begin();
	     i != detectionModules.end(); ++i) {
		delete (*i);
	}
	detectionModules.clear();
}

void ModuleContainer::killDetectionModules()
{
        for (std::vector<DetectMod*>::iterator i = detectionModules.begin();
	     i != detectionModules.end(); ++i)
                (*i)->stopModule();
}



void ModuleContainer::restartCrashedModule(pid_t pid, DetectModExporter* exporter)
{
        for (std::vector<DetectMod*>::iterator i = detectionModules.begin();
	     i != detectionModules.end(); ++i) {
                if (pid == (*i)->getPid()) {
                        (*i)->restartCrashed();
                        exporter->sendInitData(*(*i));
                        return;
                }
        }
	msg(MSG_ERROR, "ModuleContainer::restartCrashedModule(): Could "
	    "not find module with pid %i", pid);
}

void ModuleContainer::deleteModule(pid_t pid)
{
        
}

void ModuleContainer::startModules(DetectModExporter* exporter) 
{
        for (unsigned i = 0; i != detectionModules.size(); ++i) {
                msg(MSG_INFO, "Starting module number %d: %s", i+1, 
		    detectionModules[i]->getFileName().c_str());
                exporter->installNotification(*detectionModules[i]);
                detectionModules[i]->run();
                exporter->sendInitData(*detectionModules[i]);
        }
}


void ModuleContainer::createModule(const std::string& command, const std::vector<std::string>& args)
{
	DetectMod* mod = new DetectMod(command);
	mod->setArgs(args);
	detectionModules.push_back(mod);
}

void ModuleContainer::setState(pid_t pid, DetectMod::State state)
{
	for (std::vector<DetectMod*>::iterator i = detectionModules.begin();
	     i != detectionModules.end(); ++i) {
		if ((*i)->getPid() == pid) {
			(*i)->setState(state);
			return;
		}
	}
	msg(MSG_ERROR, "ModuleContainer::setState(): Could not find module "
	    "with pid %i", pid);
}

void ModuleContainer::notifyAll(DetectModExporter* exporter)
{
	/* notify the modules */
	for (std::vector<DetectMod*>::iterator i = detectionModules.begin();
	     i != detectionModules.end(); ++i) {
		exporter->notify(*i);
	}
	/* wait for them to finish data processing */
	for (std::vector<DetectMod*>::iterator i = detectionModules.begin();
	     i != detectionModules.end(); ++i) {
		if (-1 == exporter->wait(*i)) {
                        
                }
	}
}