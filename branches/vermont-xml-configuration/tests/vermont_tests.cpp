#include "configuration_tests.h"

#include "msg.h"


#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>


void printSummary(const std::vector<std::string>& vec)
{
	std::cout << std::endl << std::endl
		  << "Test summaries:" << std::endl
		  << "===============" << std::endl << std::endl;
	for (unsigned i = 0; i != vec.size(); ++i) {
		std::cout << vec[i] << std::endl;
	}
	std::cout << std::endl;
}


int main(int argc, char** argv) {
	msg_setlevel(MSG_INFO);
	std::vector<std::string> summaryMessages;

	try {
		testConfiguration();
		summaryMessages.push_back("Configuration tests passed!");
	} catch (std::exception& e) {
		summaryMessages.push_back(std::string("Configuration tests failed: ") +
					  e.what());
	}

	printSummary(summaryMessages);
}
