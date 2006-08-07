// VNQueueGroup.h: interface for the CVNQueueGroup class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VNQUEUEGROUP_H__DBF877E2_D9C4_483E_B127_B12105308BDA__INCLUDED_)
#define AFX_VNQUEUEGROUP_H__DBF877E2_D9C4_483E_B127_B12105308BDA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "VNQueue.h"

#define MAXQUEUECNT 1024

class CVNQueueGroup  
{
public:
	CVNQueueGroup();
	virtual ~CVNQueueGroup();

	virtual BOOL createQueueGroup(unsigned int nCount);
	virtual void freeQueueGroup();

	virtual BOOL insertMsg(unsigned int nQueueID,CVNNetMsg * msg);
	virtual CVNNetMsg * getMsg(unsigned int nQueueID) {
		if (nQueueID>=m_nCount)
			return NULL;
		return m_QueueGroup[nQueueID].getNode();
	};

	virtual BOOL WaitForQueueNotEmpty(unsigned int nQueueID,unsigned long nTimeout) {
		if (nQueueID>=m_nCount)
			return FALSE;
		return m_QueueGroup[nQueueID].WaitForQueueNotEmpty(nTimeout);
	}

	virtual BOOL ResetQueue(unsigned int nQueueID) {
		if (nQueueID>=m_nCount)
			return FALSE;
		m_QueueGroup[nQueueID].resetQueue();
		return TRUE;
	};


protected:
	unsigned int m_nCount;
	CVNQueue * m_QueueGroup;
};

#endif // !defined(AFX_VNQUEUEGROUP_H__DBF877E2_D9C4_483E_B127_B12105308BDA__INCLUDED_)
