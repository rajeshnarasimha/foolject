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

#define ANI "ani@127.0.0.1"
#define DNIS "dnis@127.0.0.1"
#define TIFFILE "D:/NetVoice/Sample/VXML/small.tif"
#define GUIDE "please enter the command code, 'c' for make call, 'd' for drop call, 's' for send fax, 'r' for receive fax, 'f' for 491 REQUEST PENDING, 'q' for quit test.\n\n"
	
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
	BOOL already_connect_fax;
	int fax_dir;

	CRN crn;

	int id;
	CHANNEL(int index) {id = index; already_connect_fax = FALSE;}

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
		gc_WaitCall(gc_dev, NULL, NULL, 0, EV_ASYNC);
	}

	void print_offer_info() {
		char ani[255] = "";
		char dnis[255] = "";
		int protocol = CALLPROTOCOL_H323;
		int rc = gc_GetCallInfo(crn, ORIGINATION_ADDRESS, ani);
		gc_GetCallInfo(crn, DESTINATION_ADDRESS, dnis);
		gc_GetCallInfo(crn, CALLPROTOCOL, (char*)&protocol);
		print("number %s, got %s offer from %s", 
			ani, protocol==CALLPROTOCOL_H323?"H323":"SIP", dnis);
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
	
	void accept_call() {
		print("accept_call()...");
		int rc = gc_AcceptCall(crn, 0, EV_ASYNC);
		GC_INFO gc_error_info;
		if (GC_ERROR == rc) {
			gc_ErrorInfo(&gc_error_info);
			print("gc_MakeCall return %d, GC ErrorValue:0x%hx-%s,\n  CCLibID:%i-%s, CC ErrorValue:0x%lx-%s,\n  Additional Info:%s",
				rc, gc_error_info.gcValue, gc_error_info.gcMsg,
				gc_error_info.ccLibId, gc_error_info.ccLibName,
				gc_error_info.ccValue, gc_error_info.ccMsg, 
				gc_error_info.additionalInfo);
		}
	}
	
	void answer_call() {
		print("answer_call()...");
		set_codec(GCTGT_GCLIB_CRN);
		gc_AnswerCall(crn, 0, EV_ASYNC);
	}
	
	void make_call(const char* ani, const char* dnis) {
		print("make_call(to %s)...", dnis);
		GC_PARM_BLKP gc_parm_blkp = NULL;
		GC_MAKECALL_BLK gc_mk_blk;
		GCLIB_MAKECALL_BLK gclib_mk_blk = {0};
		gc_mk_blk.cclib = NULL;
		gc_mk_blk.gclib = &gclib_mk_blk;
		strcpy(gc_mk_blk.gclib->origination.address, ani);
		gc_mk_blk.gclib->origination.address_type = GCADDRTYPE_TRANSPARENT;
		set_codec(GCTGT_GCLIB_CHAN);
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_PROTOCOL,
			IPPARM_PROTOCOL_BITMASK, sizeof(int), IP_PROTOCOL_SIP);
		gclib_mk_blk.ext_datap = gc_parm_blkp;
		
		int rc = gc_MakeCall(gc_dev, &crn, (char*)dnis, &gc_mk_blk, 30, EV_ASYNC);
		GC_INFO gc_error_info;
		if (GC_ERROR == rc) {
			gc_ErrorInfo(&gc_error_info);
			print("gc_MakeCall return %d, GC ErrorValue:0x%hx-%s,\n  CCLibID:%i-%s, CC ErrorValue:0x%lx-%s,\n  Additional Info:%s",
				rc, gc_error_info.gcValue, gc_error_info.gcMsg,
				gc_error_info.ccLibId, gc_error_info.ccLibName,
				gc_error_info.ccValue, gc_error_info.ccMsg, 
				gc_error_info.additionalInfo);
		}
		
		gc_util_delete_parm_blk(gc_parm_blkp);
	}

	void drop_call() {
		print("drop_call()...");
		if (already_connect_fax)
			restore_voice();
		gc_DropCall(crn, GC_NORMAL_CLEARING, EV_ASYNC);
	}

	void release_call() {
		print("release_call()...");
		gc_ReleaseCallEx(crn, EV_ASYNC);
	}
	
	void send_t38_request() {
		print("send_t38_request()...");	
		GC_PARM_BLKP gc_parm_blkp = NULL;	
		gc_util_insert_parm_ref(&gc_parm_blkp, IPSET_SWITCH_CODEC, IPPARM_T38_INITIATE, sizeof(int), NULL);
		gc_Extension(GCTGT_GCLIB_CRN, crn, IPEXTID_CHANGEMODE, gc_parm_blkp, NULL, EV_ASYNC);		
		gc_util_delete_parm_blk(gc_parm_blkp);
	}
	
	void response_t38_request(BOOL accept_call) {
		print("response_t38_request(%s)...", accept_call?"accept":"reject");
		GC_PARM_BLKP gc_parm_blkp = NULL;
		gc_util_insert_parm_val(&gc_parm_blkp, IPSET_SWITCH_CODEC, accept_call?IPPARM_ACCEPT:IPPARM_REJECT, sizeof(int), NULL);
		gc_Extension(GCTGT_GCLIB_CRN, crn, IPEXTID_CHANGEMODE, gc_parm_blkp, NULL, EV_ASYNC);
		gc_util_delete_parm_blk(gc_parm_blkp);
	}
	
	void do_fax(int dir) {
		print("do_fax(%s)...", dir==DF_RX?"RX":"TX");
		fax_dir = dir;
		if (already_connect_fax)
			response_t38_request(TRUE);
		else {
			connect_fax();
			send_t38_request();
			already_connect_fax = TRUE;
		}
	}
	
	void send_fax_inner() {	
		print("send_fax_inner()...");
		char fax_file[255] = "";
		char local_id[255] = "";
		char fax_header[255] = "";
		int fhandle = 0;
		unsigned long sndflag = EV_ASYNC|DF_PHASEB|DF_PHASED;		
		fx_initstat(fax_dev, DF_TX);		
		sprintf(fax_header, "FAX_HEADER_%d", id);
		fx_setparm(fax_dev, FC_HDRUSER, (void*)fax_header);
		sprintf(local_id, "FAX_LOCALID_%d", id);
		fx_setparm(fax_dev, FC_LOCALID, (void*)local_id);		
		sprintf(fax_file, TIFFILE);
		fhandle = dx_fileopen(fax_file, 0x0000|0x8000, NULL);
		fx_setiott(&fax_iott, fhandle, DF_TIFF, DFC_AUTO);
		fax_iott.io_type |= IO_EOT;
		fax_iott.io_phdcont = DFC_EOP;
		fx_sendfax(fax_dev, &fax_iott, sndflag);
	}
	
	void receive_fax_inner() {
		print("receive_fax_inner()...");
		char fax_file[255] = "";
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
		print("Connect with \"%s\" at %d baud, with %s encoding", remote_id, ATFX_SPEED(fax_dev), fax_encoding);
	}
	
	void print_fax_phase_d_info() {		
		int page_count = ATFX_PGXFER(fax_dev);		
		print("Phase D Information...\n  Page:%ld, scan lines:%ld;\n  Page width:%ld, resolution:%ld, transferred bytes:%ld;\n  Speed:%ld, bad scan lines:%ld, Retrain negative pages:%ld", 
		  page_count, ATFX_SCANLINES(fax_dev), ATFX_WIDTH(fax_dev), ATFX_RESLN(fax_dev), ATFX_TRCOUNT(fax_dev), 
		  ATFX_SPEED(fax_dev), ATFX_BADSCANLINES(fax_dev), ATFX_RTNPAGES(fax_dev));
	}

	void process_fax_sent() {
		print_fax_phase_d_info();
		dx_fileclose(fax_iott.io_fhandle);
		restore_voice();
		already_connect_fax = FALSE;
	}

	void process_fax_received() {
		process_fax_sent();
	}

	void process_fax_error() {
		print("Phase E status: %d", ATFX_ESTAT(fax_dev));
		dx_fileclose(fax_iott.io_fhandle);
		restore_voice();
		already_connect_fax = FALSE;
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
					break;
				case IPPARM_READY:
					print("  IPPARM_READY:");
					if (DF_TX == fax_dir) 
						send_fax_inner();
					if (DF_RX == fax_dir)
						receive_fax_inner();
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
} ch1(1), ch2(2);

