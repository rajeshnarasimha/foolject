// VNConnection.cpp: implementation of the CVNConnection class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>

#ifndef POSIX
#include <time.h>
#include "process.h"
#endif

#include "VNConnection.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifdef POSIX
extern char *sys_errlist[]; 
#endif

#ifdef ONLYFOR_RM
#define CHANNELCONNECTTIMEOUT     100
#else
#define CHANNELCONNECTTIMEOUT     1000
#endif

CVNConnection::CVNConnection()
{
	InitialConnection();
}

CVNConnection::CVNConnection(unsigned int nChanCnt)
{
	InitialConnection();
}
//***********************************************************************************
void CVNConnection::InitialConnection()
{
#ifndef POSIX
	WSADATA wsd;
	WSAStartup(MAKEWORD(2,2),&wsd);
#endif
	m_nRecCnt=0;
	m_nPort=0;

	m_sListener=INVALID_SOCKET;
	m_thrdLisn=NULL;
	m_nHeaderFree=0;
	m_chCast=NULL;
	m_chUDP=NULL;
	m_nCastRange=0;
	m_CastStart.s_addr=0;
	m_QueueGroup=NULL;
	
	m_nSndCnt=0;
	int i=0;
	for (i=0;i<MAXSENDCHAN;i++) {		
		m_arraySendChan[i]=NULL;
		m_arrayTimes[i]=0;
		m_thrdClnt[i]=NULL;
	}

	m_nTailFree=i-1;

	for (i=0;i<MAXRECVCHAN;i++) {
		m_arrayRecvFree[i]=i+1;
		m_arrayRecvChan[i]=NULL;
		m_thrdServ[i]=NULL;
	}

#ifndef POSIX
	InitializeCriticalSection(&m_csArray);
#else
	pthread_mutex_init(&m_csArray,NULL);
#endif
}

//***********************************************************************************

CVNConnection::~CVNConnection()
{
	ClearListenerHandle();
	ClearChannelHandle();
#ifndef POSIX
	WSACleanup();
	DeleteCriticalSection(&m_csArray);
#else
	pthread_mutex_destroy(&m_csArray);
#endif
}

//***********************************************************************************

void CVNConnection::ClearListenerHandle()
{
	if (m_sListener!=INVALID_SOCKET) {
		int nRet1=0;
		nRet1=shutdown(m_sListener,SD_BOTH);
		int nRet2=0;
		nRet2=closesocket(m_sListener);
		m_sListener=INVALID_SOCKET;
		printf("close listener socket\n");
	}
	
	if (m_thrdLisn!=0) {
#ifndef POSIX
		WaitForSingleObject(m_thrdLisn,INFINITE);
		CloseHandle(m_thrdLisn);
		m_thrdLisn=NULL;
#else
		pthread_join(m_thrdLisn,NULL);
		m_thrdLisn=0;
#endif
	}

	unsigned int i=0;
	for (i=0;i<MAXRECVCHAN;i++) {
		if (m_arrayRecvChan[i]!=NULL)
			m_arrayRecvChan[i]->Disconnect();
		//m_arrayRecvChan[i]->ResetPeerLong();
	}
	
	for (i=0;i<MAXRECVCHAN;i++) {
		if (m_thrdServ[i]!=NULL) {
#ifndef POSIX
			WaitForSingleObject(m_thrdServ[i],INFINITE);
			CloseHandle(m_thrdServ[i]);
			m_thrdServ[i]=NULL;
#else
			pthread_join(m_thrdServ[i],NULL);
			m_thrdServ[i]=0;
#endif
		}
	}

	for (i=0;i<MAXRECVCHAN;i++) {
		m_arrayRecvFree[i]=i+1;
		CVNChannel * pch=m_arrayRecvChan[i];
		if (pch!=NULL) {
			delete pch;
			m_arrayRecvChan[i]=NULL;
		}
		m_nRecCnt=0;
	}

	m_nHeaderFree=0;
	m_nTailFree=MAXRECVCHAN-1;
	printf("clear m_arrayRecvChan \n");
}
//***********************************************************************************

