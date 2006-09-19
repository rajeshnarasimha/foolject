// hmp_test.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <conio.h>
#include <fcntl.h>

#include <srllib.h>
#include <dxxxlib.h>
#include <faxlib.h>
#include <dtilib.h>
#include <msilib.h>
#include <dcblib.h>
//#include <scxlib.h>
#include <gcip_defs.h>
#include <gcip.h>
#include <ipmlib.h>
#include <gcipmlib.h>

#define MAX_CHANNELS					2
#define HMP_SIP_PORT					5060

#define USING_V17_PCM_FAX				FALSE
#define USING_MODIFY_MODE				FALSE

#define USER_DISPLAY					"foolbear"
#define USER_AGENT						"HMP test"

#define CLI_BACK						"back"

#define CLI_QUIT						"quit"
#define CLI_QUIT_MSG					"HMP test quitting...\n"

#define CLI_HELP						"help"
#define CLI_HELP_MSG					"%s by %s, please enter the command: \n\
  '%s' for make call, '%s' for drop call, \n\
  '%s' for blind transfer, '%s' for supervised transfer, \n\
  '%s' for play wave file, '%s' for record wave file, '%s' for stop all, \n\
  '%s' for send fax, '%s' for receive fax, '%s' for 491 REQUEST PENDING demo, \n\
  '%s' for request modify call, \n\
  '%s' for registration, '%s' for un-registration, \n\
  '%s' for enum device information, '%s' for enum device capabilities', \n\
  '%s' for print system status, \n\
  '%s' for print this help message, '%s' for quit this test.\n\n"
#define PRINT_CLI_HELP_MSG printf(CLI_HELP_MSG, USER_AGENT, USER_DISPLAY, \
  CLI_MAKECALL, CLI_DROPCALL, \
  CLI_BLIND_XFER, CLI_SUPER_XFER, \
  CLI_PLAYFILE, CLI_RECORDFILE, CLI_STOP, \
  CLI_SENDFAX, CLI_RECEIVEFAX, CLI_GLAREFAX, \
  CLI_MODIFYCALL, \
  CLI_REGISTER, CLI_UNREGISTER, \
  CLI_ENUM, CLI_CAPS, CLI_STAT, \
  CLI_HELP, CLI_QUIT)

#define CLI_MAKECALL					"mc"
#define CLI_DROPCALL					"dc"

#define CLI_BLIND_XFER					"bx"
#define CLI_SUPER_XFER					"sx"

#define CLI_PLAYFILE					"pw"
#define CLI_RECORDFILE					"rw"

#define CLI_SENDFAX						"sf"
#define CLI_RECEIVEFAX					"rf"

#define CLI_GLAREFAX					"gf"
#define CLI_GLAREFAX_MSG				"Be sure Call Connected between channel '0' with '1'.\n"

#define CLI_MODIFYCALL					"modify"

#define CLI_STOP						"stop"

#define CLI_REGISTER					"reg"
#define CLI_UNREGISTER					"unreg"

#define CLI_ENUM						"enum"
#define CLI_CAPS						"caps"
#define CLI_STAT						"stat"

#define CLI_REQ_INDEX					"  index(0-%u,%u): "
#define CLI_REQ_INDEX_DEFAULT			0
#define CLI_REQ_INDEX_2ND				"  index 2nd(0-%u,%u): "
#define CLI_REQ_INDEX_2ND_DEFAULT		1
#define CLI_REQ_ANI						"  ani(%s): "
#define CLI_REQ_ANI_DEFAULT				"30@192.168.101.30"
#define CLI_REQ_DNIS					"  dnis(%s): "
#define CLI_REQ_DNIS_DEFAULT			"31@192.168.101.30"
#define CLI_REQ_DNIS_ALIAS				"  dnis alias(%s): "
#define CLI_REQ_DNIS_ALIAS_DEFAULT		"31"
#define CLI_REQ_WAVE_FILE				"  wave file(%s): "
#define CLI_REQ_WAVE_FILE_DEFAULT		"play.wav"
#define CLI_REQ_FAX_FILE				"  fax file(%s): "
#define CLI_REQ_FAX_FILE_DEFAULT		"fax.tif"
#define CLI_REQ_CONFIRM					"  confirm?(%s): "
#define CLI_REQ_CONFIRM_DEFAULT			"Y"
#define CLI_REQ_PROXY_IP				"  proxy(%s): "
#define CLI_REQ_PROXY_IP_DEFAULT		"192.168.101.58"
#define CLI_REQ_LOCAL_IP				"  local(%s): "
#define CLI_REQ_LOCAL_IP_DEFAULT		"192.168.101.30"
#define CLI_REQ_ALIAS					"  alias(%s): "
#define CLI_REQ_ALIAS_DEFAULT			"30"
#define CLI_REQ_PASSWORD				"  password(%s): "
#define CLI_REQ_PASSWORD_DEFAULT		"30"
#define CLI_REQ_REALM					"  realm(%s): "
#define CLI_REQ_REALM_DEFAULT			"ewings"

class CHANNEL;

long board_dev = 0;
BOOL registered = FALSE;
CHANNEL* channls[MAX_CHANNELS] = {0}; 

char proxy_ip[GC_ADDRSIZE] = "";
char local_ip[GC_ADDRSIZE] = "";
char alias[GC_ADDRSIZE] = "";
char password[GC_ADDRSIZE] = "";
char realm[GC_ADDRSIZE] = "";

ALARM_PARM_LIST alarm_parm_list;
IPM_QOS_THRESHOLD_INFO iqti;

void enum_support_capabilities();
int get_idle_channel_id();
void authentication(const char* proxy_ip, const char* alias, const char* password, const char* realm);

class CHANNEL{
public:
	long gc_dev;
	int vox_dev;
	int fax_dev;
	int ipm_dev;

	long gc_xslot;
	long ipm_xslot;
	long vox_xslot;
	long fax_xslot;

	DV_TPT tpt;
	DX_XPB xpb;
	DX_IOTT vox_iott;
	DF_IOTT fax_iott;
	int fax_dir;	
	char fax_file[MAX_PATH];

	CRN crn;
	GCLIB_ADDRESS_BLK* rerouting_addrblkp;
	int super_xfer_primary_ch_index;

	int id;
	BOOL fax_proceeding;
	BOOL already_connect_fax;

	CHANNEL(int index) {id = index; already_connect_fax = FALSE; fax_proceeding = FALSE;}
	
	void open() {
		print("open()...");
		char dev_name[64] = "";
		long request_id = 0;
		GC_PARM_BLKP gc_parm_blkp = NULL;

		sprintf(dev_name, "dxxxB1C%d", id+1);
		vox_dev = dx_open(dev_name, NULL);
		dx_setevtmsk(vox_dev, DM_RINGS|DM_DIGITS|DM_LCOF);
		sprintf(dev_name, "dxxxB2C%d", id+1);
		fax_dev = fx_open(dev_name, NULL);
		sprintf(dev_name, ":N_iptB1T%d:P_SIP:M_ipmB1C%d", id+1, id+1);
		gc_OpenEx(&gc_dev, dev_name, EV_ASYNC, (void*)this);

		//Enabling GCEV_INVOKE_XFER_ACCEPTED Events
		gc_util_insert_parm_val(&gc_parm_blkp, GCSET_CALLEVENT_MSK, GCACT_ADDMSK, sizeof(long), GCMSK_INVOKEXFER_ACCEPTED);
		gc_SetConfigData(GCTGT_GCLIB_CHAN, gc_dev, gc_parm_blkp, 0, GCUPDATE_IMMEDIATE, &request_id, EV_SYNC);
		gc_util_delete_parm_blk(gc_parm_blkp);
	}

	void connect_voice() {
		print("connect_voice()...");
		SC_TSINFO sc_tsinfo;
		gc_GetResourceH(gc_dev, &ipm_dev, GC_MEDIADEVICE);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &gc_xslot;
		gc_GetXmitSlot(gc_dev, &sc_tsinfo);
		dx_listenEx(vox_dev, &sc_tsinfo, EV_SYNC);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &vox_xslot;
		dx_getxmitslot(vox_dev, &sc_tsinfo);
		gc_Listen(gc_dev, &sc_tsinfo, EV_SYNC);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &ipm_xslot;
		ipm_GetXmitSlot(ipm_dev, &sc_tsinfo, EV_SYNC);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &fax_xslot;
		fx_getxmitslot(fax_dev, &sc_tsinfo);
	}
	
