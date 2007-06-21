#!/bin/bash

# Creates a command file for gnuplot that plots metric-values and test-params information
# on the same graph. The following graphs will be generated:
# FOR EACH ENDPOINT-METRIC-COMBINATION:
#   FOR CUSUM:
#     g + N + alpha
#   FOR WKP:
#     metric-value + the three p-values


if [ $# != 1 ]
then
  echo "Usage: ./make-gnuplot-cmd-file.sh gnuplot_directory"
  echo "  gnuplot_directory: directory which contains the output files"
fi

cd $1
# if output file already exists, overwrite it
[ -f graphs.gnuplot ] && rm graphs.gnuplot

#
# creates gnuplot command file
#

# entries for cusumparams @ metrics
for datafile in *_metric.cusumparams.txt
do
  # set title of graph
  echo set title \'$datafile\' >> graphs.gnuplot
  # plot all graphs
  echo plot \'$datafile\' using 7:2 title \'g\' with lines, \'$datafile\' using 7:3 title \'N\' with lines,\
       \'$datafile\' using 7:4 title \'alpha\' with lines >> graphs.gnuplot
  # pause to look at plot (RETURN to resume)
  echo pause -1 >> graphs.gnuplot
done

# entries for wkpparams @ metrics
for datafile in *_metric.wkpparams.txt
do
  # WMW
  # set title of graph
  echo set title \'$datafile\' >> graphs.gnuplot
  # plot all graphs
  echo plot \'$datafile\' using 9:1 title \'Value\' with lines, \'$datafile\' using 9:2 title \'p\(wmw\)\' axes x1y2 with lines >> graphs.gnuplot
  # pause to look at plot (RETURN to resume)
  echo pause -1 >> graphs.gnuplot
  # KS
  echo set title \'$datafile\' >> graphs.gnuplot
  echo plot \'$datafile\' using 9:1 title \'Value\' with lines, \'$datafile\' using 9:4 title \'p\(ks\)\' axes x1y2 with lines >> graphs.gnuplot
  echo pause -1 >> graphs.gnuplot
  # PCS
  echo set title \'$datafile\' >> graphs.gnuplot
  echo plot \'$datafile\' using 9:1 title \'Value\' with lines, \'$datafile\' using 9:6 title \'p\(pcs\)\' axes x1y2 with lines >> graphs.gnuplot
  echo pause -1 >> graphs.gnuplot
done