void CVNConnection::ClearChannelHandle() 
{
	unsigned int i=0;
	for (i=0;i<MAXSENDCHAN;i++) {
		if (m_arraySendChan[i]!=NULL) {
			m_arraySendChan[i]->Disconnect();
			m_arraySendChan[i]->ResetPeerLong();
		}
	}

	for (i=0;i<MAXSENDCHAN;i++) {
		if (m_thrdClnt[i]!=NULL) {
#ifndef POSIX
			WaitForSingleObject(m_thrdClnt[i],INFINITE);
			CloseHandle(m_thrdClnt[i]);
			m_thrdClnt[i]=NULL;
#else
			pthread_join(m_thrdClnt[i],NULL);
			m_thrdClnt[i]=0;
#endif
		}
	}
	
	for (i=0;i<MAXSENDCHAN;i++) {
		if (m_arraySendChan[i]!=NULL) {
			delete (m_arraySendChan[i]);
			m_arraySendChan[i]=NULL;
		}
	}
	m_nSndCnt=0;
}

//***********************************************************************************
#ifdef POSIX
extern "C" void * CVNConnection::ListenProc(void * pCon)
#else
void * CVNConnection::ListenProc(void * pCon)
#endif
{
	CVNConnection * pc=(CVNConnection*)pCon;
	pc->VNListenCore();
	return NULL;
}
//***********************************************************************************
#ifdef POSIX
extern "C" void * CVNConnection::ServRecvThreadProc(void * param)
#else
void * CVNConnection::ServRecvThreadProc(void * param)
#endif
{
	tagThreadParam * p=(tagThreadParam*)param;
	CVNNetMsg * msg=NULL;
	CVNChannel * pch=NULL;
	CVNConnection * pcn=p->pcn;
	unsigned int nChanID=p->nChanID ;
	delete param;
	pch=pcn->ReturnChannelPointer(TRUE,nChanID);

	printf("server thread start\n");
	while (TRUE) {
		int nErr=0;
	
		msg=pch->RecvMsg(&nErr,0,10);
		if (nErr==RS_Event||nErr==RS_Closed||nErr==RS_Failed) {
//			printf("error : %x\n",nErr);
			break;
		}

		if (nErr==RS_Timeout) {
			continue;
		}
		
		
		int nID=msg->GetDesQueueID();
		msg->SetConnectionID(nChanID);
		if (pcn->m_QueueGroup->insertMsg(nID,msg)==FALSE)
		{
#ifdef FOOLBEAR_DEBUG
			LogByLevel(DSS_VP, DEBUGLVL, nID + 1, 0, "FOOLBEAR_DEBUG: insertMsg failed, source queue %d",
				msg->GetSrcQueueID());
#endif
			delete msg;
		}

	}
//	printf("thread quit,%d\n",nChanID);
	pcn->AddAFreeChannel(nChanID);
	return NULL;
}
//***********************************************************************************
#ifdef POSIX
extern "C" void * CVNConnection::ClientRecvThreadProc(void * param)
#else
void * CVNConnection::ClientRecvThreadProc(void * param)
#endif
{
	tagThreadParam * p=(tagThreadParam*)param;
	CVNNetMsg * msg=NULL;
	CVNChannel * pch=NULL;
	CVNConnection * pcn=p->pcn;
	unsigned int nChanID=p->nChanID ;
	delete param;
	pch=pcn->ReturnChannelPointer(FALSE,nChanID);

//	printf("thread start,%d\n",nChanID);
	while (TRUE) {
		int nErr=0;
	
		msg=pch->RecvMsg(&nErr,0,10);
		if (nErr==RS_Event||nErr==RS_Closed||nErr==RS_Failed) {
//			printf("error : %x\n",nErr);
			break;
		}

		if (nErr==RS_Timeout) {
			continue;
		}

		int nID=msg->GetDesQueueID();
//		printf("thread start,chan is %d, queue is %d\n",nChanID, nID);
		msg->SetConnectionID(nChanID);
		if (pcn->m_QueueGroup->insertMsg(nID,msg)==FALSE)
		{
#ifdef FOOLBEAR_DEBUG
			LogByLevel(DSS_VP, DEBUGLVL, nID + 1, 0, "FOOLBEAR_DEBUG: insertMsg failed, source queue %d",
				msg->GetSrcQueueID());
#endif
			delete msg;
		}

	}
	return NULL;
//	printf("thread quit,%d\n",nChanID);
}
//***********************************************************************************
BOOL CVNConnection::ChannelConnect(unsigned int nSendChanID,long nServAddr,int nPort,unsigned long lTimeout /*usecond*/)
{
	if (m_arraySendChan[nSendChanID]==NULL)
		m_arraySendChan[nSendChanID]=new CVNChannel;

	if (m_arraySendChan[nSendChanID]->Open(nServAddr,nPort,lTimeout)==FALSE) {
		return FALSE;
	}

	CVNNetMsg * msg=NULL;
	int nErr=0;

	msg=m_arraySendChan[nSendChanID]->RecvMsg(&nErr);
	
	if (msg==NULL) {
		return FALSE;
	}
	else {
		if (msg->GetType()!=Control) {
			VNReleaseMsg(msg);
			return FALSE;
		}
	}
	VNReleaseMsg(msg);

	tagThreadParam  * param=new tagThreadParam;
	param->pcn=this;
	param->nChanID=nSendChanID;
	param->puser=NULL;
	
	unsigned lThreadNum=0;
	
#ifndef POSIX
	m_thrdClnt[nSendChanID]=(HANDLE)_beginthreadex(NULL,0,(unsigned int (__stdcall *)(void *))ClientRecvThreadProc,param,0,&lThreadNum);		
#else
	pthread_t thread1;
	pthread_attr_t thrdattr;
	pthread_attr_init(&thrdattr);
	pthread_attr_setdetachstate(&thrdattr,PTHREAD_CREATE_DETACHED);
	pthread_create(&m_thrdClnt[nSendChanID],&thrdattr,CVNConnection::ClientRecvThreadProc,(void*)(param));
#endif
	return TRUE;
}

