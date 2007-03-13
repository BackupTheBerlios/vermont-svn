#include "sw-base.h"

#include <iostream>
#include <cstdlib>

int main(int argc, char ** argv) {

  if (argc == 2) {
    SWBase s(argv[1]);
    return s.exec();
  }

  std::cerr<<"Hey! You forgot to give an argument to the detection module!\n";
  exit(-1);
  // this is not supposed to happen; the collector should always call the
  // detection module with its configuration file as argv[1]

}
