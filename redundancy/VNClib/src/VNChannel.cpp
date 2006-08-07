
#include <time.h>
#include "VNChannel.h"
#include "VNNetMsg.h"

//#include "debug.h"

#ifdef   ONLYFOR_RM
#define   SENDTIMEOUT   100
#else
#define   SENDTIMEOUT   1000
#endif

CVNChannel::CVNChannel() :
    m_Socket(INVALID_SOCKET)
{
		m_nRecvTimeout=5000;
		m_nSendTimeout=SENDTIMEOUT;
		m_nID=0;
		m_addrPeer=INADDR_NONE;
		m_nPort=0;
		m_nValidCode=0;
		m_Socket=INVALID_SOCKET;
#ifndef POSIX
		InitializeCriticalSection(&m_csEvent);
#else
		pthread_mutex_init(&m_csEvent,NULL);
#endif
	}

CVNChannel::CVNChannel(	SOCKET s                       // socket handle of accepted connection
					   ) :m_Socket(s)
{

	m_nRecvTimeout=5000;
	m_nSendTimeout=SENDTIMEOUT;
	m_nID=0;
	m_addrPeer=INADDR_NONE;
	m_nPort=0;
	m_nValidCode=0;
#ifndef POSIX
	InitializeCriticalSection(&m_csEvent);
#else
	pthread_mutex_init(&m_csEvent,NULL);
#endif
}

CVNChannel::~CVNChannel()
{
	Disconnect();
	m_nID=0;
#ifndef POSIX
	::DeleteCriticalSection(&m_csEvent);
#else
	pthread_mutex_destroy(&m_csEvent);
#endif
}    

void CVNChannel::SetTimeout(unsigned int nRecvTimeout,unsigned int nSendTimeout) 
{
	m_nRecvTimeout=nRecvTimeout;
	m_nSendTimeout=nSendTimeout;
}

