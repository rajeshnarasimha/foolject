#include "fool_redundancy.h"
#include "foolbear.h"
#include <process.h>

Redundancy::Redundancy()
{
	i_am = WHO_AM_I;

	m_vnc_p = NULL;
	m_vnq_p = NULL;
	m_connecetion_id = -1;
	m_broadcast_port = 0;
	m_listen_port = 0;

	m_quit_inner_thread = 0;
	m_negotiate_thread_handle = NULL;
	m_report_quit = NULL;

	InitializeCriticalSection(&m_protect_role);
}

int Redundancy::init(unsigned serverid, const char* ip, const char* mask, 
	int broadcast_port, int listen_port, HANDLE report_quit, FuncOnRoleChanged on_role_changed)
{	

	m_server_id = serverid;
	strncpy(m_ip, ip, 16);
	strncpy(m_mask, mask, 16);
	m_vnc_p = new CVNConnection;	
	m_broadcast_port = broadcast_port;
	m_listen_port = listen_port;
	m_report_quit = report_quit;
	
	if (FALSE == m_vnc_p->VNBindBroadCastMode(m_broadcast_port)) {
		log(ERR, "VNBindBroadCastMode(%d) return ERROR!", m_broadcast_port);
		return -1;
	}
	
	//queue for heart beat (TCP) connection receiving
	m_vnq_p = m_vnc_p->VNCreateRecvQueueGroup(1);
	if (NULL == m_vnq_p) {
		log(ERR, "VNCreateRecvQueueGroup(1) return ERROR!");
		return -2;
	}

	//listen for heart beat (TCP) connection
	if (FALSE == m_vnc_p->VNListen(listen_port, m_vnq_p)) {
		log(ERR, "VNListen(%d) return ERROR!", listen_port);
		return -3;
	}

	m_negotiate_thread_handle = (HANDLE)_beginthreadex(NULL, 0, 
		(unsigned int (__stdcall*)(void*))negotiate, this, 0, NULL);
	if (NULL == m_negotiate_thread_handle) {
		log(ERR, "_beginthreadex(negotiate) return ERROR!");
		return -4;
	}

	m_heartbeat_thread_handle = (HANDLE)_beginthreadex(NULL, 0, 
		(unsigned int (__stdcall*)(void*))heartbeat, this, 0, NULL);
	if (NULL == m_heartbeat_thread_handle) {
		log(ERR, "_beginthreadex(heartbeat) return ERROR!");
		return -5;
	}

	OnRoleChanged = on_role_changed;
	if (NULL == OnRoleChanged) {
		log(ERR, "OnRoleChanged function point is NULL, ERROR!");
		return -6;
	}

	return 0;
}

int Redundancy::uninit()
{
	HANDLE objs[] = {m_negotiate_thread_handle, m_heartbeat_thread_handle};

	m_quit_inner_thread = 1;
	WaitForMultipleObjects(2, objs, TRUE, INFINITE);

	DeleteCriticalSection(&m_protect_role);

	m_vnc_p->VNHaltServer();
	m_vnc_p->VNUnBindBroadCastMode();
	m_vnc_p->VNFreeQueueGroup(m_vnq_p);

	delete m_vnc_p;
	m_vnc_p = NULL;
	m_vnq_p = NULL;

	return 0;
}

TRole Redundancy::get_my_role()
{
	TRole role = WHO_AM_I;
	EnterCriticalSection(&m_protect_role);
	role = i_am;
	LeaveCriticalSection(&m_protect_role);
	return role;
}

int Redundancy::set_my_role(TRole new_role)
{
	char old_role_name[50] = "";
	char new_role_name[50] = "";
	TRole old_role = get_my_role();

	EnterCriticalSection(&m_protect_role);
	i_am = new_role;
	LeaveCriticalSection(&m_protect_role);
	
	get_role_name(old_role, old_role_name);
	get_role_name(new_role, new_role_name);
	log(DBG, "I am %s (%s->%s)", new_role_name, 
		old_role_name, new_role_name);
	
	(*OnRoleChanged)(old_role, new_role);
	return 0;
}

