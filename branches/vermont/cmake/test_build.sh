#!/bin/sh

(mkdir -p build000 && cd build000 && cmake -DDEBUG=no  -DSUPPORT_MYSQL=no  -DUSE_PCAPMMAP=no  .. && make clean && make -j 16 && cd ..) || exit 1
(mkdir -p build001 && cd build001 && cmake -DDEBUG=no  -DSUPPORT_MYSQL=no  -DUSE_PCAPMMAP=yes .. && make clean && make -j 16 && cd ..) || exit 1
(mkdir -p build010 && cd build010 && cmake -DDEBUG=no  -DSUPPORT_MYSQL=yes -DUSE_PCAPMMAP=no  .. && make clean && make -j 16 && cd ..) || exit 1
(mkdir -p build100 && cd build100 && cmake -DDEBUG=yes -DSUPPORT_MYSQL=no  -DUSE_PCAPMMAP=no  .. && make clean && make -j 16 && cd ..) || exit 1
(mkdir -p build110 && cd build110 && cmake -DDEBUG=yes -DSUPPORT_MYSQL=yes -DUSE_PCAPMMAP=no  .. && make clean && make -j 16 && cd ..) || exit 1
(mkdir -p build111 && cd build111 && cmake -DDEBUG=yes -DSUPPORT_MYSQL=yes -DUSE_PCAPMMAP=yes .. && make clean && make -j 16 && cd ..) || exit 1

