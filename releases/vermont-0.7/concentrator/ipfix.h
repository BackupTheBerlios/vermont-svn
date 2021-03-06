#ifndef IPFIX_H
#define IPFIX_H

#ifdef __cplusplus
extern "C" {
#endif

#define IPFIX_LENGTH_octet 1
#define IPFIX_LENGTH_unsigned16 2
#define IPFIX_LENGTH_unsigned32 4
#define IPFIX_LENGTH_unsigned64 8
#define IPFIX_LENGTH_ipv4Address 4
#define IPFIX_LENGTH_ipv6Address 8
#define IPFIX_LENGTH_dateTimeSeconds 4

#define IPFIX_protocolIdentifier_ICMP         1
#define IPFIX_protocolIdentifier_TCP          6
#define IPFIX_protocolIdentifier_UDP         17
#define IPFIX_protocolIdentifier_RAW        255

#define IPFIX_SetId_Template                                    2
#define IPFIX_SetId_OptionsTemplate                             3
#define IPFIX_SetId_DataTemplate                                4
#define IPFIX_SetId_Data_Start                                  256

int string2typeid(char*s);
char* typeid2string(int i);
int string2typelength(char*s);

#define IPFIX_TYPEID_ipVersion                       60
#define IPFIX_TYPEID_sourceIPv4Address                8
#define IPFIX_TYPEID_sourceIPv6Address               27
#define IPFIX_TYPEID_sourceIPv4Mask                   9
#define IPFIX_TYPEID_sourceIPv6Mask                  29
#define IPFIX_TYPEID_sourceIPv4Prefix                44
#define IPFIX_TYPEID_destinationIPv4Address          12
#define IPFIX_TYPEID_destinationIPv6Address          28
#define IPFIX_TYPEID_destinationIPv4Mask             13
#define IPFIX_TYPEID_destinationIPv6Mask             30
#define IPFIX_TYPEID_destinationIPv4Prefix           45
#define IPFIX_TYPEID_classOfServiceIPv4               5
#define IPFIX_TYPEID_classOfServiceV6               137
#define IPFIX_TYPEID_flowLabelV6                     31
#define IPFIX_TYPEID_identificationV4                54
#define IPFIX_TYPEID_protocolIdentifier               4
#define IPFIX_TYPEID_sourceTransportPort              7
#define IPFIX_TYPEID_destinationtransportPort        11
#define IPFIX_TYPEID_icmpTypeCode                    32
#define IPFIX_TYPEID_igmpType                        33
#define IPFIX_TYPEID_sourceMacAddress                56
#define IPFIX_TYPEID_mplsLabelStackEntry1            70
#define IPFIX_TYPEID_mplsLabelStackEntry2            71
#define IPFIX_TYPEID_mplsLabelStackEntry3            72
#define IPFIX_TYPEID_mplsLabelStackEntry4            73
#define IPFIX_TYPEID_mplsLabelStackEntry5            74
#define IPFIX_TYPEID_mplsLabelStackEntry6            75
#define IPFIX_TYPEID_mplsLabelStackEntry7            76
#define IPFIX_TYPEID_mplsLabelStackEntry8            77
#define IPFIX_TYPEID_mplsLabelStackEntry9            78
#define IPFIX_TYPEID_mplsLabelStackEntry10           79
#define IPFIX_TYPEID_ipNextHopIPv4Address            15
#define IPFIX_TYPEID_ipNextHopIPv6Address            62
#define IPFIX_TYPEID_ingressInterface                10
#define IPFIX_TYPEID_egressInterface                 14
#define IPFIX_TYPEID_ipNextHopAsNumber              129
#define IPFIX_TYPEID_bgpSourceAsNumber               16
#define IPFIX_TYPEID_bgpDestinationAsNumber          17
#define IPFIX_TYPEID_bgpNextHopAsNumber             128
#define IPFIX_TYPEID_bgpNextHopIPv4Address           18
#define IPFIX_TYPEID_bgpNextHopIPv6Address           63
#define IPFIX_TYPEID_mplsTopLabelType                46
#define IPFIX_TYPEID_mplsTopLabelIPv4Address         47
#define IPFIX_TYPEID_mplsTopLabelIPv6Address        140
#define IPFIX_TYPEID_exporterIPv4Address            130
#define IPFIX_TYPEID_exporterIPv6Address            131
#define IPFIX_TYPEID_minPacketLength                 25
#define IPFIX_TYPEID_maxPacketLength                 26
#define IPFIX_TYPEID_minimumTTL                      52
#define IPFIX_TYPEID_maximumTTL                      53
#define IPFIX_TYPEID_ipv6OptionHeaders               64
#define IPFIX_TYPEID_tcpControlBits                   6
#define IPFIX_TYPEID_flowCreationTime                22
#define IPFIX_TYPEID_flowEndTime                     21
#define IPFIX_TYPEID_flowActiveTimeOut               36
#define IPFIX_TYPEID_flowInactiveTimeout             37
#define IPFIX_TYPEID_flowEndReason                  136
#define IPFIX_TYPEID_inOctetDeltaCount                1
#define IPFIX_TYPEID_outOctetDeltaCount              23
#define IPFIX_TYPEID_octetDeltaCount                138
#define IPFIX_TYPEID_octetTotalCount                 85
#define IPFIX_TYPEID_inPacketDeltaCount               2
#define IPFIX_TYPEID_outPacketDeltaCount             24
#define IPFIX_TYPEID_packetDeltaCount               139
#define IPFIX_TYPEID_packetTotalCount                86
#define IPFIX_TYPEID_droppedOctetDeltaCount         132
#define IPFIX_TYPEID_droppedOctetTotalCount         133
#define IPFIX_TYPEID_droppedPacketDeltaCount        134
#define IPFIX_TYPEID_droppedPacketTotalCount        135
#define IPFIX_TYPEID_outMulticastPacketCount         19
#define IPFIX_TYPEID_outMulticastOctetCount          20
#define IPFIX_TYPEID_observedFlowTotalCount           3
#define IPFIX_TYPEID_exportedOctetTotalCount         40
#define IPFIX_TYPEID_exportedPacketTotalCount        41
#define IPFIX_TYPEID_exportedFlowTotalCount          42

#define IPFIX_LENGTH_ipVersion                      IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceIPv4Address              IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_sourceIPv6Address              IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_sourceIPv4Mask                 IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceIPv6Mask                 IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceIPv4Prefix               IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_destinationIPv4Address         IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_destinationIPv6Address         IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_destinationIPv4Mask            IPFIX_LENGTH_octet
#define IPFIX_LENGTH_destinationIPv6Mask            IPFIX_LENGTH_octet
#define IPFIX_LENGTH_destinationIPv4Prefix          IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_classOfServiceIPv4             IPFIX_LENGTH_octet
#define IPFIX_LENGTH_classOfServiceV6               IPFIX_LENGTH_octet
#define IPFIX_LENGTH_flowLabelV6                    IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_identificationV4               IPFIX_LENGTH_octet
#define IPFIX_LENGTH_protocolIdentifier             IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceTransportPort            IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_destinationtransportPort       IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_icmpTypeCode                   IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_igmpType                       IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceMacAddress               IPFIX_LENGTH_octet
#define IPFIX_LENGTH_mplsLabelStackEntry1           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry2           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry3           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry4           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry5           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry6           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry7           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry8           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry9           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry10          IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_ipNextHopIPv4Address           IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_ipNextHopIPv6Address           IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_ingressInterface               IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_egressInterface                IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_ipNextHopAsNumber              IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_bgpSourceAsNumber              IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_bgpDestinationAsNumber         IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_bgpNextHopAsNumber             IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_bgpNextHopIPv4Address          IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_bgpNextHopIPv6Address          IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_mplsTopLabelType               IPFIX_LENGTH_octet
#define IPFIX_LENGTH_mplsTopLabelIPv4Address        IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_mplsTopLabelIPv6Address        IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_exporterIPv4Address            IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_exporterIPv6Address            IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_minPacketLength                IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_maxPacketLength                IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_minimumTTL                     IPFIX_LENGTH_octet
#define IPFIX_LENGTH_maximumTTL                     IPFIX_LENGTH_octet
#define IPFIX_LENGTH_ipv6OptionHeaders              IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_tcpControlBits                 IPFIX_LENGTH_octet
#define IPFIX_LENGTH_flowCreationTime               IPFIX_LENGTH_dateTimeSeconds
#define IPFIX_LENGTH_flowEndTime                    IPFIX_LENGTH_dateTimeSeconds
#define IPFIX_LENGTH_flowActiveTimeOut              IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_flowInactiveTimeout            IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_flowEndReason                  IPFIX_LENGTH_octet
#define IPFIX_LENGTH_inOctetDeltaCount              IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_outOctetDeltaCount             IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_octetDeltaCount                IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_octetTotalCount                IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_inPacketDeltaCount             IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_outPacketDeltaCount            IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_packetDeltaCount               IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_packetTotalCount               IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_droppedOctetDeltaCount         IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_droppedOctetTotalCount         IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_droppedPacketDeltaCount        IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_droppedPacketTotalCount        IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_outMulticastPacketCount        IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_outMulticastOctetCount         IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_observedFlowTotalCount         IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_exportedOctetTotalCount        IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_exportedPacketTotalCount       IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_exportedFlowTotalCount         IPFIX_LENGTH_unsigned64

#ifdef __cplusplus
}
#endif

#endif