unsigned Redundancy::negotiate(Redundancy* rddcp)
{
	int rc = 0;
	TRole role = WHO_AM_I;
	char role_name[50] = "";
	
	int negotiate_count = 0;
	CVNNetMsg* rcv_msg_p = NULL;
	unsigned remote_server_id = 0;
	TRole remote_role = WHO_AM_I;
	char remote_ip[16] = "";
	char remote_role_name[50] = "";

	rddcp->set_my_role(WHO_AM_I);
	rc = rddcp->broadcast_my_role();
	if (rc < 0) rddcp->log(ERR, "broadcast_my_role return ERROR!");

	while(1 != rddcp->m_quit_inner_thread) {

		role = rddcp->get_my_role();

		if (negotiate_count>NEGOTIATE_MAX_COUNT && WHO_AM_I == role) {
			rddcp->log(DBG, "reach NEGOTIATE_MAX_COUNT when my role is WHO_AM_I");
			rddcp->set_my_role(PRIMARY_WITHOUT_BACKUP);
			role = rddcp->get_my_role();
		}

		rcv_msg_p = rddcp->m_vnc_p->VNRecvBroadCastMsg(MAX_PACKET_SIZE, 
			remote_ip, 16, NEGOTIATE_RCV_TIMEOUT);
		if (NULL == rcv_msg_p) {
			//cannot receive message before timeout
			if (WHO_AM_I == role) {
				negotiate_count++;
				rc = rddcp->broadcast_my_role();
				if (rc < 0) rddcp->log(ERR, "broadcast_my_role return ERROR!");
			}
			continue;
		}

		rc = rcv_msg_p->GetInt32Param(0, (int*)&remote_server_id);
		rc = rcv_msg_p->GetInt32Param(1, (int*)&remote_role);
		rddcp->m_vnc_p->VNReleaseMsg(rcv_msg_p);
		
		if (remote_server_id != rddcp->m_server_id ||
			0 == stricmp(remote_ip, rddcp->m_ip)) {
			//rddcp->log(DBG, "NOT the same server ID or IS the same server, ignore!");
			if (WHO_AM_I == role) negotiate_count++;
			continue;
		}
		
		rddcp->get_role_name(remote_role, remote_role_name);
		rddcp->log(DBG, "receive message from %s -- [serverID:%d, role:%s]", 
			remote_ip, remote_server_id, remote_role_name);
		rddcp->get_role_name(role, role_name);

		switch(remote_role) {
		case WHO_AM_I:

			switch(role) {
			case WHO_AM_I:
				if (stricmp(rddcp->m_ip, remote_ip) > 0) {
					rddcp->set_my_role(PRIMARY_WITHOUT_BACKUP);
				}
				break;
			case PRIMARY_WITHOUT_BACKUP:
				rc = rddcp->broadcast_my_role();
				if (rc < 0) rddcp->log(ERR, "broadcast_my_role return ERROR!");
				rddcp->set_my_role(PRIMARY_WITH_BACKUP);
				strncpy(rddcp->m_mate_ip, remote_ip, 16);
				
				rddcp->m_connecetion_id = rddcp->m_vnc_p->VNConnect(
					remote_ip, rddcp->m_listen_port, HEARTBEAT_CONNECT_TIMEOUT);
				if (-1 == rddcp->m_connecetion_id) {
					rddcp->log(ERR, "VNConnect(%s:%d) return ERROR!", 
						remote_ip, rddcp->m_listen_port);
				}

				rddcp->log(DBG, "my mate server is %s", remote_ip);
				break;
			case PRIMARY_WITH_BACKUP:
			case BACKUP_WITH_PRIMARY:
				rc = rddcp->broadcast_my_role();
				if (rc < 0) rddcp->log(ERR, "broadcast_my_role return ERROR!");
				break;
			default:
				rddcp->log(ERR, "ERROR situation(remote:%s, local:%s)", 
					remote_role_name, role_name);
				break;
			}
			break;

		case ACTIVE_QUIT_ER:
			
			switch(role) {
			case WHO_AM_I:
				break;
			case PRIMARY_WITH_BACKUP:
			case BACKUP_WITH_PRIMARY:
				rddcp->set_my_role(PRIMARY_WITHOUT_BACKUP);
				
				rddcp->m_vnc_p->VNDisconnect();
				
				strncpy(rddcp->m_mate_ip, "", 16);
				break;
			case PRIMARY_WITHOUT_BACKUP:
			default:
				rddcp->log(ERR, "ERROR situation(remote:%s, local:%s)", 
					remote_role_name, role_name);
				break;
			}
			break;

		case PRIMARY_WITHOUT_BACKUP:
			
			switch(role) {
			case WHO_AM_I:
				rddcp->set_my_role(BACKUP_WITH_PRIMARY);
				strncpy(rddcp->m_mate_ip, remote_ip, 16);
				
				rddcp->m_connecetion_id = rddcp->m_vnc_p->VNConnect(
					remote_ip, rddcp->m_listen_port, HEARTBEAT_CONNECT_TIMEOUT);
				if (-1 == rddcp->m_connecetion_id) {
					rddcp->log(ERR, "VNConnect(%s:%d) return ERROR!", 
						remote_ip, rddcp->m_listen_port);
				}
				
				rddcp->log(DBG, "my mate server is %s", remote_ip);
				break;
			case PRIMARY_WITHOUT_BACKUP:
			case PRIMARY_WITH_BACKUP:
			case BACKUP_WITH_PRIMARY:
			default:
				rddcp->log(ERR, "ERROR situation(remote:%s, local:%s)", 
					remote_role_name, role_name);
				break;
			}
			break;

		case PRIMARY_WITH_BACKUP:
			
			switch(role) {
			case WHO_AM_I:
				goto inactive_quit;
				break;
			case PRIMARY_WITHOUT_BACKUP:
			case PRIMARY_WITH_BACKUP:
			case BACKUP_WITH_PRIMARY:
			default:
				rddcp->log(ERR, "ERROR situation(remote:%s, local:%s)", 
					remote_role_name, role_name);
				break;
			}
			break;

		case BACKUP_WITH_PRIMARY:
			
			switch(role) {
			case WHO_AM_I:
				goto inactive_quit;
				break;
			case PRIMARY_WITHOUT_BACKUP:
			case PRIMARY_WITH_BACKUP:
			case BACKUP_WITH_PRIMARY:
			default:
				rddcp->log(ERR, "ERROR situation(remote:%s, local:%s)", 
					remote_role_name, role_name);
				break;
			}
			break;

		default:
			rddcp->log(ERR, "Invalid remote role: %s", remote_role_name);
			
		}//end of "switch(remote_role) {"

	}//end of "while(1 != rddcp->m_quit_inner_thread) {"

	//normal quit, report to mate server
	rddcp->set_my_role(ACTIVE_QUIT_ER);
	rddcp->broadcast_my_role();

inactive_quit:
	rddcp->log(DBG, "report quit[SetEvent(rddcp->m_report_quit)] to main thread");
	SetEvent(rddcp->m_report_quit);
	return 0;
}

