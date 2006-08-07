// VNQueueGroup.cpp: implementation of the CVNQueueGroup class.
//
//////////////////////////////////////////////////////////////////////

#include "VNQueueGroup.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVNQueueGroup::CVNQueueGroup()
{
	m_nCount=0;
	m_QueueGroup=NULL;
}

CVNQueueGroup::~CVNQueueGroup()
{

}

BOOL CVNQueueGroup::createQueueGroup(unsigned int nCount)
{
	if (nCount>MAXQUEUECNT)
		return FALSE;

	m_QueueGroup=new CVNQueue[nCount];
	m_nCount=nCount;

	BOOL bRet=TRUE;
	unsigned int i=0;
	for (i=0;i<nCount;i++) {
		if (!m_QueueGroup[i].InitQueue())
			bRet=FALSE;
	}

	if (!bRet) {
		delete[] m_QueueGroup;
		m_QueueGroup=NULL;
	}
	return bRet;
}

void CVNQueueGroup::freeQueueGroup()
{
	if (m_QueueGroup!=NULL) {
		unsigned int i=0;
		for (i=0;i<m_nCount;i++) {
			m_QueueGroup[i].resetQueue();
		}
		delete[] m_QueueGroup;
	}
}

BOOL CVNQueueGroup::insertMsg(unsigned int nQueueID,CVNNetMsg * msg)
{
	if (m_QueueGroup==NULL)
		return FALSE;

	if (nQueueID>=m_nCount)
		return FALSE;

	
	BOOL bok=m_QueueGroup[nQueueID].insertNode(msg);
//	printf("server recv a msg,insert into queue %d,res=%d,m_QueueGroup=%d\n",nQueueID,bok,m_QueueGroup);
	return bok;
}
