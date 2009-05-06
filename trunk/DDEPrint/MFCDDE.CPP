/*
 * File:	mfcdde.cc
 * Purpose:	DDE class for MFC
 * Author:	Julian Smart
 */

// For compilers that support precompilation, includes "wx.h".
#include "stdafx.h"
#include "windows.h"
// #include "wx_utils.h"

#include "mfcdde.h"
#include <ddeml.h>

#ifdef WIN32
#define _EXPORT /**/
#else
#define _EXPORT _export
#endif

static CDDEConnection *DDEFindConnection(HCONV hConv);
static void DDEDeleteConnection(HCONV hConv);
static CDDEServer *DDEFindServer(const CString& s);

extern "C" HDDEDATA EXPENTRY _EXPORT _DDECallback(
WORD wType,
WORD wFmt,
HCONV hConv,
HSZ hsz1,
HSZ hsz2,
HDDEDATA hData,
DWORD lData1,
DWORD lData2);

// Add topic name to atom table before using in conversations
static HSZ DDEAddAtom(const CString& string);
static HSZ DDEGetAtom(const CString& string);
static void DDEPrintError(void);

static DWORD DDEIdInst = 0L;
static CDDEConnection *DDECurrentlyConnecting = NULL;

static CMapStringToOb DDEAtomTable;

static CObList DDEObjects;

char *DDEDefaultIPCBuffer = NULL;
int DDEDefaultIPCBufferSize = 0;

//static bool bDisconnected = false;

/*
 * Initialization
 *
 */

//BOOL DDEInitialized = FALSE;

void DDEInitialize()
{
  if (DDEIdInst)//DDEInitialized)
    return;
  //DDEInitialized = TRUE;

  // Should insert filter flags
  DdeInitialize(&DDEIdInst, (PFNCALLBACK)MakeProcInstance(
               (FARPROC)_DDECallback, AfxGetInstanceHandle()),
               APPCLASS_STANDARD,
               0L);
}

/*
 * CleanUp
 */

void DDECleanUp()
{
  if (DDEIdInst != 0)
  {
    DdeUninitialize(DDEIdInst);
    DDEIdInst = 0;
  }
  if (DDEDefaultIPCBuffer)
    delete [] DDEDefaultIPCBuffer ;
}

CDDEObject::CDDEObject(void)
{
  service_name = "";
  DDEObjects.AddTail(this);
}

CDDEObject::~CDDEObject(void)
{
	POSITION pos = DDEObjects.Find(this);
	if (pos)
		DDEObjects.RemoveAt(pos);

	pos = connections.GetHeadPosition();
	CDDEConnection *connection = NULL;
	while (pos && (connection = (CDDEConnection *)connections.GetNext(pos)))
	{
		connection->OnDisconnect(); // may delete the node implicitly
	}

	// If any left after this, delete them
	pos = connections.GetHeadPosition();
	connection = NULL;
	while (pos && (connection = (CDDEConnection *)connections.GetNext(pos)))
	{
		delete connection;
	}
}

// Global find connection
static CDDEConnection *DDEFindConnection(HCONV hConv)
{
	CDDEObject *obj = NULL;

	POSITION pos = DDEObjects.GetHeadPosition();
	while (pos && (obj = (CDDEObject *)DDEObjects.GetNext(pos)))
	{
		CDDEConnection *connection = obj->FindConnection(hConv);
		if (connection)
			return connection;
	}
	return NULL;
  /*
	wxNode *node = DDEObjects.First();
  CDDEConnection *found = NULL;
  while (node && !found)
  {
    CDDEObject *object = (CDDEObject *)node->Data();
    found = object->FindConnection(hConv);
    node = node->Next();
  }
  return found;
*/
}

// Global delete connection
static void DDEDeleteConnection(HCONV hConv)
{
	CDDEObject *obj = NULL;
	BOOL found = FALSE;

	POSITION pos = DDEObjects.GetHeadPosition();
	while (!found && pos && (obj = (CDDEObject *)DDEObjects.GetNext(pos)))
	{
		found = obj->DeleteConnection(hConv);
	}
/*
	wxNode *node = DDEObjects.First();
  BOOL found = FALSE;
  while (node && !found)
  {
    CDDEObject *object = (CDDEObject *)node->Data();
    found = object->DeleteConnection(hConv);
    node = node->Next();
  }
 */
}

CDDEConnection *CDDEObject::FindConnection(HCONV conv)
{
	POSITION pos = connections.GetHeadPosition();
	CDDEConnection *connection = NULL;
	while (pos && (connection = (CDDEConnection *)connections.GetNext(pos)))
	{
		if (connection->hConv == conv)
			return connection;
	}
	return NULL;
}

