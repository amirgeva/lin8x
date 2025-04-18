#include <utils.h>
#include <time.h>


uint millis()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}


