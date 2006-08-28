// hmp_test.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <conio.h>

#include <srllib.h>
#include <dxxxlib.h>
#include <faxlib.h>
#include <dtilib.h>
#include <msilib.h>
#include <dcblib.h>
//#include <scxlib.h>
#include <gcip_defs.h>
#include <gcip.h>

#define MAX_CHANNELS					2

#define MAKECALL_DISPLAY				"hmp_test_by_foolbear"

#define CLI_QUIT						"quit"
#define CLI_QUIT_MSG					"HMP test quitting...\n"

#define CLI_HELP						"help"
#define CLI_HELP_MSG					"HMP test by foolbear, please enter the command: \n\
  'mc' for make call, 'dc' for drop call, \n\
  'sf' for send fax, 'rf' for receive fax, 'gf' for 491 REQUEST PENDING demo, \n\
  'reg' for registration, 'unreg' for un-registration, \n\
  'help' for print this help message, 'quit' for quit this test.\n\n"

#define CLI_MAKECALL					"mc"
#define CLI_DROPCALL					"dc"

#define CLI_SENDFAX						"sf"
#define CLI_RECEIVEFAX					"rf"

#define CLI_GLAREFAX					"gf"
#define CLI_GLAREFAX_MSG				"Be sure Call Connected between channel '0' with '1'.\n"

#define CLI_REGISTER					"reg"
#define CLI_UNREGISTER					"unreg"

#define CLI_REQ_INDEX					"index(0-%u,%u): "
#define CLI_REQ_INDEX_DEFAULT			0
#define CLI_REQ_ANI						"ani(%s): "
#define CLI_REQ_ANI_DEFAULT				"ani@127.0.0.1"
#define CLI_REQ_DNIS					"dnis(%s): "
#define CLI_REQ_DNIS_DEFAULT			"dnis@127.0.0.1"
#define CLI_REQ_FAX_FILE				"fax file(%s): "
#define CLI_REQ_FAX_FILE_DEFAULT		"fax.tif"
#define CLI_REQ_CONFIRM					"confirm?(%s): "
#define CLI_REQ_CONFIRM_DEFAULT			"Y"
#define CLI_REQ_PROXY_IP				"proxy(%s): "
#define CLI_REQ_PROXY_IP_DEFAULT		"192.168.101.101"
#define CLI_REQ_LOCAL_IP				"local(%s): "
#define CLI_REQ_LOCAL_IP_DEFAULT		"192.168.101.77"
#define CLI_REQ_ALIAS					"alias(%s): "
#define CLI_REQ_ALIAS_DEFAULT			"99"

class CHANNEL{
public:
	long gc_dev;
	int vox_dev;
	int fax_dev;
	int ipm_dev;

	long ip_xslot;
	long vox_xslot;
	long fax_xslot;

	DF_IOTT fax_iott;
	int fax_dir;	
	char fax_file[MAX_PATH];

	CRN crn;

	int id;
	BOOL fax_proceeding;
	BOOL already_connect_fax;

	CHANNEL(int index) {id = index; already_connect_fax = FALSE; fax_proceeding = FALSE;}
	
	void open() {
		print("open()...");
		char dev_name[64] = "";
		sprintf(dev_name, "dxxxB1C%d", id+1);
		vox_dev = dx_open(dev_name, NULL);
		dx_setevtmsk(vox_dev, DM_RINGS|DM_DIGITS|DM_LCOF);
		sprintf(dev_name, "dxxxB2C%d", id+1);
		fax_dev = fx_open(dev_name, NULL);
		sprintf(dev_name, ":N_iptB1T%d:P_SIP:M_ipmB1C%d", id+1, id+1);
		gc_OpenEx(&gc_dev, dev_name, EV_ASYNC, (void*)this);
	}