	void connect_fax() {
		SC_TSINFO sc_tsinfo;
		print("connect_fax()...");
		gc_UnListen(gc_dev, EV_SYNC);
		if (TRUE == USING_V17_PCM_FAX) {
			sc_tsinfo.sc_numts = 1;
			sc_tsinfo.sc_tsarrayp = &gc_xslot;
			fx_listen(fax_dev, &sc_tsinfo);
			sc_tsinfo.sc_numts = 1;
			sc_tsinfo.sc_tsarrayp = &fax_xslot;
			gc_Listen(gc_dev, &sc_tsinfo, EV_SYNC);
		} else {
			GC_PARM_BLKP gc_parm_blkp = NULL;
			IP_CONNECT ip_connect;
			ip_connect.version = 0x100;
			ip_connect.mediaHandle = ipm_dev;
			ip_connect.faxHandle = fax_dev;
			ip_connect.connectType = IP_FULLDUP;
			gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_FOIP, IPPARM_T38_CONNECT, sizeof(IP_CONNECT), (void*)(&ip_connect));
			gc_SetUserInfo(GCTGT_GCLIB_CRN, crn, gc_parm_blkp, GC_SINGLECALL);
			gc_util_delete_parm_blk(gc_parm_blkp);
		}
	}
	
	void restore_voice() {
		SC_TSINFO sc_tsinfo;
		print("restore_voice()...");
		if (TRUE == USING_V17_PCM_FAX)	{
			gc_UnListen(gc_dev, EV_SYNC);
		}
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &vox_xslot;
		gc_Listen(gc_dev, &sc_tsinfo, EV_SYNC);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &gc_xslot;
		dx_listenEx(vox_dev, &sc_tsinfo, EV_SYNC);
		if (TRUE != USING_V17_PCM_FAX) {
			IP_CONNECT ip_connect;
			GC_PARM_BLKP gc_parm_blkp = NULL;
			ip_connect.version = 0x100;
			ip_connect.mediaHandle = ipm_dev;
			gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_FOIP, IPPARM_T38_DISCONNECT, sizeof(IP_CONNECT), (void*)(&ip_connect));
			gc_SetUserInfo(GCTGT_GCLIB_CRN, crn, gc_parm_blkp, GC_SINGLECALL);
			gc_util_delete_parm_blk(gc_parm_blkp);
		}
	}
	
	void set_dtmf() {
		print("set_dtmf()...");
		GC_PARM_BLKP parmblkp = NULL;
		gc_util_insert_parm_val(&parmblkp, IPSET_DTMF, IPPARM_SUPPORT_DTMF_BITMASK,
			sizeof(char), IP_DTMF_TYPE_RFC_2833);
		gc_util_insert_parm_val(&parmblkp, IPSET_DTMF, IPPARM_DTMF_RFC2833_PAYLOAD_TYPE,
			sizeof(char), IP_USE_STANDARD_PAYLOADTYPE);
		gc_SetUserInfo(GCTGT_GCLIB_CHAN, gc_dev, parmblkp, GC_ALLCALLS);
		gc_util_delete_parm_blk(parmblkp);
	}
	
	void wait_call() {
		print("wait_call()...");
		print_gc_error_info("gc_WaitCall", gc_WaitCall(gc_dev, NULL, NULL, 0, EV_ASYNC));
	}

	void print_offer_info(METAEVENT meta_evt) {
		char ani[255] = "";
		char dnis[255] = "";
		int protocol = CALLPROTOCOL_H323;
		GC_PARM_BLKP gc_parm_blkp = (GC_PARM_BLKP)(meta_evt.extevtdatap);
		GC_PARM_DATAP gc_parm_datap = NULL;
		CRN secondary_crn = 0;
		char transferring_addr[GC_ADDRSIZE] = "";

		gc_GetCallInfo(crn, ORIGINATION_ADDRESS, ani);
		gc_GetCallInfo(crn, DESTINATION_ADDRESS, dnis);
		gc_GetCallInfo(crn, CALLPROTOCOL, (char*)&protocol);
		print("%s got %s offer from %s", 
			dnis, protocol==CALLPROTOCOL_H323?"H323":"SIP", ani);

		while (gc_parm_datap = gc_util_next_parm(gc_parm_blkp, gc_parm_datap)) {
			if (GCSET_SUPP_XFER == gc_parm_datap->set_ID) {
				switch (gc_parm_datap->parm_ID) {
				case GCPARM_SECONDARYCALL_CRN:
					memcpy(&secondary_crn, gc_parm_datap->value_buf, gc_parm_datap->value_size);
					print("  GCPARM_SECONDARYCALL_CRN: 0x%x", secondary_crn);
					break;
				case GCPARM_TRANSFERRING_ADDR:
					memcpy(transferring_addr, gc_parm_datap->value_buf, gc_parm_datap->value_size);
					print("  GCPARM_TRANSFERRING_ADDR: %s", transferring_addr);
					break;
				default:
					break;
				}
			}
		}
	}
	
	void print_call_status(METAEVENT meta_evt) {
		GC_INFO call_status_info = {0};
		gc_ResultInfo(&meta_evt, &call_status_info);		
		print("CALLSTATUS Info: \n  GC InfoValue:0x%hx-%s,\n  CCLibID:%i-%s, CC InfoValue:0x%lx-%s,\n  Additional Info:%s",
			call_status_info.gcValue, call_status_info.gcMsg,
			call_status_info.ccLibId, call_status_info.ccLibName,
			call_status_info.ccValue, call_status_info.ccMsg, 
			call_status_info.additionalInfo);
	}

	void print_gc_error_info(const char* func_name, int func_return) {
		GC_INFO gc_error_info;
		if (GC_ERROR == func_return) {
			gc_ErrorInfo(&gc_error_info);
			print("%s return %d, GC ErrorValue:0x%hx-%s,\n  CCLibID:%i-%s, CC ErrorValue:0x%lx-%s,\n  Additional Info:%s",
				func_name, func_return, gc_error_info.gcValue, gc_error_info.gcMsg,
				gc_error_info.ccLibId, gc_error_info.ccLibName,
				gc_error_info.ccValue, gc_error_info.ccMsg, 
				gc_error_info.additionalInfo);
		}
	}

	void print_r4_error_info(const char* func_name, int dev, int func_return) {
		long lasterr = 0;
		if (-1 == func_return) {
			lasterr = ATDV_LASTERR(dev);
			if (EDX_SYSTEM == lasterr) {
				print("%s return %d, dev=0x%lX, ATDV_LASTERR=%d[SYSTEM ERROR], errno=%d\n  ATDV_ERRMSGP=%s",
					func_name, func_return, dev, lasterr, errno, ATDV_ERRMSGP(dev));
			} else if (EDX_BADPARM == lasterr) {
				print("%s return %d, dev=0x%lX, ATDV_LASTERR=%d[BAD PARAMETER]\n  ATDV_ERRMSGP=%s",
					func_name, func_return, dev, lasterr, ATDV_ERRMSGP(dev));
			} else {
				print("%s return %d, dev=0x%lX, ATDV_LASTERR=%d\n  ATDV_ERRMSGP=%s",
					func_name, func_return, dev, lasterr, ATDV_ERRMSGP(dev));
			}
		}
	}

	void ack_call() {
		GC_CALLACK_BLK gc_callack_blk;
		memset(&gc_callack_blk, 0, sizeof(GC_CALLACK_BLK));
		gc_callack_blk.type = GCACK_SERVICE_PROC;
		print("ack_call()...");
		print_gc_error_info("gc_CallAck", gc_CallAck(crn, &gc_callack_blk, EV_ASYNC));
	}
	
	void accept_call() {
		print("accept_call()...");
		print_gc_error_info("gc_AcceptCall", gc_AcceptCall(crn, 0, EV_ASYNC));
	}
	
	void answer_call() {
		print("answer_call()...");
		set_codec(GCTGT_GCLIB_CRN);
		print_gc_error_info("gc_AnswerCall", gc_AnswerCall(crn, 0, EV_ASYNC));
	}
	
	void make_call(const char* ani, const char* dnis) {
		GC_PARM_BLKP gc_parm_blkp = NULL;
		GC_MAKECALL_BLK gc_mk_blk = {0};
		GCLIB_MAKECALL_BLK gclib_mk_blk = {0};
		char sip_header[1024] = "";

		print("make_call(%s -> %s)...", ani, dnis);
		gc_mk_blk.cclib = NULL;
		gc_mk_blk.gclib = &gclib_mk_blk;

		sprintf(sip_header, "User-Agent: %s", USER_AGENT); //proprietary header
		gc_util_insert_parm_ref_ex(&gc_parm_blkp, IPSET_SIP_MSGINFO, IPPARM_SIP_HDR, (unsigned long)(strlen(sip_header)+1), sip_header);
		sprintf(sip_header, "From: %s<sip:%s>", USER_DISPLAY, ani); //From header
		gc_util_insert_parm_ref_ex(&gc_parm_blkp, IPSET_SIP_MSGINFO, IPPARM_SIP_HDR, (unsigned long)(strlen(sip_header)+1), sip_header);
		sprintf(sip_header, "Contact: %s<sip:%s:%d>", USER_DISPLAY, ani, HMP_SIP_PORT); //Contact header
		gc_util_insert_parm_ref_ex(&gc_parm_blkp, IPSET_SIP_MSGINFO, IPPARM_SIP_HDR, (unsigned long)(strlen(sip_header)+1), sip_header);
		gc_SetUserInfo(GCTGT_GCLIB_CHAN, gc_dev, gc_parm_blkp, GC_SINGLECALL);
		gc_util_delete_parm_blk(gc_parm_blkp);

		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_PROTOCOL, IPPARM_PROTOCOL_BITMASK, sizeof(int), IP_PROTOCOL_SIP);
		gclib_mk_blk.ext_datap = gc_parm_blkp;
		set_codec(GCTGT_GCLIB_CHAN);
		print_gc_error_info("gc_MakeCall", gc_MakeCall(gc_dev, &crn, (char*)dnis, &gc_mk_blk, 30, EV_ASYNC));		
		gc_util_delete_parm_blk(gc_parm_blkp);
	}

	void make_call(const char* dnis_alias) {
		GC_PARM_BLKP gc_parm_blkp = NULL;
		GC_MAKECALL_BLK gc_mk_blk = {0};
		GCLIB_MAKECALL_BLK gclib_mk_blk = {0};
		char sip_header[1024] = "";
		char ani[MAX_ADDRESS_LEN] = "";
		char dnis[MAX_ADDRESS_LEN] = "";
		char contact[MAX_ADDRESS_LEN] = "";

		authentication(proxy_ip, alias, password, realm);

		sprintf(ani, "%s@%s", alias, proxy_ip);
		sprintf(dnis, "%s@%s", dnis_alias, proxy_ip);
		sprintf(contact, "%s@%s", alias, local_ip);
		print("make_call(%s -> %s -> %s)...", ani, proxy_ip, dnis);
		gc_mk_blk.cclib = NULL;
		gc_mk_blk.gclib = &gclib_mk_blk;

		sprintf(sip_header, "User-Agent: %s", USER_AGENT); //proprietary header
		gc_util_insert_parm_ref_ex(&gc_parm_blkp, IPSET_SIP_MSGINFO, IPPARM_SIP_HDR, (unsigned long)(strlen(sip_header)+1), sip_header);
		sprintf(sip_header, "From: %s<sip:%s>", USER_DISPLAY, ani); //From header
		gc_util_insert_parm_ref_ex(&gc_parm_blkp, IPSET_SIP_MSGINFO, IPPARM_SIP_HDR, (unsigned long)(strlen(sip_header)+1), sip_header);
		sprintf(sip_header, "Contact: %s<sip:%s:%d>", USER_DISPLAY, contact, HMP_SIP_PORT); //Contact header
		gc_util_insert_parm_ref_ex(&gc_parm_blkp, IPSET_SIP_MSGINFO, IPPARM_SIP_HDR, (unsigned long)(strlen(sip_header)+1), sip_header);
		gc_SetUserInfo(GCTGT_GCLIB_CHAN, gc_dev, gc_parm_blkp, GC_SINGLECALL);
		gc_util_delete_parm_blk(gc_parm_blkp);

		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_PROTOCOL, IPPARM_PROTOCOL_BITMASK, sizeof(int), IP_PROTOCOL_SIP);
		gclib_mk_blk.ext_datap = gc_parm_blkp;
		set_codec(GCTGT_GCLIB_CHAN);
		print_gc_error_info("gc_MakeCall", gc_MakeCall(gc_dev, &crn, (char*)dnis, &gc_mk_blk, 30, EV_ASYNC));		
		gc_util_delete_parm_blk(gc_parm_blkp);
	}

	void drop_call() {
		print("drop_call()...");
		if (already_connect_fax)
			restore_voice();
		print_gc_error_info("gc_DropCall", gc_DropCall(crn, GC_NORMAL_CLEARING, EV_ASYNC));
	}

	void blind_xfer(const char* xferred_to) {
		print("blind_xfer(->%s)...", xferred_to);
		print_gc_error_info("InvokeXfer", gc_InvokeXfer(crn, 0, (char*)xferred_to, 0, 0, EV_ASYNC));
	}

	void init_super_xfer(int secondary_ch_index) {
		print("init_super_xfer(with %d)...", secondary_ch_index);
		channls[secondary_ch_index]->super_xfer_primary_ch_index = id;
		print_gc_error_info("g_InitXfer", gc_InitXfer(channls[secondary_ch_index]->crn, NULL, NULL, EV_ASYNC));
	}

	void super_xfer() {
		print("super_xfer()...");
		print_gc_error_info("gc_InvokeXfer", gc_InvokeXfer(channls[super_xfer_primary_ch_index]->crn, crn, 0, 0, 0, EV_ASYNC));
	}
	
	void accept_xfer(METAEVENT meta_evt) {
		GC_REROUTING_INFO* gc_rerouting_infop = (GC_REROUTING_INFO*)meta_evt.extevtdatap;
		print("accept_xfer(Refer-To address = %s)...", gc_rerouting_infop->rerouting_num);
		rerouting_addrblkp = gc_rerouting_infop->rerouting_addrblkp;
		print_gc_error_info("gc_AcceptXfer", gc_AcceptXfer(crn, 0, EV_ASYNC));
	}
	
	void make_xfer_call() {
		GC_PARM_BLKP gc_parm_blkp = NULL;
		GC_MAKECALL_BLK gc_mk_blk = {0};
		GCLIB_MAKECALL_BLK gclib_mk_blk = {0};
		int xfer_ch_index = get_idle_channel_id();
		
		print("make_xfer_call(xfer_ch_index=%d)...", xfer_ch_index);
		gc_mk_blk.cclib = NULL;
		gc_mk_blk.gclib = &gclib_mk_blk;
		
		gc_util_insert_parm_val(&gc_parm_blkp, GCSET_SUPP_XFER, GCPARM_PRIMARYCALL_CRN, sizeof(unsigned long), crn);
		gclib_mk_blk.ext_datap = gc_parm_blkp;
		gclib_mk_blk.destination = *rerouting_addrblkp;
		print_gc_error_info("gc_MakeCall", gc_MakeCall(channls[xfer_ch_index]->gc_dev, &channls[xfer_ch_index]->crn,
			NULL, &gc_mk_blk, 30, EV_ASYNC));
		gc_util_delete_parm_blk(gc_parm_blkp);
	}

	void release_call() {
		print("release_call()...");
		print_gc_error_info("gc_ReleaseCallEx", gc_ReleaseCallEx(crn, EV_ASYNC));
	}

	void req_modify_call() {
		GC_PARM_BLKP gc_parm_blkp = NULL;
		char sip_header[1024] = "";
		sprintf(sip_header, "Contact: %s by %s<sip:%s@%s:%d>", USER_AGENT, USER_DISPLAY, alias, local_ip, HMP_SIP_PORT); //Contact header
		print("req_modify_call(->%s)...", sip_header);
		gc_util_insert_parm_ref_ex(&gc_parm_blkp, IPSET_SIP_MSGINFO, IPPARM_SIP_HDR, (unsigned long)(strlen(sip_header)+1), sip_header);
		print_gc_error_info("gc_ReqModifyCall", gc_ReqModifyCall(crn, gc_parm_blkp, EV_ASYNC));
		gc_util_delete_parm_blk(gc_parm_blkp);
	}

	void accept_modify_call() {
		print("accept_modify_call()...");
		print_gc_error_info("gc_AcceptModifyCall", gc_AcceptModifyCall(crn, NULL, EV_ASYNC));
	}

	void play_wave_file(const char* file) {
		print("play_wave_file()...");
		tpt.tp_type = IO_EOT;
		tpt.tp_termno = DX_DIGMASK;
		tpt.tp_length = 0xFFFF; // any digit
		tpt.tp_flags = TF_DIGMASK;
		xpb.wFileFormat = FILE_FORMAT_WAVE;
		xpb.wDataFormat = DATA_FORMAT_MULAW;
		xpb.nSamplesPerSec = DRT_8KHZ;
		xpb.wBitsPerSample = 8;
		vox_iott.io_fhandle = dx_fileopen(file, _O_RDONLY|_O_BINARY);
		vox_iott.io_type = IO_DEV|IO_EOT;
		vox_iott.io_bufp = 0;
		vox_iott.io_offset = 0;
		vox_iott.io_length = -1;
		dx_clrdigbuf(vox_dev);
		print_r4_error_info("dx_playiottdata", vox_dev, dx_playiottdata(vox_dev, &vox_iott, &tpt, &xpb, EV_ASYNC));
	}

	void record_wave_file() {
		char file[MAX_PATH] = "";
		SYSTEMTIME t;
		print("record_wave_file()...");
		tpt.tp_type = IO_EOT;
		tpt.tp_termno = DX_DIGMASK;
		tpt.tp_length = 0xFFFF; // any digit
		tpt.tp_flags = TF_DIGMASK;
		xpb.wFileFormat = FILE_FORMAT_WAVE;
		xpb.wDataFormat = DATA_FORMAT_MULAW;
		xpb.nSamplesPerSec = DRT_8KHZ;
		xpb.wBitsPerSample = 8;
		GetLocalTime(&t);
		sprintf(file, "record_wave_ch%d_timeD%02dH%02dM%02dS%02d.%04d.wav", id, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
		vox_iott.io_fhandle = dx_fileopen(file, _O_RDWR|_O_BINARY|_O_CREAT|_O_TRUNC, 0666);
		vox_iott.io_type = IO_DEV|IO_EOT;
		vox_iott.io_bufp = 0;
		vox_iott.io_offset = 0;
		vox_iott.io_length = -1;
		dx_clrdigbuf(vox_dev);
		print_r4_error_info("dx_reciottdata", vox_dev, dx_reciottdata(vox_dev, &vox_iott, &tpt, &xpb, EV_ASYNC|RM_TONE));
	}

	void process_voice_done() {
		print_voice_done_terminal_reason();
		dx_clrtpt(&tpt, 1);
		dx_fileclose(vox_iott.io_fhandle);
		vox_iott.io_fhandle = -1;
	}

	void print_voice_done_terminal_reason() {
		int term_reason = ATDX_TERMMSK(vox_dev);
		if (TM_DIGIT == term_reason)
			print("print_voice_done_terminal_reason: TM_DIGIT");
		else if (TM_EOD == term_reason)
			print("print_voice_done_terminal_reason: TM_EOD");
		else if (TM_USRSTOP == term_reason)
			print("print_voice_done_terminal_reason: TM_USRSTOP");
		else
			print("print_voice_done_terminal_reason: 0x%x", term_reason);
	}
	
	void send_audio_request() {
		print("send_audio_request()...");	
		GC_PARM_BLKP gc_parm_blkp = NULL;	
		gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_SWITCH_CODEC, IPPARM_AUDIO_INITIATE, sizeof(int), NULL);
		gc_Extension(GCTGT_GCLIB_CRN, crn, IPEXTID_CHANGEMODE, gc_parm_blkp, NULL, EV_ASYNC);		
		gc_util_delete_parm_blk(gc_parm_blkp);
	}
	
	void send_t38_request() {
		print("send_t38_request()...");	
		GC_PARM_BLKP gc_parm_blkp = NULL;	
		gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_SWITCH_CODEC, IPPARM_T38_INITIATE, sizeof(int), NULL);
		gc_Extension(GCTGT_GCLIB_CRN, crn, IPEXTID_CHANGEMODE, gc_parm_blkp, NULL, EV_ASYNC);		
		gc_util_delete_parm_blk(gc_parm_blkp);
	}
	
	void response_codec_request(BOOL accept_call) {
		print("response_codec_request(%s)...", accept_call?"accept":"reject");
		GC_PARM_BLKP gc_parm_blkp = NULL;
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_SWITCH_CODEC, accept_call?IPPARM_ACCEPT:IPPARM_REJECT, sizeof(int), NULL);
		gc_Extension(GCTGT_GCLIB_CRN, crn, IPEXTID_CHANGEMODE, gc_parm_blkp, NULL, EV_ASYNC);
		gc_util_delete_parm_blk(gc_parm_blkp);
	}
	
	void do_fax(int dir, const char* file) {
		print("do_fax(%s)...", dir==DF_RX?"RX":"TX");
		fax_proceeding = TRUE;
		fax_dir = dir;
		sprintf(fax_file, "%s", NULL==file?"":file);

		if (FALSE == USING_V17_PCM_FAX) {
			if (already_connect_fax)
				response_codec_request(TRUE);
			else {
				connect_fax();
				send_t38_request();
				already_connect_fax = TRUE;
			}
		} else {
			connect_fax();
			if (DF_RX == dir) {
				receive_fax_inner();
			} else {
				send_fax_inner();
			}
		}
	}
	
	void send_fax_inner() {	
		print("send_fax_inner()...");
		char local_id[255] = "";
		char fax_header[255] = "";
		unsigned short tx_coding = DF_MMR|DF_ECM;
		int fhandle = 0;
		unsigned long sndflag = EV_ASYNC|DF_PHASEB|DF_PHASED;		
		fx_initstat(fax_dev, DF_TX);
		sprintf(fax_header, "FAX_HEADER_%d", id);
		fx_setparm(fax_dev, FC_HDRUSER, (void*)fax_header);
		sprintf(local_id, "FAX_LOCALID_%d", id);
		fx_setparm(fax_dev, FC_LOCALID, (void*)local_id);
//		fx_setparm(fax_dev, FC_TXCODING, (void*)&tx_coding);
		fhandle = dx_fileopen(fax_file, _O_RDONLY|_O_BINARY, NULL);
		fx_setiott(&fax_iott, fhandle, DF_TIFF, DFC_AUTO);
		fax_iott.io_type |= IO_EOT;
		fax_iott.io_phdcont = DFC_EOP;
		fx_sendfax(fax_dev, &fax_iott, sndflag);
	}
	
	void receive_fax_inner() {
		print("receive_fax_inner()...");
		char local_id[255] = "";
		SYSTEMTIME t;
		unsigned long rcvflag = EV_ASYNC|DF_PHASEB|DF_PHASED|DF_TIFF|DF_A4MAXLEN;
		fx_initstat(fax_dev, DF_RX);
		GetLocalTime(&t);
		sprintf(fax_file, "receive_fax_ch%d_timeD%02dH%02dM%02dS%02d.%04d.tif", id, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
		sprintf(local_id, "FAX_LOCALID_%d", id);
		fx_setparm(fax_dev, FC_LOCALID, (void*)local_id);		
		fx_rcvfax(fax_dev, fax_file, rcvflag);
	}

	void print_fax_phase_b_info() {
		char remote_id[255] = "";
		char fax_encoding[255] = "";
		if ((ATFX_BSTAT(fax_dev) & DFS_REMOTEID) == DFS_REMOTEID)
			fx_getparm (fax_dev, FC_REMOTEID, remote_id);
		int rc = ATFX_CODING(fax_dev);
		switch(rc) {
		case DFS_MH: sprintf(fax_encoding, "DFS_MH"); break;
		case DFS_MR: sprintf(fax_encoding, "DFS_MR"); break;
		case DFS_MMR: sprintf(fax_encoding, "DFS_MMR"); break;
		default: sprintf(fax_encoding, "INVALID(0x%04lx)", rc); break;
		}
		print("Connect with \"%s\" at %d baud, with %s%s encoding", remote_id, ATFX_SPEED(fax_dev), fax_encoding, 
			DFS_ECM == ATFX_ECM(fax_dev)?"|DFS_ECM":"");
	}
	
	void print_fax_phase_d_info() {
		int page_count = ATFX_PGXFER(fax_dev);		
		print("Phase D Information...\n  Page:%ld, scan lines:%ld;\n  Page width:%ld, resolution:%ld, transferred bytes:%ld;\n  Speed:%ld, bad scan lines:%ld",//, Retrain negative pages:%ld", 
		  page_count, ATFX_SCANLINES(fax_dev), ATFX_WIDTH(fax_dev), ATFX_RESLN(fax_dev), ATFX_TRCOUNT(fax_dev), 
		  ATFX_SPEED(fax_dev), ATFX_BADSCANLINES(fax_dev)/*, ATFX_RTNPAGES(fax_dev)*/);
	}

	void process_fax_done() {
		dx_fileclose(fax_iott.io_fhandle);
		restore_voice();
		already_connect_fax = FALSE;
		fax_proceeding = FALSE;
	}

	void process_fax_sent() {
		print_fax_phase_d_info();
		process_fax_done();
//		if (DF_TX == fax_dir) send_audio_request(); //fax sender send audio request
	}

	void process_fax_received() {
		process_fax_sent();
	}

	void process_fax_error() {
		print("Phase E status: %d", ATFX_ESTAT(fax_dev));
		process_fax_done();
//		if (DF_TX == fax_dir) send_audio_request(); //fax sender send audio request
	}

	void stop() {
		print("stop()...");
		dx_stopch(vox_dev, EV_ASYNC);
		fx_stopch(fax_dev, EV_ASYNC);
	}
	
	void process_extension(METAEVENT meta_evt) {	
		GC_PARM_BLKP gc_parm_blkp = &(((EXTENSIONEVTBLK*)(meta_evt.extevtdatap))->parmblk);
		GC_PARM_DATA* gc_parm_datap = NULL;
		IP_CAPABILITY* ip_capp = NULL;
		RTP_ADDR rtp_addr;
		struct in_addr ip_addr;

		while (gc_parm_datap = gc_util_next_parm(gc_parm_blkp, gc_parm_datap)) {
			switch(gc_parm_datap->set_ID) {
			case IPSET_SWITCH_CODEC:
				print("IPSET_SWITCH_CODEC:");
				switch(gc_parm_datap->parm_ID) {
				case IPPARM_T38_REQUESTED:
					print("  IPPARM_T38_REQUESTED:");
					if (!already_connect_fax) {
						connect_fax();
						already_connect_fax = TRUE;
					}
					break;
				case IPPARM_AUDIO_REQUESTED:
					print("  IPPARM_AUDIO_REQUESTED:");
					response_codec_request(TRUE);
					break;
				case IPPARM_READY:
					print("  IPPARM_READY:");
					if (TRUE == fax_proceeding) {
						if (DF_TX == fax_dir) 
							send_fax_inner();
						if (DF_RX == fax_dir)
							receive_fax_inner();
					}
					break;
				default:
					print("  Got unknown extension parmID %d", gc_parm_datap->parm_ID);
					break;
				}
				break;				
			case IPSET_MEDIA_STATE:
				print("IPSET_MEDIA_STATE:");
				switch(gc_parm_datap->parm_ID) {
				case IPPARM_TX_CONNECTED:
					print("  IPPARM_TX_CONNECTED");
					break;
				case IPPARM_TX_DISCONNECTED:
					print("  IPPARM_TX_DISCONNECTED");
					break;						
				case IPPARM_RX_CONNECTED:
					print("  IPPARM_RX_CONNECTED");
					break;						
				case IPPARM_RX_DISCONNECTED:
					print("  IPPARM_RX_DISCONNECTED");
					break;
				default:
					print("  Got unknown extension parmID %d", gc_parm_datap->parm_ID);
					break;
				}
				if (sizeof(IP_CAPABILITY) == gc_parm_datap->value_size) {
					ip_capp = (IP_CAPABILITY*)(gc_parm_datap->value_buf);
					print("    stream codec infomation for TX: capability(%d), dir(%d), frames_per_pkt(%d), VAD(%d)", 
						ip_capp->capability, ip_capp->direction, ip_capp->extra.audio.frames_per_pkt, ip_capp->extra.audio.VAD);
				}
				break;
			case IPSET_IPPROTOCOL_STATE:
				print("IPSET_IPPROTOCOL_STATE:");
				switch(gc_parm_datap->parm_ID) {
				case IPPARM_SIGNALING_CONNECTED:
					print("  IPPARM_SIGNALING_CONNECTED");
					break;
				case IPPARM_SIGNALING_DISCONNECTED:
					print("  IPPARM_SIGNALING_DISCONNECTED");
					break;
				case IPPARM_CONTROL_CONNECTED:
					print("  IPPARM_CONTROL_CONNECTED");
					break;
				case IPPARM_CONTROL_DISCONNECTED:
					print("  IPPARM_CONTROL_DISCONNECTED");
					break;
				default:
					print("  Got unknown extension parmID %d", gc_parm_datap->parm_ID);
					break;
				}
				break;
			case IPSET_RTP_ADDRESS:
				print("IPSET_RTP_ADDRESS:");			
				switch(gc_parm_datap->parm_ID) {
				case IPPARM_LOCAL:
					memcpy(&rtp_addr, gc_parm_datap->value_buf, gc_parm_datap->value_size);
					ip_addr.S_un.S_addr = rtp_addr.u_ipaddr.ipv4;
					print("  IPPARM_LOCAL: address:%s, port %d", inet_ntoa(ip_addr), rtp_addr.port);
					break;
				case IPPARM_REMOTE:
					memcpy(&rtp_addr, gc_parm_datap->value_buf, gc_parm_datap->value_size);
					ip_addr.S_un.S_addr = rtp_addr.u_ipaddr.ipv4;
					print("  IPPARM_REMOTE: address:%s, port %d", inet_ntoa(ip_addr), rtp_addr.port);
					break;
				default:
					print("  Got unknown extension parmID %d", gc_parm_datap->parm_ID);
					break;
				}
				break;
			default:
				print("Got unknown set_ID(%d).", gc_parm_datap->set_ID);
				break;
			}
		}
	}
	
	void set_codec(int crn_or_chan) {
		print("set_codec(g711U[103]/g711A[101]/g729[116/118/119/120]/t38[12])...");
		IP_CAPABILITY ip_cap[7];		
		ip_cap[0].capability = GCCAP_AUDIO_g711Ulaw64k;
		ip_cap[0].type = GCCAPTYPE_AUDIO;
		ip_cap[0].direction = IP_CAP_DIR_LCLTRANSMIT;
		ip_cap[0].payload_type = IP_USE_STANDARD_PAYLOADTYPE;
		ip_cap[0].extra.audio.frames_per_pkt = 20;
		ip_cap[0].extra.audio.VAD = GCPV_DISABLE;
		ip_cap[1].capability = GCCAP_AUDIO_g711Ulaw64k;
		ip_cap[1].type = GCCAPTYPE_AUDIO;
		ip_cap[1].direction = IP_CAP_DIR_LCLRECEIVE;
		ip_cap[1].payload_type = IP_USE_STANDARD_PAYLOADTYPE;
		ip_cap[1].extra.audio.frames_per_pkt = 20;
		ip_cap[1].extra.audio.VAD = GCPV_DISABLE;
		ip_cap[2].capability = GCCAP_AUDIO_g711Alaw64k;
		ip_cap[2].type = GCCAPTYPE_AUDIO;
		ip_cap[2].direction = IP_CAP_DIR_LCLTRANSMIT;
		ip_cap[2].payload_type = IP_USE_STANDARD_PAYLOADTYPE;
		ip_cap[2].extra.audio.frames_per_pkt = 20;
		ip_cap[2].extra.audio.VAD = GCPV_DISABLE;
		ip_cap[3].capability = GCCAP_AUDIO_g711Alaw64k;
		ip_cap[3].type = GCCAPTYPE_AUDIO;
		ip_cap[3].direction = IP_CAP_DIR_LCLRECEIVE;
		ip_cap[3].payload_type = IP_USE_STANDARD_PAYLOADTYPE;
		ip_cap[3].extra.audio.frames_per_pkt = 20;
		ip_cap[3].extra.audio.VAD = GCPV_DISABLE;
		ip_cap[4].capability = GCCAP_AUDIO_g729AnnexAwAnnexB;
		ip_cap[4].type = GCCAPTYPE_AUDIO;
		ip_cap[4].direction = IP_CAP_DIR_LCLTRANSMIT;
		ip_cap[4].payload_type = IP_USE_STANDARD_PAYLOADTYPE;
		ip_cap[4].extra.audio.frames_per_pkt = 2;
		ip_cap[4].extra.audio.VAD = GCPV_ENABLE;
		ip_cap[5].capability = GCCAP_AUDIO_g729AnnexAwAnnexB;
		ip_cap[5].type = GCCAPTYPE_AUDIO;
		ip_cap[5].direction = IP_CAP_DIR_LCLRECEIVE;
		ip_cap[5].payload_type = IP_USE_STANDARD_PAYLOADTYPE;
		ip_cap[5].extra.audio.frames_per_pkt = 2;
		ip_cap[5].extra.audio.VAD = GCPV_ENABLE;
		ip_cap[6].capability = GCCAP_DATA_t38UDPFax;
		ip_cap[6].type = GCCAPTYPE_RDATA;
		ip_cap[6].direction = IP_CAP_DIR_LCLTXRX;
		ip_cap[6].payload_type = 0;
		ip_cap[6].extra.data.max_bit_rate = 144;
		GC_PARM_BLKP parmblkp = NULL;
		for (int i=0; i<7; i++)
			gc_util_insert_parm_ref(&parmblkp, GCSET_CHAN_CAPABILITY, IPPARM_LOCAL_CAPABILITY, sizeof(IP_CAPABILITY), &ip_cap[i]);
		if (GCTGT_GCLIB_CRN == crn_or_chan)
			gc_SetUserInfo(GCTGT_GCLIB_CRN, crn, parmblkp, GC_SINGLECALL);
		else
			gc_SetUserInfo(GCTGT_GCLIB_CHAN, gc_dev, parmblkp, GC_SINGLECALL);
		gc_util_delete_parm_blk(parmblkp);
	}

	void set_alarm() {
		print("set_alarm()...");
		print_gc_error_info("gc_SetAlarmNotifyAll", gc_SetAlarmNotifyAll(ipm_dev, ALARM_SOURCE_ID_NETWORK_ID, ALARM_NOTIFY));
		print_gc_error_info("gc_SetAlarmParm", gc_SetAlarmParm(ipm_dev, ALARM_SOURCE_ID_NETWORK_ID, ParmSetID_qosthreshold_alarm, &alarm_parm_list, EV_SYNC));
	}

	void print_alarm_info(METAEVENT meta_evt) {
		long alarm_number = 0;
		char* alarm_name = NULL;
		unsigned long alarm_source_object_id = 0;
		char* alarm_source_object_name = NULL;
		gc_AlarmNumber(&meta_evt, &alarm_number); // Will be of type TYPE_LAN_DISCONNECT = 0x01 or TYPE_LAN_DISCONNECT + 0x10 (LAN connected).
		gc_AlarmName(&meta_evt, &alarm_name); // Will be "Lan Cable Disconnected" or "Lan cable connected".
		gc_AlarmSourceObjectID(&meta_evt, &alarm_source_object_id); // Will usually be 7.
		gc_AlarmSourceObjectName(&meta_evt, &alarm_source_object_name); // Will be IPCCLIBAsoId.
		print("alarm: %s(0x%lx) occurred on ASO %s(%d)", alarm_name, alarm_number, alarm_source_object_name, (int) alarm_source_object_id);
	}

	void close() {
		print("close()...");
		dx_close(vox_dev);
		fx_close(fax_dev);
		gc_Close(gc_dev);
	}

	void print(const char *format, ...) {
		char buf[1024] = "";
		SYSTEMTIME t;
		va_list argptr;
		va_start(argptr, format);
		_vsnprintf(buf, 1023, format, argptr);
		buf[1023] = '\0';
		va_end(argptr);		
		GetLocalTime(&t);
		printf("%02d:%02d:%02d.%04d CH %d: %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, id, buf);
	}
};

void enum_dev_information()
{
	int i = 0;
	int j = 0;
	int board_count = 0;
	int sub_dev_count = 0;
	int dsp_resource_count = 0;
	long handle = 0;
	long dev_handle = 0;
	char board_name[20] = "";
	char dev_name[20] = "";
	FEATURE_TABLE ft = {0};

	printf("enum_dev_information()...\n");

	sr_getboardcnt(DEV_CLASS_VOICE, &board_count);
	printf("  voice board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, "dxxxB%d", i);
		handle = dx_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("    voice board %s has %d sub-devs.\n", board_name, sub_dev_count);
		for (j=1; j<=sub_dev_count; j++) {
			sprintf(dev_name, "dxxxB%dC%d", i, j);
			dev_handle = dx_open(dev_name, 0);
			dx_getfeaturelist(dev_handle, &ft);
			printf("      %s %ssupport fax, %ssupport T38 fax, %ssupport CSP.\n", dev_name, 
				(ft.ft_fax & FT_FAX)?"":"NOT ",
				(ft.ft_fax & FT_FAX_T38UDP)?"":"NOT ", 
				(ft.ft_e2p_brd_cfg & FT_CSP)?"":"NOT ");
			dx_close(dev_handle);
		}
		dx_close(handle);
	}
	
	sr_getboardcnt(DEV_CLASS_DTI, &board_count);
	printf("  dti board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, "dtiB%d", i);
		handle = dt_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("    dti board %s has %d sub-devs.\n", board_name, sub_dev_count);
		dt_close(handle);
	}
	
	sr_getboardcnt(DEV_CLASS_MSI, &board_count);
	printf("  msi board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, "msiB%d", i);
		handle = ms_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("    msi board %s has %d sub-devs.\n", board_name, sub_dev_count);
		ms_close(handle);
	}

	sr_getboardcnt(DEV_CLASS_DCB, &board_count);
	printf("  dcb board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, "dcbB%d", i);
		handle = dcb_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("    dcb board %s has %d sub-devs(DSP).\n", board_name, sub_dev_count);
		for (j=1; j<=sub_dev_count; j++) {
			sprintf(dev_name, "%sD%d", board_name, j);
			dev_handle = dcb_open(dev_name, 0);
			dcb_dsprescount(dev_handle, &dsp_resource_count);
			printf("      DSP %s has %d conference resource.\n", dev_name, dsp_resource_count);
			dcb_close(dev_handle);
		}
		dcb_close(handle);
	}

	//	DEV_CLASS_SCX
	//	DEV_CLASS_AUDIO_IN	

	sr_getboardcnt(DEV_CLASS_IPT, &board_count);
	printf("  ipt board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, ":N_iptB%d:P_IP", i);
		gc_OpenEx(&handle, board_name, EV_SYNC, NULL);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("    ipt board %s has %d sub-devs.\n", board_name, sub_dev_count);
		gc_Close(handle);
	}

	sr_getboardcnt("IPM", &board_count);
	printf("  ipm board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, ":M_ipmB%d", i);
		gc_OpenEx(&handle, board_name, EV_SYNC, NULL);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("    ipm board %s has %d sub-devs.\n", board_name, sub_dev_count);
		gc_Close(handle);
	}
}

void enum_support_capabilities() 
{
	int count = 0;
	IPM_CAPABILITIES* caps = NULL;	
	printf("enum_support_capabilities()...\n");
	for (int index=0; index<MAX_CHANNELS; index++) {
		count = ipm_GetCapabilities(channls[index]->ipm_dev, CAPABILITY_CODERLIST, 0, NULL, EV_SYNC);
		caps = (IPM_CAPABILITIES*)malloc(sizeof(IPM_CAPABILITIES)*count);
		count = ipm_GetCapabilities(channls[index]->ipm_dev, CAPABILITY_CODERLIST, count, caps, EV_SYNC);
		channls[index]->print("support 1890 Coder: ");
		channls[index]->print("  [Type\tCoder\tFrSize\tFr/Pkt\tVAD]");
		for (int i=0; i<count; i++) 
			channls[index]->print("  [%d\t%d\t%d\t%d\t%d]", 
				caps[i].Coder.unCoderPayloadType, caps[i].Coder.eCoderType, caps[i].Coder.eFrameSize, 
				caps[i].Coder.unFramesPerPkt, caps[i].Coder.eVadEnable);
		free(caps);
	}
}

void authentication(const char* proxy_ip, const char* alias, const char* password, const char* realm)
{
	GC_PARM_BLKP gc_parm_blkp = NULL;
	IP_AUTHENTICATION auth;
	char identity[GC_ADDRSIZE] = "";
	printf("authentication()...\n");
	sprintf(identity, "sip:%s@%s", alias, proxy_ip);
	INIT_IP_AUTHENTICATION (&auth);
	auth.realm = (char *)realm;
	auth.identity = (char *)identity;
	auth.username = (char *)alias;
	auth.password = (char *)password;
	gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_CONFIG, IPPARM_AUTHENTICATION_CONFIGURE, (unsigned char)(sizeof(IP_AUTHENTICATION)), &auth);
	gc_SetAuthenticationInfo(GCTGT_CCLIB_NETIF, board_dev, gc_parm_blkp);
	gc_util_delete_parm_blk(gc_parm_blkp);
}

