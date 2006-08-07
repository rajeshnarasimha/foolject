#ifndef _FOOL_REDUNDANCY_H_
#define _FOOL_REDUNDANCY_H_

#include "vnconnection.h"

#define MAX_HEARTBEAT_ERROR_COUNT	3
#define HEARTBEAT_CONNECT_TIMEOUT	1000
#define HEARTBEAT_TIMEOUT			500
#define HEARTBEAT_STRING			"PING"
#define PASSIVE_QUIT_STRING			"QUIT"

#define NEGOTIATE_MAX_COUNT			15
#define NEGOTIATE_RCV_TIMEOUT		500

#define MAX_PACKET_SIZE				256

#define _FOOL_PRINT_CASE_(x) case x: sprintf(buf, "%s", #x); break

enum TLogLevel {CRI = 0, ERR, WRN, KEY, INF, DBG, LOGLEVEL_QTY};

enum TRole {
	WHO_AM_I =0,
	ACTIVE_QUIT_ER,
	PRIMARY_WITHOUT_BACKUP,
	PRIMARY_WITH_BACKUP,
	BACKUP_WITH_PRIMARY
};

typedef int (*FuncOnRoleChanged)(TRole old_role, TRole new_role);

class Redundancy
{
	enum TLogLevel 
	{ CRI = 0, ERR, WRN, KEY, INF, DBG, LOGLEVEL_QTY };

public:
	Redundancy();
	~Redundancy() {};

public:
	int init(unsigned serverid, const char* ip, const char* mask, 
	int broadcast_port, int listen_port, HANDLE report_quit, FuncOnRoleChanged on_role_changed);
	int uninit();

	TRole get_my_role();
	int set_my_role(TRole new_role);

	FuncOnRoleChanged OnRoleChanged;

public:
	static unsigned negotiate(Redundancy* rddcp);
	static unsigned heartbeat(Redundancy* rddcp);

protected:
	void log(TLogLevel level, const char *msg, ...);
	int broadcast_my_role();
	char* get_role_name(TRole role, char* buf);

private:
	TRole i_am;
	CRITICAL_SECTION m_protect_role;
	unsigned m_server_id;
	char m_ip[16];
	char m_mask[16];
	char m_mate_ip[16];
	
	CVNConnection* m_vnc_p;
	CVNQueueGroup* m_vnq_p;
	int m_connecetion_id;
	int m_broadcast_port;
	int m_listen_port;

	int m_quit_inner_thread;
	HANDLE m_negotiate_thread_handle;
	HANDLE m_heartbeat_thread_handle;
	HANDLE m_report_quit;

};

#endif _FOOL_REDUNDANCY_H_