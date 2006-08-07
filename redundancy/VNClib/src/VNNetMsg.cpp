

#include "VNNetMsg.h"
#include <stdlib.h>
#include <string.h>
#include "Nettypes.h"

// Default size of message buffer. This represent the typical 
// message size, to prevent having to realloc the buffer.
// This must be at least the size of EndOfHeader
static const int DEF_MSG_LENGTH = 128;    // in bytes

static const int DEF_EXTRA_MSG_SPACE = HeaderSize + 32;

/*****************************************************************************\
*        NAME: CVNNetMsg constructor
* DESCRIPTION: Initializes the message object using the specified header.
*              This differs slightly from InitHeader() in that it doesn't
*              check to see if there is an already buffer.
*        ARGS: char* headerStream -- must be at least EndOfHeader bytes
*     RETURNS: None
\*****************************************************************************/
CVNNetMsg::CVNNetMsg(
    char* headerStream
    )
    // Initializes the object with the stream of bytes representing 
    // the message header. 
{
       // Extract length from headerStream
    u_long* ptr = (u_long*)&headerStream[MsgLength_Offset];
    int length = ntohl(*ptr);

    // Allocate a new buffer that will hold which size
    // is larger: the default message or the one indicated in
    // the header. This is done to try and prevent reallocs
    // if we add parms later.
    m_MaxLength = max(DEF_MSG_LENGTH, EndOfHeader + length);

	char * hg=new char[m_MaxLength];
    //HGLOBAL hg = GlobalAlloc(GMEM_FIXED, m_MaxLength);
    if (hg!=NULL)
    {
        m_Data = hg;

        // Copy the new header into the buffer.        
        memcpy(m_Data, headerStream, EndOfHeader);

        // Position the write pointer to the end of the header
        m_Pos = EndOfHeader;
    }
    else 
    {
        m_Data = 0;
        m_MaxLength = 0;
        m_Pos = 0;
    }
}

/*****************************************************************************\
*        NAME: InitHeader
* DESCRIPTION: Initializes the message object using the specified header.
*              The length defined in the specified header is used to allocate
*              the buffer used for the body of the mesage.
*        ARGS: char* headerStream -- must be at least EndOfHeader bytes
*     RETURNS: None
\*****************************************************************************/
void CVNNetMsg::InitHeader(
    char* headerStream
    )
{
     // Extract length from headerStream
    u_long* ptr = (u_long*)&headerStream[MsgLength_Offset];
    int length = ntohl(*ptr);

    // Test to see if new header indicates a message larger
    // than our current buffer.
    if ((length + EndOfHeader) > m_MaxLength)
    {
        // It is larger, so free the existing buffer
        // and allocate a new one
        if (m_Data)
			delete[] m_Data;
            //GlobalFree(GlobalHandle((void*)m_Data));

        m_MaxLength = EndOfHeader + length;
		char * hg = new char[m_MaxLength];
        //HGLOBAL hg = GlobalAlloc(GMEM_FIXED, m_MaxLength);
        if (hg)
        {
            m_Data = hg;
        }
        else
        {
            m_Data = 0;
            m_MaxLength = 0;
        }

    }
    
    // Copy the new header into the buffer.        
    memcpy(m_Data, headerStream, EndOfHeader);

    // Position the write pointer to the end of the header
    m_Pos = EndOfHeader;
}

/*****************************************************************************\
*        NAME: CVNNetMsg constructor
* DESCRIPTION: Creates a default message of the specified type           
*        ARGS: Type type -- indicates type of message object
*     RETURNS: None
\*****************************************************************************/
CVNNetMsg::CVNNetMsg(
    Type type
    )
{
    InitEmptyMsg(type, DEF_MSG_LENGTH);
}

