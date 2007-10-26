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

#define HMP_SIP_PORT					5060
#define CLI_BACK						"back"
#define CLI_QUIT						"quit"
#define CLI_MAKECALL					"mc"
#define CLI_DROPCALL					"dc"
#define CLI_SENDFAX						"sf"
#define CLI_RECEIVEFAX					"rf"
#define USER_DISPLAY					"foolbear"
#define USER_AGENT						"HMP test"
#define FAX_FILE						"fax.tif"
#define ANI								"30@192.168.101.30"
#define DNIS							"2150@192.168.101.100"

class CHANNEL;

CHANNEL* channl = NULL;

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

	DF_IOTT fax_iott;
	char fax_file[MAX_PATH];

	CRN crn;

	int id;
	BOOL fax_proceeding;

	CHANNEL(int index) {id = index; fax_proceeding = FALSE;}
	
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
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &gc_xslot;
		fx_listen(fax_dev, &sc_tsinfo);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &fax_xslot;
		gc_Listen(gc_dev, &sc_tsinfo, EV_SYNC);
	}
	
	void restore_voice() {
		SC_TSINFO sc_tsinfo;
		print("restore_voice()...");
		gc_UnListen(gc_dev, EV_SYNC);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &vox_xslot;
		gc_Listen(gc_dev, &sc_tsinfo, EV_SYNC);
		sc_tsinfo.sc_numts = 1;
		sc_tsinfo.sc_tsarrayp = &gc_xslot;
		dx_listenEx(vox_dev, &sc_tsinfo, EV_SYNC);
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

	void drop_call() {
		print("drop_call()...");
		print_gc_error_info("gc_DropCall", gc_DropCall(crn, GC_NORMAL_CLEARING, EV_ASYNC));
	}

	void release_call() {
		print("release_call()...");
		print_gc_error_info("gc_ReleaseCallEx", gc_ReleaseCallEx(crn, EV_ASYNC));
	}
	
	void do_fax(int dir, const char* file) {
		print("do_fax(%s)...", dir==DF_RX?"RX":"TX");
		fax_proceeding = TRUE;
		sprintf(fax_file, "%s", NULL==file?"":file);
		connect_fax();
		if (DF_RX == dir) {
			receive_fax_inner();
		} else {
			send_fax_inner();
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
		fax_proceeding = FALSE;
	}

	void process_fax_sent() {
		print_fax_phase_d_info();
		process_fax_done();
	}

	void process_fax_received() {
		process_fax_sent();
	}

	void process_fax_error() {
		print("Phase E status: %d", ATFX_ESTAT(fax_dev));
		process_fax_done();
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
					print("    stream codec infomation: capability(%d), dir(%d), frames_per_pkt(%d), VAD(%d)", 
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
		print("set_codec(g711U[103]/g711A[101]/g729[116/118/119/120])...");
		IP_CAPABILITY ip_cap[6];		
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
		GC_PARM_BLKP parmblkp = NULL;
		for (int i=0; i<6; i++)
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
		GetLocalTime(&t);
		printf("%02d:%02d:%02d.%04d CH %d: %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, id, buf);
	}
};

BOOL gets_quit_if_esc(char* output, unsigned size)
{
	gets(output);
	return (0 == strnicmp(output, CLI_BACK, strlen(CLI_BACK)))?TRUE:FALSE;
}

BOOL analyse_cli()
{
	char input[GC_ADDRSIZE] = "";
	char ani[GC_ADDRSIZE] = "";
	char dnis[GC_ADDRSIZE] = "";
	
	if (kbhit()) {
		if (gets_quit_if_esc(input, GC_ADDRSIZE-1)) 
		{ 
			return TRUE; 
		}
		if (0 == strnicmp(input, CLI_QUIT, strlen(CLI_QUIT))) 
		{
			printf("%s", "HMP test quitting...\n");
			return FALSE;
		}
		else if (0 == strnicmp(input, CLI_MAKECALL, strlen(CLI_MAKECALL))) 
		{
			channl->make_call(ANI, DNIS);
		}
		else if (0 == strnicmp(input, CLI_DROPCALL, strlen(CLI_DROPCALL))) 
		{
			channl->drop_call();
		} 
		else if (0 == strnicmp(input, CLI_SENDFAX, strlen(CLI_SENDFAX))) 
		{
			channl->do_fax(DF_TX, FAX_FILE);
		}
		else if (0 == strnicmp(input, CLI_RECEIVEFAX, strlen(CLI_RECEIVEFAX))) 
		{
			channl->do_fax(DF_RX, NULL);
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
				break;
			case GCEV_CALLSTATUS:
				pch->print_call_status(meta_evt);
				break;
			case GCEV_CONNECTED:
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
			case GCEV_RELEASECALL:
				pch->fax_proceeding = FALSE;
				pch->crn = 0;
				break;
			case GCEV_TASKFAIL:
				pch->print_call_status(meta_evt);
				if (TRUE == pch->fax_proceeding)
					pch->restore_voice();
				break;
			default:
				pch->print_call_status(meta_evt);
				break;
			}
		} else {
			
			if (channl->vox_dev == evt_dev || channl->fax_dev == evt_dev)
				pch = channl;
			if (NULL == pch)
				continue;

			switch(evt_code)
			{
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
	GC_START_STRUCT	gclib_start;
	IPCCLIB_START_DATA cclib_start_data;
	IP_VIRTBOARD virt_boards[1];

	printf("global call start...\n");
	{
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

	channl = new CHANNEL(0);
	channl->open();
	
	loop();
	
	channl->close();
	gc_Stop();

	return 0;
}
