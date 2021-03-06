#!/bin/bash -x
cat mapping_use.pl | sed 's/^\$output=.*/\$output="gperf";/' > mapping_use.pl.tmp
perl mapping_use.pl.tmp IPFIX-NR-Mapping.dat > output.gperf
(cat gperf.header; cat output.gperf) > gperf
gperf -D -C -t -L ANSI-C gperf > gperf.c
cat mapping_use.pl | sed 's/^\$output=.*/\$output="c";/' > mapping_use.pl.tmp
perl mapping_use.pl.tmp IPFIX-NR-Mapping.dat > output.c
(echo "// ***************************************************************************";
 echo "// ATTENTION: do not edit this file directly, edit files utils/ipfix_names.*";
 echo "// ***************************************************************************";
 echo; echo;
 cat ipfix_names.header; cat output.c; cat ipfix_names.center; cat gperf.c; 
 cat ipfix_names.footer) |
 	#sed 's/struct ipfix_identifier *IDTAB\[\]={/static const struct ipfix_identifier IPFIXTAB\[\] = {/' > ../ipfix_names.c
 	sed 's/struct ipfix_identifier \*IDTAB\[\]={/static const struct ipfix_identifier IPFIXTAB\[\] = {/' > ../ipfix_names.c
rm mapping_use.pl.tmp output.c output.gperf gperf.c