BOOL CVNChannel::OpenCore(
    long nAddr,			            // IP address in "127.0.0.1" dotted format
    int port,		                // Port used by NetVoice
    int timeout                     // Maximum time to wait for connection in seconds
    )
{
    if (m_Socket != INVALID_SOCKET)
    {
        // Connection is in an unknown state
#ifdef FOOLBEAR_DEBUG
		LogByLevel(DSS_VP,DEBUGLVL, 0, 0,"FOOLBEAR_DEBUG: [OpenCore]m_Socket != INVALID_SOCKET(%d)", 
			m_Socket);
#endif
        return FALSE;
    }

	if (nAddr==INADDR_NONE) {
#ifdef FOOLBEAR_DEBUG
		LogByLevel(DSS_VP,DEBUGLVL, 0, 0,"FOOLBEAR_DEBUG: [OpenCore]nAddr==INADDR_NONE(%d)", nAddr);
#endif
		return FALSE;
	}

    BOOL connected = FALSE;
    SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
         // For some reason, using WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, 0);
         // to create the socket would cause the connect() below to fail
         // when setting the socket to non-blocking by using WSAEventSelect
    if (Socket != INVALID_SOCKET)
    {
			int nRet=0;

#ifdef POSIX
		nRet=fcntl(Socket,F_SETFL,O_NONBLOCK);
#else
		unsigned long ul=1;
		nRet=ioctlsocket(Socket,FIONBIO,(unsigned long *)&ul);
#endif
		
		if (nRet==SOCKET_ERROR) {
			////printf("initchannel faild,reson=%s\n",sys_errlist[errno]);
#ifdef FOOLBEAR_DEBUG
			LogByLevel(DSS_VP,DEBUGLVL, 0, 0,"FOOLBEAR_DEBUG: [OpenCore]ioctlsocket return SOCKET_ERROR for %d", 
				WSAGetLastError());
#endif
			return FALSE;
		}

        int optvalue = SO_RCVBUF_SIZE;
        setsockopt(Socket,SOL_SOCKET,SO_RCVBUF,(char*)&optvalue,sizeof(int));
        optvalue = SO_SNDBUF_SIZE;
        setsockopt(Socket,SOL_SOCKET,SO_SNDBUF,(char*)&optvalue,sizeof(int));
        
        SOCKADDR_IN sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
		sa.sin_addr.s_addr=nAddr;

                // Initiate the connection
		////printf("start to connect \n");
        if (connect(Socket, (SOCKADDR*)&sa, sizeof(SOCKADDR_IN))==0) {
            connected = TRUE;
        } else {
#ifdef FOOLBEAR_DEBUG
            LogByLevel(DSS_VP,DEBUGLVL, 0, 0,"FOOLBEAR_DEBUG: [OpenCore]connect return failed for %d", 
				WSAGetLastError());
#endif
			//printf("start to wait select\n");
            fd_set fdwrite;
			FD_ZERO(&fdwrite);
			FD_SET(Socket,&fdwrite);

			struct timeval tvalout;
			if (timeout>1000) {
				tvalout.tv_sec=timeout/1000;
				tvalout.tv_usec=0;
			}
			else {
				tvalout.tv_sec=0;
				tvalout.tv_usec=timeout*1000;
			}

			//printf("start to wait select\n");
			switch (select(Socket + 1, NULL,&fdwrite, NULL, &tvalout)) {  
			case -1:  
#ifdef FOOLBEAR_DEBUG
				LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
					"FOOLBEAR_DEBUG: select return SOCKET_ERROR for %d in OpenCore", 
					WSAGetLastError());
#endif
				connected=FALSE;
			case 0:  
				connected=FALSE;
			default:  
				if (FD_ISSET(Socket,&fdwrite)) {
					int error=0;
					int errlen=sizeof(int);
					getsockopt(Socket, SOL_SOCKET, SO_ERROR, (char*)&error, &errlen); 
					if (error==0)
						connected=TRUE;
					else {
#ifdef FOOLBEAR_DEBUG
						LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
							"FOOLBEAR_DEBUG: getsockopt return SOCKET_ERROR for %d in OpenCore", 
							WSAGetLastError());
#endif
						connected=FALSE;
					}
				}  
			}  
        }

        if (connected)
        {
            // Save socket handle
            m_Socket = Socket;
        }
        else
        {
            // Failed to establish connection. close the passive socket
            closesocket(Socket);
			Socket=INVALID_SOCKET;
        }
	} else {
#ifdef FOOLBEAR_DEBUG
		LogByLevel(DSS_VP, CRITICALLVL, 0, 0, "FOOLBEAR_DEBUG: Socket == INVALID_SOCKET in OpenCore");
#endif
	}

    return connected;    
}
    

void CVNChannel::Delete()
{
    delete this;
}


/*****************************************************************************\
*        NAME: LoadMsg
* DESCRIPTION: Loads a message header and body from the socket. If the
*              function does not return Recv_Success the contents of the
*              NetMsg object are undefined.
*        ARGS: NetMsg& msg -- the message to load
*              HANDLE extevt -- external event when signalled recv returns
*     RETURNS: Recv_Success -- if the entire header was received
*              Recv_Timeout -- if no response was received within the timeout
*              Recv_Event -- the external event was signalled
*              Recv_Failed -- receive failed
\*****************************************************************************/
int CVNChannel::LoadMsg(
    CVNNetMsg& msg,
    HANDLE extevt
    )
    // Same functionality as RecvMsg(), but instead of creating the
    // message on the stack, it uses an existing message object to
    // populate with the received data.
    // Returns same as exterrr above:
    //      Recv_Success == no error
    //      Recv_Timeout == timeout error
    //      Recv_Event == external event becomes signalled
{
    char header[EndOfHeader];

    int rc = RecvHeader(header, extevt,m_nRecvTimeout);
    if (rc == RS_Success)
    {
        msg.InitHeader(header);
        rc = RecvBody(&msg, extevt);
    }
    return rc;
}

