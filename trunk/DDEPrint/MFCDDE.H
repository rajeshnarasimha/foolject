/*
 * File:	mfcdde.h
 * Purpose:	MFC DDE classes
 * Author:	Julian Smart
 */

#ifndef MFCDDEH
#define MFCDDEH

#include <ddeml.h>

/*
 * Mini-DDE implementation

   Most transactions involve a topic name and an item name (choose these
   as befits your application).

   A client can:

   - ask the server to execute commands (data) associated with a topic
   - request data from server by topic and item
   - poke data into the server
   - ask the server to start an advice loop on topic/item
   - ask the server to stop an advice loop

   A server can:

   - respond to execute, request, poke and advice start/stop
   - send advise data to client

   Note that this limits the server in the ways it can send data to the
   client, i.e. it can't send unsolicited information.
 *
 */

class CDDEServer;
class CDDEClient;

class CDDEConnection: public CObject
{
 public:
  char *buf_ptr;
  CString topic_name;
  int buf_size;
  CDDEServer *server;
  CDDEClient *client;

  HCONV hConv;
  char *sending_data;
  int data_size;
  int data_type;

  CDDEConnection(char *buffer, int size);
  CDDEConnection(void);
  ~CDDEConnection(void);

  // Calls that CLIENT can make
  virtual BOOL Execute(char *data, int size = -1, int format = CF_TEXT);
  virtual BOOL Execute(const CString& str) { return Execute((char *)(const char *)str, -1, CF_TEXT); }
  virtual char *Request(const CString& item, int *size = NULL, int format = CF_TEXT);
  virtual BOOL Poke(const CString& item, char *data, int size = -1, int format = CF_TEXT);
  virtual BOOL StartAdvise(const CString& item);
  virtual BOOL StopAdvise(const CString& item);

  // Calls that SERVER can make
  virtual BOOL Advise(const CString& item, char *data, int size = -1, int format = CF_TEXT);

  // Calls that both can make
  virtual BOOL Disconnect(void);
  virtual void Notify(BOOL notify);  // Internal use only

  // Callbacks to SERVER - override at will
  virtual BOOL OnExecute(const CString& topic, char *data, int size, int format) { return FALSE; };
  virtual char *OnRequest(const CString& topic, const CString& item, int *size, int format) { return NULL; };
  virtual BOOL OnPoke(const CString& topic, const CString& item, char *data, int size, int format) { return FALSE; };
  virtual BOOL OnStartAdvise(const CString& topic, const CString& item) { return FALSE; };
  virtual BOOL OnStopAdvise(const CString& topic, const CString& item) { return FALSE; };

  // Callbacks to CLIENT - override at will
  virtual BOOL OnAdvise(const CString& topic, const CString& item, char *data, int size, int format) { return FALSE; };

  // Callbacks to BOTH

  // Default behaviour is to delete connection and return TRUE
  virtual BOOL OnDisconnect(void);
};

class CDDEObject: public CObject
{
 public:
  int lastError;
  CString service_name; // Server only
  CObList connections;

  CDDEObject(void);
  ~CDDEObject(void);

  // Find/delete CDDEConnection corresponding to the HCONV
  CDDEConnection *FindConnection(HCONV conv);
  BOOL DeleteConnection(HCONV conv);
};

class CDDEServer: public CDDEObject
{
 public:

  CDDEServer(void);
  ~CDDEServer(void);
  BOOL Create(const CString& server_name); // Returns FALSE if can't create server (e.g. port
                                  // number is already in use)
  virtual CDDEConnection *OnAcceptConnection(const CString& topic);
};

class CDDEClient: public CDDEObject
{
 public:
  CDDEClient(void);
  ~CDDEClient(void);
  BOOL ValidHost(const CString& host);
  virtual CDDEConnection *MakeConnection(const CString& host, const CString& server, const CString& topic);
                                                // Call this to make a connection.
                                                // Returns NULL if cannot.
  virtual CDDEConnection *OnMakeConnection(void); // Tailor this to return own connection.
};

void DDEInitialize();
void DDECleanUp();


#endif