//***********************************************************************************

BOOL CVNConnection::VNBindUDPMode(int port)
{
	if (port<0)
		return FALSE;

	if (m_chUDP!=NULL)
		return TRUE;

	m_chUDP=new CVNChannel;
	if ( m_chUDP->BindBroadCastMode(port,FALSE)==FALSE) {
		delete m_chUDP;
		m_chUDP=NULL;
		m_CastStart.s_addr=0;
		m_nCastRange=0;
		return FALSE;
	}

	return TRUE;	
}

//***********************************************************************************

BOOL CVNConnection::VNBindBroadCastMode(int port)
{
	if (port<0)
		return FALSE;

	if (m_chCast!=NULL)
		return TRUE;

	m_chCast=new CVNChannel;
	if ( m_chCast->BindBroadCastMode(port)==FALSE) {
		delete m_chCast;
		m_chCast=NULL;
		m_CastStart.s_addr=0;
		m_nCastRange=0;
		return FALSE;
	}

	return TRUE;	
}

//***********************************************************************************

BOOL CVNConnection::VNUnBindBroadCastMode()
{
	if (m_chCast==NULL)
		return TRUE;

	delete m_chCast;
	m_chCast=NULL;
	return TRUE;
}

//***********************************************************************************

BOOL CVNConnection::VNUnBindUDPMode()
{
	return VNUnBindBroadCastMode();
}

//***********************************************************************************