void registration(const char* proxy_ip, const char* local_ip, const char* alias, const char* password, const char* realm)
{
	GC_PARM_BLKP gc_parm_blkp = NULL; 
	IP_REGISTER_ADDRESS register_address;
	unsigned long serviceID = 1;
	char contact[250] = "";

	if (!registered) {
		authentication(proxy_ip, alias, password, realm);

		printf("registration()...\n");

		gc_util_insert_parm_val(&gc_parm_blkp, GCSET_SERVREQ, PARM_REQTYPE, sizeof(unsigned char), IP_REQTYPE_REGISTRATION);
		gc_util_insert_parm_val(&gc_parm_blkp, GCSET_SERVREQ, PARM_ACK, sizeof(unsigned char), IP_REQTYPE_REGISTRATION);
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_PROTOCOL, IPPARM_PROTOCOL_BITMASK, sizeof(char), IP_PROTOCOL_SIP);
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_REG_INFO, IPPARM_OPERATION_REGISTER, sizeof(char), IP_REG_SET_INFO);
		
		memset((void*)&register_address, 0, sizeof(IP_REGISTER_ADDRESS));
		sprintf(register_address.reg_server, "%s", proxy_ip);// Request-URI
		sprintf(register_address.reg_client, "%s@%s", alias, proxy_ip);// To header field
		sprintf(contact, "sip:%s@%s:%d", alias, local_ip, HMP_SIP_PORT);// Contact header field
		register_address.time_to_live = 3600;
		register_address.max_hops = 30;

		gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_REG_INFO, IPPARM_REG_ADDRESS, (unsigned char)sizeof(IP_REGISTER_ADDRESS), &register_address);
		gc_util_insert_parm_ref( &gc_parm_blkp, IPSET_LOCAL_ALIAS, IPPARM_ADDRESS_TRANSPARENT, (unsigned char)strlen(contact)+1, (void *)contact);
		gc_ReqService(GCTGT_CCLIB_NETIF, board_dev, &serviceID, gc_parm_blkp, NULL, EV_ASYNC);
		gc_util_delete_parm_blk(gc_parm_blkp);
		printf("  serviceID is 0x%x.\n", serviceID);

		registered = TRUE;
	}
}