	void connect_voice() {
		print("connect_voice()...");
		SC_TSINFO sc_tsinfo;
		gc_GetResourceH(gc_dev, &ipm_dev, GC_MEDIADEVICE);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &ip_xslot;
		gc_GetXmitSlot(gc_dev, &sc_tsinfo);
		dx_listen(vox_dev, &sc_tsinfo);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &vox_xslot;
		dx_getxmitslot(vox_dev, &sc_tsinfo);
		gc_Listen(gc_dev, &sc_tsinfo, EV_SYNC);
	}
	
	void connect_fax() {
		print("connect_fax()...");
		IP_CONNECT ip_connect;
		GC_PARM_BLKP gc_parm_blkp = NULL;
		gc_UnListen(gc_dev, EV_SYNC);
		ip_connect.version = 0x100;
		ip_connect.mediaHandle = ipm_dev;
		ip_connect.faxHandle = fax_dev;
		ip_connect.connectType = IP_FULLDUP;
		gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_FOIP, IPPARM_T38_CONNECT, sizeof(IP_CONNECT), (void*)(&ip_connect));
		gc_SetUserInfo(GCTGT_GCLIB_CRN, crn, gc_parm_blkp, GC_SINGLECALL);
		gc_util_delete_parm_blk(gc_parm_blkp);
	}
	
	void restore_voice() {
		print("restore_voice()...");
		IP_CONNECT ip_connect;
		GC_PARM_BLKP gc_parm_blkp = NULL;
		SC_TSINFO sc_tsinfo;
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &vox_xslot;
		gc_Listen(gc_dev, &sc_tsinfo, EV_SYNC);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &ip_xslot;	
		dx_listen(vox_dev, &sc_tsinfo);
		ip_connect.version = 0x100;
		ip_connect.mediaHandle = ipm_dev;
		gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_FOIP, IPPARM_T38_DISCONNECT, sizeof(IP_CONNECT), (void*)(&ip_connect));
		gc_SetUserInfo(GCTGT_GCLIB_CRN, crn, gc_parm_blkp, GC_SINGLECALL);
		gc_util_delete_parm_blk(gc_parm_blkp);
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

	void print_offer_info() {
		char ani[255] = "";
		char dnis[255] = "";
		int protocol = CALLPROTOCOL_H323;
		gc_GetCallInfo(crn, ORIGINATION_ADDRESS, ani);
		gc_GetCallInfo(crn, DESTINATION_ADDRESS, dnis);
		gc_GetCallInfo(crn, CALLPROTOCOL, (char*)&protocol);
		print("number %s, got %s offer from %s", 
			dnis, protocol==CALLPROTOCOL_H323?"H323":"SIP", ani);
	}
	
	void print_call_status(METAEVENT meta_evt) {
		GC_INFO call_status_info = {0};
		gc_ResultInfo(&meta_evt, &call_status_info);		
		print("CALLSTATUS Info: \n GC InfoValue:0x%hx-%s,\n CCLibID:%i-%s, CC InfoValue:0x%lx-%s,\n Additional Info:%s",
			call_status_info.gcValue, call_status_info.gcMsg,
			call_status_info.ccLibId, call_status_info.ccLibName,
			call_status_info.ccValue, call_status_info.ccMsg, 
			call_status_info.additionalInfo);
	}

	void print_gc_error_info(const char *func_name, int func_return) {
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
		print("make_call(%s -> %s)...", ani, dnis);
		GC_PARM_BLKP gc_parm_blkp = NULL;
		GC_MAKECALL_BLK gc_mk_blk;
		GCLIB_MAKECALL_BLK gclib_mk_blk = {0};
		gc_mk_blk.cclib = NULL;
		gc_mk_blk.gclib = &gclib_mk_blk;
		strcpy(gc_mk_blk.gclib->origination.address, ani);
		gc_mk_blk.gclib->origination.address_type = GCADDRTYPE_TRANSPARENT;
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_PROTOCOL, IPPARM_PROTOCOL_BITMASK, sizeof(int), IP_PROTOCOL_SIP);
		gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_CALLINFO, IPPARM_DISPLAY, (unsigned char)(strlen(MAKECALL_DISPLAY)+1), MAKECALL_DISPLAY);
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

	void release_call() {
		print("release_call()...");
		print_gc_error_info("gc_ReleaseCallEx", gc_ReleaseCallEx(crn, EV_ASYNC));
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
		if (already_connect_fax)
			response_codec_request(TRUE);
		else {
			connect_fax();
			send_t38_request();
			already_connect_fax = TRUE;
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
		fhandle = dx_fileopen(fax_file, 0x0000|0x8000, NULL);
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
		GetSystemTime(&t);
		sprintf(fax_file, "rcv_fax_ch%d_timeD%02dH%02dM%02dS%02d.%04d.tif", id, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
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
	
	void process_extension(METAEVENT meta_evt) {	
		GC_PARM_BLKP gc_parm_blkp = &(((EXTENSIONEVTBLK*)(meta_evt.extevtdatap))->parmblk);		
		GC_PARM_DATA* parm_datap = NULL;		
		IP_CAPABILITY* ip_capp = NULL;	
		RTP_ADDR rtp_addr;
		struct in_addr ip_addr;

		while (parm_datap = gc_util_next_parm(gc_parm_blkp, parm_datap)) {
			switch(parm_datap->set_ID) {
			case IPSET_SWITCH_CODEC:
				print("IPSET_SWITCH_CODEC:");
				switch(parm_datap->parm_ID) {
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
					print("  Got unknown extension parmID %d", parm_datap->parm_ID);
					break;
				}
				break;				
			case IPSET_MEDIA_STATE:
				print("IPSET_MEDIA_STATE:");
				switch(parm_datap->parm_ID) {
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
					print("  Got unknown extension parmID %d", parm_datap->parm_ID);
					break;
				}
				if (sizeof(IP_CAPABILITY) == parm_datap->value_size) {
					ip_capp = (IP_CAPABILITY*)(parm_datap->value_buf);
					print("    stream codec infomation for TX: capability(%d), dir(%d), frames_per_pkt(%d), VAD(%d)", 
						ip_capp->capability, ip_capp->direction, ip_capp->extra.audio.frames_per_pkt, ip_capp->extra.audio.VAD);
				}
				break;
			case IPSET_IPPROTOCOL_STATE:
				print("IPSET_IPPROTOCOL_STATE:");
				switch(parm_datap->parm_ID) {
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
					print("  Got unknown extension parmID %d", parm_datap->parm_ID);
					break;
				}
				break;
			case IPSET_RTP_ADDRESS:
				print("IPSET_RTP_ADDRESS:");			
				switch(parm_datap->parm_ID) {
				case IPPARM_LOCAL:
					memcpy(&rtp_addr, parm_datap->value_buf, parm_datap->value_size);
					ip_addr.S_un.S_addr = rtp_addr.u_ipaddr.ipv4;
					print("  IPPARM_LOCAL: address:%s, port %d", inet_ntoa(ip_addr), rtp_addr.port);
					break;
				case IPPARM_REMOTE:
					memcpy(&rtp_addr, parm_datap->value_buf, parm_datap->value_size);
					ip_addr.S_un.S_addr = rtp_addr.u_ipaddr.ipv4;
					print("  IPPARM_REMOTE: address:%s, port %d", inet_ntoa(ip_addr), rtp_addr.port);
					break;
				default:
					print("  Got unknown extension parmID %d", parm_datap->parm_ID);
					break;
				}
				break;				
			default:
				print("Got unknown set_ID(%d).", parm_datap->set_ID);
				break;
			}
		}
	}
	
	void set_codec(int crn_or_chan) {
		print("set_codec(g711Ulaw64k/t38UDPFax)...");
		IP_CAPABILITY ip_cap[3];		
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
		ip_cap[2].capability = GCCAP_DATA_t38UDPFax;
		ip_cap[2].type = GCCAPTYPE_RDATA;
		ip_cap[2].direction = IP_CAP_DIR_LCLTXRX;
		ip_cap[2].payload_type = 0;
		ip_cap[2].extra.data.max_bit_rate = 144;
		GC_PARM_BLKP parmblkp = NULL;
		for (int i=0; i<3; i++)
			gc_util_insert_parm_ref(&parmblkp, GCSET_CHAN_CAPABILITY, IPPARM_LOCAL_CAPABILITY, sizeof(IP_CAPABILITY), &ip_cap[i]);
		if (GCTGT_GCLIB_CRN == crn_or_chan)
			gc_SetUserInfo(GCTGT_GCLIB_CRN, crn, parmblkp, GC_SINGLECALL);
		else
			gc_SetUserInfo(GCTGT_GCLIB_CHAN, gc_dev, parmblkp, GC_SINGLECALL);
		gc_util_delete_parm_blk(parmblkp);
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
		GetSystemTime(&t);
		printf("%02d:%02d:%02d.%04d CH %d: %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, id, buf);
	}
};

long board_dev = 0;
BOOL registered = FALSE;
CHANNEL* channls[4] = {0}; 

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
	printf("voice board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, "dxxxB%d", i);
		handle = dx_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("\tvoice board %d has %d sub-devs.\n", i, sub_dev_count);
		for (j=1; j<=sub_dev_count; j++) {
			sprintf(dev_name, "dxxxB%dC%d", i, j);
			dev_handle = dx_open(dev_name, 0);
			dx_getfeaturelist(dev_handle, &ft);
			printf("\t\t%s %ssupport fax, %ssupport T38 fax, %ssupport CSP.\n", dev_name, 
				(ft.ft_fax & FT_FAX)?"":"NOT ",
				(ft.ft_fax & FT_FAX_T38UDP)?"":"NOT ", 
				(ft.ft_e2p_brd_cfg & FT_CSP)?"":"NOT ");
			dx_close(dev_handle);
		}
		dx_close(handle);
	}
	
	sr_getboardcnt(DEV_CLASS_DTI, &board_count);
	printf("dti board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, "dtiB%d", i);
		handle = dt_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("\tdti board %d has %d sub-devs.\n", i, sub_dev_count);
		dt_close(handle);
	}
	
	sr_getboardcnt(DEV_CLASS_MSI, &board_count);
	printf("msi board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, "msiB%d", i);
		handle = ms_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("\tmsi board %d has %d sub-devs.\n", i, sub_dev_count);
		ms_close(handle);
	}

	sr_getboardcnt(DEV_CLASS_DCB, &board_count);
	printf("dcb board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, "dcbB%d", i);
		handle = dcb_open(board_name, 0);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("\tdcb board %d has %d sub-devs(DSP).\n", i, sub_dev_count);
		for (j=1; j<=sub_dev_count; j++) {
			sprintf(dev_name, "%sD%d", board_name, j);
			dev_handle = dcb_open(dev_name, 0);
			dcb_dsprescount(dev_handle, &dsp_resource_count);
			printf("\t\tDSP %s has %d conference resource.\n", dev_name, dsp_resource_count);
			dcb_close(dev_handle);
		}
		dcb_close(handle);
	}

	//	DEV_CLASS_SCX
	//	DEV_CLASS_AUDIO_IN	

	sr_getboardcnt(DEV_CLASS_IPT, &board_count);
	printf("ipt board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, ":N_iptB%d:P_IP", i);
		gc_OpenEx(&handle, board_name, EV_SYNC, NULL);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("\tipt board %d(handle=%d) has %d sub-devs.\n", i, handle, sub_dev_count);
		gc_Close(handle);
	}

	sr_getboardcnt("IPM", &board_count);
	printf("ipm board count=%d.\n", board_count);
	for (i=1; i<=board_count; i++) {
		sprintf(board_name, ":M_ipmB%d", i);
		gc_OpenEx(&handle, board_name, EV_SYNC, NULL);
		sub_dev_count = ATDV_SUBDEVS(handle);
		printf("\tipm board %d(handle=%d) has %d sub-devs.\n", i, handle, sub_dev_count);
		gc_Close(handle);
	}

	printf("enum_dev_information done.\n");
}

void authentication()
{
	GC_PARM_BLKP gc_parm_blkp = NULL;
	char realm[] = "oakleyding";
	char identity[] = "sip:192.168.101.101";
	char username[] = "99";
	char password [] = "99";
	IP_AUTHENTICATION auth;

	printf("authentication()...\n");
	INIT_IP_AUTHENTICATION (&auth);
	auth.realm = realm;
	auth.identity = identity;
	auth.username = username;
	auth.password = password;
	gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_CONFIG, IPPARM_AUTHENTICATION_CONFIGURE, (unsigned char)(sizeof(IP_AUTHENTICATION)), &auth);
	gc_SetAuthenticationInfo(GCTGT_CCLIB_NETIF, board_dev, gc_parm_blkp);
	gc_util_delete_parm_blk(gc_parm_blkp);
}

void registration(const char* proxy_ip, const char* local_ip, const char* alias)
{
	GC_PARM_BLKP gc_parm_blkp = NULL; 
	IP_REGISTER_ADDRESS register_address;
	unsigned long serviceID = 1;
	char contact[250] = "";

	if (!registered) {
		printf("registration()...\n");

		gc_util_insert_parm_val(&gc_parm_blkp, GCSET_SERVREQ, PARM_REQTYPE, sizeof(unsigned char), IP_REQTYPE_REGISTRATION);
		gc_util_insert_parm_val(&gc_parm_blkp, GCSET_SERVREQ, PARM_ACK, sizeof(unsigned char), IP_REQTYPE_REGISTRATION);
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_PROTOCOL, IPPARM_PROTOCOL_BITMASK, sizeof(char), IP_PROTOCOL_SIP);
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_REG_INFO, IPPARM_OPERATION_REGISTER, sizeof(char), IP_REG_SET_INFO);
		
		memset((void*)&register_address, 0, sizeof(IP_REGISTER_ADDRESS));
		sprintf(register_address.reg_server, "%s", proxy_ip);// REGISTER request
		sprintf(register_address.reg_client, "%s@%s", alias, proxy_ip);// From/To header
		sprintf(contact, "%s@%s", alias, local_ip);// Contact header
		register_address.time_to_live = 3600;
		register_address.max_hops = 30;

		gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_REG_INFO, IPPARM_REG_ADDRESS, (unsigned char)sizeof(IP_REGISTER_ADDRESS), &register_address);
		gc_util_insert_parm_ref( &gc_parm_blkp, IPSET_LOCAL_ALIAS, IPPARM_ADDRESS_URL, (unsigned char)strlen(contact)+1, (void *)contact);
		gc_ReqService(GCTGT_CCLIB_NETIF, board_dev, &serviceID, gc_parm_blkp, NULL, EV_ASYNC);
		gc_util_delete_parm_blk(gc_parm_blkp);

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

		registered = FALSE;
	}	
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
	
	virt_boards[0].localIP.ip_ver = IPVER4;					// must be set to IPVER4
	virt_boards[0].localIP.u_ipaddr.ipv4 = IP_CFG_DEFAULT;	// or specify host NIC IP address
	virt_boards[0].h323_signaling_port = IP_CFG_DEFAULT;	// or application defined port for H.323 
	virt_boards[0].sip_signaling_port = IP_CFG_DEFAULT;		// or application defined port for SIP
	virt_boards[0].sup_serv_mask = IP_SUP_SERV_CALL_XFER;	// Enable SIP Transfer Feature
	virt_boards[0].sip_msginfo_mask = IP_SIP_MSGINFO_ENABLE;// Enable SIP header
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

	//setting T.38 fax server operating mode: IP MANUAL mode
	gc_util_insert_parm_val(&gc_parm_blk_p, IPSET_CONFIG, IPPARM_OPERATING_MODE, sizeof(long), IP_MANUAL_MODE);
	
	//Enabling and Disabling Unsolicited Notification Events
	gc_util_insert_parm_val(&gc_parm_blk_p, IPSET_EXTENSIONEVT_MSK, GCACT_ADDMSK, sizeof(long), 
		EXTENSIONEVT_DTMF_ALPHANUMERIC|EXTENSIONEVT_SIGNALING_STATUS|EXTENSIONEVT_STREAMING_STATUS|EXTENSIONEVT_T38_STATUS);
	gc_SetConfigData(GCTGT_CCLIB_NETIF, board_dev, gc_parm_blk_p, 0, GCUPDATE_IMMEDIATE, &request_id, EV_ASYNC);
	gc_util_delete_parm_blk(gc_parm_blk_p);

	authentication();

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
	char input = 0;
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
	return FALSE;
}