CVNNetMsg* CVNConnection::VNRecvUDPMsg(int nLength,char * lpAddr,int nAddrLen,unsigned int nTimeout)
{
	if (nLength<128)
		return NULL;

	if (nAddrLen<15)
		return NULL;

	if (lpAddr==NULL)
		return NULL;

	if (m_chUDP==NULL)
		return NULL;

	CVNNetMsg* msg=NULL;
	
	return m_chUDP->RecvBroadCastMsg(NULL,nLength,lpAddr,nAddrLen,nTimeout);
}

//***********************************************************************************

CVNNetMsg * CVNConnection::VNRecvBroadCastMsg(int nLength,char * lpAddr,int nAddrLen,unsigned int nTimeout)
{
	if (nLength<128)
		return NULL;

	if (nAddrLen<15)
		return NULL;

	if (lpAddr==NULL)
		return NULL;

	if (m_chCast==NULL)
		return NULL;

	CVNNetMsg* msg=NULL;
	
	return m_chCast->RecvBroadCastMsg(NULL,nLength,lpAddr,nAddrLen,nTimeout);
	
}

//***********************************************************************************

BOOL CVNConnection::VNSendUDPMsg(const char* lpHost,CVNNetMsg * msg,int port)
{
	if (lpHost==NULL)
		return FALSE;

	if (port<0)
		return FALSE;

	CVNChannel chan;
	if (chan.SendUDPMsg(lpHost,msg,port)==-1)
		return FALSE;
	return TRUE;
}

//***********************************************************************************

BOOL CVNConnection::VNSendBroadCastMsg(const char* lpHost,const char * lpMask,CVNNetMsg * msg,int port)
{
	if (lpHost==NULL || lpMask==NULL)
		return FALSE;

	struct in_addr hostaddr;
	hostaddr.s_addr=inet_addr(lpHost);
	if (hostaddr.s_addr==INADDR_NONE)
		return FALSE;

	struct in_addr maskaddr;
	maskaddr.s_addr=inet_addr(lpMask);
	if (maskaddr.s_addr==INADDR_NONE)
		return FALSE;

	if (maskaddr.S_un.S_un_b.s_b4==255)
		return FALSE;

	int nMask=0x1;
	unsigned int lMask=ntohl(maskaddr.s_addr);
	BOOL bZero=FALSE;
	unsigned int nRs=0;
	int i=0;
	for (i=0;i<32;i++) {
		if (i==0) {
			if (nRs=(lMask & nMask) ==1)
				return FALSE;
			bZero=TRUE;
		}
		else {
			if (nRs=(lMask & nMask) ==0) {
				if (bZero!=TRUE)
					return FALSE;
			}
			else
				bZero=FALSE;
		}
		nMask=nMask*2;
	}

	if (port<0)
		return FALSE;

	unsigned int lHost=ntohl(hostaddr.s_addr);
	unsigned int lCast=(~lMask)+(lMask & lHost);
	char lpCast[16]="";
	struct in_addr addrCast;
	addrCast.s_addr=htonl(lCast);
	char * lpCastAddr=inet_ntoa(addrCast);
	strncpy(lpCast,lpCastAddr,16);

	CVNChannel chan;
	if (chan.SendBroadCastMsg(lpCastAddr,msg,port)==-1)
		return FALSE;
	return TRUE;
}

//***********************************************************************************

int CVNConnection::VNConnect(const char* lpHost,unsigned int port,unsigned int nTimeout/*usecond*/)
{
	
	if (lpHost==NULL)
		return -1;
	if (port>65535)
		return -1;
	if (strlen(lpHost)>MAXHOSTADDRLEN)
		return -1;

	if (m_QueueGroup==NULL)
		return -1;

	long lhost=::inet_addr(lpHost);
	if (lhost==INADDR_NONE)
		return -1;


	int nSendID=0;

	int nCnt=m_nSndCnt;
	int i=0;
	for (i=0;i<nCnt;i++) {
		if (m_arraySendChan[i]->GetPeerLong()==lhost) {
			break;
		}
	}
	
	if (i==nCnt) {	
		if (i==MAXSENDCHAN) {
			nSendID=-1;
		}
		else {
			nSendID=i;
#ifndef POSIX
			::InterlockedIncrement(&m_nSndCnt);
#else
			pthread_mutex_lock(&m_csArray);
			m_nSndCnt++;
			pthread_mutex_unlock(&m_csArray);
#endif
			if (ChannelConnect(nSendID,lhost,port,nTimeout)==FALSE)
				nSendID=-1;
		}
		
	}
	else
		nSendID=i;
		
	return nSendID;
}