/*****************************************************************************\
*        NAME: RecvMsg
* DESCRIPTION: Receives a message, returning a new NetMsg object object. The
*              caller is responsible for destroying the object. This version
*              is slightly more efficient than LoadMsg() since it avoids
*              the default construction of the NetMsg object.
*        ARGS: int* exterr -- returns the extended error, which can be:
*                Recv_Success -- if the entire header was received
*                Recv_Timeout -- if no response was received within the timeout
*                Recv_Event -- the external event was signalled
*                Recv_Failed -- receive failed
*              HANDLE extevt -- external event when signalled recv returns
*     RETURNS: NetMsg pointer if successful
*              0 if failed
\*****************************************************************************/
CVNNetMsg* CVNChannel::RecvMsg(
    int* exterr,   /*=0*/
    HANDLE extevt,  /*=0*/
	unsigned int nTimeout
    )
    // Returns null if no message to receive or error
    // If exterr is not null, an extended error is returned
    //      Recv_Timeout == timeout error
    //      Recv_Event == returning because external event signalled
{
    CVNNetMsg* msg = 0;
	int rc=0;
	//EnterCriticalSection(&m_csEvent);
	if (m_Socket==INVALID_SOCKET) {
#ifdef FOOLBEAR_DEBUG
		LogByLevel(DSS_VP, DEBUGLVL, 0, 0, 
			"FOOLBEAR_DEBUG: m_Socket==INVALID_SOCKET in CVNChannel::RecvMsg");
#endif
		rc=RS_Failed;
	}
	else {
		char header[EndOfHeader];
		rc = RecvHeader(header, extevt,nTimeout);
		if (rc == RS_Success)
		{
			msg = new CVNNetMsg(header);
			
			if ((rc = RecvBody(msg, extevt)) != RS_Success)
			{
				delete msg;
				
				msg = 0;
				
				closesocket(m_Socket);
				m_Socket=INVALID_SOCKET;
#ifdef FOOLBEAR_DEBUG
				LogByLevel(DSS_VP, DEBUGLVL, 0, 0, 
					"FOOLBEAR_DEBUG: RecvBody failed in CVNChannel::RecvMsg");
#endif
			}
		}
		else {
			if (rc!=RS_Timeout) {
				closesocket(m_Socket);
				m_Socket=INVALID_SOCKET;
#ifdef FOOLBEAR_DEBUG
				LogByLevel(DSS_VP, DEBUGLVL, 0, 0, 
					"FOOLBEAR_DEBUG: RecvHeader failed and not RS_Timeout in CVNChannel::RecvMsg");
#endif
			}
		}
	}
	
	if (exterr)
		*exterr = rc;
	
	//LeaveCriticalSection(&m_csEvent);
	
	return msg;    
}


/*****************************************************************************\
*        NAME: RecvHeader
* DESCRIPTION: Receives the header from the socket. This must be called 
*              with a user supplied buffer that must be large enough to
*              hold the entire header. 
*        ARGS: char* headerBuf -- pointer to buffer 
*              HANDLE extevt -- external event when signalled recv returns
*     RETURNS: Recv_Success -- if the entire header was received
*              Recv_Timeout -- if no response was received within the timeout
*              Recv_Event -- the external event was signalled
*              Recv_Failed -- receive failed
\*****************************************************************************/
int CVNChannel::RecvHeader(
    char* headerBuf,
    HANDLE extevt,
	unsigned int nTimeout
    )
{
    int pos = 0;

	fd_set fdread;
	
    while (pos < EndOfHeader)
    {
        
		FD_ZERO(&fdread);
		FD_SET(m_Socket,&fdread);
		
		struct timeval tvalout;
		if (nTimeout>1000) {
			tvalout.tv_sec=nTimeout/1000;
			tvalout.tv_usec=0;
		}
		else {
			tvalout.tv_sec=0;
			tvalout.tv_usec=nTimeout*1000;
		}
		
		switch (select(m_Socket + 1, &fdread,NULL, NULL, &tvalout)) {  
		case -1: 
#ifdef FOOLBEAR_DEBUG
			LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
				"FOOLBEAR_DEBUG: select return SOCKET_ERROR for %d in RecvHeader", 
				WSAGetLastError());
#endif
			return RS_Failed;
        case 0:
            return RS_Timeout;
        default: 
			int count = recv(m_Socket, &headerBuf[pos], EndOfHeader-pos, 0);
			if (count == SOCKET_ERROR)				
				LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
					"VNC recv return SOCKET_ERROR for %d in RecvHeader", WSAGetLastError());
			if (count == 0 || count == -1)
			{
				// Closed or an error, either signal the message
				// loop that's calling this method to stop calling
				// RecvMsg().
				return RS_Failed;
			}
			pos += count;
		}
    }
    return RS_Success;
}