// Only delete the entry in the map, not the actual connection
BOOL CDDEObject::DeleteConnection(HCONV conv)
{
	POSITION pos = connections.GetHeadPosition();
	POSITION oldPos = pos;
	CDDEConnection *connection = NULL;
	while (pos && (connection = (CDDEConnection *)connections.GetNext(pos)))
	{
		if (connection->hConv == conv)
		{
			connections.RemoveAt(oldPos);
			return TRUE;
		}
		oldPos = pos;
	}
	return FALSE;
}

// Find a server from a service name
static CDDEServer *DDEFindServer(const CString& s)
{
	CDDEObject *obj = NULL;

	POSITION pos = DDEObjects.GetHeadPosition();
	while (pos && (obj = (CDDEObject *)DDEObjects.GetNext(pos)))
	{
		if (obj->service_name == s)
			return (CDDEServer *)obj;
	}
	return NULL;
/*
	wxNode *node = DDEObjects.First();
  CDDEServer *found = NULL;
  while (node && !found)
  {
    CDDEObject *object = (CDDEObject *)node->Data();
    
    if (object->service_name == s)
      found = (CDDEServer *)object;
    else node = node->Next();
  }
  return found;
  */
}

/*
 * Server
 *
 */

CDDEServer::CDDEServer(void)
{
}

BOOL CDDEServer::Create(const CString& server_name)
{
  service_name = server_name;
  HSZ serviceName = DdeCreateStringHandle(DDEIdInst, (const char *)server_name, CP_WINANSI);

  if (DdeNameService(DDEIdInst, serviceName, NULL, DNS_REGISTER) == 0)
  {
    DDEPrintError();
    return FALSE;
  }
  return TRUE;
}

CDDEServer::~CDDEServer(void)
{
  if (service_name != "")
  {
    HSZ serviceName = DdeCreateStringHandle(DDEIdInst, (const char *)service_name, CP_WINANSI);
    if (DdeNameService(DDEIdInst, serviceName, NULL, DNS_UNREGISTER) == 0)
    {
      DDEPrintError();
    }
  }
}

CDDEConnection *CDDEServer::OnAcceptConnection(const CString& /* topic */)
{
	return new CDDEConnection;
}


/*
 * Client
 *
 */


CDDEClient::CDDEClient(void)
{
}

CDDEClient::~CDDEClient(void)
{
	POSITION pos = connections.GetHeadPosition();
	CDDEConnection *connection = NULL;
	while (pos && (connection = (CDDEConnection *)connections.GetNext(pos)))
	{
		delete connection; // Deletes entry in connections implicitly
	}
}

BOOL CDDEClient::ValidHost(const CString& /* host */)
{
  return TRUE;
}

CDDEConnection *CDDEClient::MakeConnection(const CString& /* host */, const CString& server_name, const CString& topic)
{
  HSZ serviceName = DdeCreateStringHandle(DDEIdInst, (const char *)server_name, CP_WINANSI);
  HSZ topic_atom = DdeCreateStringHandle(DDEIdInst, (const char *)topic, CP_WINANSI);

  HCONV hConv = DdeConnect(DDEIdInst, serviceName, topic_atom, (PCONVCONTEXT)NULL);
  BOOL rt = DdeFreeStringHandle(DDEIdInst, serviceName);
  rt = DdeFreeStringHandle(DDEIdInst, topic_atom);
  if (hConv == NULL)
    return NULL;
  else
  {
    CDDEConnection *connection = OnMakeConnection();
    if (connection)
    {
      connection->hConv = hConv;
      connection->topic_name = topic;
	  connection->client = this;
      connections.AddTail(connection);
//	  bDisconnected = true;
      return connection;
    }
    else return NULL;
  }
}

CDDEConnection *CDDEClient::OnMakeConnection(void)
{
	return new CDDEConnection;
}

/*
 * Connection
 */

CDDEConnection::CDDEConnection(char *buffer, int size)
{
  if (buffer == NULL)
  {
    if (DDEDefaultIPCBuffer == NULL)
      DDEDefaultIPCBuffer = new char[DDEDefaultIPCBufferSize];
    buf_ptr = DDEDefaultIPCBuffer;
    buf_size = DDEDefaultIPCBufferSize;
  }
  else
  {
    buf_ptr = buffer;
    buf_size = size;
  }

  topic_name = "";

  client = NULL;
  server = NULL;

  hConv = NULL;
  sending_data = NULL;
}