unsigned Redundancy::heartbeat(Redundancy* rddcp)
{
	TRole role = WHO_AM_I;
	char remote_ip[16] = "";
	CVNNetMsg snd_msg(Request);
	CVNNetMsg* rcv_msg_p = NULL;
	int err_count = 0;
	int err_code = 0;
	char heartbeat_string[5] = "";

	snd_msg.SetStringParam(0, rddcp->m_ip);

	while(1 != rddcp->m_quit_inner_thread) {

		role = rddcp->get_my_role();

		if (PRIMARY_WITH_BACKUP == role ||
			BACKUP_WITH_PRIMARY == role) {
			
			if (-1 != rddcp->m_connecetion_id) {
				snd_msg.SetStringParam(1, HEARTBEAT_STRING);
				if (RS_Success != rddcp->m_vnc_p->VNSendMsg(
					rddcp->m_connecetion_id, 0, 0, &snd_msg)) {
					rddcp->log(ERR, "VNSendMsg return ERROR!");
					//continue;
				}
			}

			rcv_msg_p = rddcp->m_vnc_p->VNRecvMsg(0, &err_code, HEARTBEAT_TIMEOUT*2);

			if (NULL != rcv_msg_p) {
				rcv_msg_p->GetStringParam(0, remote_ip, 16);
				rcv_msg_p->GetStringParam(1, heartbeat_string, 5);
				//rddcp->log(DBG, "got TCP message from %s:%s", remote_ip, heartbeat_string);

				if (0 == stricmp(remote_ip, rddcp->m_mate_ip)) {
					if (0 == stricmp(heartbeat_string, HEARTBEAT_STRING)) {
						//reset err_count
						err_count =  0;
					} else if (0 == stricmp(heartbeat_string, PASSIVE_QUIT_STRING)) {
						//application quit
						rddcp->m_quit_inner_thread = 1;
					}
				}

			} else {
				if (err_code == RS_Failed) {
					rddcp->m_vnc_p->VNReleaseMsg(rcv_msg_p);
					continue;
				}

				//timeout
				if (err_count++ > MAX_HEARTBEAT_ERROR_COUNT) {
					//report mate server to quit
					rddcp->set_my_role(PRIMARY_WITHOUT_BACKUP);	
					
					if (-1 != rddcp->m_connecetion_id) {
						snd_msg.SetStringParam(1, PASSIVE_QUIT_STRING);
						if (RS_Success != rddcp->m_vnc_p->VNSendMsg(
							rddcp->m_connecetion_id, 0, 0, &snd_msg)) {
							rddcp->log(ERR, "VNSendMsg return ERROR!");
							rddcp->m_vnc_p->VNReleaseMsg(rcv_msg_p);
							continue;
						}
					}
				}
			}
			rddcp->m_vnc_p->VNReleaseMsg(rcv_msg_p);

		} else {
			fool_wait(HEARTBEAT_TIMEOUT*2);
			err_count = 0;
		}

	}
	return 0;
}

