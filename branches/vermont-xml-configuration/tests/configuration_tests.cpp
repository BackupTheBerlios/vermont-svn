#include "configuration_tests.h"


#include "../vermont_configuration.h"


#include <unistd.h>


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
		std::cout << "Testing valid1.xml:" << std::endl
			  << "===================" << std::endl;
		//testFile("valid1.xml");
		std::cout << "valid1.xml passed" << std::endl;
		std::cout << "Testing valid2.xml:" << std::endl
			  << "===================" << std::endl;
		testFile("valid2.xml");
		std::cout << "valid2.xml passed" << std::endl;
	} catch (std::runtime_error& e) {
		std::cout << "test failed!" << std::endl;
		std::cout << e.what() << std::endl;
		throw std::exception(e);
	}
}		

void testErrnoneousFiles()
{

}


void testFile(const std::string& filename)
{
	std::cout << "Creating VermontConfiguration object..." << std::endl;
	VermontConfiguration vermont_config(filename);
	std::cout << "Reading subsystem configuration ..." << std::endl;
	vermont_config.readSubsystemConfiguration();
	std::cout << "Connecting subsystems ..." << std::endl;
	vermont_config.connectSubsystems();
	std::cout << "Starting subsystems..." << std::endl;
	vermont_config.startSubsystems();
	//std::cout << "Up and running! Waiting for some seconds ... " << std::endl;
	//sleep(3);
	//std::cout << "cleaning up!" << std::endl;
}
