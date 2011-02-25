#ifndef UNITTEST_SIGNALTRANSLATOR_H
#define UNITTEST_SIGNALTRANSLATOR_H

#include <setjmp.h>
#include <signal.h>

namespace UnitTest {

namespace {

void SignalHandler(int sig);
}

class SignalTranslator
{
public:
    SignalTranslator() 
    {
    	m_oldJumpTarget = s_jumpTarget();
        s_jumpTarget() = &m_currentJumpTarget;

        struct sigaction action;
        action.sa_flags = 0;
        action.sa_handler = SignalHandler;
        sigemptyset( &action.sa_mask );

        sigaction( SIGSEGV, &action, &m_old_SIGSEGV_action );
        sigaction( SIGFPE , &action, &m_old_SIGFPE_action  );
        sigaction( SIGTRAP, &action, &m_old_SIGTRAP_action );
        sigaction( SIGBUS , &action, &m_old_SIGBUS_action  );
        sigaction( SIGILL , &action, &m_old_SIGBUS_action  );
    }
    
    ~SignalTranslator() 
    {
		sigaction( SIGILL , &m_old_SIGBUS_action , 0 );
		sigaction( SIGBUS , &m_old_SIGBUS_action , 0 );
		sigaction( SIGTRAP, &m_old_SIGTRAP_action, 0 );
		sigaction( SIGFPE , &m_old_SIGFPE_action , 0 );
		sigaction( SIGSEGV, &m_old_SIGSEGV_action, 0 );
		s_jumpTarget() = m_oldJumpTarget;
    }

    static sigjmp_buf*& s_jumpTarget()
    {
    	static sigjmp_buf* s = 0;
    	
    	return s;
    }

private:
    sigjmp_buf m_currentJumpTarget;
    sigjmp_buf* m_oldJumpTarget;

    struct sigaction m_old_SIGFPE_action;
    struct sigaction m_old_SIGTRAP_action;
    struct sigaction m_old_SIGSEGV_action;
    struct sigaction m_old_SIGBUS_action;
    struct sigaction m_old_SIGABRT_action;
    struct sigaction m_old_SIGALRM_action;

};

namespace {


inline void SignalHandler(int sig)
{
    siglongjmp(*SignalTranslator::s_jumpTarget(), sig );
}

}

#if !defined (__GNUC__)
    #define UNITTEST_EXTENSION
#else
    #define UNITTEST_EXTENSION __extension__
#endif

#define UNITTEST_THROW_SIGNALS \
	UnitTest::SignalTranslator sig; \
	if (UNITTEST_EXTENSION sigsetjmp(*UnitTest::SignalTranslator::s_jumpTarget(), 1) != 0) \
        throw ("Unhandled system exception"); 

}

#endif
