/*
 * FPA PCAP Exporter
 * Copyright (C) 2009 Tobias Limmer <http://www.cs.fau.de/~limmer>
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
#ifndef FPA_PCAP_EXPORTER_CFG_H_
#define FPA_PCAP_EXPORTER_CFG_H_

#include "core/Cfg.h"
#include "FpaPcapExporter.h"

#include <vector>


class FpaPcapExporterCfg
	: public CfgHelper<FpaPcapExporter, FpaPcapExporterCfg>
{
	friend class ConfigManager;
public:
	virtual ~FpaPcapExporterCfg();

	virtual FpaPcapExporterCfg* create(XMLElement* elem);

	virtual FpaPcapExporter* createInstance();

	bool deriveFrom(FpaPcapExporterCfg* old);

protected:
	FpaPcapExporterCfg(XMLElement* elem);

private:
	std::string fileName;
	int link_type;
	int snaplen;
};


#endif