CDDEConnection::CDDEConnection(void)
{
  hConv = NULL;
  sending_data = NULL;
  server = NULL;
  client = NULL;
  if (DDEDefaultIPCBuffer == NULL)
    DDEDefaultIPCBuffer = new char[DDEDefaultIPCBufferSize];

  buf_ptr = DDEDefaultIPCBuffer;
  buf_size = DDEDefaultIPCBufferSize;
  topic_name = "";
}

CDDEConnection::~CDDEConnection(void)
{
	CDDEObject *obj = NULL;
	if (server)
		obj = server;
	else obj = client;
	if (obj)
	{
		POSITION pos = obj->connections.Find(this);
		if (pos)
			obj->connections.RemoveAt(pos);
	}
}

// Calls that CLIENT can make
BOOL CDDEConnection::Disconnect(void)
{
  DDEDeleteConnection(hConv);
  return DdeDisconnect(hConv);
}

BOOL CDDEConnection::Execute(char *data, int size, int format)
{
  DWORD result;
  if (size < 0)
    size = strlen(data);

  size ++;
  HDDEDATA rt = DdeClientTransaction((LPBYTE)data, size, hConv,
    NULL, format, XTYP_EXECUTE, 5000, &result);
  if (!rt)
  {
	/*if (bDisconnected)
		return TRUE;*/
	  printf("Warning: DDE result is 0x%x\n", DdeGetLastError(DDEIdInst));
	  printf("Failed to exe cmd: %s\n", data);
  }
  return (rt ? TRUE : FALSE);
}

char *CDDEConnection::Request(const CString& item, int *size, int format)
{
  DWORD result;
  HSZ atom = DDEGetAtom(item);

  HDDEDATA returned_data = DdeClientTransaction(NULL, 0, hConv,
    atom, format, XTYP_REQUEST, 5000, &result);

  DWORD len = DdeGetData(returned_data, (LPBYTE)(buf_ptr), buf_size, 0);

  DdeFreeDataHandle(returned_data);

  if (size) *size = (int)len;
  if (len > 0)
  {
    return buf_ptr;
  }
  else return NULL;
}

BOOL CDDEConnection::Poke(const CString& item, char *data, int size, int format)
{
  DWORD result;
  if (size < 0)
    size = strlen(data);

  size ++;

  HSZ item_atom = DDEGetAtom(item);
  return (DdeClientTransaction((LPBYTE)data, size, hConv,
    item_atom, format, XTYP_POKE, 5000, &result) ? TRUE : FALSE);
}

BOOL CDDEConnection::StartAdvise(const CString& item)
{
  DWORD result;
  HSZ atom = DDEGetAtom(item);

  return (DdeClientTransaction(NULL, 0, hConv,
    atom, CF_TEXT, XTYP_ADVSTART, 5000, &result) ? TRUE : FALSE);
}

BOOL CDDEConnection::StopAdvise(const CString& item)
{
  DWORD result;
  HSZ atom = DDEGetAtom(item);

  return (DdeClientTransaction(NULL, 0, hConv,
    atom, CF_TEXT, XTYP_ADVSTOP, 5000, &result) ? TRUE : FALSE);
}

// Calls that SERVER can make
BOOL CDDEConnection::Advise(const CString& item, char *data, int size, int format)
{
  if (size < 0)
    size = strlen(data);

  size ++;

  HSZ item_atom = DDEGetAtom(item);
  HSZ topic_atom = DDEGetAtom(topic_name);
  sending_data = data;
  data_size = size;
  data_type = format;
  return DdePostAdvise(DDEIdInst, topic_atom, item_atom);
}

void CDDEConnection::Notify(BOOL /* notify */)
{
}

BOOL CDDEConnection::OnDisconnect(void)
{
	delete this;
	return TRUE;
}


#define DDERETURN HDDEDATA