BOOL analyse_cli()
{
	char input[GC_ADDRSIZE] = "";
	unsigned index = 0;
	char ani[GC_ADDRSIZE] = "";
	char dnis[GC_ADDRSIZE] = "";
	char fax_file[MAX_PATH] = "";
	char confirm[2] = "";
	char proxy_ip[GC_ADDRSIZE] = "";
	char local_ip[GC_ADDRSIZE] = "";
	char alias[GC_ADDRSIZE] = "";
	
	if (kbhit()) {
		if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
		if (0 == strnicmp(input, CLI_QUIT, strlen(CLI_QUIT))) 
		{
			printf("%s", CLI_QUIT_MSG);
			return FALSE;
		} 
		else if (0 == strnicmp(input, CLI_HELP, strlen(CLI_HELP))) 
		{
			printf("%s", CLI_HELP_MSG);
		} 
		else if (0 == strnicmp(input, CLI_MAKECALL, strlen(CLI_MAKECALL))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			printf(CLI_REQ_ANI, CLI_REQ_ANI_DEFAULT);
			if (gets_quit_if_esc(ani, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			if (0 == ani[0])
				sprintf(ani, "%s", CLI_REQ_ANI_DEFAULT);
			printf(CLI_REQ_DNIS, CLI_REQ_DNIS_DEFAULT);
			if (gets_quit_if_esc(dnis, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			if (0 == dnis[0])
				sprintf(dnis, "%s", CLI_REQ_DNIS_DEFAULT);
			channls[index]->make_call(ani, dnis);
		} 
		else if (0 == strnicmp(input, CLI_DROPCALL, strlen(CLI_DROPCALL))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			channls[index]->drop_call();
		} 
		else if (0 == strnicmp(input, CLI_SENDFAX, strlen(CLI_SENDFAX))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			printf(CLI_REQ_FAX_FILE, CLI_REQ_FAX_FILE_DEFAULT);
			if (gets_quit_if_esc(fax_file, MAX_PATH-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			if (0 == fax_file[0])
				sprintf(fax_file, "%s", CLI_REQ_FAX_FILE_DEFAULT);
			channls[index]->do_fax(DF_TX, fax_file);
		} 
		else if (0 == strnicmp(input, CLI_RECEIVEFAX, strlen(CLI_RECEIVEFAX))) 
		{
			printf(CLI_REQ_INDEX, MAX_CHANNELS-1, CLI_REQ_INDEX_DEFAULT);
			if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			index = atoi(input);
			if (index>=MAX_CHANNELS || index<0)
				index = CLI_REQ_INDEX_DEFAULT;
			channls[index]->do_fax(DF_RX, NULL);
		} 
		else if (0 == strnicmp(input, CLI_GLAREFAX, strlen(CLI_GLAREFAX))) 
		{
			printf("%s", CLI_GLAREFAX_MSG);
			printf(CLI_REQ_CONFIRM, CLI_REQ_CONFIRM_DEFAULT);
			if (gets_quit_if_esc(confirm, 1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			if (0 != confirm[0] && 'Y' != confirm[0] && 'y' != confirm[0])
				return TRUE;
			printf(CLI_REQ_FAX_FILE, CLI_REQ_FAX_FILE_DEFAULT);
			if (gets_quit_if_esc(fax_file, MAX_PATH-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			if (0 == fax_file[0])
				sprintf(fax_file, "%s", CLI_REQ_FAX_FILE_DEFAULT);
			channls[0]->do_fax(DF_TX, fax_file);
			channls[1]->do_fax(DF_RX, NULL);
		}
		else if (0 == strnicmp(input, CLI_REGISTER, strlen(CLI_REGISTER)))
		{
			printf(CLI_REQ_PROXY_IP, CLI_REQ_PROXY_IP_DEFAULT);
			if (gets_quit_if_esc(proxy_ip, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			if (0 == proxy_ip[0])
				sprintf(proxy_ip, "%s", CLI_REQ_PROXY_IP_DEFAULT);
			printf(CLI_REQ_LOCAL_IP, CLI_REQ_LOCAL_IP_DEFAULT);
			if (gets_quit_if_esc(local_ip, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			if (0 == local_ip[0])
				sprintf(local_ip, "%s", CLI_REQ_LOCAL_IP_DEFAULT);
			printf(CLI_REQ_ALIAS, CLI_REQ_ALIAS_DEFAULT);
			if (gets_quit_if_esc(alias, GC_ADDRSIZE-1)) { printf("%s", CLI_HELP_MSG); return TRUE; }
			if (0 == alias[0])
				sprintf(alias, "%s", CLI_REQ_ALIAS_DEFAULT);
			registration(proxy_ip, local_ip, alias);
		}
		else if (0 == strnicmp(input, CLI_UNREGISTER, strlen(CLI_UNREGISTER)))
		{
			unregistration();
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

	printf("\n%s", CLI_HELP_MSG);
	
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

			//for register
			if (evt_dev == board_dev && GCEV_SERVICERESP == meta_evt.evttype) {
				gc_parm_blkp = (GC_PARM_BLKP)(meta_evt.extevtdatap);
				gc_parm_datap = gc_util_next_parm(gc_parm_blkp, gc_parm_datap);
				
				while(NULL != gc_parm_datap) {
					if (IPSET_REG_INFO == gc_parm_datap->set_ID) {					
						if (IPPARM_REG_STATUS == gc_parm_datap->parm_ID) {
							value = (int)(gc_parm_datap->value_buf[0]);
							switch(value) {
							case IP_REG_CONFIRMED:
								printf("IPSET_REG_INFO/IPPARM_REG_STATUS: IP_REG_CONFIRMED\n");
								break;
							case IP_REG_REJECTED:
								printf("IPSET_REG_INFO/IPPARM_REG_STATUS: IP_REG_REJECTED\n");
								break;
							default:
								break;
							}
						}
					}
					gc_parm_datap = gc_util_next_parm(gc_parm_blkp, gc_parm_datap);
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
				break;
			case GCEV_UNBLOCKED:
				pch->wait_call();
				break;
			case GCEV_OFFERED:
				pch->print_offer_info();
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
				pch->restore_voice();
				pch->drop_call();
				break;
			case GCEV_EXTENSIONCMPLT:
			case GCEV_EXTENSION:
				pch->process_extension(meta_evt);
				break;
			case GCEV_RELEASECALL:
				pch->already_connect_fax = FALSE;
				pch->fax_proceeding = FALSE;
				break;
			case GCEV_TASKFAIL:
				pch->print_call_status(meta_evt);
				pch->restore_voice();
				break;
			default:
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
