#ifndef LINE_H
#define LINE_H

#include "LogLevel.h"
#include "LineState.h"
#include "CallState.h"

//------------------------------------------------------------------------
class TLine
{
	friend class TTelecom;
    
public:
    TLine();
    ~TLine();

    BOOL open();
    void close();
	BOOL playFile(const char *filename, const char *DTMFabort);
	BOOL stopPlay();

    BOOL makeCall(const char *ANI, const char *DNIS);
    BOOL answerCall();
    BOOL acceptCall();
    BOOL rejectCall();
    BOOL hangupCall();
    BOOL releaseCall();

    void setState(TLineState state);
    void setCallState(TCallState state);   
    void log(TLogLevel level, const char *msg, ...);
    void log_va(TLogLevel level, const char *msg, va_list argptr);

    unsigned    Index;
    TLineState	State;    
    TCallState	CallState;
    unsigned    CallDir;
    char		ANI[64];
    char		DNIS[64];
	
	int m_voice_dev;	
	char m_voice_name[MAX_DEV_NAME];
	LINEDEV m_gc_dev;
	char m_gc_dev_name[MAX_DEV_NAME];
	
	DV_TPT m_play_tpt[2];
	DX_IOTT m_play_iott;
	DX_XPB m_play_xpb;

	CRN m_crn;
	BOOL m_call_established;
	BOOL m_call_near_droped;
	CallExitReason m_call_exit_reason;
	
	BOOL TLine::getCallStatus(METAEVENT meta_evt);
	BOOL processGlobeCallEvent(METAEVENT meta_evt);
	BOOL processD4Event(int evt_code, DX_CST* evt_datap);
    
private:
    mutex_t MakeCallMutex;

    void OnClosed();
    void OnOpened();
    void OnInitialized();
    void OnUninitialized();
    void OnBlock();
    void OnUnblock();
    void OnOutOfService(const char *reason);
    void OnInService();
    
    void OnPlayFileDone(const char *reason);
    void OnDTMF(char dtmf);       
    
    void OnSeizureDetected();
    void OnDigitReceived(char digit);
    void OnIncoming(const char *aANI, const char *aDNIS);             
    void OnAccepting();
    void OnRejecting();
    void OnAnswering();

    void OnOutboundInitiated();
    void OnPlacing();
    void OnProceeding();
    void OnAlerting();
    
    void OnConnected(const char *reason);     
    void OnDisconnected(const char *reason);	           
    void OnReleased();
    
    void reportEvent(const char *msg, ...);
};
//------------------------------------------------------------------------------

#endif /* LINE_H */