void unregistration()
{
	GC_PARM_BLKP gc_parm_blkp = NULL;
	unsigned long serviceID = 1;

	if (registered) {
		printf("unregistration()...\n");
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_REG_INFO, IPPARM_OPERATION_DEREGISTER, sizeof(char), IP_REG_DELETE_ALL);	
		gc_util_insert_parm_val(&gc_parm_blkp, GCSET_SERVREQ, PARM_REQTYPE, sizeof(unsigned char), IP_REQTYPE_REGISTRATION);
		gc_util_insert_parm_val(&gc_parm_blkp, GCSET_SERVREQ, PARM_ACK, sizeof(unsigned char), IP_REQTYPE_REGISTRATION);
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_PROTOCOL, IPPARM_PROTOCOL_BITMASK, sizeof(char), IP_PROTOCOL_SIP);
		gc_ReqService(GCTGT_CCLIB_NETIF, board_dev, &serviceID, gc_parm_blkp, NULL, EV_ASYNC);
		gc_util_delete_parm_blk(gc_parm_blkp);
		printf("  serviceID is 0x%x.\n", serviceID);

		registered = FALSE;
	}	
}

int get_idle_channel_id(){
	for (unsigned index=0; index<MAX_CHANNELS; index++) {
		if (0 == channls[index]->crn)
			return index;
	}
	return -1;
}

