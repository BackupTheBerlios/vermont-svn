/**************************************************************************/
/*    Copyright (C) 2005-2007 Lothar Braun <mail@lobraun.de>              */
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
/*    License along with this library; if not, write to the Free Software  */
/*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA    */
/**************************************************************************/

/**
 * @author Lothar Braun <braunl@informatik.uni-tuebingen.de>
 */

#include "examplemodule.h"

#include <iostream>
#include <cstdlib>

using namespace TOPAS;

/* demonstrates the use of libdetectionModule */
int main(int argc, char** argv) 
{
        std::cerr << "Got command line arguments: " << std::endl;
        for (int i = 0; i != argc; ++i) {
                std::cerr << "Argument " << i << ": " << argv[i] << std::endl;
        }

        if (argc == 2) {
                ExampleModule m(argv[1]);
                return m.exec();
        }

        ExampleModule m;
        return m.exec();

}
