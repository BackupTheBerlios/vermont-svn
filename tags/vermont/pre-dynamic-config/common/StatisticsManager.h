#if !defined(STATISTICS_MANAGER_H)
#define STATISTICS_MANAGER_H

#include "Mutex.h"
#include "Thread.h"

#include <list>
#include <string>


// statistics output
class StatisticsModule
{
	public:
		virtual ~StatisticsModule() {}
		virtual std::string getStatistics() = 0;
};

class StatisticsManager : Thread
{
	private:	
		std::list<StatisticsModule*> statModules;
		unsigned long			interval;
		Mutex					mutex;
		std::string					outputFile;

		StatisticsManager();
		static void* threadWrapper(void* sm);
		void runStats();

	public:
		virtual ~StatisticsManager();
		static StatisticsManager& getInstance();
		void addModule(StatisticsModule* statmodule);
		void removeModule(StatisticsModule* statmodule);
		void start();
		void stop();
		void setInterval(long milliseconds);
		void setOutput(const std::string& output);

};


#endif 