void print_sys_status()
{
	int gc_status = 0;
	long vox_status = 0;
	long fax_status = 0;
	printf("print_sys_status()...\n");
	for (unsigned index=0; index<MAX_CHANNELS; index++) {
		if (0 == channls[index]->crn)
			gc_status = GCST_NULL;
		else 
			gc_GetCallState(channls[index]->crn, &gc_status);
		vox_status = ATDX_STATE(channls[index]->vox_dev);
		fax_status = ATFX_STATE(channls[index]->fax_dev);
		printf("  channel %d status: gc %sIDLE(0x%x), vox %sIDLE(0x%x), fax %sIDLE(0x%x).\n", index, 
			GCST_NULL==gc_status?"":"NOT ", gc_status,
			CS_IDLE==vox_status?"":"NOT ", vox_status,
			CS_IDLE==fax_status?"":"NOT ", fax_status);
	}
	printf("  %sregistered.\n", TRUE == registered?"":"NOT ");
}

void print_alarm_info(METAEVENT meta_evt)
{
	long alarm_number = 0;
	char* alarm_name = NULL;
	unsigned long alarm_source_object_id = 0;
	char* alarm_source_object_name = NULL;
	gc_AlarmNumber(&meta_evt, &alarm_number); // Will be of type TYPE_LAN_DISCONNECT = 0x01 or TYPE_LAN_DISCONNECT + 0x10 (LAN connected).
	gc_AlarmName(&meta_evt, &alarm_name); // Will be "Lan Cable Disconnected" or "Lan cable connected".
	gc_AlarmSourceObjectID(&meta_evt, &alarm_source_object_id); // Will usually be 7.
	gc_AlarmSourceObjectName(&meta_evt, &alarm_source_object_name); // Will be IPCCLIBAsoId.
	printf("  Alarm: %s(0x%lx) occurred on ASO %s(%d)\n", alarm_name, alarm_number, alarm_source_object_name, (int) alarm_source_object_id);
}