//***********************************************************************************

BOOL CVNConnection::VNHaltServer()
{
	ClearListenerHandle();
	return TRUE;
}

//***********************************************************************************

BOOL CVNConnection::VNDisconnect()
{
	ClearChannelHandle();
	return TRUE;
}

//***********************************************************************************

RSINFO CVNConnection::VNSendMsg(int nConnectionID,unsigned int nSrcQueueID,unsigned int nDesQueueID,CVNNetMsg* msg)
{	
	if ((nConnectionID<0)||(nConnectionID>=MAXSENDCHAN)) {
#ifdef FOOLBEAR_DEBUG
		LogByLevel(DSS_VP, DEBUGLVL, 0, 0, 
			"FOOLBEAR_DEBUG: CVNConnection::VNSendMsg 1");
#endif
		return RS_Failed;
	}

	if ((nSrcQueueID>=MAXQUEUECNT)||(nDesQueueID>=MAXQUEUECNT)) {
#ifdef FOOLBEAR_DEBUG
		LogByLevel(DSS_VP, DEBUGLVL, 0, 0, 
			"FOOLBEAR_DEBUG: CVNConnection::VNSendMsg 2");
#endif
		return RS_Failed;
	}

	if (m_arraySendChan[nConnectionID]==NULL) {
#ifdef FOOLBEAR_DEBUG
		LogByLevel(DSS_VP, DEBUGLVL, 0, 0, 
			"FOOLBEAR_DEBUG: CVNConnection::VNSendMsg 3");
#endif
		return RS_Failed;
	}

	msg->SetSrcQueueID(nSrcQueueID);
	msg->SetDesQueueID(nDesQueueID);

	int rc=m_arraySendChan[nConnectionID]->SendMsg(msg,NULL);
	if (rc==RS_Timeout)
		return RS_Timeout;

	if (rc<0) {
		if (ChannelConnect(nConnectionID,-1,-1,CHANNELCONNECTTIMEOUT)==FALSE) {
#ifdef FOOLBEAR_DEBUG
			LogByLevel(DSS_VP, DEBUGLVL, 0, 0, 
				"FOOLBEAR_DEBUG: CVNConnection::VNSendMsg 4");
#endif
			return RS_Failed;
		} else { 
			int rc2=m_arraySendChan[nConnectionID]->SendMsg(msg,NULL);
			if (rc2>0) return RS_Success;
			else {
#ifdef FOOLBEAR_DEBUG
				LogByLevel(DSS_VP, DEBUGLVL, 0, 0, 
					"FOOLBEAR_DEBUG: CVNConnection::VNSendMsg return %d", rc2);
#endif
				return (RSINFO)rc2;
			}
		}
	}
	return RS_Success;
}

//***********************************************************************************
RSINFO CVNConnection::VNSendReplyMsg(int nConnectionID,unsigned int nSrcQueueID,unsigned int nDesQueueID,CVNNetMsg* msg)
{
	if ((nConnectionID<0)||(nConnectionID>=MAXSENDCHAN))
		return RS_Failed;
	if ((nSrcQueueID>=MAXQUEUECNT)||(nDesQueueID>=MAXQUEUECNT))
		return RS_Failed;
	CVNChannel * pch=NULL;

	pch=ReturnChannelPointer(TRUE,nConnectionID);
	if (pch==NULL)
		return RS_Failed;

	msg->SetSrcQueueID(nSrcQueueID);
	msg->SetDesQueueID(nDesQueueID);

	int rc=pch->SendMsg(msg,NULL);
	if (rc>0)
		return RS_Success;
	else
		return (RSINFO)rc;
}