int main(int argc, char* argv[])
{
/*	int i = 0;
	int j = 0;
	int board_count = 0;
	int sub_dev_count = 0;
	int dsp_resource_count = 0;
	long handle = 0;
	long dev_handle = 0;
	char board_name[20] = "";
	char dev_name[20] = "";
	FEATURE_TABLE ft = {0};

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
*/	
	gc_Start(NULL);
	printf("gc_Start() done...\n\n%s", GUIDE);

/*	sr_getboardcnt(DEV_CLASS_IPT, &board_count);
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
	}*/

	//////////////////////////////////////////////////////////////////////////
	long board_dev = 0;
	long request_id = 0;
	GC_PARM_BLKP gc_parm_blk_p = NULL;
	
	gc_OpenEx(&board_dev, ":N_iptB1:P_IP", EV_SYNC, NULL);
	gc_util_insert_parm_val(&gc_parm_blk_p, IPSET_CONFIG, IPPARM_OPERATING_MODE, sizeof(long), IP_MANUAL_MODE);
	gc_util_insert_parm_val(&gc_parm_blk_p,
		IPSET_EXTENSIONEVT_MSK, GCACT_ADDMSK, sizeof(long), 
		EXTENSIONEVT_DTMF_ALPHANUMERIC|EXTENSIONEVT_SIGNALING_STATUS|EXTENSIONEVT_STREAMING_STATUS|EXTENSIONEVT_T38_STATUS);
	gc_SetConfigData(GCTGT_CCLIB_NETIF,
		board_dev, gc_parm_blk_p, 0, GCUPDATE_IMMEDIATE, &request_id, EV_ASYNC);
	gc_util_delete_parm_blk(gc_parm_blk_p);
	
	ch1.vox_dev = dx_open("dxxxB1C1", NULL);
	ch1.fax_dev = fx_open("dxxxB2C1", NULL);
	gc_OpenEx(&ch1.gc_dev, ":N_iptB1T1:P_SIP:M_ipmB1C1", EV_ASYNC, &ch1);
	dx_setevtmsk(ch1.vox_dev, DM_RINGS|DM_DIGITS|DM_LCOF);

	ch2.vox_dev = dx_open("dxxxB1C2", NULL);
	ch2.fax_dev = fx_open("dxxxB2C2", NULL);
	gc_OpenEx(&ch2.gc_dev, ":N_iptB1T2:P_SIP:M_ipmB1C2", EV_ASYNC, &ch2);
	dx_setevtmsk(ch2.vox_dev, DM_RINGS|DM_DIGITS|DM_LCOF);

	int timeout = 0;
	long event_handle = 0;
	int evt_dev = 0;
	int evt_code = 0;
	int evt_len = 0;
	void* evt_datap = NULL;
	METAEVENT meta_evt;
	CHANNEL* pch = NULL;

	BOOL quit_flag = FALSE;
	char input = 0;
	
	while (TRUE)
	{
		do {			
			timeout = sr_waitevt( 500 );
			if (kbhit()) {
				input = getch();
				printf("%c\n", input);
				switch(input)
				{
				case 'e':
				case 'q':
					quit_flag = TRUE;
					break;
				case 'c':
				case 'm':
					ch1.make_call(ANI, DNIS);
				    break;
				case 'd':
				case 'h':
					ch1.drop_call();
					break;
				case 's':
				case 't':
					ch1.do_fax(DF_TX);
					break;
				case 'r':
					ch2.do_fax(DF_RX);
					break;
				case 'f':
					ch2.do_fax(DF_RX);
					ch1.do_fax(DF_TX);
					break;
				case 13:
					printf("%s", GUIDE);
					break;
				default:
				    break;
				}
			}
			if (TRUE == quit_flag) {
				goto QUIT_TEST;
			}
		} while(timeout == SR_TMOUT);
		
		evt_dev   = (int)sr_getevtdev(event_handle);
		evt_code  = (int)sr_getevttype(event_handle);
		evt_len   = (int)sr_getevtlen(event_handle);
		evt_datap = (void*)sr_getevtdatap(event_handle);

		if (evt_dev == board_dev) continue;

		gc_GetMetaEventEx(&meta_evt, event_handle);
		if (meta_evt.flags & GCME_GC_EVENT) {
			gc_GetUsrAttr(meta_evt.linedev, (void**)&pch);
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
				pch->drop_call();
				break;
			case GCEV_EXTENSIONCMPLT:
			case GCEV_EXTENSION:
				pch->process_extension(meta_evt);
			default:
				break;
			}
		} else {
			if (evt_dev == ch1.fax_dev) pch = &ch1;
			else if (evt_dev == ch2.fax_dev) pch = &ch2;
			else continue;

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
				break;
			}
		}
	}

QUIT_TEST:
	printf("QUIT_TEST\n");
	gc_Close(board_dev);
	ch1.close();
	ch2.close();

	//////////////////////////////////////////////////////////////////////////
	gc_Stop();
	
	return 0;
}