void global_call_start()
{
	GC_START_STRUCT	gclib_start;
	IPCCLIB_START_DATA cclib_start_data;
	IP_VIRTBOARD virt_boards[1];

	printf("global_call_start()...\n");
	
	memset(&cclib_start_data, 0, sizeof(IPCCLIB_START_DATA));
	memset(virt_boards, 0, sizeof(IP_VIRTBOARD));
	INIT_IPCCLIB_START_DATA(&cclib_start_data, 1, virt_boards);
	INIT_IP_VIRTBOARD(&virt_boards[0]);

	cclib_start_data.delimiter = ',';
	cclib_start_data.num_boards = 1;
	cclib_start_data.board_list = virt_boards;
	cclib_start_data.max_parm_data_size = 1024;				// set maximum SIP header length to 1k
	
	virt_boards[0].localIP.ip_ver = IPVER4;					// must be set to IPVER4
	virt_boards[0].localIP.u_ipaddr.ipv4 = IP_CFG_DEFAULT;	// or specify host NIC IP address
	virt_boards[0].h323_signaling_port = IP_CFG_DEFAULT;	// or application defined port for H.323
	virt_boards[0].sip_signaling_port = HMP_SIP_PORT;		// or application defined port for SIP
	virt_boards[0].sup_serv_mask = IP_SUP_SERV_CALL_XFER;	// Enable SIP Transfer Feature
	virt_boards[0].sip_msginfo_mask = IP_SIP_MSGINFO_ENABLE;// Enabling Access to SIP Header Information
//		| IP_SIP_MIME_ENABLE;								// Enabling SIP-T/MIME feature */
	virt_boards[0].reserved = NULL;							// must be set to NULL

	CCLIB_START_STRUCT cclib_start[]={{"GC_DM3CC_LIB", NULL}, {"GC_H3R_LIB", &cclib_start_data}, {"GC_IPM_LIB", NULL}};
	gclib_start.num_cclibs = 3;
	gclib_start.cclib_list = cclib_start;
	gc_Start(&gclib_start);
}