/*****************************************************************************\
*        NAME: RecvBody
* DESCRIPTION: Receives the message body given an NetMsg object that has
*              been initialized with the message header.
*        ARGS: NetMsg* msg -- message containing header that will receive the
*                   message body.
*              HANDLE extevt -- external event when signalled recv returns
*     RETURNS: Recv_Success -- if the entire body was received
*              Recv_Timeout -- if no response was received within the timeout
*              Recv_Event -- the external event was signalled
*              Recv_Failed -- receive failed
\*****************************************************************************/
int CVNChannel::RecvBody(
    CVNNetMsg* msg,
    HANDLE extevt
    )
{
    int pos = 0;

	fd_set fdread;
    // Have entire header
    int length = msg->GetLength();    // length remaining to receive

    // Obtain pointer to start of parameter area. This is guaranteed
    // to be enough space to contain 'length' bytes.
    char* buffer = msg->GetBufferParamStart();
    while (pos < length)
    {
        
        FD_ZERO(&fdread);
		FD_SET(m_Socket,&fdread);
		
		struct timeval tvalout;
		if (m_nRecvTimeout>1000) {
			tvalout.tv_sec=m_nRecvTimeout/1000;
			tvalout.tv_usec=0;
		}
		else {
			tvalout.tv_sec=0;
			tvalout.tv_usec=m_nRecvTimeout*1000;
		}

		switch (select(m_Socket + 1,&fdread,NULL, NULL, &tvalout)) {  
		case -1:
#ifdef FOOLBEAR_DEBUG
			LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
				"FOOLBEAR_DEBUG: select return SOCKET_ERROR for %d in RecvBody", 
				WSAGetLastError());
#endif
			return RS_Failed;
        case 0:
            return RS_Failed;
        default: 		
			int count = recv(m_Socket, &buffer[pos], length - pos, 0);
			if (count == SOCKET_ERROR)				
				LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
					"VNC recv return SOCKET_ERROR for %d in RecvBody", WSAGetLastError());
			if (count == 0 || count == -1)
			{
				// Closed or an error, either signal the message
				// loop that's calling this method to stop calling
				// RecvMsg().
				return RS_Failed;
			}
			pos += count;
		}
	}
    // Update the buffer write position based on what we actually
    // received from the client.
	msg->IncrBufferPos(pos);
	return RS_Success;
}

int CVNChannel::SendMsg(CVNNetMsg * msg,HANDLE extevt) 
{
	int rc=0;
#ifndef POSIX
	EnterCriticalSection(&m_csEvent);	
#else
	pthread_mutex_lock(&m_csEvent);
#endif	

	if (m_Socket==INVALID_SOCKET) {		
#ifdef FOOLBEAR_DEBUG
		LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
			"FOOLBEAR_DEBUG: m_Socket==INVALID_SOCKET in SendMsg");
#endif
		rc= RS_Failed;
	} else {
		rc=SendMsgCore(msg,extevt);
		if (rc!=RS_Timeout) {
			if (rc<0) {	
#ifdef FOOLBEAR_DEBUG
				LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
					"FOOLBEAR_DEBUG: SendMsgCore return %d in SendMsg, error", rc);
#endif
				closesocket(m_Socket);
				m_Socket=INVALID_SOCKET;
			}
		}
	}
#ifndef POSIX
	LeaveCriticalSection(&m_csEvent);
#else
	pthread_mutex_unlock(&m_csEvent);
