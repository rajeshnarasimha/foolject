#ifndef TELECOM_H
#define TELECOM_H

#pragma warning(disable:4786)

#include <winsock2.h>
#include <process.h>
#include <io.h>                 //for "access"

#define STDCALL                 __stdcall
#define vsnprintf				_vsnprintf
#define sleep_ms(t)             Sleep(t)
#define mutex_t                 CRITICAL_SECTION
#define mutex_create(obj)       InitializeCriticalSection(&(obj))
#define mutex_destroy(obj)      DeleteCriticalSection(&(obj))
#define mutex_lock(obj)         EnterCriticalSection(&(obj))
#define mutex_unlock(obj)       LeaveCriticalSection(&(obj))
#define thread_t                unsigned
#define	thread_self()		    GetCurrentThreadId()
#define THREAD_RET              unsigned
#define THREAD_RETURN           return(0)
#define FOOL_THREAD_RETURN_TYPE unsigned
typedef FOOL_THREAD_RETURN_TYPE (STDCALL *ThreadFunc)(void *);


#include <string>
#include <queue>
#include <vector>
using namespace std;

#include "dlg_ex.h"
#include "Line.h"
#include "callback.h"

#define NUM_OF_QUEUES 1

//------------------------------------------------------------------------------
class TTelecom
{
    friend class TBoard;
    friend class TLine;

public:
    TTelecom();    
    ~TTelecom();
    
    BOOL initialize(LOG_PROC logProc, EVENT_HANDLER eventHandler);
	void uninitialize();
    BOOL openLines();
    void closeLines();

	int get_voice_dev_count();
	void configLines();
	
    unsigned NumOfLines;
    std::vector<TLine> Lines;
    unsigned TotalLinesInState[LINESTATE_QTY];
	TLine* getLine(unsigned index);
    
private:
    LOG_PROC LogProc;
    EVENT_HANDLER EventHandler;

    BOOL done;
    mutex_t TLISMutex;
    TLogLevel LogLevel;
    
    void log(TLogLevel level, const char *msg, ...);
	void logCode(TLogLevel level, DWORD ctacode, const char *prompt = "");
	
    static THREAD_RET STDCALL workThread(void*);
	HANDLE thread_handle[MAX_EVENT_GROUP_COUNT];
	BOOL gc_started_b;
};

extern TTelecom *tel;
extern const char *LogLevelName[];

#endif /* TELECOM_H */