void pre_test()
{
	long request_id = 0;
	GC_PARM_BLKP gc_parm_blk_p = NULL;
	
	global_call_start();
	printf("global_call_start() done.\n");

	//enum_dev_information();

	gc_OpenEx(&board_dev, ":N_iptB1:P_IP", EV_SYNC, NULL);

	// Enable Alarm notification on the board handle with generic ASO ID
	gc_SetAlarmNotifyAll(board_dev, ALARM_SOURCE_ID_NETWORK_ID, ALARM_NOTIFY);
	iqti.unCount=2;
	iqti.QoSThresholdData[0].eQoSType = QOSTYPE_LOSTPACKETS;
	iqti.QoSThresholdData[0].unFaultThreshold = 20;
	iqti.QoSThresholdData[0].unDebounceOn = 10000;
	iqti.QoSThresholdData[0].unDebounceOff = 10000;
	iqti.QoSThresholdData[0].unTimeInterval = 1000;
	iqti.QoSThresholdData[0].unPercentSuccessThreshold = 60;
	iqti.QoSThresholdData[0].unPercentFailThreshold = 40;
	iqti.QoSThresholdData[1].eQoSType = QOSTYPE_JITTER;
	iqti.QoSThresholdData[1].unFaultThreshold = 60;
	iqti.QoSThresholdData[1].unDebounceOn = 20000;
	iqti.QoSThresholdData[1].unDebounceOff = 60000;
	iqti.QoSThresholdData[1].unTimeInterval = 5000;
	iqti.QoSThresholdData[1].unPercentSuccessThreshold = 60;
	iqti.QoSThresholdData[1].unPercentFailThreshold = 40;
	alarm_parm_list.n_parms = 1;
	alarm_parm_list.alarm_parm_fields[0].alarm_parm_data.pstruct = (void *) &iqti;
	gc_SetAlarmParm(board_dev, ALARM_SOURCE_ID_NETWORK_ID, ParmSetID_qosthreshold_alarm, &alarm_parm_list, EV_SYNC);

	if (FALSE == USING_MODIFY_MODE) {
		//setting T.38 fax server operating mode: IP MANUAL mode
		gc_util_insert_parm_val(&gc_parm_blk_p, IPSET_CONFIG, IPPARM_OPERATING_MODE, sizeof(long), IP_T38_MANUAL_MODE);
	} else {
		//access to SIP re-INVITE requests
		gc_util_insert_parm_val(&gc_parm_blk_p, IPSET_CONFIG, IPPARM_OPERATING_MODE, sizeof(long), IP_T38_MANUAL_MODIFY_MODE);
	}
	
	//Enabling and Disabling Unsolicited Notification Events
	gc_util_insert_parm_val(&gc_parm_blk_p, IPSET_EXTENSIONEVT_MSK, GCACT_ADDMSK, sizeof(long), 
		EXTENSIONEVT_DTMF_ALPHANUMERIC|EXTENSIONEVT_SIGNALING_STATUS|EXTENSIONEVT_STREAMING_STATUS|EXTENSIONEVT_T38_STATUS);
	gc_SetConfigData(GCTGT_CCLIB_NETIF, board_dev, gc_parm_blk_p, 0, GCUPDATE_IMMEDIATE, &request_id, EV_ASYNC);
	gc_util_delete_parm_blk(gc_parm_blk_p);

	for (int i=0; i<MAX_CHANNELS; i++) {
		channls[i] = new CHANNEL(i);
		channls[i]->open();
	}
}

void post_test()
{
	printf("post_test()...\n");
	unregistration();
	gc_Close(board_dev);
	for (int i=0; i<MAX_CHANNELS; i++) {
		channls[i]->close();
	}
	gc_Stop();
}

BOOL gets_quit_if_esc(char* output, unsigned size)
{
	/*char input = 0;
	unsigned index = 0;
	while (TRUE) {
		input = _getche();
		if (input == 13) {// input ENTER
			printf("\n");
			if (index <= size) output[index] = '\0';
			break;
		} else if (input == 27) {// input ESC
			printf("\n");
			if (index <= size) output[index] = '\0';
			return TRUE;
		} else if (input == 8) {// input BACKSAPCE ?
			if (index > 0) {
				output[index-1] = '\0';
				index = index - 2;
				printf("%c", 0);
				printf("\b");
			} else {
				output[index] = '\0';
			}
		} else {
			if (index < size) output[index] = input;
			else output[index] = '\0';
		}
		index++;
	}
	return FALSE;*/
	gets(output);
	return (0 == strnicmp(output, CLI_BACK, strlen(CLI_BACK)))?TRUE:FALSE;
}

