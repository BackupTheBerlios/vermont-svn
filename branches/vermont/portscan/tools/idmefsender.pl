#!/usr/bin/perl
require "IncidentManagement.pm";

use strict;
use DBI;
use XML::IDMEF;   
use IO::Socket::SSL;

if (!$ARGV[0]) {
	print "Usage: ims_idmefsender.pl <srcIP> <dstSubnet> <dstSubnetMask> <numSuccessfulConns> <numFailedConns>\n";
	exit 1;
}


my ($ip_src, $dst_subnet, $dst_subnetmask, $numsuccconns, $numfailconns)  = @ARGV;
my $idmef = new XML::IDMEF();
$idmef->create_ident();
$idmef->create_time();
$idmef->add("AlertAnalyzermodel", "Vermont portscan detector");
$idmef->add("AlertAnalyzeranalyzerid", "vermont\@vermont.rrze");
$idmef->add("AlertAnalyzerNodecategory", "hosts");
$idmef->add("AlertAnalyzerNodename", "vermont.rrze.uni-erlangen.de");
$idmef->add("AlertAnalyzerNodeAddresscategory", "ipv4-addr");
$idmef->add("AlertAnalyzerNodeAddressaddress", "131.188.2.46");

$idmef->add("AlertSourceNodecategory", "hosts");
#$idmef->add("AlertSourceNodename", );
#$idmef->add("AlertSourceNodelocation", $source_location);
$idmef->add("AlertSourceNodeAddresscategory", "ipv4-addr");
$idmef->add("AlertSourceNodeAddressaddress", $ip_src);
$idmef->add("AlertTargetNodecategory", "hosts");
#$idmef->add("AlertTargetNodename", $target_host);
#$idmef->add("AlertTargetNodelocation", $target_location);
$idmef->add("AlertTargetNodeAddresscategory", "ipv4-addr");
$idmef->add("AlertTargetNodeAddressaddress", $dst_subnet);
$idmef->add("AlertClassificationtext", "portscan");
$idmef->add("AlertClassificationident", "$numsuccconns succ. conns., $numfailconns failed conns., destination: $dst_subnet/$dst_subnetmask");
my $idmef_text = "IDMEF=".$idmef->out();
my $client = new IO::Socket::SSL("ims.rrze.uni-erlangen.de:https");

print "IDMEF MESSAGE:\n$idmef_text";
exit 0;

if (defined $client) {
	print LOG "Message: $idmef_text\n";
	print $client "POST /idmef/receiver_events.cgi HTTP/1.0\n";
	print $client "Content-type: application/x-www-form-urlencoded\n";
	print $client "Content-length: ".length($idmef_text)."\n\n";
	print $client "$idmef_text\n";
	<$client>;
	close $client;
}
else { print LOG "SLL connection failed: ", IO::Socket::SSL::errstr()."\n" }
