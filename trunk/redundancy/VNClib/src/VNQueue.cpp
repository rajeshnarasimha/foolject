// VNQueue.cpp: implementation of the CVNQueue class.
//
//////////////////////////////////////////////////////////////////////

#include "VNQueue.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVNQueue::CVNQueue()
{
	m_nCount=0;
	m_Header=NULL;
	m_Tail=NULL;

#ifndef POSIX
	m_semaphore=NULL;
	::InitializeCriticalSection(&m_csQueue);
#else
	pthread_mutex_init(&m_csQueue,NULL);
	pthread_cond_init(&m_semaphore,NULL);
#endif
	
}

CVNQueue::~CVNQueue()
{
#ifndef POSIX
	::DeleteCriticalSection(&m_csQueue);
	if (m_semaphore!=NULL) {
		CloseHandle(m_semaphore);
		m_semaphore=NULL;
	}
#else
	pthread_mutex_destroy(&m_csQueue);
	pthread_cond_destroy(&m_semaphore);
#endif
}

BOOL CVNQueue::InitQueue()
{
#ifndef POSIX
	if (m_semaphore!=NULL)
		return TRUE;
	m_semaphore=CreateSemaphore(NULL,0,0xFFFF,NULL);
	if (m_semaphore==NULL)
		return FALSE;
#endif
	return TRUE;
}

BOOL CVNQueue::insertNode(CVNNetMsg * msg)
{

	if (msg==NULL)
		return FALSE;

	QueueNode * node=new QueueNode;
	if (node==NULL)
		return FALSE;

#ifndef POSIX
	::EnterCriticalSection(&m_csQueue);
#else
	pthread_mutex_lock(&m_csQueue);
#endif
	node->msg=msg;
	node->next=NULL;

	if (m_Header==NULL) {
		m_Header=node;
	}
	else {
		m_Tail->next=node;
	}

	m_Tail=node;

#ifndef POSIX
	::ReleaseSemaphore(m_semaphore,1,NULL);
	m_nCount++;
	::LeaveCriticalSection(&m_csQueue);
#else
	if (m_nCount==0) {
		pthread_cond_signal(&m_semaphore);
		
	}
	m_nCount++;
	pthread_mutex_unlock(&m_csQueue);
#endif
	return TRUE;
}

CVNNetMsg * CVNQueue::getNode()
{

#ifndef POSIX
	::EnterCriticalSection(&m_csQueue);
#else
	pthread_mutex_lock(&m_csQueue);
#endif

	if ((m_Header==NULL)||(m_Tail==NULL)) {
#ifndef POSIX
		::LeaveCriticalSection(&m_csQueue);
#else
		pthread_mutex_unlock(&m_csQueue);
#endif

		return NULL;
	}

	QueueNode * node=m_Header;

	m_Header=node->next;

	CVNNetMsg * msg=node->msg;

	delete node;

	m_nCount--;
#ifndef POSIX
	::LeaveCriticalSection(&m_csQueue);
#else
	pthread_mutex_unlock(&m_csQueue);
#endif
	return msg;
}

void CVNQueue::resetQueue()
{
#ifndef POSIX
	::EnterCriticalSection(&m_csQueue);
#else
	pthread_mutex_lock(&m_csQueue);
#endif

	if (m_Header!=NULL) {
		
		QueueNode * curnode=m_Header;
		QueueNode * nextnode=curnode->next;
		while (curnode!=NULL) {
			delete curnode->msg;
			delete curnode;
			curnode=nextnode;
			if (curnode!=NULL)
				nextnode=curnode->next;
		}
	}
	
	m_nCount=0;
	m_Header=NULL;
	m_Tail=NULL;

	CloseHandle(m_semaphore);
	m_semaphore=CreateSemaphore(NULL,0,0xFFFF,NULL);

#ifndef POSIX
	::LeaveCriticalSection(&m_csQueue);
#else
	pthread_mutex_unlock(&m_csQueue);
#endif

}