/*****************************************************************************\
*        NAME: CVNNetMsg constructor
* DESCRIPTION: Explicit constructor used to initialize the CVNNetMsg internal
*              buffer with a more appropriate buffer size.  This is an
*              optimization to avoid wasting space or in the case of large
*              messages, realloc's.
*        ARGS: Type type -- indicates type of message object
*              int defMsgLength -- specifies a default message length. 
*     RETURNS: None
\*****************************************************************************/
CVNNetMsg::CVNNetMsg(
    Type type, 
    int defMsgLength
    )
{
    // Add in extra space to include the header plus a few
    // paramater fields. This is in an attempt to avoid
    // having to do a realloc.
    InitEmptyMsg(type, DEF_EXTRA_MSG_SPACE + defMsgLength);
}

/*****************************************************************************\
*        NAME: InitEmptyMsg
* DESCRIPTION: Initializes the message object to hold an empty message of
*              the specified length.
*        ARGS: Type type -- indicates type of message object
*              int defMsgLength -- specifies a default message length. 
*     RETURNS: None
\*****************************************************************************/
void CVNNetMsg::InitEmptyMsg(
    Type type,
    int defMsgLength
    )
{
   if (defMsgLength < EndOfHeader)
    {
        // The default message length specified is not large
        // enough to hold the header. Therefore, we redefine
        // the default message length to include the header size.
        defMsgLength = EndOfHeader;
    }

    // Create the internal message buffer
	char * hg = new char[defMsgLength];
    //HGLOBAL hg = GlobalAlloc(GMEM_FIXED, defMsgLength);
    if (hg)
    {
        m_Data = hg;
    }
    else
    {
        return;
    }

    m_MaxLength = defMsgLength;

    // This sets the write pointer just beyond the header
    m_Pos = EndOfHeader;

    // Initialize only the header to ensure if the message
    // is sent it doesn't lead to unexpected behavior. We don't
    // clear the entire message as an optimization.
    memset(m_Data, 0, EndOfHeader);

    // Sets the initial message type
    SetType(type);
}

/*****************************************************************************\
*        NAME: CVNNetMsg destructor
* DESCRIPTION: 
*        ARGS: 
*     RETURNS: None
\*****************************************************************************/
CVNNetMsg::~CVNNetMsg()
{
    if (m_Data)
    {
		delete[] m_Data;
        //GlobalFree(GlobalHandle((void*)m_Data));
    }
}

/*****************************************************************************\
*        NAME: Delete
* DESCRIPTION: Used by callers to delete the message object. This is
*              to ensure the object is deleted by the same heap that
*              dynamically allocated it (via NetConnection::RecvMsg())
*        ARGS: 
*     RETURNS: None
\*****************************************************************************/
void CVNNetMsg::Delete()
{
    delete this;
}

/*****************************************************************************\
*        NAME: GetType
* DESCRIPTION: Returns the message type
*        ARGS: 
*     RETURNS: -1 if failed
*              >= 0 for message type
\*****************************************************************************/
int CVNNetMsg::GetType()
{
    if (m_Data == 0)
        return -1;

    u_char* ptr = (u_char*)&m_Data[MsgType_Offset];
    return (u_short)*ptr;
}

int CVNNetMsg::GetFlags()
{
    if (m_Data == 0)
        return -1;

    u_char* ptr = (u_char*)&m_Data[MsgFlags_Offset];
    return (u_short)*ptr;
}

int CVNNetMsg::GetLength()
{
    if (m_Data == 0)
        return -1;

    u_long* ptr = (u_long*)&m_Data[MsgLength_Offset];
    return ntohl(*ptr);
}

int CVNNetMsg::GetPeerID()
{
	if (m_Data == 0)
        return -1;

    u_long* ptr = (u_long*)&m_Data[PeerLong_Offset];
    return ntohl(*ptr);
}

int CVNNetMsg::GetServiceID()
{
    if (m_Data == 0)
        return -1;

    u_short* ptr = (u_short*)&m_Data[ServiceID_Offset];
    return ntohs(*ptr);
}

int CVNNetMsg::GetValidCode()
{
	if (m_Data == 0)
        return -1;

    u_long* ptr = (u_long*)&m_Data[PeerLong_Offset];
    return ntohl(*ptr);
}

