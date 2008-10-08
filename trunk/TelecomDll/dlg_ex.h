#ifndef _DLG_EX_H_
#define _DLG_EX_H_

/*
* Dialogic header files
*/
#include <srllib.h>				/* SRL header file				*/
#include <dxxxlib.h>			/* Voice library header file	*/
#include <dtilib.h>				/* DTI library header file		*/
#include <faxlib.h>				/* Fax library header file		*/
#include <eclib.h>				/* Speech header file			*/
#include <gclib.h>				/* Global Call API header file	*/
#include <gcerr.h>				/* Global Call API errors file	*/
#include <sctools.h>			/* nr_scroute header file		*/
#include <gcip.h>				/* IPT Global Call header file	*/
#include <ipmlib.h>				/* IP Media header file			*/
#include <msilib.h>				/* MSI library header file	    */
#include <dcblib.h>				/* DCB library header file	    */
#include <cclib.h>				/* Call Control header file	    */
#include <gcisdn.h>				/* ISDN translation layer		*/
#include <d42lib.h>				/* D42 header file				*/

#include <fcntl.h>				/* for _O_RDONLY, _O_BINARY etc */

//#define CHANNEL_GROUP_COUNT	1
#define SR_WAIT_TIME_OUT		500
#define MAX_CHANNEL_COUNT		512
#define MAX_EVENT_GROUP_COUNT	MAX_CHANNEL_COUNT
#define INVALID_CHANNEL_INDEX	MAX_CHANNEL_COUNT

#define MAX_DEV_NAME			64

#define FAX_TONE_ID				300

#define _FOOL_PRINT_CASE_(x) case x: sprintf(buf, "%s", #x); break
#define _FOOL_PRINT_BIT_(x) if (x == (value & x)) sprintf(buf, "%s(%s)", buf, #x)

/*
* Definition for cross-compatibility support
*/
#if (defined (__BORLANDC__) || defined (CLIB)) 
	#define	LOAD_GC_LIBRARY	gcapi_libinit(DLGC_MT);
	#define BCEXTRAPARM 0L
#else
	#define	LOAD_GC_LIBRARY
	#define BCEXTRAPARM
#endif

typedef enum {
	PLAY_FILE					=0,
	RECORD_FILE
}IO_Dirction;

typedef enum {
	CER_NEAR_END				=0,	//original channel end the bridge
	CER_FAR_END,					//destination end the bridge
	CER_BUSY,						//destination busy
	CER_NETWORK_BUSY,				//intermediate network refused the call
	CER_NO_ANSWER,					//destination no answer
	CER_FAILED,						//call failed
}CallExitReason;

int playFileAbort2TPT(const char* abort, DV_TPT* tpt, int size);
int clearTPT(DV_TPT* tpt, int size);

int buildIOTT(const char* file, DX_IOTT* iott, IO_Dirction dir=PLAY_FILE);
int clearIOTT(DX_IOTT* iott);

int buildXPB(DX_XPB* xpb);
int clearXPB(DX_XPB* xpb);

char* getCERName(CallExitReason cer, char*buf);
char* getGCEvtName(DWORD value, char* buf);
char* getD4EvtName(DWORD value, char* buf);
char* getTermReasonName(DWORD value, char* buf);
char* getConnectTypeName(int value, char* buf);

int gcFuncReturnMsg(char* func_name, int rc, LINEDEV ldev, char* msg_buf);
int d4FuncReturnMsg(char* func_name, int rc, int dev, char* msg_buf);

#endif