HDDEDATA EXPENTRY _EXPORT _DDECallback(
WORD wType,
WORD wFmt,
HCONV hConv,
HSZ hsz1,
HSZ hsz2,
HDDEDATA hData,
DWORD /* lData1 */,
DWORD /* lData2 */)
{
  switch (wType)
  {
    case XTYP_CONNECT:
    {
      char topic_buf[100];
      char server_buf[100];
      DdeQueryString(DDEIdInst, hsz1, (LPSTR)topic_buf, sizeof(topic_buf),
                     CP_WINANSI);
      DdeQueryString(DDEIdInst, hsz2, (LPSTR)server_buf, sizeof(topic_buf),
                     CP_WINANSI);
      CDDEServer *server = DDEFindServer(server_buf);
      if (server)
      {
        CDDEConnection *connection =
          server->OnAcceptConnection(CString(topic_buf));
        if (connection)
        {
          connection->server = server;
          server->connections.AddTail(connection);
          connection->hConv = 0;
          connection->topic_name = topic_buf;
          DDECurrentlyConnecting = connection;
          return (DDERETURN)TRUE;
        }
      }
      else return (DDERETURN)0;
      break;
    }

    case XTYP_CONNECT_CONFIRM:
    {
      if (DDECurrentlyConnecting)
      {
        DDECurrentlyConnecting->hConv = hConv;
        DDECurrentlyConnecting = NULL;
        return (DDERETURN)TRUE;
      }
      else return 0;
      break;
    }

    case XTYP_DISCONNECT:
    {
		//InterlockedExchange((long*)&bDisconnected, 1);
      //CDDEConnection *connection = DDEFindConnection(hConv);
      //if (connection && connection->OnDisconnect())
      //{
      //  DDEDeleteConnection(hConv);  // Delete mapping: hConv => connection
      //  return (DDERETURN)TRUE;
      //}
      //else return (DDERETURN)0;
	  return (DDERETURN)TRUE;
      break;
    }

    case XTYP_EXECUTE:
    {
      CDDEConnection *connection = DDEFindConnection(hConv);

      if (connection)
      {
        DWORD len = DdeGetData(hData, (LPBYTE)(connection->buf_ptr), connection->buf_size, 0);
        DdeFreeDataHandle(hData);
        if (connection->OnExecute(connection->topic_name, connection->buf_ptr, (int)len, wFmt))
          return (DDERETURN)DDE_FACK;
        else
          return (DDERETURN)DDE_FNOTPROCESSED;
      } else return (DDERETURN)DDE_FNOTPROCESSED;
      break;
    }

    case XTYP_REQUEST:
    {
      CDDEConnection *connection = DDEFindConnection(hConv);

      if (connection)
      {
        char item_name[200];
        DdeQueryString(DDEIdInst, hsz2, (LPSTR)item_name, sizeof(item_name),
                     CP_WINANSI);

        int user_size = -1;
        char *data = connection->OnRequest(connection->topic_name, CString(item_name), &user_size, wFmt);
        if (data)
        {
          if (user_size < 0) user_size = strlen(data);

          HDDEDATA handle = DdeCreateDataHandle(DDEIdInst,
                 (LPBYTE)data, user_size + 1, 0, hsz2, wFmt, 0);
          return (DDERETURN)handle;
        } else return (DDERETURN)0;
      } else return (DDERETURN)0;
      break;
    }

    case XTYP_POKE:
    {
      CDDEConnection *connection = DDEFindConnection(hConv);

      if (connection)
      {
        char item_name[200];
        DdeQueryString(DDEIdInst, hsz2, (LPSTR)item_name, sizeof(item_name),
                     CP_WINANSI);
        DWORD len = DdeGetData(hData, (LPBYTE)(connection->buf_ptr), connection->buf_size, 0);
        DdeFreeDataHandle(hData);
        connection->OnPoke(connection->topic_name, CString(item_name), connection->buf_ptr, (int)len, wFmt);
        return (DDERETURN)DDE_FACK;
      } else return (DDERETURN)DDE_FNOTPROCESSED;
      break;
    }

    case XTYP_ADVSTART:
    {
      CDDEConnection *connection = DDEFindConnection(hConv);

      if (connection)
      {
        char item_name[200];
        DdeQueryString(DDEIdInst, hsz2, (LPSTR)item_name, sizeof(item_name),
                     CP_WINANSI);

        return (DDERETURN)connection->OnStartAdvise(connection->topic_name, CString(item_name));
      } else return (DDERETURN)0;
      break;
    }

    case XTYP_ADVSTOP:
    {
      CDDEConnection *connection = DDEFindConnection(hConv);

      if (connection)
      {
        char item_name[200];
        DdeQueryString(DDEIdInst, hsz2, (LPSTR)item_name, sizeof(item_name),
                     CP_WINANSI);
        return (DDERETURN)connection->OnStopAdvise(connection->topic_name, CString(item_name));
      } else return (DDERETURN)0;
      break;
    }

    case XTYP_ADVREQ:
    {
      CDDEConnection *connection = DDEFindConnection(hConv);

      if (connection && connection->sending_data)
      {
        HDDEDATA data = DdeCreateDataHandle(DDEIdInst,
                          (LPBYTE)connection->sending_data,
                          connection->data_size, 0, hsz2, connection->data_type, 0);
        connection->sending_data = NULL;
        return (DDERETURN)data;
      } else return (DDERETURN)NULL;
      break;
    }

    case XTYP_ADVDATA:
    {
      CDDEConnection *connection = DDEFindConnection(hConv);

      if (connection)
      {
        char item_name[200];
        DdeQueryString(DDEIdInst, hsz2, (LPSTR)item_name, sizeof(item_name),
                     CP_WINANSI);

        DWORD len = DdeGetData(hData, (LPBYTE)(connection->buf_ptr), connection->buf_size, 0);
        DdeFreeDataHandle(hData);
        if (connection->OnAdvise(connection->topic_name, CString(item_name), connection->buf_ptr, (int)len, wFmt))
          return (DDERETURN)DDE_FACK;
        else
          return (DDERETURN)DDE_FNOTPROCESSED;
      } else return (DDERETURN)DDE_FNOTPROCESSED;
      break;
    }
  }
  return 0;
}

