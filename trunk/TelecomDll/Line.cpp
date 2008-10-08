#include "Telecom.h"
//------------------------------------------------------------------------------
const char *LineStateName[LINESTATE_QTY] =
{
    "Closed", 
    "Uninitialized", 
    "Idle", 
    "Blocking", 
    "Out Of Service",
    "Active"
};
//------------------------------------------------------------------------------
const char *CallStateName[CALLSTATE_QTY] =
{
    "INVALID",
    "SEIZURE", 
    "RECEIVING_DIGITS",
    "INCOMING",
    "ACCEPTING",
    "ANSWERING",
    "REJECTING",
    "CONNECTED",
    "DISCONNECTED",
    "OUTBOUND_INITIATED",
    "PLACING",
    "PROCEEDING",
    "ALERTING"
};
//------------------------------------------------------------------------------
TLine::TLine()
{
	State = LINESTATE_CLOSED;
	CallState = CALLSTATE_INVALID;
	CallDir = 0;
	m_gc_dev = -1;
	m_voice_dev = -1;
	strcpy(m_gc_dev_name, "");
	strcpy(m_voice_name, "");	
	m_call_exit_reason = CER_FAR_END;	
	m_call_established = FALSE;
	m_call_near_droped = TRUE;
}
//------------------------------------------------------------------------------
TLine::~TLine()
{
}
//------------------------------------------------------------------------------
void TLine::setState(TLineState state)
{
    mutex_lock(tel->TLISMutex);
    tel->TotalLinesInState[State]--;
    State = state;
    tel->TotalLinesInState[State]++;
    log(INF, "State: '%s', total %u line(s).", LineStateName[State], tel->TotalLinesInState[State]);
    mutex_unlock(tel->TLISMutex);
}
//------------------------------------------------------------------------------
void TLine::setCallState(TCallState state)
{
    CallState = state;
    log(INF, "Call State: '%s'", CallStateName[CallState]);
      
    switch(CallState) 
    {
    case CALLSTATE_INVALID:
        setState(LINESTATE_IDLE);
        break;
    case CALLSTATE_SEIZURE:
    case CALLSTATE_OUTBOUND_INITIATED:
        setState(LINESTATE_ACTIVE);
        break;
    }
}
//-------------------------------------------------------------------------------------
BOOL TLine::open()
{
	char msg[1024] = "";
	int result = 0;
	
	try {
		result = gc_OpenEx(&m_gc_dev, m_gc_dev_name, EV_SYNC, (void*)this);
		if (EGC_NOERR != result) {
			gcFuncReturnMsg("gc_OpenEx", result, 0, msg);
			log(ERR, msg);
			return FALSE;
		}
		if (0 == m_gc_dev) {
			log(ERR, "gc_OpenEx return gc_OpenEx=0, ERROR!");
			return FALSE;
		}
	} catch (...) {
		log(CRI, "Please check Dialogic service Running NOW!");
		return FALSE;
	}
	
	result = gc_GetVoiceH(m_gc_dev, &m_voice_dev);
	if (EGC_NOERR != result) {
		gcFuncReturnMsg("gc_GetVoiceH", result, m_gc_dev, msg);
		log(ERR, msg);
		return FALSE;
	}
	log(DBG, "line(l=0x%X, v=0x%X, this=0x%X) opened!", m_gc_dev, m_voice_dev, this);

	if (-1 == dx_setevtmsk(m_voice_dev, DM_RINGS|DM_DIGITS|DM_LCOF))
		log(DBG, "dx_setevtmsk(DM_RINGS|DM_DIGITS|DM_LCOF) return ERROR!");
	else 
		log(DBG, "dx_setevtmsk(DM_RINGS|DM_DIGITS|DM_LCOF) done!");

	OnOpened();
    return TRUE;
}
//------------------------------------------------------------------------------
void TLine::close()
{
	dx_setevtmsk(m_voice_dev, 0);
	
	if (m_gc_dev > 0) {
		if (0 != gc_Close(m_gc_dev))
			log(ERR, "gc_Close in DLine::close return ERROR!");
		m_gc_dev = -1;
	}
	return;
}
//------------------------------------------------------------------------------
BOOL TLine::playFile(const char *file, const char *DTMFabort)
{
	char buf[255] = "";
	log(INF, "playFile(%s)...", file);

	if (0 != playFileAbort2TPT(DTMFabort, m_play_tpt, 2) ||
		0 != buildIOTT(file, &m_play_iott) ||
		0 != buildXPB(&m_play_xpb))
		return FALSE;
	
	dx_clrdigbuf(m_voice_dev);
	int rc = dx_playiottdata(m_voice_dev, &m_play_iott, m_play_tpt, &m_play_xpb, EV_ASYNC);
	if (-1 == rc) {
		d4FuncReturnMsg("playFile", rc, m_voice_dev, buf);
		log(ERR, buf);
		clearIOTT(&m_play_iott);
		return FALSE;
	}
	return TRUE;
}
//------------------------------------------------------------------------------
BOOL TLine::stopPlay()
{
	int rc = 0;
	log(INF, "Stopping play file...");

	rc = dx_stopch(m_voice_dev, EV_ASYNC);
	if (-1 == rc) {
		log(ERR, "stop return ERROR!");
		return FALSE;
	}

	return TRUE;
}
//------------------------------------------------------------------------------
BOOL TLine::makeCall(const char *aANI, const char *aDNIS)
{    
	int rc = 0;
	char msg[1024] = "";
    log(INF, "making... ANI=%s, DNIS=%s", aANI, aDNIS);
	mutex_lock(MakeCallMutex);	

	rc = gc_SetCallingNum(m_gc_dev, (char*)aANI);
	if (EGC_NOERR != rc) {
		gcFuncReturnMsg(" ", rc, m_gc_dev, msg);
		log(ERR, "gc_SetCallingNum(%s)%s", aANI, msg);
	}
	rc = gc_MakeCall(m_gc_dev, &m_crn, (char*)aDNIS, NULL, 60, EV_ASYNC);	
	if (EGC_NOERR == rc) {
		log(DBG, "gc_MakeCall(%s->%s) return crn=%d", aANI, aDNIS, m_crn);
		strncpy(ANI, aANI, 63); ANI[63] = '\0';
		strncpy(DNIS,aDNIS,63); DNIS[63] = '\0';
		OnOutboundInitiated();
		mutex_unlock(MakeCallMutex);
		return TRUE;
	} else {
		gcFuncReturnMsg("gc_MakeCall", rc, m_gc_dev, msg);
		log(ERR, msg);
	}

    mutex_unlock(MakeCallMutex);
	return FALSE;
}
//------------------------------------------------------------------------------
BOOL TLine::answerCall()
{
	int rc = 0;
	char msg[1024] = "";
    log(INF, "answer call...");

	rc = gc_AnswerCall(m_crn, 0, EV_ASYNC);
	if (EGC_NOERR != rc) {
		gcFuncReturnMsg("gc_AnswerCall", rc, m_gc_dev, msg);
		log(ERR, msg);
		return FALSE;
	}
	return TRUE;
}
//------------------------------------------------------------------------------
BOOL TLine::acceptCall()
{
	int rc = 0;
	char msg[1024] = "";
    log(INF, "accept call...");	

	rc = gc_AcceptCall(m_crn, 0, EV_ASYNC);
	if (EGC_NOERR == rc) {
		gcFuncReturnMsg("gc_AcceptCall", rc, m_gc_dev, msg);
		log(ERR, msg);
		return FALSE;
	}
	return TRUE;
}
//------------------------------------------------------------------------------
BOOL TLine::rejectCall()
{
	int rc = 0;
	char msg[1024] = "";
    log(INF, "reject call...");

	OnRejecting();
	rc = gc_DropCall(m_crn, GC_CALL_REJECTED, EV_ASYNC);
	if (EGC_NOERR != rc) {
		gcFuncReturnMsg("gc_DropCall", rc, m_gc_dev, msg);
		log(ERR, msg);
		return FALSE;
	}
	return TRUE;
}
//------------------------------------------------------------------------------
BOOL TLine::hangupCall()
{
	int rc = 0;
	char msg[1024] = "";
    log(INF, "hangup call...");
	
	rc = gc_DropCall(m_crn, GC_NORMAL_CLEARING, EV_ASYNC);
	if (EGC_NOERR != rc) {
		gcFuncReturnMsg("gc_DropCall", rc, m_gc_dev, msg);
		log(ERR, msg);
		return FALSE;
	}
	return TRUE;
}
//------------------------------------------------------------------------------
BOOL TLine::releaseCall()
{
	int rc = 0;
	char msg[1024] = "";
    log(INF, "release call...");
	
	rc = gc_ReleaseCallEx(m_crn, EV_ASYNC);	
	if (EGC_NOERR != rc) {
		gcFuncReturnMsg("gc_ReleaseCallEx", rc, m_gc_dev, msg);
		log(ERR, msg);
		return FALSE;
	}
	return TRUE;
}
//------------------------------------------------------------------------------
void TLine::log(TLogLevel level, const char *msg, ...)
{
    va_list argptr;
    va_start(argptr, msg);
    log_va(level, msg, argptr);
    va_end(argptr);
}
//------------------------------------------------------------------------------
void TLine::log_va(TLogLevel level, const char *msg, va_list argptr)
{
    if(level > tel->LogLevel || msg == NULL || msg[0] == '\0') return;
    
    char buf[1024] = "";
    
    int pos = strlen(buf);
    if(EOF==vsnprintf(buf+pos, 1023, msg, argptr)) return;
    buf[1023] = '\0';
    
    tel->LogProc(level, Index, buf);
}
//------------------------------------------------------------------------------
BOOL TLine::getCallStatus(METAEVENT meta_evt)
{
	GC_INFO call_status_info = {0};

	if (EGC_NOERR != gc_ResultInfo(&meta_evt, &call_status_info)) {
		log(ERR, "gc_ResultValue in getCallStatus return ERROR!");
		return FALSE;
	}
	
	switch(call_status_info.gcValue) {
	case GCRV_BUSY:				// Line is busy
		m_call_exit_reason = CER_BUSY;
		log(DBG, "CALLSTATUS: CER_BUSY!");
		break;
	case GCRV_NOANSWER:			// event caused by no answer
	case GCRV_TIMEOUT:
		m_call_exit_reason = CER_NO_ANSWER;
		log(DBG, "CALLSTATUS: CER_NO_ANSWER!");
		break;
	case GCRV_REJECT:			// call rejected
		m_call_exit_reason = CER_NETWORK_BUSY;
		log(DBG, "CALLSTATUS: CER_BUSY!");
		break;
	default:
		m_call_exit_reason = CER_FAILED;
		log(DBG, "CALLSTATUS: CER_FAILED(0x%x)!", call_status_info.gcValue);
		break;
	}
	return TRUE;
}
//------------------------------------------------------------------------------
BOOL TLine::processD4Event(int evt_code, DX_CST* evt_datap)
{
	TTelecom* this_tel = tel;
	TLine* lp = NULL;
	char digit;
	char buf[1024];	
	char r4_evt_name[255] = "";
	int term_reason = -1;
	char term_reason_name[1024] = "";
	int page_count = 0;
	long fax_estat = 0;

	getD4EvtName(evt_code, r4_evt_name);
	
	log(DBG, "received R4 event %s on Line 0x%X", r4_evt_name, this);

	switch(evt_code) {

	case TDX_PLAY:
		term_reason = ATDX_TERMMSK(m_voice_dev);
		getTermReasonName(term_reason, term_reason_name);
		log(DBG, "TermReason is %s", term_reason_name);
		OnPlayFileDone(term_reason_name);
		break;

	case TDX_CST:
		switch(evt_datap->cst_event) {
		case DE_DIGITS:
			digit = (char)evt_datap->cst_data;
			log(DBG, "DE_DIGITS: [%c]", digit);
			OnDigitReceived(digit);
			break;
		default:
			log(DBG, "Unknow TDX_CST event: [%d], ignored!", evt_datap->cst_event);
			break;
		}
		break;
		
	case TDX_ERROR:
		d4FuncReturnMsg("LAST FUNCTION ERROR", -1, m_voice_dev, buf);
		log(ERR, buf);
		break;

	default:
		break;
	}

	return TRUE;
}
//------------------------------------------------------------------------------
BOOL TLine::processGlobeCallEvent(METAEVENT meta_evt)
{
	CRN crn = 0;
	int rc = 0;
	char msg[1024] = "";
	int voice_dev = meta_evt.evtdev;
	int evt_code = meta_evt.evttype;

	char ani[GC_ADDRSIZE] = ""; // ANI digit Buffer
	char dnis[GC_ADDRSIZE] = ""; // DNIS digit Buffer
	char caller_name[GC_ADDRSIZE] = ""; // CALLNAME Buffer

	int ip1 = 0;
	int ip2 = 0;
	int ip3 = 0;
	int ip4 = 0;

	char gc_evt_name[255] = {0};
	getGCEvtName(evt_code, gc_evt_name);
			
	/* get CRN corresponding to this event */
	if (EGC_NOERR != (rc = gc_GetCRN(&crn, &meta_evt))) {
		gcFuncReturnMsg("gc_GetCRN", rc, m_gc_dev, msg);
		log(ERR, msg);
		return FALSE;
	}

	log(DBG, "received globe call event %s on device 0x%X, crn=%d", gc_evt_name, voice_dev, crn);
//	printfEvtResultInfo(meta_evt);
	
	switch (evt_code) {
		
		//------Channel Initialization------
	case GCEV_BLOCKED:
		OnBlock();
		break;
		
	case GCEV_UNBLOCKED:
		if (EGC_NOERR != (rc = gc_WaitCall(m_gc_dev, NULL, NULL, 0, EV_ASYNC))) {
			gcFuncReturnMsg("gc_WaitCall", rc, m_gc_dev, msg);
			log(ERR, msg);
			return FALSE;
		}
		log(DBG, "gc_WaitCall...");
		OnUnblock();
		break;
		
		//------Inbound Call------
	case GCEV_DETECTED:
		OnSeizureDetected();
		break;
		
	case GCEV_OFFERED:
		//get ani,dnis
		if (EGC_NOERR != (rc = gc_GetCallInfo(crn, ORIGINATION_ADDRESS, ani))) {
			gcFuncReturnMsg("gc_GetCallInfo(ani)", rc, m_gc_dev, msg);
			log(ERR, msg);
			strcpy(ani, "NULL");
		}
		log(DBG, "\tani: %s", ani);
		if (EGC_NOERR != (rc = gc_GetCallInfo(crn, DESTINATION_ADDRESS, dnis))) {
			gcFuncReturnMsg("gc_GetCallInfo(dnis)", rc, m_gc_dev, msg);
			log(ERR, msg);
			strcpy(dnis, "NULL");
		}
		log(DBG, "\tdnis: %s", dnis);
		m_crn = crn;
		OnIncoming((const char*)ani, (const char*)dnis);
		break;
		
	case GCEV_ACCEPT:
		OnAccepting();
		break;
		
	case GCEV_ANSWERED:
		m_call_established = TRUE;
		OnAnswering();
		OnConnected("GCEV_ANSWERED");
		break;
		
		//------Outbound Call------
	case GCEV_DIALING:
		OnPlacing();
		break;
		
	case GCEV_PROCEEDING:
		OnProceeding();
		break;
		
	case GCEV_ALERTING:
		OnAlerting();
		break;
		
	case GCEV_CONNECTED:
		OnConnected("GCEV_CONNECTED");
		m_call_established = TRUE;
		break;

	case GCEV_CALLSTATUS:		
		if (TRUE == m_call_established) {
			m_call_exit_reason = CER_FAR_END;
		} 
		//analysis Outbound Call fail reason, e.g. busy or no answer
		else if (FALSE == getCallStatus(meta_evt)) {
			log(ERR, "getCallStatus when got GCEV_CALLSTATUS ERROR!");
		}
		m_call_near_droped = FALSE;
		hangupCall();
		break;
		
		//------Tear Down------
	case GCEV_DISCONNECTED:
		if (TRUE == m_call_established) {
			m_call_exit_reason = CER_FAR_END;
			log(DBG, "far end disconnected, near end will hangup call!");
		} else 
		//analysis Outbound Call fail reason, e.g. busy or no answer
		if (FALSE == getCallStatus(meta_evt)) {
			log(ERR, "getCallStatus when got GCEV_CALLSTATUS ERROR!");
		}
		m_call_near_droped = FALSE;
		hangupCall();
		break;

	case GCEV_DROPCALL:
		//analysis GCEV_DROPCALL reason		
		if (TRUE == m_call_near_droped) m_call_exit_reason = CER_NEAR_END;
		getCERName(m_call_exit_reason, msg);
		OnDisconnected(msg); 
		break;
		
	case GCEV_RELEASECALL:
		OnReleased();
		m_call_established = FALSE;
		m_call_near_droped = TRUE;
		break;
		
		//------Others------
	case GCEV_FATALERROR:
	case GCEV_TASKFAIL:
		if (EGC_NOERR != (rc = gc_ResetLineDev(m_gc_dev, EV_ASYNC))) {
			gcFuncReturnMsg("gc_ResetLineDev", rc, m_gc_dev, msg);
			log(ERR, msg);
			return FALSE;
		}
		break;
		
	case GCEV_RESETLINEDEV:
		OnClosed();
		OnOpened();
		/* Issue gc_WaitCall() */
		if (EGC_NOERR != (rc = gc_WaitCall(m_gc_dev, NULL, NULL, 0, EV_ASYNC))) {
			gcFuncReturnMsg("gc_WaitCall", rc, m_gc_dev, msg);
			log(ERR, msg);
			return FALSE;
		}
		log(DBG, "gc_WaitCall...");
		OnUnblock();
		break;

	default:
		log(DBG, "UNKNOW Event code = %d, IGNORE!", evt_code);
		
	}//end of "switch (evt_code)"
	
	return TRUE;
}
//------------------------------------------------------------------------------
void TLine::OnClosed() 
{ 
	setState(LINESTATE_CLOSED);
    mutex_destroy(MakeCallMutex);
    reportEvent("evt=LineClosed");
}
//------------------------------------------------------------------------------
void TLine::OnOpened() 
{ 
	if(State == LINESTATE_CLOSED) 
	{
		setState(LINESTATE_UNINITIALIZED);
        mutex_create(MakeCallMutex);
	}
    reportEvent("evt=LineOpened");
}
//------------------------------------------------------------------------------
void TLine::OnInitialized() 
{
	if(State == LINESTATE_UNINITIALIZED) 
	{
		setState(LINESTATE_IDLE);
	}
    reportEvent("evt=LineInitialized");
}
//------------------------------------------------------------------------------
void TLine::OnUninitialized() 
{
	setState(LINESTATE_UNINITIALIZED);
    reportEvent("evt=LineUninitialized");
}
//------------------------------------------------------------------------------
void TLine::OnBlock() 
{
	setState(LINESTATE_BLOCKING);
    reportEvent("evt=LineBlock");
}
//------------------------------------------------------------------------------
void TLine::OnUnblock() 
{ 
	if(State == LINESTATE_BLOCKING) 
	{
		setState(LINESTATE_IDLE);
	}    
    reportEvent("evt=LineUnblock");
}
//------------------------------------------------------------------------------
void TLine::OnOutOfService(const char *reason) 
{ 
	setState(LINESTATE_OUT_OF_SERVICE);
    reportEvent("evt=LineOutOfService|reason=%s", reason);
}
//------------------------------------------------------------------------------
void TLine::OnInService() 
{ 
	if(State == LINESTATE_OUT_OF_SERVICE) 
	{
		setState(LINESTATE_IDLE);
	}
    reportEvent("evt=LineInService");
}
//------------------------------------------------------------------------------
void TLine::OnPlayFileDone(const char *reason)
{
    reportEvent("evt=PlayFileDone|reason=%s", reason);
}
//-------------------------------------------------------------------------------
void TLine::OnDTMF(char dtmf)
{               
    reportEvent("evt=DTMF|DTMF=%c", dtmf);
}
//-----------------------------------------------------------------------------
void TLine::OnSeizureDetected()
{
    if(State != LINESTATE_IDLE) return;
    
    setCallState(CALLSTATE_SEIZURE);
    CallDir = 0;
    reportEvent("evt=SeizureDetected");
}
//------------------------------------------------------------------------------
void TLine::OnDigitReceived(char digit) 
{ 
    if(CallState == CALLSTATE_SEIZURE) setCallState(CALLSTATE_RECEIVING_DIGITS);
    reportEvent("evt=DigitReceived|digit=%c", digit);
}
//-----------------------------------------------------------------------------
void TLine::OnIncoming(const char *aANI, const char *aDNIS)
{
    if(CallState == CALLSTATE_SEIZURE || CallState == CALLSTATE_RECEIVING_DIGITS)
    {
        setCallState(CALLSTATE_INCOMING);
        ANI[0] = DNIS[0] = '\0';
        if(aANI  != NULL) { strncpy(ANI,  aANI,  63); ANI[63]  = '\0'; }
        if(aDNIS != NULL) { strncpy(DNIS, aDNIS, 63); DNIS[63] = '\0'; }
    }
    reportEvent("evt=IncomingCall|ANI=%s|DNIS=%s", aANI, aDNIS);
}
//------------------------------------------------------------------------------
void TLine::OnAccepting()
{
    if(CallState != CALLSTATE_INCOMING) return;
    
    setCallState(CALLSTATE_ACCEPTING);
    reportEvent("evt=CallAccepting");
}
//------------------------------------------------------------------------------
void TLine::OnRejecting()
{
    if(CallState == CALLSTATE_INCOMING || CallState == CALLSTATE_ACCEPTING)
    {
        setCallState(CALLSTATE_REJECTING);
    }
    reportEvent("evt=CallRejecting");
}
//------------------------------------------------------------------------------
void TLine::OnAnswering()
{
    if(CallState == CALLSTATE_INCOMING || CallState == CALLSTATE_ACCEPTING)
    {
        setCallState(CALLSTATE_ANSWERING);
    }
    reportEvent("evt=CallAnswering");
}
//------------------------------------------------------------------------------
void TLine::OnOutboundInitiated()
{
    if(CallState != CALLSTATE_INVALID) return;
    
    setCallState(CALLSTATE_OUTBOUND_INITIATED);
    CallDir = 1;
    reportEvent("evt=CallMaked");
}
//------------------------------------------------------------------------------
void TLine::OnPlacing()
{
    mutex_lock(MakeCallMutex);    
    mutex_unlock(MakeCallMutex);
    
    if(CallState == CALLSTATE_OUTBOUND_INITIATED) 
    {
        setCallState(CALLSTATE_PLACING);	
    }
    reportEvent("evt=CallPlacing");
}
//------------------------------------------------------------------------------
void TLine::OnProceeding()
{
    mutex_lock(MakeCallMutex);
    mutex_unlock(MakeCallMutex);
    
    if(CallState == CALLSTATE_OUTBOUND_INITIATED || CallState == CALLSTATE_PLACING) 
    {
        setCallState(CALLSTATE_PROCEEDING);
    }
    reportEvent("evt=CallProceeding");
}
//------------------------------------------------------------------------------
void TLine::OnAlerting()
{
    mutex_lock(MakeCallMutex);
    mutex_unlock(MakeCallMutex);
    
    if(CallState == CALLSTATE_OUTBOUND_INITIATED 
        || CallState == CALLSTATE_PLACING
        || CallState == CALLSTATE_PROCEEDING)
    {
        setCallState(CALLSTATE_ALERTING);
    }
    reportEvent("evt=CallAlerting");  
}
//------------------------------------------------------------------------------
void TLine::OnConnected(const char *reason)
{
    mutex_lock(MakeCallMutex);
    mutex_unlock(MakeCallMutex);
    
    if(CallState != CALLSTATE_ANSWERING
        && CallState != CALLSTATE_ACCEPTING				
        && CallState != CALLSTATE_OUTBOUND_INITIATED	
        && CallState != CALLSTATE_PLACING
        && CallState != CALLSTATE_PROCEEDING 
        && CallState != CALLSTATE_ALERTING) return;
      
    setCallState(CALLSTATE_CONNECTED);
    reportEvent("evt=CallConnected|reason=%s", reason);
}
//------------------------------------------------------------------------------
void TLine::OnDisconnected(const char *reason)
{
    mutex_lock(MakeCallMutex);
    mutex_unlock(MakeCallMutex);
    
	if(CallState == CALLSTATE_INVALID)
    {
        log(ERR, "call state invalid for disconnected event");
        return;
    }
	setCallState(CALLSTATE_DISCONNECTED);
    reportEvent("evt=CallDisconnected|reason=%s", reason);
    
    releaseCall();
}
//------------------------------------------------------------------------------
void TLine::OnReleased()
{
    mutex_lock(MakeCallMutex);
    mutex_unlock(MakeCallMutex);
    
    setCallState(CALLSTATE_INVALID);
    reportEvent("evt=CallReleased");
}
//------------------------------------------------------------------------------
void TLine::reportEvent(const char *msg, ...)
{
    va_list argptr;
    va_start(argptr, msg);

    char buf[256] = "";
    if(EOF==vsnprintf(buf, 200, msg, argptr)) 
    {
        log(ERR, "Failed to report [%s]", msg);
        return;
    }
    buf[199] = '\0';
    va_end(argptr);

    sprintf(buf + strlen(buf), "|LineState=%d|CallState=%d|CallDir=%u", State, CallState, CallDir);
    tel->EventHandler(Index, buf);
}