/**************************************************************************/
/*    Copyright (C) 2007 Gerhard Muenz                                    */
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

#ifndef _MSGSTREAM_H_
#define _MSGSTREAM_H_

#include <string>
#include <sstream>
#include <iostream>

class MsgStream {
public:
	typedef enum {
	    FATAL, ERROR, WARN, INFO, DEBUG
	} MsgLevel;
	
	typedef enum {endl} MsgControl;
	
	/**
	 * Creates a new message stream.
	 */
	MsgStream() : outputLevel(ERROR), name("unknown"), printThis(false) {}

	/**
	 * Destroyes the message stream
	 */
	~MsgStream() {}

	/**
	 * Sets the messaging level.
	 * @level new messaging level
	 */
	void setOutputLevel(MsgLevel level);

	/**
	 * Sets the name of the module issuing the messages.
	 * @name new name
	 */
	void setName(std::string newname);

	/**
	 * Print a message at given messaging level.
	 * @level messaging level
	 * @msg message to print
	 */
	void print(MsgLevel level, const std::string& msg);


private:
	void printIntro(MsgLevel level);
	    
	MsgLevel outputLevel;
	std::string name;
	bool printThis;

	friend MsgStream& operator<<(MsgStream&, MsgStream::MsgLevel);
	friend MsgStream& operator<<(MsgStream&, MsgStream::MsgControl);
	friend MsgStream& operator<<(MsgStream&, const std::string&);
	friend MsgStream& operator<<(MsgStream&, int16_t);
	friend MsgStream& operator<<(MsgStream&, uint16_t);
	friend MsgStream& operator<<(MsgStream&, int32_t);
	friend MsgStream& operator<<(MsgStream&, uint32_t);
	friend MsgStream& operator<<(MsgStream&, int64_t);
	friend MsgStream& operator<<(MsgStream&, uint64_t);
};

inline MsgStream& operator<<(MsgStream& ms, MsgStream::MsgLevel input)
{
    if(input <= ms.outputLevel)
    {
	ms.printIntro(input);
	ms.printThis = true;
    }
    return ms;
}

inline MsgStream& operator<<(MsgStream& ms, MsgStream::MsgControl input)
{
    if(input == MsgStream::endl)
	std::cout << std::endl;
    ms.printThis = false;
    return ms;
}

inline MsgStream& operator<<(MsgStream& ms, const std::string& input)
{
    if(ms.printThis)
	std::cout << input;
    return ms;
}

inline MsgStream& operator<<(MsgStream& ms, int16_t input)
{
    if(ms.printThis)
	std::cout << input;
    return ms;
}

inline MsgStream& operator<<(MsgStream& ms, uint16_t input)
{
    if(ms.printThis)
	std::cout << input;
    return ms;
}

inline MsgStream& operator<<(MsgStream& ms, int32_t input)
{
    if(ms.printThis)
	std::cout << input;
    return ms;
}

inline MsgStream& operator<<(MsgStream& ms, uint32_t input)
{
    if(ms.printThis)
	std::cout << input;
    return ms;
}

inline MsgStream& operator<<(MsgStream& ms, int64_t input)
{
    if(ms.printThis)
	std::cout << input;
    return ms;
}

inline MsgStream& operator<<(MsgStream& ms, uint64_t input)
{
    if(ms.printThis)
	std::cout << input;
    return ms;
}

#endif 
