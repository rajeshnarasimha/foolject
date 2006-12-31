// hmpvideo.cpp : Defines the entry point for the console application.
//

#include "stdio.h"
#include "conio.h"

#include <srllib.h>
#include <dxxxlib.h>
#include <faxlib.h>
#include <ipmlib.h>
#include <devmgmt.h>
#include <mmlib.h>

#define USER_EVT_BOTH_MEDIA_READY 0x10000

void Log(const char *format, ...) {
	char buf[1024] = "";
	SYSTEMTIME t;
	va_list argptr;
	va_start(argptr, format);
	_vsnprintf(buf, 1023, format, argptr);
	buf[1023] = '\0';
	va_end(argptr);		
	GetLocalTime(&t);
	printf("%02d:%02d:%02d.%04d %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, buf);
}

class LINE;

LINE* line_0 = NULL;
LINE* line_1 = NULL;

long media_ready_count = 0;

const char video_play[] = "show.vid";//"p.vid";
const char audio_play[] = "show.pcm";//"p.pcm";
const char video_record[] = "r.vid";
const char audio_record[] = "r.pcm";

class LINE
{
public:
	long index;
	
	char dx_name[64];
	char mm_name[64];
	char ipm_name[64];

	long dx_handle;
	long mm_handle;
	long ipm_handle;

	long audio_port;
	long video_port;
	char* local_ip;

public:
	LINE(long id, const char* dx, const char* mm, const char* ipm) {
		index = id;
		sprintf(dx_name, dx);
		sprintf(mm_name, mm);
		sprintf(ipm_name, ipm);
	}

	~LINE(){};

	void Log(const char *format, ...) {
		char buf[1024] = "";
		SYSTEMTIME t;
		va_list argptr;
		va_start(argptr, format);
		_vsnprintf(buf, 1023, format, argptr);
		buf[1023] = '\0';
		va_end(argptr);		
		GetLocalTime(&t);
		printf("%02d:%02d:%02d.%04d [%d] %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, index, buf);
	}

	void open() {
		Log("open()...");
		LINE* lp = this;
		dx_handle = dx_open(dx_name, 0);
		ipm_handle = ipm_Open(ipm_name, 0, EV_ASYNC);
		sr_setparm(dx_handle, SR_USERCONTEXT, (void*)&lp);
		sr_setparm(ipm_handle, SR_USERCONTEXT, (void*)&lp);
		dx_setevtmsk(dx_handle, DM_DIGITS);
	}

	void close() {
		Log("close()...");
		dx_close(dx_handle);
		ipm_Close(ipm_handle, 0);
		mm_Close(mm_handle, 0);
	}

	void open_mm() {
		Log("open_mm()...");
		LINE* lp = this;
		mm_handle = mm_Open(mm_name, 0, NULL);
		sr_setparm(mm_handle, SR_USERCONTEXT, (void*)&lp);
	}

	void get_local_media_info() {
		IPM_MEDIA_INFO media_info;
		Log("get_local_media_info()...");
		media_info.unCount = 2;
		media_info.MediaData[0].eMediaType = MEDIATYPE_VIDEO_LOCAL_RTP_INFO;
		media_info.MediaData[1].eMediaType = MEDIATYPE_AUDIO_LOCAL_RTP_INFO;
		ipm_GetLocalMediaInfo( ipm_handle, &media_info, EV_ASYNC);
	}

	void dx_listen_to_ipm() {
		long ts = 0;
		SC_TSINFO scts;
		Log("dx_listen_to_ipm()...");
		scts.sc_numts = 1;
		scts.sc_tsarrayp = &ts;
		ipm_GetXmitSlot(ipm_handle, &scts, EV_SYNC);
		scts.sc_numts = 1;
		*(scts.sc_tsarrayp) = ts;
		dx_listen(dx_handle, &scts);
	}

	void ipm_listen_to_dx() {
		long ts = 0;
		SC_TSINFO scts;
		Log("ipm_listen_to_dx()...");
		scts.sc_numts = 1;
		scts.sc_tsarrayp = &ts;
		dx_getxmitslot(dx_handle, &scts);
		scts.sc_numts = 1;
		*(scts.sc_tsarrayp) = ts;
		ipm_Listen(ipm_handle, &scts, EV_SYNC);
	}
	
	void ipm_unlisten_to_dx() {
		Log("ipm_unlisten_to_dx()...");
		ipm_UnListen(ipm_handle, EV_SYNC);
	}

	void send_dtmf() {
		Log("send_dtmf()...");
		dx_dial(dx_handle, "0123456789*#abcd", (DX_CAP *)NULL, EV_ASYNC);
	}

	void notify_both_media_ready() {
		sr_putevt(ipm_handle, USER_EVT_BOTH_MEDIA_READY, 0, NULL, 0);
	}

	void set_dtmf_mode(eIPM_DTMFXFERMODE dtmf_mode) {
		IPM_PARM_INFO parm_info;
		Log("set_dtmf_mode(%s)...", dtmf_mode==DTMFXFERMODE_INBAND?"INBAND":dtmf_mode==DTMFXFERMODE_RFC2833?"RFC2833":"OUTOFBAND");
		parm_info.eParm = PARMCH_DTMFXFERMODE;
		parm_info.pvParmValue = &dtmf_mode;
		ipm_SetParm(ipm_handle, &parm_info, EV_SYNC);// set dtmf mode
		if (DTMFXFERMODE_RFC2833 == dtmf_mode) {
			int pl_type = 101;
			parm_info.eParm = PARMCH_RFC2833EVT_TX_PLT;// set the TX Payload Type
			parm_info.pvParmValue = &pl_type;
			ipm_SetParm(ipm_handle, &parm_info, EV_SYNC);
			parm_info.eParm = PARMCH_RFC2833EVT_RX_PLT;
			parm_info.pvParmValue = &pl_type;
			ipm_SetParm(ipm_handle, &parm_info, EV_SYNC);
		}
	}

	void send_digits() {
		IPM_DIGIT_INFO digit_info;
		Log("send_digits()...");
		digit_info.eDigitType = DIGIT_ALPHA_NUMERIC;//DIGIT_SIGNAL
		digit_info.eDigitDirection = DIGIT_TDM;
		strcpy(digit_info.cDigits,"abcd*#1234567890");
		digit_info.unNumberOfDigits = 16;
		ipm_SendDigits(ipm_handle, &digit_info, EV_ASYNC);
	}

	void receive_digits() {
		IPM_DIGIT_INFO digit_info;
		Log("receive_digits()...");
		digit_info.eDigitType = DIGIT_ALPHA_NUMERIC;
		digit_info.eDigitDirection = DIGIT_TDM;
		ipm_ReceiveDigits(ipm_handle, &digit_info, EV_ASYNC);
	}

	void start_media() {
		IPM_MEDIA_INFO media_info;
		LINE* peer = (1 == index)?line_0:line_1;
		Log("start_media()...");
		set_dtmf_mode(DTMFXFERMODE_INBAND);//DTMFXFERMODE_INBAND;//DTMFXFERMODE_RFC2833;//DTMFXFERMODE_OUTOFBAND
		receive_digits();
		media_info.unCount = 8;
		media_info.MediaData[0].eMediaType = MEDIATYPE_AUDIO_REMOTE_CODER_INFO;
		media_info.MediaData[0].mediaInfo.CoderInfo.eCoderType = CODER_TYPE_G711ULAW64K;
		media_info.MediaData[0].mediaInfo.CoderInfo.eFrameSize = (eIPM_CODER_FRAMESIZE)CODER_FRAMESIZE_30;
		media_info.MediaData[0].mediaInfo.CoderInfo.unFramesPerPkt = 1;
		media_info.MediaData[0].mediaInfo.CoderInfo.eVadEnable = CODER_VAD_DISABLE;
		media_info.MediaData[0].mediaInfo.CoderInfo.unCoderPayloadType = 0;
		media_info.MediaData[0].mediaInfo.CoderInfo.unRedPayloadType = 0;
		media_info.MediaData[1].eMediaType = MEDIATYPE_AUDIO_LOCAL_CODER_INFO;
		media_info.MediaData[1].mediaInfo.CoderInfo.eCoderType = CODER_TYPE_G711ULAW64K;
		media_info.MediaData[1].mediaInfo.CoderInfo.eFrameSize = (eIPM_CODER_FRAMESIZE)CODER_FRAMESIZE_30; 
		media_info.MediaData[1].mediaInfo.CoderInfo.unFramesPerPkt = 1;
		media_info.MediaData[1].mediaInfo.CoderInfo.eVadEnable = CODER_VAD_DISABLE;
		media_info.MediaData[1].mediaInfo.CoderInfo.unCoderPayloadType = 0;
		media_info.MediaData[1].mediaInfo.CoderInfo.unRedPayloadType = 0;
		media_info.MediaData[2].eMediaType = MEDIATYPE_VIDEO_REMOTE_CODER_INFO;
		media_info.MediaData[2].mediaInfo.VideoCoderInfo.eCoderType = CODER_TYPE_H263;
		media_info.MediaData[2].mediaInfo.VideoCoderInfo.unVersion = 0;
		media_info.MediaData[2].mediaInfo.VideoCoderInfo.unCoderPayloadType = 100;
		media_info.MediaData[3].eMediaType = MEDIATYPE_VIDEO_LOCAL_CODER_INFO;
		media_info.MediaData[3].mediaInfo.VideoCoderInfo.eCoderType = CODER_TYPE_H263;
		media_info.MediaData[3].mediaInfo.VideoCoderInfo.unVersion = 0;
		media_info.MediaData[3].mediaInfo.VideoCoderInfo.unCoderPayloadType	= 100;
		media_info.MediaData[4].eMediaType = MEDIATYPE_AUDIO_REMOTE_RTP_INFO;
		strcpy(media_info.MediaData[4].mediaInfo.PortInfo.cIPAddress, peer->local_ip);
		media_info.MediaData[4].mediaInfo.PortInfo.unPortId = peer->audio_port;
		media_info.MediaData[5].eMediaType = MEDIATYPE_AUDIO_REMOTE_RTCP_INFO;
		strcpy(media_info.MediaData[5].mediaInfo.PortInfo.cIPAddress, peer->local_ip);
		media_info.MediaData[5].mediaInfo.PortInfo.unPortId = peer->audio_port+1;
		media_info.MediaData[6].eMediaType = MEDIATYPE_VIDEO_REMOTE_RTP_INFO;
		strcpy(media_info.MediaData[6].mediaInfo.PortInfo.cIPAddress, peer->local_ip);
		media_info.MediaData[6].mediaInfo.PortInfo.unPortId = peer->video_port; 
		media_info.MediaData[7].eMediaType = MEDIATYPE_VIDEO_REMOTE_RTCP_INFO;
		strcpy(media_info.MediaData[7].mediaInfo.PortInfo.cIPAddress, peer->local_ip);
		media_info.MediaData[7].mediaInfo.PortInfo.unPortId = peer->video_port+1;
		ipm_StartMedia(ipm_handle, &media_info, DATA_IP_TDM_BIDIRECTIONAL, EV_ASYNC);
	}

	void stop_media() {
		Log("stop_media()...");
		ipm_Stop(ipm_handle, STOP_ALL, EV_ASYNC);
	}

	void play() {
		MM_PLAY_INFO play_info;
		MM_MEDIA_ITEM_LIST audio_list;
		MM_MEDIA_ITEM_LIST video_list;
		MM_PLAY_RECORD_LIST mm_list[2]; 
		Log("play()...");
		audio_list.item.audio.codec.unBitsPerSample	= 16;
		audio_list.item.audio.codec.unCoding = 1;
		audio_list.item.audio.codec.unSampleRate = 8000;
		audio_list.item.audio.unMode = 0x0020;
		audio_list.item.audio.unOffset = 0;
		audio_list.item.audio.szFileName = audio_play;
		audio_list.ItemChain = EMM_ITEM_EOT;
		video_list.item.video.unMode = 0;
		video_list.item.video.codec.FramesPerSec = EMM_VIDEO_FRAMESPERSEC_UNDEFINED;
		video_list.item.video.codec.ImageHeight = EMM_VIDEO_IMAGE_HEIGHT_UNDEFINED;
		video_list.item.video.codec.ImageWidth = EMM_VIDEO_IMAGE_WIDTH_UNDEFINED;
		video_list.item.video.codec.Level = EMM_VIDEO_LEVEL_UNDEFINED;
		video_list.item.video.codec.Profile = EMM_VIDEO_PROFILE_UNDEFINED;
		video_list.item.video.szFileName = video_play;
		video_list.ItemChain = EMM_ITEM_EOT;
		mm_list[0].ItemType = EMM_MEDIA_TYPE_AUDIO;
		mm_list[0].list = &audio_list; 
		mm_list[0].ItemChain = EMM_ITEM_CONT;
		mm_list[1].ItemType = EMM_MEDIA_TYPE_VIDEO;
		mm_list[1].list = &video_list;
		mm_list[1].ItemChain = EMM_ITEM_EOT;
		play_info.unVersion = 0x100;
		play_info.eFileFormat = EMM_FILE_FORMAT_PROPRIETARY;
		play_info.list = mm_list;
		mm_Play(mm_handle, &play_info, NULL, NULL);
	}

	void record() {
		MM_RECORD_INFO record_info; 
		MM_MEDIA_ITEM_LIST audio_list;
		MM_MEDIA_ITEM_LIST video_list;
		MM_PLAY_RECORD_LIST mm_list[2]; 
		Log("record()...");
		audio_list.item.audio.codec.unBitsPerSample = 16;
		audio_list.item.audio.codec.unCoding = 1;
		audio_list.item.audio.codec.unSampleRate = 8000;
		audio_list.item.audio.unMode = 0x0020;
		audio_list.item.audio.unOffset = 0;
		audio_list.item.audio.szFileName = audio_record;
		audio_list.ItemChain = EMM_ITEM_EOT;
		video_list.item.video.unMode = 0;
		video_list.item.video.codec.FramesPerSec = EMM_VIDEO_FRAMESPERSEC_UNDEFINED;
		video_list.item.video.codec.ImageHeight = EMM_VIDEO_IMAGE_HEIGHT_UNDEFINED;
		video_list.item.video.codec.ImageWidth = EMM_VIDEO_IMAGE_WIDTH_UNDEFINED;
		video_list.item.video.codec.Level = EMM_VIDEO_LEVEL_UNDEFINED;
		video_list.item.video.codec.Profile = EMM_VIDEO_PROFILE_UNDEFINED;
		video_list.item.video.szFileName = video_record;
		video_list.ItemChain = EMM_ITEM_EOT;
		mm_list[0].ItemType = EMM_MEDIA_TYPE_AUDIO;
		mm_list[0].list = &audio_list;
		mm_list[0].ItemChain = EMM_ITEM_CONT;
		mm_list[1].ItemType = EMM_MEDIA_TYPE_VIDEO;
		mm_list[1].list = &video_list;
		mm_list[1].ItemChain = EMM_ITEM_EOT;
		record_info.unVersion = 0x100;
		record_info.eFileFormat = EMM_FILE_FORMAT_PROPRIETARY;
		record_info.list = mm_list;
		mm_Record(mm_handle, &record_info, NULL, NULL);
	}

	void stop_record() {
		Log("stop_record()...");
		MM_STOP stop[2];
		stop[0].unVersion	= MM_STOP_VERSION_0;
		stop[0].ItemChain	= EMM_ITEM_CONT;
		stop[0].ItemType	= EMM_STOP_VIDEO_RECORD;
		stop[1].unVersion	= MM_STOP_VERSION_0;
		stop[1].ItemChain	= EMM_ITEM_EOT;
		stop[1].ItemType	= EMM_STOP_AUDIO_RECORD;
		mm_Stop(mm_handle, stop, NULL);
	}

	void stop_play() {
		Log("stop_play()...");
		MM_STOP stop[2];
		stop[0].unVersion	= MM_STOP_VERSION_0;
		stop[0].ItemChain	= EMM_ITEM_CONT;
		stop[0].ItemType	= EMM_STOP_VIDEO_PLAY;
		stop[1].unVersion	= MM_STOP_VERSION_0;
		stop[1].ItemChain	= EMM_ITEM_EOT;
		stop[1].ItemType	= EMM_STOP_AUDIO_PLAY;
		mm_Stop(mm_handle, stop, NULL);
	}

	void Handle(long evt_dev, long evt_code, DX_CST* evt_datap, long evt_len) {
		IPM_MEDIA_INFO* media_infop;
		IPM_DIGIT_INFO* digit_infop;
		unsigned int i = 0;
		LINE* peer = (1 == index)?line_0:line_1;

		if (evt_dev == dx_handle) {
			switch(evt_code)
			{
			case TDX_CST:
				Log("TDX_CST");
				if (DE_DIGITS == evt_datap->cst_event) {
					Log("DE_DIGITS: '%c'.", (char)evt_datap->cst_data);
				}
				break;
			case TDX_DIAL:
				Log("TDX_DIAL");
				ipm_unlisten_to_dx();
//				send_digits();
				break;
			default:
				Log("unknown event(%d) for dx device(%d).", evt_code, evt_dev);
				break;
			}
		} else if (evt_dev == mm_handle) {
			switch(evt_code)
			{
			case MMEV_OPEN:
				Log("MMEV_OPEN");
				dev_Connect(ipm_handle, mm_handle, DM_FULLDUP, EV_ASYNC);
				break;
			case MMEV_OPEN_FAIL:
				Log("MMEV_OPEN_FAIL");
				break;
			case MMEV_PLAY_ACK:
				Log("MMEV_PLAY_ACK");
				ipm_listen_to_dx();
				send_dtmf();
				break;
			case MMEV_PLAY_ACK_FAIL:
				Log("MMEV_PLAY_ACK_FAIL");
				break;
			case MMEV_PLAY:
				Log("MMEV_PLAY");
				stop_media();
				peer->stop_record();
				break;
			case MMEV_PLAY_FAIL:
				Log("MMEV_PLAY_FAIL");
				break;
			case MMEV_RECORD_ACK:
				Log("MMEV_RECORD_ACK");
				break;
			case MMEV_RECORD_ACK_FAIL:
				Log("MMEV_RECORD_ACK_FAIL");
				break;
			case MMEV_RECORD:
				Log("MMEV_RECORD");
				stop_media();
				break;
			case MMEV_RECORD_FAIL:
				Log("MMEV_RECORD_FAIL");
				break;
			case MMEV_STOP_ACK:
				Log("MMEV_STOP_ACK");
				break;
			case MMEV_STOP_ACK_FAIL:
				Log("MMEV_STOP_ACK_FAIL");
				break;
			case MMEV_VIDEO_RECORD_STARTED:
				Log("MMEV_VIDEO_RECORD_STARTED");
				break;
			case MMEV_VIDEO_RECORD_STARTED_FAIL:
				Log("MMEV_VIDEO_RECORD_STARTED_FAIL");
				break;				
			case MMEV_ERROR:
				Log("MMEV_ERROR");
				break;
			case DMEV_CONNECT:
				Log("mm.DMEV_CONNECT");
				break;
			case DMEV_DISCONNECT:
				Log("mm.DMEV_DISCONNECT");
				break;
			default:
				Log("unknown event(%d) for mm device(%d).", evt_code, evt_dev);
				break;
			}
		} else if (evt_dev == ipm_handle) {
			switch(evt_code)
			{
			case IPMEV_OPEN:
				Log("IPMEV_OPEN");
				open_mm();
				break;
			case DMEV_CONNECT:
				Log("ipm.DMEV_CONNECT");
				get_local_media_info();
				break;
			case IPMEV_GET_LOCAL_MEDIA_INFO:
				Log("IPMEV_GET_LOCAL_MEDIA_INFO");
				media_infop = (IPM_MEDIA_INFO*)evt_datap;
				for(i=0; i<media_infop->unCount; i++) {
					switch(media_infop->MediaData[i].eMediaType)
					{
					case MEDIATYPE_AUDIO_LOCAL_RTP_INFO:
						audio_port = media_infop->MediaData[i].mediaInfo.PortInfo.unPortId;
						local_ip = media_infop->MediaData[i].mediaInfo.PortInfo.cIPAddress;
						break;
					case MEDIATYPE_VIDEO_LOCAL_RTP_INFO:
						video_port = media_infop->MediaData[i].mediaInfo.PortInfo.unPortId;
						local_ip = media_infop->MediaData[i].mediaInfo.PortInfo.cIPAddress;
					    break;
					default:
					    break;
					}
				}
				Log("local ip = %s, audio port = %u, video port = %u.", local_ip, audio_port, video_port);
				dx_listen_to_ipm();
				if (++media_ready_count == 2) {
					media_ready_count = 0;
					line_0->notify_both_media_ready();
					line_1->notify_both_media_ready();
				}
				break;
			case USER_EVT_BOTH_MEDIA_READY:
				start_media();
				break;
			case DMEV_DISCONNECT:
				Log("ipm.DMEV_DISCONNECT");
				break;
			case IPMEV_STARTMEDIA:
				Log("IPMEV_STARTMEDIA");
				if (0 == index) play();
				else record();
				break;
			case IPMEV_STOPPED:
				Log("IPMEV_STOPPED");
				dev_Disconnect(ipm_handle, EV_ASYNC);
				dev_Disconnect(mm_handle, EV_ASYNC);
				break;
			case IPMEV_SEND_DIGITS:
				Log("IPMEV_SEND_DIGITS");
				break;
			case IPMEV_RECEIVE_DIGITS:
				Log("IPMEV_RECEIVE_DIGITS");
				break;
			case IPMEV_DIGITS_RECEIVED:
				Log("IPMEV_DIGITS_RECEIVED");
				digit_infop = (IPM_DIGIT_INFO*)evt_datap;
				Log("number of digits = %d, digit=%s.", digit_infop->unNumberOfDigits, digit_infop->cDigits);
				break;
			case IPMEV_ERROR:
				Log("IPMEV_ERROR");
				break;
			default:
				Log("unknown event(%d) for ipm device(%d).", evt_code, evt_dev);
				break;
			}
		} else
			Log("event(%d) for unknown device(%d).", evt_code, evt_dev);
	}
};

int main() {
	LINE* lp = NULL;
	long timeout = 0;
	long evt_dev = 0;
	long evt_code = 0;
	void* evt_datap = NULL;
	long evt_len = 0;
	char input = 'a';

	line_0 = new LINE(0, "dxxxB1C1", "mmB1C1", "ipmB1C1");
	line_1 = new LINE(1, "dxxxB1C2", "mmB1C2", "ipmB1C2");

	line_0->open();
	line_1->open();

	while (TRUE) {
		do {			
			timeout = sr_waitevt( 500 );
			if (_kbhit()) {
				input = _getche();
				if (input == 'q') {
					Log("quit...");
					goto QUIT;
				} else if (input == 's'){
					line_0->stop_play();
				}
			}
		} while(timeout == SR_TMOUT);

		evt_dev = (long)sr_getevtdev();
		evt_code = (long)sr_getevttype();
		evt_datap = (void*)sr_getevtdatap();
		evt_len = (long)sr_getevtlen();

		sr_getparm(evt_dev, SR_USERCONTEXT, (void *)&lp);
		lp->Handle(evt_dev, evt_code, (DX_CST*)evt_datap, evt_len);
	}

QUIT:
	line_0->close();
	line_1->close();

	delete line_0;
	delete line_1;

	return 0;
}