#endif	
	return rc;
}
int CVNChannel::SendMsgCore(CVNNetMsg* msg,HANDLE extevt)
{
    char* buffer=NULL;
    int length=0;
    int rc = -1;

	fd_set fdwrite;

    if (msg->GetBuffer(&buffer, &length) >= 0)
    {
		
        int pos = 0;
        while (pos < length)
        {
			FD_ZERO(&fdwrite);
			FD_SET(m_Socket,&fdwrite);
			
			struct timeval tvalout;
			if (m_nSendTimeout>1000) {
				tvalout.tv_sec=m_nSendTimeout/1000;
				tvalout.tv_usec=0;
			}
			else {
				tvalout.tv_sec=0;
				tvalout.tv_usec=m_nSendTimeout*1000;
			}
			
			switch (select(m_Socket + 1, NULL,&fdwrite, NULL, &tvalout)) {  
			case -1://SOCKET_ERROR
#ifdef FOOLBEAR_DEBUG
				LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
					"FOOLBEAR_DEBUG: select return SOCKET_ERROR for %d in SendMsgCore", WSAGetLastError());
#endif
				return RS_Failed;
			case 0:
				return RS_Timeout;
			default: 		
				int count = send(m_Socket, &buffer[pos], length-pos, 0);
				if (count == -1) {//SOCKET_ERROR
#ifdef FOOLBEAR_DEBUG
					LogByLevel(DSS_VP, CRITICALLVL, 0, 0, 
						"FOOLBEAR_DEBUG: send return SOCKET_ERROR for %d in SendMsgCore", WSAGetLastError());
#endif
					return RS_Failed;
				}
				pos += count;
				rc+=count;
			}
        }       
    }

    return rc;
}

void CVNChannel::SetSocket(SOCKET sNew)
{
	m_Socket=sNew;
}

void CVNChannel::Disconnect()
{
    if (m_Socket != INVALID_SOCKET)
    {
        shutdown(m_Socket, SD_BOTH);
        closesocket(m_Socket);
		//printf("Set m_Socket invald in disconnect\n");
		m_Socket=INVALID_SOCKET;
		m_addrPeer=0;
    }
	m_nValidCode=0;
}

BOOL CVNChannel::BindBroadCastMode(int port,BOOL bBroad)
{
	if (OpenBroadCastMode(bBroad)==FALSE)
		return FALSE;

	SOCKADDR_IN local;

	local.sin_family=AF_INET;
	local.sin_port=htons((short)port);
	local.sin_addr.s_addr=htonl(INADDR_ANY);

	if (bind(m_Socket,(SOCKADDR*)&local,sizeof(local))==SOCKET_ERROR)
		return FALSE;

	return TRUE;
}

BOOL CVNChannel::OpenBroadCastMode(BOOL bOpen)
{
	BOOL bCast=FALSE;
	if (m_Socket!=INVALID_SOCKET) {
		if (bOpen==TRUE) {
			int bufsize=sizeof(bCast);
			::getsockopt(m_Socket,SOL_SOCKET,SO_BROADCAST,(char *)&bCast,&bufsize);
			if (bCast==FALSE)
				return FALSE;
		}
	}
	else {
		m_Socket=socket(AF_INET,SOCK_DGRAM,0);
		if (m_Socket==INVALID_SOCKET)
			return FALSE;
		if (bOpen==TRUE) {
			bCast=TRUE;
			setsockopt(m_Socket,SOL_SOCKET,SO_BROADCAST,(char*)&bCast,sizeof(bCast));
		}
	}
	return TRUE;
}

int CVNChannel::SendBroadCastMsg(const char * lpCastAddr,CVNNetMsg* msg,int port)
{	
	if (lpCastAddr==NULL)
		return RS_Failed;

	if (OpenBroadCastMode()==FALSE)
		return -1;

	//EnterCriticalSection(&m_csEvent);
	int rc=SendBroadCastMsgCore(lpCastAddr,msg,port);
	if (rc<0) {
		closesocket(m_Socket);
		m_Socket=INVALID_SOCKET;
	}
	//LeaveCriticalSection(&m_csEvent);
	return rc;
}

int CVNChannel::SendUDPMsg(const char * lpCastAddr,CVNNetMsg* msg,int port)
{	
	if (lpCastAddr==NULL)
		return RS_Failed;

	if (OpenBroadCastMode(FALSE)==FALSE)
		return -1;

	int rc=SendBroadCastMsgCore(lpCastAddr,msg,port);
	if (rc<0) {
		closesocket(m_Socket);
		m_Socket=INVALID_SOCKET;
	}
	return rc;
}

