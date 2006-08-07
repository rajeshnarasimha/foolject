
#ifndef __NetConnection_h__
#define __NetConnection_h__

#include "debug.h"

#ifdef WIN32
#include <stdlib.h>
#endif
#include "VNNetMsg.h"
enum RSINFO
{
	// Exterr values used by RecvMsg()
	RS_Success = 0,
	RS_Timeout = -1,
	RS_Event = -2,
	RS_Failed = -3,
	RS_Closed = -4,
	RS_NOCHAN=-5		
};

#define SO_SNDBUF_SIZE 64*1024
#define SO_RCVBUF_SIZE 64*1024

class CVNChannel
{
    public:
	    void SetSocket(SOCKET sNew);

		void Disconnect();
        

        CVNChannel();
            // Default constructor

        CVNChannel(
            SOCKET s                       // socket handle of accepted connection
            );
            // Explicit constructor initializes connection with
            // an already established socket connection

        ~CVNChannel();
            // Destructor will close the connection

		void SetServAddr(long nAddr) {m_addrPeer=nAddr;};
		void SetServPort(int nPort) {m_nPort=nPort;};
		
		BOOL Open(long nAddr,	                 // IP address in "127.0.0.1" dotted format
            int port,                       // Port used by NetVoice
            int timeout  ) {
#ifndef POSIX
			EnterCriticalSection(&m_csEvent);	
#else
			pthread_mutex_lock(&m_csEvent);
#endif
			if (nAddr!=-1)
				m_addrPeer=nAddr;
			if (port!=-1)
				m_nPort=port;
			BOOL bRet= OpenCore(m_addrPeer,m_nPort,timeout);
#ifndef POSIX
			LeaveCriticalSection(&m_csEvent);
#else
			pthread_mutex_unlock(&m_csEvent);
#endif
			return bRet;
		};

        BOOL OpenCore(
            long nAddr,	                 // IP address in "127.0.0.1" dotted format
            int port,                       // Port used by NetVoice
            int timeout                     // Maximum time to wait for connection in seconds
            );
            // Establishes a client connection with a NetVoice server

        void Delete();
            // Used to delete the object

        CVNNetMsg* RecvMsg(
            int* exterr = 0,
            HANDLE extevt =0,
			unsigned int nTimeout=5000
            );
            // Receives a response message
            //
            // Returns null if no message to receive and error or user event occurs
            // If exterr is not null, an extended error is returned
            //      Recv_Success == no error
            //      Recv_Timeout == timeout error
            //      Recv_Event == external event becomes signalled

        int LoadMsg(
            CVNNetMsg& msg,
            HANDLE extevt
            );
            // Receives a response message
            //
            // Same functionality as RecvMsg(), but instead of creating the
            // message on the stack, it uses an existing message object to
            // populate with the received data.
            // Returns same as exterrr above:
            //      Recv_Success == no error
            //      Recv_Timeout == timeout error
            //      Recv_Event == external event becomes signalled

        int SendMsg(CVNNetMsg* msg,HANDLE extevt);
            // Sends a request message to NetVoice
            //
            // Returns >=0 on success
		void SetTimeout(unsigned int nRecvTimeout,unsigned int nSendTimeout) ;
		
		int GetValidCode() {return m_nValidCode;};
		void ResetValidCode() {m_nValidCode=0;};
		void SetValidCode(int nValidCode) {m_nValidCode=nValidCode;};
		int AssignValidCode() 
		{
			do {
				m_nValidCode=rand();
			}while (m_nValidCode==0);
			return m_nValidCode;
		};

		void SetChannelID(unsigned int nChanID) {m_nID=nChanID;};
		unsigned int GetChannelID() {return m_nID;};

        SOCKET m_Socket;        // this is the connected socket

		BOOL OpenBroadCastMode(BOOL bOpen=TRUE);
		int SendBroadCastMsg(const char * lpCastAddr,CVNNetMsg* msg,int port);
		int SendUDPMsg(const char * lpCastAddr,CVNNetMsg* msg,int port);
		BOOL BindBroadCastMode(int port,BOOL bBroad=TRUE);
		CVNNetMsg* RecvBroadCastMsg(
			HANDLE extevt,
			int nLen,
			char * addrSender,
			int nAddrLen,
			unsigned int nTimeout=5000
			);

		void SetPeerLong() {
			sockaddr_in name;
			int len=sizeof(name);
			::getpeername(m_Socket,(sockaddr *)&name,&len);
			m_addrPeer=name.sin_addr.S_un.S_addr;
		};
		long GetPeerLong() {return m_addrPeer;};
		void ResetPeerLong() {m_addrPeer=INADDR_NONE;};


		BOOL GetPeerAddr(char * lpAddr,int nLen);

		//void AddConnectionRef() {
		//	::InterlockedIncrement(&m_nRef);
		//};
		//void ReduceConnectionRef() {
		//	::InterlockedDecrement(&m_nRef);
		//};

		//long GetConnectionRef() {
		//	return m_nRef;
		//};

    protected:
        int RecvHeader(
            char* headerBuf,
            HANDLE extevt,
			unsigned int nTimeout
            );

        int RecvBody(
            CVNNetMsg* msg,
            HANDLE extevt
            );
		int SendBroadCastMsgCore(const char * lpAddrCast,CVNNetMsg* msg,int port);
	private:

		unsigned int m_nRecvTimeout;      // 5 minutes
		unsigned int m_nSendTimeout;               // 25 seconds  
#ifndef POSIX
		CRITICAL_SECTION m_csEvent;
#else
		pthread_mutex_t m_csEvent;
#endif

		int m_nValidCode;

		unsigned int m_nID;

		int SendMsgCore(CVNNetMsg* msg,HANDLE extevt);

		long m_addrPeer;
		int m_nPort;

		long m_nRef;

};


#endif /* __NetConnection_h__ */
