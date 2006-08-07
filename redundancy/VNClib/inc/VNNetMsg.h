
#ifndef __CVNNetMsg_h__
#define __CVNNetMsg_h__

#ifdef WIN32
#include <winsock2.h>
#endif

#ifdef POSIX
#include <sys/socket.h>
#include "global.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#endif

// Don't change these unless you are certain of the ramifications
enum MsgHeaderOffsets
    // Offsets are from the start of the message (byte 0)
{
    MsgType_Offset          = 0,
    MsgFlags_Offset         = MsgType_Offset + 1,
    MsgLength_Offset        = MsgFlags_Offset + 1,
    ServiceID_Offset        = MsgLength_Offset + 4,
	ConnectionID_Offset		= ServiceID_Offset + 2,
	SrcQueue_Offset			= ConnectionID_Offset + 2,
	DesQueue_Offset			= SrcQueue_Offset + 2,
    CommandID_Offset        = DesQueue_Offset + 2,
    ParamCount_Offset       = CommandID_Offset + 2,
	PeerLong_Offset			= ParamCount_Offset + 2,
    EndOfHeader             = PeerLong_Offset + 4,
    HeaderSize              = EndOfHeader
};

enum MsgParamOffsets
    // These are relative to the end of the header
{
    ParamID_Offset          = 0,
    ParamType_Offset        = ParamID_Offset + 1,
    ParamLength_Offset      = ParamType_Offset + 1,
    ParamDataStart_Offset   = ParamLength_Offset + 4
};

enum Type
{
	Response = 0x01,
	Request = 0x02,
	Test = 0x04,
	Control = 0xff
};

class CVNNetMsg
{
    public:
        enum Flags
        {
            // These are protocol level flags that are returned by the
            // server in a response message. They are used to determine
            // the result of the request.
            MsgValid = (1L << 0),
                // This indicates a valid message and the
                // sender can crack open the message parms.
            MsgServiceNotAvailable = (1L << 1),
                // This indicates the requested service does
                // not exist or has been disabled.
            MsgServiceFailed = (1L << 2)
                // This indicates the service returned
                // a critical failure. 
        };

        

        CVNNetMsg(
            char* headerStream
            );
            // Initializes the object with the stream of bytes representing 
            // the message header. 

        CVNNetMsg(
            Type type
            );
            // Creates a default message of the specified type           

        CVNNetMsg(
            Type type, 
            int defMsgLength
            );

        ~CVNNetMsg();

        void Delete();
            // Destroy message object

        int GetType();

        int GetFlags();

        int GetLength();

        int GetServiceID();

		int GetValidCode();

		int GetConnectionID();

        int GetCommandID();

		int GetSrcQueueID();

		int GetDesQueueID();

		int GetPeerID();

        int GetParamCount();

        int GetParam(
            int id,                 // the requested parameter id
            int* type,              // returns the parameter type 
            int* length,            // in/out length of data buffer or returns required length
            unsigned char* data              // out buffer
            );
            // if < 0 there was an error
            // if 0 then the required length is returned in 'length, but no data
            // is copied. This is useful for determining the required length
            // to allocate your buffer by passing 0 in the 'data' argument.
            //
            // if return is > 0 its the number of bytes copied into data

        void SetType(
            int type
            );

        void SetFlags(
            int flags
            );

        void SetServiceID(
            int id
            );

		void SetValidCode(
			int nValidCode
			);

		void SetConnectionID(
            int id
            );

		void SetSrcQueueID(
			int id
			);

		void SetDesQueueID(
			int id
			);

        void SetCommandID(
            int id
            );

		void SetPeerID(
			int nPeerLong
			);

        int GetBuffer(
            char** buffer,
            int* length
            );

        char* GetBufferParamStart();

        void InitHeader(
            char* headerStream
            );

        BOOL GetDataParamPtr(
            int id,                             // id of parameter
            char** dataPtr,                     // pointer to parameter data
            int* dataLen                        // Length of parameter data
            );
            // Provides access to the paramater within the internal
            // buffer. This data should be considered read-only.

    public:
        BOOL IsValid();

        BOOL GetInt8Param(
            int id, 
            char* value
            );

        BOOL GetInt16Param(
            int id, 
            short* value
            );

        BOOL GetInt32Param(
            int id, 
            int* value
            );

        BOOL GetStringParam(
            int id, 
            char* value, 
            int length
            );
            // The caller must be sure the buffer is large enough
            // by using GetParm() to determine the parm length first.
            // defaultValue should be null terminated, value returned is
            // null terminated.

        BOOL SetInt8Param(
            int id, 
            char value
            );

        BOOL SetInt16Param(
            int id, 
            short value
            );

        BOOL SetInt32Param(
            int id, 
            int value
            );

        BOOL SetStringParam(
            int id, 
            const char* value
            );

        void IncrBufferPos(
            int incrPos
            );

       BOOL AddParam(
            int id, 
            int type, 
            int length, 
            char* data
            );

	   void SetWritePos(int pos) {
		   if (pos<=EndOfHeader)
			   return;
			m_Pos=pos;
	   };

    protected:
        char* m_Data;
        int m_Pos;
        int m_MaxLength;

        void InitEmptyMsg(
            Type type,
            int defMsgLength
            );

    private:

        void SetParamCount(
            int count
            );

        void SetLength(
            int length
            );

        CVNNetMsg();       // prevent default construction
};


#endif /* __CVNNetMsg_h__ */
