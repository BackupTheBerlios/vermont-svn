#!/usr/bin/perl
#
# 2005 (C) by Freek; part of VERMONT
#
# Quick and very dirty perl script to generate
# 1) C struct tabs with {name, size}
# 2) gperf-suitable input with hash(name) -> ID
# 3) some kind of html output
#
# Choose backend for yourself in the main while() loop


# mappings: datatype -> length, in bytes
my %DATAMAP=(
	     "octet" => 1,
	     "unsigned16" => 2,
	     "unsigned32" => 4,
	     "unsigned64" => 8,
	     "float32" => 4,
	     "dateTimeSeconds" => 4,
	     "ipv4Address" => 4,
	     "ipv6Address" => 8
);

$i=0;

#c_pre();

while(<>) {
	chomp;
	s/ //g;
	next if(/^#/);
	($nr, $id, $datat, $desc)=split(/:/);
	
	$i++;
	
	# later lookup the datalength
	#$length=$DATAMAP{$datat};
	$length=0;
	$id=lc($id);
		
	#c_ize($nr, $id, $datat, $desc);
	#html_ize($nr, $id, $datat, $desc);
	#gperf_ize($nr, $id);
}

#c_post();


sub c_pre() {
	my $name="struct ipfix_identifier *IDTAB[]={\n";
	
	print $name;
}

sub c_post() {
	print "};\n";
        print "/* written $i entries */";
}

sub c_ize() {
	my $nr=shift;
	my $id=shift;
	my $type=shift;
	my $desc=shift;
	
	$dsize=$DATAMAP{$type};
	
	if (! defined $dsize) {
		$dsize=0;
	}
	
	print "\t{ \"$id\", $nr, $dsize },\n";
}

sub gperf_ize() {
	my $nr=shift;
	my $id=shift;
	
	print "$id, $nr\n";
}



sub html_ize() {
	my $nr=shift;
	my $id=shift;
	my $datatype=shift;
	
	print "<tr>\n";
	print "\t<td>$nr</td>\n\t<td>$id</td>\n\t<td>$datatype</td>\n";
	print "</tr>\n";
}

