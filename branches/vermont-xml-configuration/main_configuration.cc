#include "main_configuration.h"


MainConfiguration::MainConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint)
{
	id = configTypes::main;	
}

MainConfiguration::~MainConfiguration()
{
	
}

void MainConfiguration::configure()
{
	
}
