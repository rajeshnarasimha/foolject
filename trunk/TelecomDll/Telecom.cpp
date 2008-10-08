#include "Telecom.h"

#define PROTOCOL_DEFINE "cn_r2_io"

//---------------------------------------------------------------------------------
TTelecom *tel = NULL;
//------------------------------------------------------------------------------
const char *LogLevelName[LOGLEVEL_QTY] = {"CRI", "ERR", "WRN", "KEY", "INF", "DBG"};
//------------------------------------------------------------------------------
TTelecom::TTelecom():LogLevel(DBG)
{
    tel = this;
	gc_started_b = FALSE;
    mutex_create(TLISMutex);
}
//------------------------------------------------------------------------------
TTelecom::~TTelecom()
{
    mutex_destroy(TLISMutex);
}
//------------------------------------------------------------------------------
BOOL TTelecom::initialize(LOG_PROC logProc, EVENT_HANDLER eventHandler)
{
	unsigned group_id = 0;
	unsigned thread_id = 0;
	
    LogProc = logProc;
    EventHandler = eventHandler;

    done = FALSE;

	LOAD_GC_LIBRARY	
	int mode = SR_POLLMODE | SR_MTASYNC;
	if (sr_setparm(SRL_DEVICE, SR_MODEID, &mode) == -1) {
		log(CRI, "sr_setparm SR_MODEID=%d FAILED!", mode);
		return FALSE;
	}

	if (EGC_NOERR != gc_Start(NULL)) {
		log(CRI, "gc_Start FAILED!");
		return FALSE;
	}
	gc_started_b = TRUE;
	log(KEY, "gc_Start OK!");

    if(!openLines())
    {
        log(CRI, "No boards found!");
        return FALSE;
    }
    
    //create gc work thread	
	for (; group_id<NUM_OF_QUEUES; group_id++) {
		thread_handle[group_id] = (HANDLE)_beginthreadex(NULL, 4096, 
			(ThreadFunc)TTelecom::workThread, (void*)group_id, 0, &thread_id);
		if ((HANDLE)0 == thread_handle[group_id]) {
			log(CRI, "_beginthread return FAILED!");
			return FALSE;
		}
	}
	
	return TRUE;
}
//------------------------------------------------------------------------------
void TTelecom::uninitialize()
{
	int rc = 0;
    done = TRUE;
	WaitForMultipleObjects(NUM_OF_QUEUES, thread_handle, TRUE, INFINITE);
	closeLines();
	
	// stop GlobalCall
	if (TRUE == gc_started_b) {
		rc = gc_Stop();
		if (EGC_NOERR != rc) log(ERR, "gc_Stop return ERROR!");
		log(DBG, "gc_Stop OK!");
	}
}
//------------------------------------------------------------------------------
int TTelecom::get_voice_dev_count()
{
	int voice_dev_count = 0;
	int board_count = 0;
	int sub_dev_count = 0;
	long board_dev = 0;
	char board_name[MAX_DEV_NAME] = {0};
	
	sr_getboardcnt(DEV_CLASS_VOICE, &board_count);
	for (int i=0; i<board_count; i++) {
		sprintf(board_name, "dxxxB%d", i+1);
		board_dev = dx_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(board_dev);
		voice_dev_count += sub_dev_count;
		dx_close(board_dev);
	}
	return voice_dev_count;
}
//------------------------------------------------------------------------------
void TTelecom::configLines()
{
	char voice_dev_name[MAX_CHANNEL_COUNT][MAX_DEV_NAME] = {0};
	char dti_dev_name[MAX_CHANNEL_COUNT][MAX_DEV_NAME] = {0};
	int voice_dev_count = 0;
	int dti_dev_count = 0;

	int board_count = 0;
	char board_name[MAX_DEV_NAME] = {0};
	long board_dev = 0;
	int sub_dev_count = 0;
	int index = 0;
	int i = 0;
	int j = 0;
	int rc = 0;
	char msg[1024] = "";

	//voice dev name
	sr_getboardcnt(DEV_CLASS_VOICE, &board_count);
	for (i=0; i<board_count; i++) {
		sprintf(board_name, "dxxxB%d", i+1);
		board_dev = dx_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(board_dev);
		for (j=0; j<sub_dev_count; j++) {
			sprintf(voice_dev_name[index++], "%sC%d", board_name, j+1);
		}
		dx_close(board_dev);
	}
	voice_dev_count = index;
	log(DBG, "voice dev count is %d!", voice_dev_count);

	//dti dev name
	index = 0;
	sr_getboardcnt(DEV_CLASS_DTI, &board_count);
	for (i=0; i<board_count; i++) {
		sprintf(board_name, "dtiB%d", i+1);
		board_dev = dt_open(board_name, NULL);		
		sub_dev_count = ATDV_SUBDEVS(board_dev);
		for (j=0; j<sub_dev_count; j++) {
			sprintf(dti_dev_name[index++], "%sT%d", board_name, j+1);
		}
	}	
	dti_dev_count = index;
	log(DBG, "dti dev count is %d!", dti_dev_count);
	
	for (index=0; index<NumOfLines; index++){
		sprintf(Lines[index].m_voice_name, "%s", voice_dev_name[index]);
		sprintf(Lines[index].m_gc_dev_name, ":N_%s:P_%s:V_%s", 
			dti_dev_name[index], PROTOCOL_DEFINE, voice_dev_name[index]);
		Lines[index].Index = index;
		log(DBG, "m_gc_dev_name = %s!", Lines[index].m_gc_dev_name);
	}
	return;
}
//------------------------------------------------------------------------------
BOOL TTelecom::openLines()
{
    unsigned i;
    TotalLinesInState[LINESTATE_CLOSED] = 4;
    for(i = LINESTATE_CLOSED+1; i < LINESTATE_QTY; i++) TotalLinesInState[i] = 0;
    
    log(KEY, "Open %u lines...", NumOfLines);
    
	NumOfLines = get_voice_dev_count();
	if (NumOfLines<1) return FALSE;

	Lines.resize(NumOfLines);
	configLines();
    
	for(i = 0; i < NumOfLines; i++)
    {
        Lines[i].State = LINESTATE_CLOSED;
        if(!Lines[i].open()) 
        {
            Lines[i].log(ERR, "Open failed!");
            return FALSE;
        }      
    }
    return TRUE;
}
//------------------------------------------------------------------------------
void TTelecom::closeLines()
{
    log(KEY, "Close %u lines...", NumOfLines);
    
    for(unsigned i = 0; i < NumOfLines; i++)
    {
        Lines[i].close();
    }
    Lines.clear();
}
//------------------------------------------------------------------------------
TLine* TTelecom::getLine(unsigned index)
{
    if(index >= Lines.size()) return NULL;
    return &Lines[index];
}
//------------------------------------------------------------------------------
void TTelecom::log(TLogLevel level, const char *msg, ...)
{
    if(level>LogLevel || msg==NULL || msg[0]=='\0') return;
    
    char buf[1024] = "";
    
    va_list argptr;
    va_start(argptr, msg);
    int pos = strlen(buf);
    if(EOF == vsnprintf(buf+pos, 1023, msg, argptr)) return;
    buf[1023] = '\0';
    va_end(argptr);
    
    LogProc(level, -1, buf);
}
//------------------------------------------------------------------------------
THREAD_RET STDCALL TTelecom::workThread(void *param)
{
	TLine* pline = NULL;
	long timeout = 0;
	
	long event_handle = 0;
	int evt_dev = 0;
	int evt_code = 0;
	int evt_len = 0;
	void* evt_datap = NULL;

	unsigned i = 0;
	char tempbuf[10] = {0};
	char strlinelist[1024] = {0};
	
	METAEVENT meta_evt;
	
	long devs[MAX_CHANNEL_COUNT*2] = {0};
	unsigned group_handle_count = 0;
	unsigned group_id = (unsigned)param;

	if (NULL == tel) {
		tel->log(ERR, "tel is NULL, ERROR!");
		goto QUIT_DLG_WORK_THREAD;
	}
	
	sprintf(strlinelist, "workThread for Queue %d [line:", group_id);
	for (; i<tel->NumOfLines; i++) {
		if (group_id == (i%NUM_OF_QUEUES)) {
			pline = tel->getLine(i);
			if (NULL == pline) {
				tel->log(ERR, "getLine in dlg_work_thread return ERROR!");
				continue;
			}
			devs[group_handle_count++] = pline->m_gc_dev;
			devs[group_handle_count++] = pline->m_voice_dev;
			sprintf(tempbuf, "%d,", pline->Index);
			strcat(strlinelist, tempbuf);
		}
	}

	i = strlen(strlinelist);
	strlinelist[i-1] = '\0';
	strcat(strlinelist, "] running...");
	tel->log(DBG, strlinelist);
	
	while (TRUE) {
		
		//wait for an event
		do {
			timeout = sr_waitevtEx(devs, group_handle_count, SR_WAIT_TIME_OUT, &event_handle);
			//timeout = sr_waitevt(SR_WAIT_TIME_OUT);
			if (TRUE == tel->done) {
				goto QUIT_DLG_WORK_THREAD;
			}
		} while(timeout == SR_TMOUT);
		
		//Retrieve event related information : event type, line dev
		evt_dev = (int)sr_getevtdev(event_handle);
		evt_code = (int) sr_getevttype(event_handle);
		evt_datap = (void*) sr_getevtdatap(event_handle);
		evt_len = (int) sr_getevtlen(event_handle);

		tel->log(DBG, "[Q%03d] event occur on device 0x%X !", group_id, evt_dev);
		
		//Retrieve the Metaevent block, gc_GetMetaEventEx for WIN32 only
		if (EGC_NOERR != gc_GetMetaEventEx(&meta_evt, event_handle)) {
			tel->log(ERR, "[Q%03d] gc_GetMetaEvent return ERROR!", group_id);
			continue;
		}
		
		if (meta_evt.flags & GCME_GC_EVENT) {
			
			if (meta_evt.evtdev != evt_dev ||
				meta_evt.evttype != evt_code ||
				meta_evt.evtdatap != evt_datap ||
				meta_evt.evtlen != evt_len) {
				tel->log(CRI, "[Q%03d] gc_MetaEvent return mismatched event handle data, ERROR!", group_id);
				continue;
			}
			
			/* GCAPI event - get channel ptr corresponding to this line dev */
			if (EGC_NOERR != gc_GetUsrAttr(meta_evt.linedev, (void**)&pline)) {
				tel->log(CRI, "[Q%03d] gc_GetUsrAttr for pline return ERROR!", group_id);
				continue;
			}
			if (FALSE == pline->processGlobeCallEvent(meta_evt))
				continue;
		}
		
		else if ((evt_code & DT_DTI) == DT_DTI) {
			/* unsolicited DTI event! */
			tel->log(WRN, "[Q%03d] Unkown DTI event 0x%X on device 0x%X(%s) - Ignored\n", 
				group_id, evt_code, evt_dev, ATDV_NAMEP(evt_dev));
			continue;			
		}
		
		else {
			/* now that it must be a D40 event (fax or voice), look for corresponding channel
			   since this is not a GCAPI event.	*/
			for (i=0; i<tel->NumOfLines; i++) {
				pline = (TLine*)(tel->getLine(i));
				if (evt_dev == pline->m_voice_dev) break;
			}
			if (i == tel->NumOfLines) {
				tel->log(ERR, "[Q%03d] R4 event occur on UNKNOW Line, Ignored!", group_id);
				continue;
			} 
			tel->log(DBG, "[Q%03d] R4 event occur on pline=0x%X!", group_id, pline);
			if (FALSE == pline->processD4Event(evt_code, (DX_CST*)evt_datap))
				continue;
			
		}//end of "if (meta_evt.flags & GCME_GC_EVENT)"
		
	}//end of "while (FOOL_FOR_EVER)"
	
QUIT_DLG_WORK_THREAD:
	tel->log(DBG, "workThread for Queue %03d eixt!", group_id);
	THREAD_RETURN;
}