int CVNNetMsg::GetConnectionID()
{
    if (m_Data == 0)
        return -1;

    u_short* ptr = (u_short*)&m_Data[ConnectionID_Offset];
    return ntohs(*ptr);
}

int CVNNetMsg::GetSrcQueueID()
{
    if (m_Data == 0)
        return -1;

	u_short* ptr = (u_short*)&m_Data[SrcQueue_Offset];
    return *ptr;
}

int CVNNetMsg::GetDesQueueID()
{
    if (m_Data == 0)
        return -1;

	u_short* ptr = (u_short*)&m_Data[DesQueue_Offset];
    return *ptr;
}

int CVNNetMsg::GetCommandID()
{
    if (m_Data == 0)
        return -1;

    u_short* ptr = (u_short*)&m_Data[CommandID_Offset];
    return ntohs(*ptr);
}

int CVNNetMsg::GetParamCount()
{
    if (m_Data == 0)
        return -1;

    u_short* ptr = (u_short*)&m_Data[ParamCount_Offset];
    return ntohs(*ptr);
}

int CVNNetMsg::GetParam(
    int id,                 // the requested parameter id
    int* type,              // returns the parameter type 
    int* length,            // in/out length of data buffer or returns required length
    unsigned char* data              // out buffer
    )
    // if < 0 there was an error
    // if 0 then the required length is returned in 'length, but no data
    // is copied. This is useful for determining the required length
    // to allocate your buffer by passing 0 in the 'data' argument.
    //
    // if return is > 0 its the number of bytes copied into data
{
    if (m_Data == 0)
        return -1;

    char* ptr = m_Data + EndOfHeader;   // parameters start after header
    char* end = m_Data + m_Pos;
    while (ptr < end)
    {
        u_char* ucptr = (u_char*)ptr;
        int paramID = ucptr[ParamID_Offset];
        int paramType = ucptr[ParamType_Offset];

        u_long* lptr = (u_long*)&ptr[ParamLength_Offset];
        int paramLength = htonl(*lptr);

        ptr += ParamDataStart_Offset;

        if (id == paramID)
        {
            // Found the requested parameter
            *type = paramType;                  // return found type

            if (data == 0)
            {
                // User is requesting the length of the parameter
                return 0; // ok, user only wanted the length
            }
            if (*length < paramLength)
            {
                // User supplied buffer is not large enough
                return -1; // error, user thought they had enough space
            }
			
			*length = paramLength;      // return actual length

            if ((ptr + paramLength) <= end)
            {          
                // Copy the parameter and return
                memcpy(data, ptr, paramLength);
                return *length;
            }
        }

        ptr += paramLength;
    }
    return -1;  // error, not found
}

BOOL CVNNetMsg::GetDataParamPtr(
    int id,                             // id of parameter
    char** dataPtr,                     // pointer to parameter data
    int* dataLen                        // Length of parameter data
    )
    // Provides access to the paramater within the internal
    // buffer. This data should be considered read-only.
{
    if (m_Data == 0)
        return FALSE;

    char* ptr = m_Data + EndOfHeader;   // parameters start after header
    char* end = m_Data + m_Pos;
    while (ptr < end)
    {
        u_char* ucptr = (u_char*)ptr;
        int paramID = ucptr[ParamID_Offset];
        int paramType = ucptr[ParamType_Offset];

        u_long* lptr = (u_long*)&ptr[ParamLength_Offset];
        int paramLength = htonl(*lptr);

        ptr += ParamDataStart_Offset;

        if (id == paramID)
        {
            // Ensure that we have the amount of data indicated 
            // by the parameter field.
            if ((ptr + paramLength) <= end)
            {          
                *dataLen = paramLength;      // return actual length
                *dataPtr = ptr;
                return TRUE;
            }
        }

        ptr += paramLength;
    }
    return FALSE;
}

void CVNNetMsg::SetType(
    int type
    )
{
    if (m_Data == 0)
        return;

    m_Data[MsgType_Offset] = (u_char)type;
}

