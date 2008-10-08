#include "dlg_ex.h"

int playFileAbort2TPT(const char* abort, DV_TPT* tpt, int size)
{
	int index = 0;

	if (NULL != tpt)
		clearTPT(tpt, size);

	if ('\0' != abort[0]) {
		tpt[index].tp_type = IO_CONT;
		tpt[index].tp_termno = DX_DIGMASK;
		tpt[index].tp_length = (0 == strcmp(abort, "*#")?0x1800:0xFFFF);
		tpt[index].tp_flags = TF_DIGMASK;
		index++;
	}

	tpt[index].tp_type = IO_EOT;
	tpt[index].tp_termno = DX_MAXTIME;
	tpt[index].tp_length = 65533;
	tpt[index].tp_flags = TF_MAXTIME;

	return 0;
}

int clearTPT(DV_TPT* tpt, int size)
{
	for(int i=0; i<size-1; i++) tpt[i].tp_type = IO_CONT;
	tpt[i].tp_type = IO_EOT;

	dx_clrtpt(tpt, size);
	if (NULL != tpt) {
		memset(tpt, 0, size*sizeof(DV_TPT));
	}
	return 0;
}

int buildIOTT(const char* file, DX_IOTT* iott, IO_Dirction dir)
{
	if (NULL != iott)
		clearIOTT(iott);

	if (PLAY_FILE == dir)
		iott->io_fhandle = dx_fileopen(file, _O_RDONLY|_O_BINARY);
	else if (RECORD_FILE == dir)
		iott->io_fhandle = dx_fileopen(file, _O_RDWR|_O_BINARY|_O_CREAT|_O_TRUNC, 0666);
	else
		return -1;
	
	if (-1 == iott->io_fhandle)
		return -2;
	
	iott->io_type = IO_DEV|IO_EOT;
	iott->io_bufp = 0;
	iott->io_offset = 0;
	iott->io_length = -1;

	return 0;
}

int clearIOTT(DX_IOTT* iott)
{
	if (NULL != iott) {
		dx_fileclose(iott->io_fhandle);
		memset(iott, 0 , sizeof(DX_IOTT));
	}
	return 0;
}

int buildXPB(DX_XPB* xpb)
{
	if (NULL != xpb)
		clearXPB(xpb);
	
	xpb->wBitsPerSample = 8;
	xpb->nSamplesPerSec = DRT_8KHZ;
	xpb->wFileFormat = FILE_FORMAT_WAVE;
	xpb->wDataFormat = DATA_FORMAT_MULAW;
	return 0;
}

int clearXPB(DX_XPB* xpb)
{
	if (NULL != xpb) {
		memset(xpb, 0, sizeof(DX_XPB));
	}
	return 0;
}

char* getGCEvtName(DWORD value, char* buf)
{
	sprintf(buf, "%s", GCEV_MSG(value));
	return buf;
}

char* getCERName(CallExitReason cer, char*buf)
{
	switch(cer) {
		_FOOL_PRINT_CASE_(CER_NEAR_END);
		_FOOL_PRINT_CASE_(CER_FAR_END);
		_FOOL_PRINT_CASE_(CER_BUSY);
		_FOOL_PRINT_CASE_(CER_NETWORK_BUSY);
		_FOOL_PRINT_CASE_(CER_NO_ANSWER);
		_FOOL_PRINT_CASE_(CER_FAILED);
	default: sprintf(buf, "UNKNOWN_R4_EVT(0x%04lx)", cer); break;
	}
	return buf;
}

char* getD4EvtName(DWORD value, char* buf)
{
	switch(value) {
		_FOOL_PRINT_CASE_(TDX_PLAY);
		_FOOL_PRINT_CASE_(TDX_RECORD);
		_FOOL_PRINT_CASE_(TDX_DIAL);
		_FOOL_PRINT_CASE_(TDX_CST);
		_FOOL_PRINT_CASE_(TDX_ERROR);
		_FOOL_PRINT_CASE_(TDX_BARGEIN);
		_FOOL_PRINT_CASE_(TDX_GETDIG);	
		_FOOL_PRINT_CASE_(TDX_CALLP);	
		_FOOL_PRINT_CASE_(TDX_SETHOOK);	
		_FOOL_PRINT_CASE_(TDX_WINK);			
		_FOOL_PRINT_CASE_(TDX_PLAYTONE);	
		_FOOL_PRINT_CASE_(TDX_GETR2MF);	
		_FOOL_PRINT_CASE_(TDX_TXDATA);	
		_FOOL_PRINT_CASE_(TDX_NOSTOP);	
		_FOOL_PRINT_CASE_(TDX_RXDATA);	
		_FOOL_PRINT_CASE_(TDX_UNKNOWN);	
		
		_FOOL_PRINT_CASE_(TEC_STREAM);
		_FOOL_PRINT_CASE_(TEC_VAD);

		//fax related event
		_FOOL_PRINT_CASE_(TFX_PHASEB);
		_FOOL_PRINT_CASE_(TFX_PHASED);
		_FOOL_PRINT_CASE_(TFX_FAXSEND);
		_FOOL_PRINT_CASE_(TFX_FAXRECV);
		_FOOL_PRINT_CASE_(TFX_FAXERROR);
	default: sprintf(buf, "UNKNOWN_R4_EVT(0x%04lx)", value); break;
	}
	return buf;
}

