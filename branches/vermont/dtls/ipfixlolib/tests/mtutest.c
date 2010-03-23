#include <stdio.h>
#include "ipfixlolib/ipfixlolib.h"
#include "common/msg.h"
#include "concentrator/ipfix.hpp"

#define OBSERVATION_DOMAIN_ID 1
#define TEMPLATE_ID 260
#define COLLECTOR_IP_ADDRESS "127.0.0.1"
// #define COLLECTOR_IP_ADDRESS "8.8.8.8"
#define COLLECTOR_PORT 4739

int main(int argc, char **argv) {
	ipfix_exporter *exporter;
	ipfix_aux_config_udp acu = {
		.mtu = 0
	};
	uint8_t data[256];
	int n = 20;
	int i,s;

	for (i=0;i<sizeof(data);i++) {
		data[i] = i;
	}

	msg_setlevel(MSG_VDEBUG);

	if (ipfix_init_exporter(OBSERVATION_DOMAIN_ID, &exporter)) {
		fprintf(stderr, "ipfix_init_exporter() failed.\n");
		exit(1);
	};
	if (ipfix_add_collector(exporter, COLLECTOR_IP_ADDRESS, COLLECTOR_PORT, UDP, &acu)) {
		fprintf(stderr, "ipfix_add_collector() failed.\n");
		exit(1);
	}
	ipfix_start_template_set(exporter, TEMPLATE_ID, 1); // field_count == 1
	ipfix_put_template_field(exporter, TEMPLATE_ID, IPFIX_TYPEID_sourceIPv4Mask, 1, 0); // length == 1, enterprise_id == 1
	ipfix_end_template_set(exporter, TEMPLATE_ID );
	ipfix_start_data_set(exporter, htons(TEMPLATE_ID));
	// while ((s = ipfix_get_remaining_space(exporter))){
		printf("Remaining space: %d\n",s);
		ipfix_put_data_field(exporter,&data[i], sizeof(*data));
	// }
	ipfix_end_data_set(exporter, 1);
	ipfix_send(exporter);
	ipfix_remove_collector(exporter, COLLECTOR_IP_ADDRESS, COLLECTOR_PORT);
	ipfix_deinit_exporter(exporter);
	return 0;
}
