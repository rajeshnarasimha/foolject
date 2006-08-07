// VNConnection.h: interface for the CVNConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VNCONNECTION_H__52429C67_3AC4_4BDD_B2A7_6D3AB6AD8C8F__INCLUDED_)
#define AFX_VNCONNECTION_H__52429C67_3AC4_4BDD_B2A7_6D3AB6AD8C8F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "VNChannel.h"
#include "VNQueueGroup.h"

#define MAXSENDCHAN		256
#define MAXRECVCHAN		1024
#define MAXHOSTADDRLEN	256

struct tagThreadParam;
class CVNConnection  
{

public:
	CVNConnection();
	CVNConnection(unsigned int nChanCnt);
	virtual ~CVNConnection();

	BOOL VNListen(unsigned int port,CVNQueueGroup * qg);
	
	void VNSetVNCQueue(CVNQueueGroup * qg) {
		m_QueueGroup=qg;
	};
	int VNConnect(const char * lpHost, unsigned int port,unsigned int nTimeout);
	BOOL VNDisconnect();
	BOOL VNHaltServer();

	RSINFO VNSendMsg(int nConnectionID,unsigned int nSrcQueueID,unsigned int nDesQueueID,CVNNetMsg* msg);
	RSINFO VNSendReplyMsg(int nConnectionID,unsigned int nSrcQueueID,unsigned int nDesQueueID,CVNNetMsg* msg);
	CVNNetMsg* CVNConnection::VNRecvMsg(unsigned int nRecQueueID,int * nErr,unsigned int nTimeout);

	void VNReleaseMsg(CVNNetMsg * msg) 
	{
		if (msg!=NULL) {
			delete msg;
			msg=NULL;
		}
	};

	BOOL VNSendBroadCastMsg(const char* lpHost,const char * lpMask,CVNNetMsg * msg,int port);
	BOOL VNSendUDPMsg(const char* lpHost,CVNNetMsg * msg,int port);
	BOOL VNBindBroadCastMode(int port);
	BOOL VNUnBindBroadCastMode();
	BOOL VNBindUDPMode(int port);
	BOOL VNUnBindUDPMode();
	CVNNetMsg* VNRecvUDPMsg(int nLength,char * lpAddr,int nAddrLen,unsigned int nTimeout);
	CVNNetMsg * VNRecvBroadCastMsg(int nLength,char * lpAddr,int nAddrLen,unsigned int nTimeout=5000);

	CVNQueueGroup * VNCreateRecvQueueGroup(unsigned int nCount) 
	{
		CVNQueueGroup * qg=NULL;
		qg=new CVNQueueGroup;
		if (qg->createQueueGroup(nCount)==FALSE) {
			delete qg;
			qg=NULL;
		}
		return qg;
	};

	void VNFreeQueueGroup(CVNQueueGroup * qg) 
	{
		if (qg!=NULL) {
			qg->freeQueueGroup();
			delete qg;
		}
	};

	static void * ClientRecvThreadProc(void * param);
	static void * ListenProc(void * pCon);
	static void * ServRecvThreadProc(void * param);
	
	BOOL VNGetClientPeerAddr(unsigned int nQueueID,char * lpAddr,int nLen);
//	void AddConnectionRef(unsigned int nConnectionID) {
//		m_arraySendChan[nConnectionID]->AddConnectionRef();
//	};
//	void ReduceConnectionRef(unsigned int nConnectionID) {
//		m_arraySendChan[nConnectionID]->ReduceConnectionRef();
//	};
//
//	long GetConnectionRef(unsigned int nConnectionID) {
//		return m_arraySendChan[nConnectionID]->GetConnectionRef();
//	};

protected:
	BOOL VNListenCore();

	void ClearChannelHandle();
	void ClearListenerHandle();

	void InitialConnection();
	void AddAFreeChannel(unsigned int nChanID);
	unsigned int ReduceAFreeChannel();

	BOOL ChannelConnect(unsigned int nSendChanID,long nServAddr,int nPort,unsigned long lTimeout);
	CVNChannel * ReturnChannelPointer(BOOL bServ,unsigned int nChanID) 
	{
		if (bServ) {
			if (nChanID>MAXRECVCHAN) return NULL;	
			return m_arrayRecvChan[nChanID];
		}
		else
		{
			if (nChanID>MAXSENDCHAN) return NULL;	
			return m_arraySendChan[nChanID];
		}
	};


private:
	unsigned int m_nPort;
	
	long m_nSndCnt;
	CVNChannel * m_arraySendChan[MAXSENDCHAN];
	unsigned long m_arrayTimes[MAXSENDCHAN];

	SOCKET m_sListener;

	CVNChannel * m_chCast;
	CVNChannel * m_chUDP;
	int m_nCastRange;
	struct in_addr m_CastStart;

	long m_nRecCnt;
	int m_arrayRecvFree[MAXRECVCHAN];
	CVNChannel * m_arrayRecvChan[MAXRECVCHAN];

	unsigned int m_nHeaderFree;
	unsigned int m_nTailFree;

#ifndef POSIX
	CRITICAL_SECTION m_csArray;
	HANDLE m_thrdLisn;
	HANDLE m_thrdServ[MAXRECVCHAN];
	HANDLE m_thrdClnt[MAXSENDCHAN];
#else
	pthread_mutex_t m_csArray;
	pthread_t m_thrdLisn;
	pthread_t m_thrdServ[MAXRECVCHAN];
	pthread_t m_thrdClnt[MAXSENDCHAN];
#endif

	CVNQueueGroup * m_QueueGroup;
};

struct tagThreadParam {
	CVNConnection * pcn;
	unsigned int nChanID;
	void * puser;
};
#endif // !defined(AFX_VNCONNECTION_H__52429C67_3AC4_4BDD_B2A7_6D3AB6AD8C8F__INCLUDED_)
