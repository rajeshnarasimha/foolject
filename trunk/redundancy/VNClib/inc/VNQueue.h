// VNQueue.h: interface for the CVNQueue class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VNQUEUE_H__0218BEBF_F2DC_431D_BD9F_49503CEED144__INCLUDED_)
#define AFX_VNQUEUE_H__0218BEBF_F2DC_431D_BD9F_49503CEED144__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdio.h"
#include "VNNetMsg.h"
class CVNQueue  
{
	typedef struct tagQueueNode {
		CVNNetMsg * msg;
		tagQueueNode * next;
	}QueueNode;

public:
	CVNQueue();
	virtual ~CVNQueue();

	BOOL InitQueue();

	BOOL insertNode(CVNNetMsg * msg);
	CVNNetMsg * getNode();
	void resetQueue();

	BOOL WaitForQueueNotEmpty(unsigned long nTimeout) {
#ifdef POSIX
		struct timespec wait;
		time_t tnow=0;
		time(&tnow);
		if (nTimeout>1000) {
			wait.tv_sec=nTimeout/1000+tnow;
			wait.tv_nsec=0;
		}
		else {
			wait.tv_sec=tnow;
			wait.tv_nsec=nTimeout*1000*1000;
		}
		BOOL bOk=TRUE;
		pthread_mutex_lock(&m_csQueue);
		while (m_nCount==0) {
			if (pthread_cond_timedwait(&m_semaphore,&m_csQueue,&wait)==0) {
				bOk=TRUE;
				printf("get signal for data\n");
			}
			else {
				bOk=FALSE;
				break;
			}
		}
		pthread_mutex_unlock(&m_csQueue);
		
		return bOk;

#else
		if (m_semaphore!=NULL) {
			unsigned long nRes=0;
			//printf("m_semaphore = %d in waitformsg\n",m_semaphore);
			nRes=WaitForSingleObject(m_semaphore,nTimeout);
			//printf("nRes = %d in waitformsg\n",nRes);
			switch (nRes) {
			case WAIT_OBJECT_0:
				return TRUE;
				break;
			case WAIT_TIMEOUT:
				return FALSE;
				break;
			default:
				return FALSE;
				break;
			}
		}
		else
			return FALSE;
#endif
	};
protected:
	int m_nCount;
	QueueNode * m_Header;
	QueueNode * m_Tail;

#ifndef POSIX
	CRITICAL_SECTION m_csQueue;
	HANDLE m_semaphore;
#else
	pthread_mutex_t m_csQueue;
	pthread_cond_t m_semaphore;
#endif
};

#endif // !defined(AFX_VNQUEUE_H__0218BEBF_F2DC_431D_BD9F_49503CEED144__INCLUDED_)