void CVNNetMsg::SetFlags(
    int flags
    )
{
    if (m_Data == 0)
        return;

    m_Data[MsgFlags_Offset] = flags;
}

void CVNNetMsg::SetLength(
    int length
    )
{
    if (m_Data == 0)
        return;

    u_long* ptr = (u_long*)&m_Data[MsgLength_Offset];
    *ptr = htonl(length);
}

void CVNNetMsg::SetPeerID(
    int nPeerLong
    )
{
    if (m_Data == 0)
        return;

    u_long* ptr = (u_long*)&m_Data[PeerLong_Offset];
    *ptr = htonl(nPeerLong);
}

void CVNNetMsg::SetServiceID(
    int id
    )
{
    if (m_Data == 0)
        return;

    u_short* ptr = (u_short*)&m_Data[ServiceID_Offset];
    *ptr = htons(id);
}

void CVNNetMsg::SetValidCode(
    int nValidCode
    )
{
    if (m_Data == 0)
        return;

    u_long* ptr = (u_long*)&m_Data[PeerLong_Offset];
    *ptr = htonl(nValidCode);
}

void CVNNetMsg::SetConnectionID(
    int id
    )
{
    if (m_Data == 0)
        return;

    u_short* ptr = (u_short*)&m_Data[ConnectionID_Offset];
    *ptr = htons(id);
}

void CVNNetMsg::SetSrcQueueID(
    int id
    )
{
    if (m_Data == 0)
        return;

	u_short* ptr = (u_short*)&m_Data[SrcQueue_Offset];
    *ptr = id;
}

void CVNNetMsg::SetDesQueueID(
    int id
    )
{
    if (m_Data == 0)
        return;

	u_short* ptr = (u_short*)&m_Data[DesQueue_Offset];
    *ptr = id;
}

void CVNNetMsg::SetCommandID(
    int id
    )
{
    if (m_Data == 0)
        return;

    u_short* ptr = (u_short*)&m_Data[CommandID_Offset];
    *ptr = htons(id);
}

void CVNNetMsg::SetParamCount(
    int count
    )
{
    if (m_Data == 0)
        return;

    u_short* ptr = (u_short*)&m_Data[ParamCount_Offset];
    *ptr = htons(count);
}

BOOL CVNNetMsg::AddParam(
    int id, 
    int type, 
    int length, 
    char* data
    )
    // Current assumption is any 2, 4 byte integers are already
    // in network byte order.
{
    if (m_Data == 0)
        return FALSE;

    // Ensure the parameter doesn't exist
    int tmp;
    if (GetParam(id, &tmp, &tmp, 0) == 0)
    {
        // The parameter already exists, we don't support
        // having two parameters of the same ID. There is
        // currently no method to modify a parameter within
        // a message either.
        return FALSE;
    }

    int requiredLength = ParamDataStart_Offset + length;
    if ((m_Pos + requiredLength) > m_MaxLength)
    {
        // Need to realloc the data
        int oldLength = m_MaxLength;
        m_MaxLength = m_Pos + requiredLength;
		char * hg = new char[m_MaxLength];
        //HGLOBAL hg = GlobalAlloc(GMEM_FIXED, m_MaxLength);
        if (hg)
        {
            char* data = hg;
            memcpy(data, m_Data, oldLength);
			delete[] m_Data;
            //GlobalFree(GlobalHandle((void*)m_Data));
            m_Data = data;          
        } 
        else
        {
            m_Data = 0;
            m_MaxLength = 0;
            return FALSE;
        }
    }

    // Update parmaeter count
    SetParamCount(GetParamCount() + 1);

    u_char* ptr = (u_char*)&m_Data[m_Pos];   
    ptr[ParamID_Offset] = id;
    ptr[ParamType_Offset] = type;

    u_long* lptr = (u_long*)&ptr[ParamLength_Offset];
    *lptr = htonl(length);

    memcpy(&ptr[ParamDataStart_Offset], data, length);

    m_Pos += requiredLength;

    // Update message length
    int addedLength = ParamDataStart_Offset + length;
    SetLength(GetLength() + addedLength);

    return TRUE;
}