int CVNChannel::SendBroadCastMsgCore(const char * lpAddrCast,CVNNetMsg* msg,int port)
{
	if (lpAddrCast==NULL)
		return RS_Failed;

	SOCKADDR_IN addrCast;

	addrCast.sin_family=AF_INET;
	addrCast.sin_addr.s_addr=inet_addr(lpAddrCast);
	addrCast.sin_port=htons(port);

	char* buffer=NULL;
    int length=0;
    int rc = -1;
	
	fd_set fdwrite;

    if (msg->GetBuffer(&buffer, &length) >= 0)
    {
		FD_ZERO(&fdwrite);
		FD_SET(m_Socket,&fdwrite);
		int pos = 0;
        while (pos < length)
        {
			
			struct timeval tvalout;
			if (m_nSendTimeout>1000) {
				tvalout.tv_sec=m_nSendTimeout/1000;
				tvalout.tv_usec=0;
			}
			else {
				tvalout.tv_sec=0;
				tvalout.tv_usec=m_nSendTimeout*1000;
			}
			
			switch (select(m_Socket + 1, NULL,&fdwrite, NULL, &tvalout)) {  
			case -1:
				return RS_Failed;
			case 0:
				return RS_Timeout;
			default: 		
				int count = sendto(m_Socket, &buffer[pos], length-pos, 0,(SOCKADDR*)&addrCast,sizeof(addrCast));
				if (count == -1)
				{
					// socket error
					return RS_Failed;
				}
				pos += count;
				rc+=count;
			}
		}
		
    }
    return rc;
}

CVNNetMsg* CVNChannel::RecvBroadCastMsg(
    HANDLE extevt,
	int nLen,
	char * addrSender,
	int nAddrLen,
	unsigned int nTimeout
    )
    // Returns null if no message to receive or error
    // If exterr is not null, an extended error is returned
    //      Recv_Timeout == timeout error
    //      Recv_Event == returning because external event signalled
{
    fd_set fdread;

	FD_ZERO(&fdread);
	FD_SET(m_Socket,&fdread);
	
	struct timeval tvalout;
	if (nTimeout>1000) {
		tvalout.tv_sec=nTimeout/1000;
		tvalout.tv_usec=0;
	}
	else {
		tvalout.tv_sec=0;
		tvalout.tv_usec=nTimeout*1000;
	}
	
	switch (select(m_Socket + 1, &fdread,NULL, NULL, &tvalout)) {  
	case -1:
		return NULL;
	case 0:
		return NULL;
	default: 
		
		SOCKADDR_IN sender;
		int nSender=sizeof(sender);
		
		CVNNetMsg * msg=new CVNNetMsg(Request,nLen);
		char * buf=NULL;
		int nSize=0;
		msg->GetBuffer(&buf,&nSize);
		
		int count = recvfrom(m_Socket, buf, nLen,0,(SOCKADDR*)&sender,&nSender);
		if (count == 0 || count == -1)
		{
			// Closed or an error, either signal the message
			// loop that's calling this method to stop calling
			// RecvMsg().
			closesocket(m_Socket);
			m_Socket=INVALID_SOCKET;
			return NULL;
		}
		
		msg->SetWritePos(count);
		
		strncpy(addrSender,inet_ntoa(sender.sin_addr),nAddrLen);

		return msg;
	}

	return NULL;
}

BOOL CVNChannel::GetPeerAddr(char * lpAddr,int nLen)
{
	if (m_Socket==INVALID_SOCKET)
		return FALSE;
	if (lpAddr==NULL)
		return FALSE;
	if (nLen<=0)
		return FALSE;

	SOCKADDR_IN sender;
	int nSender=sizeof(sender);
	::getpeername(m_Socket,(SOCKADDR*)&sender,&nSender);

	strncpy(lpAddr,inet_ntoa(sender.sin_addr),nLen);

	return TRUE;
}
