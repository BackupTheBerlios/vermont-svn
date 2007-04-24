/*
 Hooking Filter
 (c) by Ronny T. Lampert

 This filter serves as the interface between the
 Sampler part, which deals with Packets, and the
 Concentrator part, which deals with Flows.

 */

#ifndef HOOKING_FILTER_H
#define HOOKING_FILTER_H

#include "PacketProcessor.h"
#include "concentrator/FlowSink.hpp"

class HookingFilter : public PacketProcessor {

	public:
		static TemplateInfo ip_traffic_template;
		static TemplateInfo icmp_traffic_template;
		static TemplateInfo udp_traffic_template;
		static TemplateInfo tcp_traffic_template;

		HookingFilter(FlowSink *flowSink = 0) : flowSink(flowSink) {
		}

		virtual ~HookingFilter() {
		}

		/*
		   the hook function may need additional context.
		   in our case we need the instance of the aggregator that gets data
		   */
		void setFlowSink(FlowSink *flowSink) {
			this->flowSink=flowSink;
		}

		virtual bool processPacket(const Packet *p);

	protected:
		FlowSink *flowSink;

		static FieldInfo ip_traffic_fi[];
		static FieldInfo icmp_traffic_fi[];
		static FieldInfo udp_traffic_fi[];
		static FieldInfo tcp_traffic_fi[];
};




#endif