int CVNNetMsg::GetBuffer(
    char** buffer,
    int* length
    )
{
    if (m_Data == 0)
    {
        *buffer = 0;
        return -1;
    }

    *buffer = &m_Data[0];
    *length = m_Pos;
    return 0;
}

void CVNNetMsg::IncrBufferPos(
    int incrPos
    )
{
    if (m_Data == 0)
        return;

    m_Pos += incrPos;
}

char* CVNNetMsg::GetBufferParamStart()
{
    if (m_Data == 0)
        return 0;

    return &m_Data[EndOfHeader];
}

BOOL CVNNetMsg::GetInt8Param(
    int id, 
    char* value
    )
{
    if (m_Data == 0)
        return FALSE;

    int type;
    char ucval;
    int len = 1;
    if ((GetParam(id, &type, &len, (u_char*)&ucval) < 0) ||
        (type != Param_Int8)
        )
    {
        // Failed to get parameter or incorrect type
        return FALSE;
    }
    *value = ucval;
    return TRUE;
}

BOOL CVNNetMsg::GetInt16Param(
    int id, 
    short* value
    )
{
    if (m_Data == 0)
        return FALSE;

    int type;
    short usval;
    int len = 2;
    if ((GetParam(id, &type, &len, (u_char*)&usval) < 0) ||
        (type != Param_Int16)
        )
    {
        // Failed to get parameter or incorrect type
        return FALSE;
    }
    *value = ntohs(usval);      // convert from network to host byte-order
    return TRUE;
}

BOOL CVNNetMsg::GetInt32Param(
    int id, 
    int* value
    )
{
    if (m_Data == 0)
        return FALSE;

    int type;
    u_long ulval;
    int len = 4;
    if ((GetParam(id, &type, &len, (u_char*)&ulval) < 0) ||
        (type != Param_Int32)
        )
    {
        // Failed to get parameter or incorrect type
        return FALSE;
    }
    *value = ntohl(ulval);      // convert from network to host byte-order  
    return TRUE;
}

BOOL CVNNetMsg::GetStringParam(
    int id, 
    char* value, 
    int length
    )
    // The caller must be sure the buffer is large enough
    // by using GetParm() to determine the parm length first.
{
    if (m_Data == 0)
        return FALSE;

    int type;
    int valueLen = GetParam(id, &type, &length, (u_char*)value);
    if ((valueLen < 0) ||
        (type != Param_String)
        )
    {
        // Failed to get parameter or incorrect type
        return FALSE;
    }

    // null terminate returning data
    value[valueLen] = 0; 
    return TRUE;
}

BOOL CVNNetMsg::SetInt8Param(
    int id, 
    char value
    )
{
    if (m_Data == 0)
        return FALSE;

    u_char val = (u_char)value;
    return AddParam(id, Param_Int8, 1, (char*)&val);
}

BOOL CVNNetMsg::SetInt16Param(
    int id, 
    short value
    )
{
    if (m_Data == 0)
        return FALSE;

    short val = htons((u_short)value);
    return AddParam(id, Param_Int16, 2, (char*)&val);
}

BOOL CVNNetMsg::SetInt32Param(
    int id, 
    int value
    )
{
    if (m_Data == 0)
        return FALSE;

    u_long val = htonl((u_long)value);
    return AddParam(id, Param_Int32, 4, (char*)&val);
}

BOOL CVNNetMsg::SetStringParam(
    int id, 
    const char* value
    )
{
    if (m_Data == 0)
        return FALSE;

    int length = strlen(value);
    return AddParam(id, Param_String, length, (char *)value);
}

BOOL CVNNetMsg::IsValid()
{
    if (m_Data == 0)
        return FALSE;

    unsigned short flags = GetFlags();
    return (flags & MsgValid) ? TRUE : FALSE;
}
