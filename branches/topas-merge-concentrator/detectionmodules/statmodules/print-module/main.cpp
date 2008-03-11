/**************************************************************************/
/*    Copyright (C) 2006 Romain Michalec                                  */
/*                                                                        */
/*    This library is free software; you can redistribute it and/or       */
/*    modify it under the terms of the GNU Lesser General Public          */
/*    License as published by the Free Software Foundation; either        */
/*    version 2.1 of the License, or (at your option) any later version.  */
/*                                                                        */
/*    This library is distributed in the hope that it will be useful,     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/*    Lesser General Public License for more details.                     */
/*                                                                        */
/*    You should have received a copy of the GNU Lesser General Public    */
/*    License along with this library; if not, write to the Free Software   */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,          */
/*    MA  02110-1301, USA                                                 */
/*                                                                        */
/**************************************************************************/

#include "print-main.h"

#include <iostream>
#include <cstdlib>

using namespace TOPAS;

int main(int argc, char ** argv) {

  if (argc == 2) {
    Print p(argv[1]);
    // argv[1] is supposed to be the name of a configuration file
    return p.exec();
  }

  std::cerr << "Print-module: Hey! You forgot to give me an argument!\n";
  exit(-1);
  // this is not supposed to happen; the collector should always call the
  // detection module with its configuration file as argv[1]

}
