#ifndef UNITTEST_TIMEHELPERS_H
#define UNITTEST_TIMEHELPERS_H

#include <sys/time.h>
#include <unistd.h>

namespace UnitTest {

class Timer
{
public:
    Timer()
    {
        m_startTime.tv_sec = 0;
        m_startTime.tv_usec = 0;
    }

    void Start()
    {
    	gettimeofday(&m_startTime, 0);
    }
    	
    int GetTimeInMs() const
    {
        struct timeval currentTime;
        gettimeofday(&currentTime, 0);
        int const dsecs = currentTime.tv_sec - m_startTime.tv_sec;
        int const dus = currentTime.tv_usec - m_startTime.tv_usec;
        return dsecs*1000 + dus/1000;
    }

private:
    struct timeval m_startTime;    
};


namespace TimeHelpers
{
	inline void SleepMs (int ms)
	{
	    usleep(ms * 1000);
	}
	
}


}

#endif
