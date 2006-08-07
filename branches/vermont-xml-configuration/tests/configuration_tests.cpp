#include "configuration_tests.h"


#include "../vermont_configuration.h"


#include <iostream>
#include <stdexcept>


static void testValidFiles();
static void testErrnoneousFiles();
static void testFile(const std::string& filename);


void testConfiguration() 
{
	std::cout << "Testing with valid configuration files: " << std::endl;
	testValidFiles();
	std::cout << "Testing with erroneous configuration files: " << std::endl;
	testErrnoneousFiles();
	std::cout << "All configuration tests passed!" << std::endl;
}

void testValidFiles()
{
	try {
		std::cout << "Testing valid1.xml: ";
		testFile("valid1.xml");
		std::cout << "passed" << std::endl;
		std::cout << "Testing valid2.xml: ";
		testFile("valid2.xml");
		std::cout << " passed" << std::endl;
	} catch (std::runtime_error& e) {
		std::cout << " failed" << std::endl;
		std::cout << e.what() << std::endl;
		throw std::exception(e);
	}
}		

void testErrnoneousFiles()
{

}


void testFile(const std::string& filename)
{
	VermontConfiguration vermont_config(filename);
	vermont_config.configureObservers();
	vermont_config.configureCollectors();
	vermont_config.configureConcentrators();
	vermont_config.configureExporters();
	vermont_config.connectSubsystems();
	vermont_config.startSubsystems();
}