int Redundancy::broadcast_my_role()
{
	CVNNetMsg msg(Request);
	TRole role = get_my_role();

	msg.SetInt32Param(0, m_server_id);
	msg.SetInt32Param(1, role);

	if (FALSE == m_vnc_p->VNSendBroadCastMsg(m_ip, m_mask, &msg, m_broadcast_port)) {
		log(ERR, "VNSendBroadCastMsg return ERROR!");
		return -1;
	}

	return 0;
}

char* Redundancy::get_role_name(TRole role, char* buf)
{
	switch(role) {
		_FOOL_PRINT_CASE_(WHO_AM_I);
		_FOOL_PRINT_CASE_(ACTIVE_QUIT_ER);
		_FOOL_PRINT_CASE_(PRIMARY_WITHOUT_BACKUP);
		_FOOL_PRINT_CASE_(PRIMARY_WITH_BACKUP);
		_FOOL_PRINT_CASE_(BACKUP_WITH_PRIMARY);
	default: sprintf(buf, "UNKNOW ROLE(%d)", role); break;
	}
	return buf;
}

void Redundancy::log(TLogLevel level, const char *msg, ...)
{
	char buf[1024] = "";
	va_list arg;
	SYSTEMTIME now;

	va_start(arg, msg);
	vsprintf(buf, msg, arg);
	va_end(arg);

	GetLocalTime(&now);
	printf("%04d/%02d/%02d %02d:%02d:%02d.%03d [%d]: %s\n", 
		now.wYear, now.wMonth, now.wDay, 
		now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
		level, buf);
}