BOOL analyse_cli()
{
	char input[GC_ADDRSIZE] = "";
	unsigned index = 0;
	unsigned index_2nd = 0;
	char ani[GC_ADDRSIZE] = "";
	char dnis[GC_ADDRSIZE] = "";
	char dnis_alias[GC_ADDRSIZE] = "";
	char file[MAX_PATH] = "";
	char confirm[2] = "";
	
	if (kbhit()) {
		if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
		if (0 == strnicmp(input, CLI_QUIT, strlen(CLI_QUIT))) 
		{
			printf("%s", CLI_QUIT_MSG);
			return FALSE;
		} 
		else if (0 == strnicmp(input, CLI_HELP, strlen(CLI_HELP))) 
		{
			PRINT_CLI_HELP_MSG;
		} 
		else if (0 == strnicmp(input, CLI_MAKECALL, strlen(CLI_MAKECALL))) 
		{
			if (registered) {
				printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
				if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
				index = atoi(input);
				if (index>=MAX_CHANNELS || index<0)
					index = CLI_REQ_INDEX_DEFAULT;
				printf(CLI_REQ_DNIS_ALIAS, CLI_REQ_DNIS_ALIAS_DEFAULT);
				if (gets_quit_if_esc(dnis_alias, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
				if (0 == dnis_alias[0])
					sprintf(dnis_alias, "%s", CLI_REQ_DNIS_ALIAS_DEFAULT);
				channls[index]->make_call(dnis_alias);
			} else {
				printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
				if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
				index = atoi(input);
				if (index>=MAX_CHANNELS || index<0)
					index = CLI_REQ_INDEX_DEFAULT;
				printf(CLI_REQ_ANI, CLI_REQ_ANI_DEFAULT);
				if (gets_quit_if_esc(ani, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
				if (0 == ani[0])
					sprintf(ani, "%s", CLI_REQ_ANI_DEFAULT);
				printf(CLI_REQ_DNIS, CLI_REQ_DNIS_DEFAULT);
				if (gets_quit_if_esc(dnis, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
				if (0 == dnis[0])
					sprintf(dnis, "%s", CLI_REQ_DNIS_DEFAULT);
				channls[index]->make_call(ani, dnis);
			}
		} 
		else if (0 == strnicmp(input, CLI_DROPCALL, strlen(CLI_DROPCALL))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			channls[index]->drop_call();
		} 
		else if (0 == strnicmp(input, CLI_BLIND_XFER, strlen(CLI_BLIND_XFER))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			printf(CLI_REQ_DNIS, CLI_REQ_DNIS_DEFAULT);
			if (gets_quit_if_esc(dnis, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 == dnis[0])
				sprintf(dnis, "%s", CLI_REQ_DNIS_DEFAULT);
			channls[index]->blind_xfer(dnis);
		} 
		else if (0 == strnicmp(input, CLI_SUPER_XFER, strlen(CLI_SUPER_XFER))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			printf(CLI_REQ_INDEX_2ND, MAX_CHANNELS-1, CLI_REQ_INDEX_2ND_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index_2nd = atoi(input);
			if (index_2nd>=MAX_CHANNELS || index_2nd<0)
				index_2nd = CLI_REQ_INDEX_2ND_DEFAULT;
			channls[index]->init_super_xfer(index_2nd);
		}    
		else if (0 == strnicmp(input, CLI_PLAYFILE, strlen(CLI_PLAYFILE))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			printf(CLI_REQ_WAVE_FILE, CLI_REQ_WAVE_FILE_DEFAULT);
			if (gets_quit_if_esc(file, MAX_PATH-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 == file[0])
				sprintf(file, "%s", CLI_REQ_WAVE_FILE_DEFAULT);
			channls[index]->play_wave_file(file);
		} 
		else if (0 == strnicmp(input, CLI_RECORDFILE, strlen(CLI_RECORDFILE))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			channls[index]->record_wave_file();
		} 
		else if (0 == strnicmp(input, CLI_SENDFAX, strlen(CLI_SENDFAX))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			printf(CLI_REQ_FAX_FILE, CLI_REQ_FAX_FILE_DEFAULT);
			if (gets_quit_if_esc(file, MAX_PATH-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 == file[0])
				sprintf(file, "%s", CLI_REQ_FAX_FILE_DEFAULT);
			channls[index]->do_fax(DF_TX, file);
		} 
		else if (0 == strnicmp(input, CLI_RECEIVEFAX, strlen(CLI_RECEIVEFAX))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			channls[index]->do_fax(DF_RX, NULL);
		} 
		else if (0 == strnicmp(input, CLI_GLAREFAX, strlen(CLI_GLAREFAX))) 
		{
			printf("%s", CLI_GLAREFAX_MSG);
			printf(CLI_REQ_CONFIRM, CLI_REQ_CONFIRM_DEFAULT);
			if (gets_quit_if_esc(confirm, 1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 != confirm[0] && 'Y' != confirm[0] && 'y' != confirm[0])
				return TRUE;
			printf(CLI_REQ_FAX_FILE, CLI_REQ_FAX_FILE_DEFAULT);
			if (gets_quit_if_esc(file, MAX_PATH-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 == file[0])
				sprintf(file, "%s", CLI_REQ_FAX_FILE_DEFAULT);
			channls[0]->do_fax(DF_TX, file);
			channls[1]->do_fax(DF_RX, NULL);
		} 
		else if (0 == strnicmp(input, CLI_STOP, strlen(CLI_STOP))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			channls[index]->stop();
		} 
		else if (0 == strnicmp(input, CLI_MODIFYCALL, strlen(CLI_MODIFYCALL))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			channls[index]->req_modify_call();
		} 
		else if (0 == strnicmp(input, CLI_REGISTER, strlen(CLI_REGISTER)))
		{
			printf(CLI_REQ_PROXY_IP, CLI_REQ_PROXY_IP_DEFAULT);
			if (gets_quit_if_esc(proxy_ip, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 == proxy_ip[0])
				sprintf(proxy_ip, "%s", CLI_REQ_PROXY_IP_DEFAULT);
			printf(CLI_REQ_LOCAL_IP, CLI_REQ_LOCAL_IP_DEFAULT);
			if (gets_quit_if_esc(local_ip, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 == local_ip[0])
				sprintf(local_ip, "%s", CLI_REQ_LOCAL_IP_DEFAULT);
			printf(CLI_REQ_ALIAS, CLI_REQ_ALIAS_DEFAULT);
			if (gets_quit_if_esc(alias, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 == alias[0])
				sprintf(alias, "%s", CLI_REQ_ALIAS_DEFAULT);
			printf(CLI_REQ_PASSWORD, CLI_REQ_PASSWORD_DEFAULT);
			if (gets_quit_if_esc(password, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 == password[0])
				sprintf(password, "%s", CLI_REQ_PASSWORD_DEFAULT);
			printf(CLI_REQ_REALM, CLI_REQ_REALM_DEFAULT);
			if (gets_quit_if_esc(realm, GC_ADDRSIZE-1)) { PRINT_CLI_HELP_MSG; return TRUE; }
			if (0 == realm[0])
				sprintf(realm, "%s", CLI_REQ_REALM_DEFAULT);
			registration(proxy_ip, local_ip, alias, password, realm);
		}
		else if (0 == strnicmp(input, CLI_UNREGISTER, strlen(CLI_UNREGISTER)))
		{
			unregistration();
		} else if (0 == strnicmp(input, CLI_ENUM, strlen(CLI_ENUM)))
		{
			enum_dev_information();
		}
		else if (0 == strnicmp(input, CLI_CAPS, strlen(CLI_CAPS)))
		{
			enum_support_capabilities();
		}
		else if (0 == strnicmp(input, CLI_STAT, strlen(CLI_STAT)))
		{
			print_sys_status();
		}
	}
	return TRUE;
}

void loop()
{
	int timeout = 0;
	long event_handle = 0;
	int evt_dev = 0;
	int evt_code = 0;
	int evt_len = 0;
	void* evt_datap = NULL;
	METAEVENT meta_evt;
	CHANNEL* pch = NULL;
	GC_PARM_BLKP gc_parm_blkp = NULL;
	GC_PARM_DATAP gc_parm_datap = NULL;
	int value = 0;

	PRINT_CLI_HELP_MSG;
	
	while (TRUE) {
		do {			
			timeout = sr_waitevt( 50 );
			if (FALSE == analyse_cli()) {
				return;
			}
		} while(timeout == SR_TMOUT);
		
		evt_dev   = (int)sr_getevtdev(event_handle);
		evt_code  = (int)sr_getevttype(event_handle);
		evt_len   = (int)sr_getevtlen(event_handle);
		evt_datap = (void*)sr_getevtdatap(event_handle);

		gc_GetMetaEventEx(&meta_evt, event_handle);
		if (meta_evt.flags & GCME_GC_EVENT) {

			if (evt_dev == board_dev) {
				printf("board got GC event : %s\n", GCEV_MSG(evt_code));

				if (GCEV_ALARM == meta_evt.evttype) {//for alarm
					print_alarm_info(meta_evt);
				} else if (GCEV_SERVICERESP == meta_evt.evttype) {//for proxy register
					gc_parm_blkp = (GC_PARM_BLKP)(meta_evt.extevtdatap);
					
					while(gc_parm_datap = gc_util_next_parm(gc_parm_blkp, gc_parm_datap)) {
						if (IPSET_REG_INFO == gc_parm_datap->set_ID) {					
							if (IPPARM_REG_STATUS == gc_parm_datap->parm_ID) {
								value = (int)(gc_parm_datap->value_buf[0]);
								switch(value) {
								case IP_REG_CONFIRMED:
									printf("  IPSET_REG_INFO/IPPARM_REG_STATUS: IP_REG_CONFIRMED\n");
									break;
								case IP_REG_REJECTED:
									printf("  IPSET_REG_INFO/IPPARM_REG_STATUS: IP_REG_REJECTED\n");
									break;
								default:
									break;
								}
							} else if (IPPARM_REG_SERVICEID == gc_parm_datap->parm_ID) {
								value = (int)(gc_parm_datap->value_buf[0]);
								printf("  IPSET_REG_INFO/IPPARM_REG_SERVICEID: 0x%x\n", value);
							}
						}
					}
				}
				continue;
			}

			gc_GetUsrAttr(meta_evt.linedev, (void**)&pch);
			if (NULL == pch)
				continue;

			pch->print("got GC event : %s", GCEV_MSG(evt_code));
			gc_GetCRN(&pch->crn, &meta_evt);
			switch(evt_code)
			{
			case GCEV_OPENEX:
				pch->set_dtmf();
				pch->connect_voice();
				pch->set_alarm();
				break;
			case GCEV_UNBLOCKED:
				pch->wait_call();
				break;
			case GCEV_OFFERED:
				pch->print_offer_info(meta_evt);
				pch->ack_call();
				break;
			case GCEV_CALLPROC:
				pch->accept_call();
				break;
			case GCEV_ACCEPT:
				pch->answer_call();
				break;
			case GCEV_ANSWERED:	
				//pch->do_fax(DF_TX);
				break;
			case GCEV_CALLSTATUS:
				pch->print_call_status(meta_evt);
				break;
			case GCEV_CONNECTED:
				//pch->do_fax(DF_RX);
				break;
			case GCEV_DROPCALL:
				pch->release_call();
				break;
			case GCEV_DISCONNECTED:
				pch->print_call_status(meta_evt);
				if (TRUE == pch->fax_proceeding)
					pch->restore_voice();
				pch->drop_call();
				break;
			case GCEV_EXTENSIONCMPLT:
			case GCEV_EXTENSION:
				pch->process_extension(meta_evt);
				break;
			case GCEV_REQ_MODIFY_CALL:
				pch->process_extension(meta_evt);
				pch->accept_modify_call();
				break;
			case GCEV_RELEASECALL:
				pch->already_connect_fax = FALSE;
				pch->fax_proceeding = FALSE;
				pch->crn = 0;
				break;
			case GCEV_TASKFAIL:
				pch->print_call_status(meta_evt);
				if (TRUE == pch->fax_proceeding)
					pch->restore_voice();
				break;
			case GCEV_REQ_XFER:
				pch->accept_xfer(meta_evt);
				break;
			case GCEV_ACCEPT_XFER:
				pch->make_xfer_call();
				break;
			case GCEV_INIT_XFER:
				pch->super_xfer();
				break;
			case GCEV_ALARM:
				pch->print_alarm_info(meta_evt);
				break;
			default:
				pch->print_call_status(meta_evt);
				break;
			}
		} else {

			for(int i=0; i<MAX_CHANNELS; i++) {
				if (channls[i]->vox_dev == evt_dev || channls[i]->fax_dev == evt_dev)
					pch = channls[i];
			}
			if (NULL == pch)
				continue;

			switch(evt_code)
			{
			case TDX_PLAY:
				pch->print("got voice event : TDX_PLAY");
				pch->process_voice_done();
				break;
			case TDX_RECORD:
				pch->print("got voice event : TDX_RECORD");
				pch->process_voice_done();
				break;
			case TDX_CST:
				pch->print("got voice event : TDX_CST");
				if (DE_DIGITS == ((DX_CST*)evt_datap)->cst_event) {
					pch->print("DE_DIGITS: [%c]", (char)((DX_CST*)evt_datap)->cst_data);
				}
				break;
			case TFX_PHASEB:
				pch->print("got fax event : TFX_PHASEB");
				pch->print_fax_phase_b_info();
				break;
			case TFX_PHASED:
				pch->print("got fax event : TFX_PHASED");
				pch->print_fax_phase_b_info();
				break;
			case TFX_FAXSEND:
				pch->print("got fax event : TFX_FAXSEND");
				pch->process_fax_sent();
				break;
			case TFX_FAXRECV:
				pch->print("got fax event : TFX_FAXRECV");
				pch->process_fax_received();
				break;
			case TFX_FAXERROR:
				pch->print("got fax event : TFX_FAXERROR");
				pch->process_fax_error();
				break;
			default:
				pch->print("unexcepted R4 event(0x%x)", evt_code);
				break;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	pre_test();
	loop();
	post_test();
	return 0;
}