char* getTermReasonName(DWORD value, char* buf)
{
	sprintf(buf, "0x%06lx:", value);
	_FOOL_PRINT_BIT_(TM_MAXDTMF);
	_FOOL_PRINT_BIT_(TM_MAXSIL);
	_FOOL_PRINT_BIT_(TM_MAXNOSIL);
	_FOOL_PRINT_BIT_(TM_LCOFF);
	_FOOL_PRINT_BIT_(TM_IDDTIME);
	_FOOL_PRINT_BIT_(TM_MAXTIME);
	_FOOL_PRINT_BIT_(TM_DIGIT);		
	_FOOL_PRINT_BIT_(TM_PATTERN);
	_FOOL_PRINT_BIT_(TM_USRSTOP);
	_FOOL_PRINT_BIT_(TM_EOD);
	_FOOL_PRINT_BIT_(TM_TONE);
	_FOOL_PRINT_BIT_(TM_BARGEIN);
	_FOOL_PRINT_BIT_(TM_ERROR);
	_FOOL_PRINT_BIT_(TM_MAXDATA);
	return buf;
}

char* getConnectTypeName(int value, char* buf)
{
	switch(value) {
		_FOOL_PRINT_CASE_(GCCT_CAD);
		_FOOL_PRINT_CASE_(GCCT_LPC);
		_FOOL_PRINT_CASE_(GCCT_PVD);
		_FOOL_PRINT_CASE_(GCCT_PAMD);
		_FOOL_PRINT_CASE_(GCCT_FAX);
		_FOOL_PRINT_CASE_(GCCT_NA);
	default: sprintf(buf, "UNKNOWN_CONNECT_TYPE(0x%04lx)", value); break;
	}
	return buf;
}

int gcFuncReturnMsg(char* func_name, int rc, LINEDEV ldev, char* msg_buf)
{
	GC_INFO gc_error_info; // GlobalCall error information data

	if (GC_ERROR == rc) {
		if (GC_ERROR == gc_ErrorInfo(&gc_error_info)) return(-1);
		sprintf(msg_buf, "%s return %d, ldev=0x%lX,\n\t\
				GC ErrorValue:0x%hx-%s,\n\t\
				CCLibID:%i-%s, CC ErrorValue:0x%lx-%s,\n\t\
				Additional Info:%s\n",
				func_name, rc, ldev, 
				gc_error_info.gcValue, gc_error_info.gcMsg,
				gc_error_info.ccLibId, gc_error_info.ccLibName,
				gc_error_info.ccValue, gc_error_info.ccMsg, 
				gc_error_info.additionalInfo);
		return rc;
	} else {
		sprintf(msg_buf, "%s returned %d on line device 0x%X\n", func_name, rc, ldev);
		return(0);
	}
}

int d4FuncReturnMsg(char* func_name, int rc, int dev, char* msg_buf)
{
	long lasterr = 0;

	if (-1 == rc) {

		if (-1 == dev) {
			sprintf(msg_buf, "%s return %d, errno=%d", 
				func_name, rc, dx_fileerrno());
		} else {
			lasterr = ATDV_LASTERR(dev);
			if (EDX_SYSTEM == lasterr) {
				sprintf(msg_buf, "%s return %d, dev=0x%lX, \
					ATDV_LASTERR=%d[SYSTEM ERROR], errno=%d\nATDV_ERRMSGP=%s",
					func_name, rc, dev, lasterr, errno, ATDV_ERRMSGP(dev));
			} else if (EDX_BADPARM == lasterr) {
				sprintf(msg_buf, "%s return %d, dev=0x%lX, ATDV_LASTERR=%d[BAD PARAMETER]\nATDV_ERRMSGP=%s",
					func_name, rc, dev, lasterr, ATDV_ERRMSGP(dev));
			} else {
				sprintf(msg_buf, "%s return %d, dev=0x%lX, ATDV_LASTERR=%d\nATDV_ERRMSGP=%s",
					func_name, rc, dev, lasterr, ATDV_ERRMSGP(dev));
			}
		}

	} else {
		sprintf(msg_buf, "%s return %d on device 0x%X\n", func_name, rc, dev);
	}
	return(0);
}
