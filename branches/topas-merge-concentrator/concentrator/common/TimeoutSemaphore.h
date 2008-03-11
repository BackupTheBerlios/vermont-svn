/*
 * PSAMP Reference Implementation
 *
 * TimeoutSemaphore.h
 *
 * Class which encapsulates a semaphore with timeout constraint
 *
 * original version: Michael Drueing <michael@drueing.de>
 * new version: Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
 */

#ifndef TIMEOUT_SEMAPHORE_H
#define TIMEOUT_SEMAPHORE_H

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#include "msg.h"
#include "Time.h"

namespace VERMONT {

class TimeoutSemaphore
{
private:
	static const int STANDARD_TIMEOUT = 100; // when no timeout is given by calling function, this amount of ms will be waited until the exitFlag is checked
	sem_t* sem;

	// variables for global management of all semaphores
	static bool 		exitFlag;

	sem_t last_sem;
public:
	TimeoutSemaphore(int initialValue = 0)
	{
	    	sem = new sem_t;
		int retval = sem_init(sem, 0, initialValue);

		if (retval != 0) {
		    THROWEXCEPTION("failed to initialize semaphore, sem_init exited with code %d", errno);
		}

	};

	virtual ~TimeoutSemaphore()
	{
	    int retval = sem_destroy(sem);
	    if (retval != 0) {
		THROWEXCEPTION("given semaphore is not valid, failed to destroy it");
	    }
	    delete sem;
	}

	// Acquire the lock if possible, or wait max. timeout_ms milliseconds
	// for the lock to become available.
	// if the timeout is reaced, return false
	// if the semaphore was successfully acquired, return true
	// a timeout of -1 means infinite wait
	inline bool wait(long timeout_ms = -1)
	{
		struct timespec ts;
		int retval;
		// globalTimeout is for wait with timeout_ms==-1 and is used to determine
		// next timeout, when exitFlag is to be checked next time
		// this method saves lots of calls to gettimeofday()
		static struct timespec globalTimeout = {0, 0};

		// if program requested to shut down, just return a failure
		if (exitFlag) return false;

		if (timeout_ms >= 0) {
		    // wait until timeout
		    addToCurTime(&ts, timeout_ms);
		    retval = sem_timedwait(sem, &ts);
		    if (retval != 0) {
			switch (errno) {
			    case EINVAL:
				    THROWEXCEPTION("semaphore is invalid");
				    return false;
				    break;
			    default:
				    // semaphore could not be aquired because of several reasons, but none are fatal
				    DPRINTFL(MSG_VDEBUG, "timedwait (<0) returned with %d", errno);
				    return false;
			}
		    }
		} else {
		    // wait and check the exitFlag regularly
		    do {
			retval = sem_timedwait(sem, &globalTimeout);
			if (retval != 0 && errno != ETIMEDOUT) {
			    switch (errno) {
				case EINVAL:
				    /*char text[1000];
				    char tmp[10];
				    strcpy(text, "last_sem: ");
				    for (unsigned int i=0; i<sizeof(sem_t); i++) {
					sprintf(tmp, "%hhX", ((char*)&sem)[i]);
					strcat(text, tmp);
				    }
				    strcat(text, ", sem: ");
				    for (unsigned int i=0; i<sizeof(sem_t); i++) {
					sprintf(tmp, "%hhX", ((char*)&last_sem)[i]);
					strcat(text, tmp);
				    }

				    msg(THROWEXCEPTION, text);*/
				    THROWEXCEPTION("semaphore is invalid");
				    return false;
				    break;
				default:
				    // semaphore could not be aquired because of several reasons, but none are fatal
				    DPRINTFL(MSG_VDEBUG, "timedwait (>=0) returned with %d", errno);
			    }
			}
			if (errno == ETIMEDOUT) {
				// calculate absolute time from timeout
				struct timespec tmp;
				addToCurTime(&tmp, STANDARD_TIMEOUT);
				// attention: next command may collide between threads, but collision does not matter
				globalTimeout = tmp;
			}

			// if program was shutdown, exit without success
			if (exitFlag) {
				DPRINTFL(MSG_VDEBUG, "exitFlag is set", errno);
			    return false;
			}

		    } while (retval != 0);

		}
		
		return true;
	}


	// like wait() but with absolute time instead of delta. makes things easier!
	// Use this instead of the above function
	inline bool waitAbs(const struct timeval &timeout)
	{
		struct timespec ts;
		int retval;

		// if program requested to shut down, just return a failure
		if (exitFlag) return false;

		TIMEVAL_TO_TIMESPEC(&timeout, &ts);

		// wait until timeout
		retval = sem_timedwait(sem, &ts);
		switch (retval) {
			case 0:
				return true;
				break;
			case EINVAL:
				THROWEXCEPTION("semaphore is invalid");
				return false;
				break;
			default:
				// semaphore could not be aquired because of several reasons, but none are fatal
				DPRINTFL(MSG_VDEBUG, "timedwait returned with %d", retval);
				return false;
		}

		// this statement should not be reached
		return false;
	}


	/**
	 * increases the semaphore's value by 1
	 */
	inline void post()
	{
	    int retval = sem_post(sem);

	    if (retval != 0) {
		THROWEXCEPTION("semaphore is invalid, sem_post returned with %d", errno);
	    }
	}

	/**
	 * returns the current value of the semaphore
	 */
	inline int getCount()
	{
	    int val;
	    int retval = sem_getvalue(sem, &val);

	    if (retval != 0) {
		THROWEXCEPTION("semaphore is invalid, sem_getvalue returned with %d", errno);
	    }
	
	    return val;
	}


	/**
	 * shuts down the semaphore: all waiting threads will be restarted
	 * and _all_ semaphores will never lock again
	 */
	inline static void shutdown()
	{
	    DPRINTF("shutting down");
	    exitFlag = true;
	}

	/**
	 * when semaphore is shut down, it can be restarted again using this method
	 */
	static void restart()
	{
	    DPRINTF("restarting");
	    exitFlag = false;
	}
};

};

#endif