//***********************************************************************************

CVNNetMsg* CVNConnection::VNRecvMsg(unsigned int nRecQueueID,int * nErr,unsigned int nTimeout)
{
	int nRes=RS_Success;
	CVNNetMsg * msg=NULL;
	if (m_QueueGroup==NULL) {
		nRes=RS_Failed;
	}
	else {
		if (m_QueueGroup->WaitForQueueNotEmpty(nRecQueueID,nTimeout)) {
			msg=m_QueueGroup->getMsg(nRecQueueID);
			if (msg==NULL)
				nRes=RS_Failed;
		}
		else {
			nRes=RS_Timeout;
		}
	}

	if (nErr!=NULL)
		*nErr=nRes;
	return msg;
}

//***********************************************************************************

BOOL CVNConnection::VNListen(unsigned int port,CVNQueueGroup * qg)
{
	if (port>65535)
		return FALSE;

	if (m_QueueGroup=NULL)
		return FALSE;

	if (m_sListener!=INVALID_SOCKET)
		return FALSE;

	m_sListener = socket(AF_INET, SOCK_STREAM, 0);

	if (m_sListener == INVALID_SOCKET) {
		ClearListenerHandle();
		return FALSE;
	}

	int nRet=0;
#ifdef POSIX
	nRet=fcntl(m_sListener,F_SETFL,O_NONBLOCK);
#else
	unsigned long ul=1;
	nRet=ioctlsocket(m_sListener,FIONBIO,&ul);
#endif

	BOOL bReuse=TRUE;
	setsockopt(m_sListener,SOL_SOCKET,SO_REUSEADDR,(char*)&bReuse,sizeof(bReuse));

	m_nPort=port;
	
	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(m_nPort);
	sa.sin_addr.s_addr=htonl(INADDR_ANY);

	if (bind(m_sListener,(PSOCKADDR)&sa,sizeof(sa))!=0) {
		printf("bind failed\n");
#ifdef POSIX
		printf(sys_errlist[errno]);
		printf("\n");
#endif
		ClearListenerHandle();
		return FALSE;
	}

	listen(m_sListener,SOMAXCONN);

	m_QueueGroup=qg;

#ifdef POSIX
	if (pthread_create(&m_thrdLisn,NULL,CVNConnection::ListenProc,(void*)(this))!=0) {
		return FALSE;
	}
#else
	unsigned int thrd=0;
	m_thrdLisn=(HANDLE)_beginthreadex(NULL,0,(unsigned int (__stdcall *)(void *))ListenProc,this,0,&thrd);
#endif 
	return TRUE;
}



//***********************************************************************************

BOOL CVNConnection::VNListenCore()
{

	unsigned long index=0;
	
	fd_set fdread;
	
	int nID=0;
	while (TRUE) {
		FD_ZERO(&fdread);
		FD_SET(m_sListener,&fdread);
		
		struct timeval tvalout={0,10};
		int nRet=select(m_sListener + 1, &fdread,NULL, NULL, &tvalout);
		switch (nRet) {  
		case -1:  
			printf("listen quit\n");
			break;
		case 0: 
			continue;
		default:  
			
			if (FD_ISSET(m_sListener,&fdread)) {
				printf("listened a accept try\n");
				unsigned int nTemp=ReduceAFreeChannel();
				if (nTemp>=MAXRECVCHAN) {
					//printf("cant accept a connection\n");
					continue;
				}
				
				tagThreadParam  * param=new tagThreadParam;
				param->pcn=this;
				param->nChanID=nTemp;
				param->puser=NULL;
				
				unsigned lThreadNum=0;
				
#ifndef POSIX
				m_thrdServ[nTemp]=(HANDLE)_beginthreadex(NULL,0,(unsigned int (__stdcall *)(void *))ServRecvThreadProc,param,0,&lThreadNum);		
#else
				pthread_t thread1;
				pthread_attr_t thrdattr;
				pthread_attr_init(&thrdattr);
				pthread_attr_setdetachstate(&thrdattr,PTHREAD_CREATE_DETACHED);
				pthread_create(&m_thrdServ[nTemp],&thrdattr,CVNConnection::ServRecvThreadProc,(void *)(param));
				printf("accept a connection try\n");
#endif
			}  
		}
		if (nRet==-1)
			break;

	}
	return TRUE;
}

