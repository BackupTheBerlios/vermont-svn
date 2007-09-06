#include "CountingSemaphore.h"
#include "msg.h"
#include "Time.h"

#include <errno.h>
#include <pthread.h>

CountingSemaphore::CountingSemaphore (unsigned int startvalue)
{
	if (pthread_mutex_init (&mutex, NULL) != 0) {
		perror ("initialization of mutex failed");
		return;
	}
	if (pthread_cond_init (&cond, NULL) != 0) {
		perror ("initialization of condition variable failed");
		return;
	}
	val = startvalue;
	exitFlag = false;
}

CountingSemaphore::~CountingSemaphore () {
	if (pthread_mutex_destroy (&mutex) != 0) {
		perror ("destroy of mutex failed");
		return;
	}
	if (pthread_cond_destroy (&cond) != 0) {
		perror ("destroy of condition variable failed");
		return;
	}
}

bool CountingSemaphore::dec (unsigned int dec, long timeout_ms)
{
	if (pthread_mutex_lock (&mutex) != 0)
		THROWEXCEPTION("lock of mutex failed");

	if (timeout_ms <= 0) {
		while (val < dec) {
			if (pthread_cond_wait (&cond, &mutex) != 0)
				THROWEXCEPTION("condition wait failed");
		}
	} else {
		struct timespec timeout;

		while (val < dec) {
			int retval;
			do {
				addToCurTime(&timeout, timeout_ms);

				retval = pthread_cond_timedwait (&cond, &mutex, &timeout);
				if (retval != 0 && errno == ETIMEDOUT) {
					if (exitFlag)
						return false; // FIXME: is the lock here held?
					addToCurTime(&timeout, timeout_ms);
				} else
					THROWEXCEPTION("condition wait failed");
			} while (retval != 0);
		}
	}
	val -= dec;

	if (pthread_mutex_unlock (&mutex) != 0)
		THROWEXCEPTION("unlock of mutex failed");

	return true;
}

void CountingSemaphore::inc (unsigned int inc) {
	if (pthread_mutex_lock (&mutex) != 0) {
		THROWEXCEPTION("lock of mutex failed");
	}

	val += inc;
	if (pthread_cond_broadcast (&cond) != 0) {
		perror ("condition broadcast failed");
		return;
	}

	if (pthread_mutex_unlock (&mutex) != 0) {
		perror ("unlock of mutex failed");
		return;
	}
}