// Atom table stuff
static HSZ DDEAddAtom(const CString& string)
{
  HSZ atom = DdeCreateStringHandle(DDEIdInst, (const char *)string, CP_WINANSI);
//  DDEAtomTable.Append(string, (CObject *)atom);
  DDEAtomTable.SetAt(string, (CObject *)atom);
  return atom;
}

static HSZ DDEGetAtom(const CString& string)
{
	CObject *data = NULL;
	if (DDEAtomTable.Lookup(string, data))
	{
		return (HSZ)data;
	}
	else
	{
		return DDEAddAtom(string);
	}
/*
wxNode *node = DDEAtomTable.Find(string);
  if (node)
    return (HSZ)node->Data();
  else
  {
    DDEAddAtom(string);
    return (HSZ)(DDEAtomTable.Find(string)->Data());
  }
 */
}

void DDEPrintError(void)
{
  char *err = NULL;
  switch (DdeGetLastError(DDEIdInst))
  {
    case DMLERR_ADVACKTIMEOUT:
      err = "A request for a synchronous advise transaction has timed out.";
      break;
    case DMLERR_BUSY:
      err = "The response to the transaction caused the DDE_FBUSY bit to be set.";
      break;
    case DMLERR_DATAACKTIMEOUT:
      err = "A request for a synchronous data transaction has timed out.";
      break;
    case DMLERR_DLL_NOT_INITIALIZED:
      err = "A DDEML function was called without first calling the DdeInitialize function,\n\ror an invalid instance identifier\n\rwas passed to a DDEML function.";
      break;
    case DMLERR_DLL_USAGE:
      err = "An application initialized as APPCLASS_MONITOR has\n\rattempted to perform a DDE transaction,\n\ror an application initialized as APPCMD_CLIENTONLY has \n\rattempted to perform server transactions.";
      break;
    case DMLERR_EXECACKTIMEOUT:
      err = "A request for a synchronous execute transaction has timed out.";
      break;
    case DMLERR_INVALIDPARAMETER:
      err = "A parameter failed to be validated by the DDEML.";
      break;
    case DMLERR_LOW_MEMORY:
      err = "A DDEML application has created a prolonged race condition.";
      break;
    case DMLERR_MEMORY_ERROR:
      err = "A memory allocation failed.";
      break;
    case DMLERR_NO_CONV_ESTABLISHED:
      err = "A client's attempt to establish a conversation has failed.";
      break;
    case DMLERR_NOTPROCESSED:
      err = "A transaction failed.";
      break;
    case DMLERR_POKEACKTIMEOUT:
      err = "A request for a synchronous poke transaction has timed out.";
      break;
    case DMLERR_POSTMSG_FAILED:
      err = "An internal call to the PostMessage function has failed. ";
      break;
    case DMLERR_REENTRANCY:
      err = "Reentrancy problem.";
      break;
    case DMLERR_SERVER_DIED:
      err = "A server-side transaction was attempted on a conversation\n\rthat was terminated by the client, or the server\n\rterminated before completing a transaction.";
      break;
    case DMLERR_SYS_ERROR:
      err = "An internal error has occurred in the DDEML.";
      break;
    case DMLERR_UNADVACKTIMEOUT:
      err = "A request to end an advise transaction has timed out.";
      break;
    case DMLERR_UNFOUND_QUEUE_ID:
      err = "An invalid transaction identifier was passed to a DDEML function.\n\rOnce the application has returned from an XTYP_XACT_COMPLETE callback,\n\rthe transaction identifier for that callback is no longer valid.";
      break;
    default:
      err = "Unrecognised error type.";
      break;
  }
  //MessageBox(NULL, (LPCSTR)err, "DDE Error", MB_OK | MB_ICONINFORMATION);
}