//***********************************************************************************

void CVNConnection::AddAFreeChannel(unsigned int nChanID)
{
#ifndef POSIX
	EnterCriticalSection(&m_csArray);
#else
	pthread_mutex_lock(&m_csArray);
#endif	
	if (m_nTailFree<MAXRECVCHAN)
		m_arrayRecvFree[m_nTailFree]=nChanID;
	m_nTailFree=nChanID;
	m_arrayRecvFree[m_nTailFree]=MAXRECVCHAN;
	if (m_nHeaderFree==MAXRECVCHAN)
		m_nHeaderFree=m_nTailFree;
#ifndef POSIX
	::InterlockedDecrement(&m_nRecCnt);
	LeaveCriticalSection(&m_csArray);
#else
	m_nRecCnt--;
	pthread_mutex_unlock(&m_csArray);
#endif
}

//***********************************************************************************

unsigned int CVNConnection::ReduceAFreeChannel()
{
#ifndef POSIX
	EnterCriticalSection(&m_csArray);
#else
	pthread_mutex_lock(&m_csArray);
#endif	
	unsigned int nFree=0;
	nFree=m_nHeaderFree;

	SOCKET sNew=accept(m_sListener,NULL,NULL);
	if (sNew==INVALID_SOCKET) {
#ifndef POSIX
		LeaveCriticalSection(&m_csArray);
#else
		pthread_mutex_unlock(&m_csArray);
#endif
		return MAXRECVCHAN;
	}

    int optvalue = SO_RCVBUF_SIZE;
    setsockopt(sNew,SOL_SOCKET,SO_RCVBUF,(char*)&optvalue,sizeof(int));
    optvalue = SO_SNDBUF_SIZE;
    setsockopt(sNew,SOL_SOCKET,SO_SNDBUF,(char*)&optvalue,sizeof(int));

	if (nFree<MAXRECVCHAN) {

		//printf("A conncectiong has be accpeted, %d\n",nFree);
		
		if (m_arrayRecvChan[nFree]==NULL) {
			CVNChannel * ch=new CVNChannel(sNew);
			m_arrayRecvChan[nFree]=ch;
		}
		else {
			m_arrayRecvChan[nFree]->SetSocket(sNew);
		}

		m_arrayRecvChan[nFree]->SetChannelID(nFree);
		CVNNetMsg msg(Control);
		m_arrayRecvChan[nFree]->SendMsg(&msg,NULL);

		if (m_nHeaderFree==m_nTailFree) {
			m_nHeaderFree=MAXRECVCHAN;
			m_nTailFree=MAXRECVCHAN;
		}
		else {
			m_nHeaderFree=m_arrayRecvFree[nFree];
		}
		
#ifndef POSIX		
		::InterlockedIncrement(&m_nRecCnt);
#else
		m_nRecCnt++;
#endif
	}
	else {
		closesocket(sNew);
	}
#ifndef POSIX
	LeaveCriticalSection(&m_csArray);
#else
	pthread_mutex_unlock(&m_csArray);
#endif		

	return nFree;
}
//***********************************************************************************
BOOL CVNConnection::VNGetClientPeerAddr(unsigned int nQueueID,char * lpAddr,int nLen)
{
	if (m_QueueGroup==NULL) 
		return FALSE;

	if (lpAddr==NULL)
		return FALSE;

	if (nLen<=0)
		return FALSE;

	return m_arrayRecvChan[nQueueID]->GetPeerAddr(lpAddr,nLen);
}