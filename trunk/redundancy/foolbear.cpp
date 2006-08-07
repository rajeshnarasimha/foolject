#include "foolbear.h"

#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

int fool_ulaw2linear(unsigned char ulawbyte)
{
    static int exp_lut[8] = {0,132,396,924,1980,4092,8316,16764};
    int sign, exponent, mantissa, sample;
	
    ulawbyte = ~ulawbyte;
    sign = (ulawbyte & 0x80);
    exponent = (ulawbyte >> 4) & 0x07;
    mantissa = ulawbyte & 0x0F;
    sample = exp_lut[exponent] + (mantissa << (exponent + 3));
    if ( sign != 0 ) sample = -sample;
	
    return(sample);
}

int fool_getwavefileformat(const char* file, unsigned short* data_encoding, 
						   unsigned long* samples_per_sec, unsigned short* bits_per_sample)
{
    struct WAVE_FORMAT
    {
        WORD    wFormatTag;        /* format type */
        WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
        DWORD   nSamplesPerSec;    /* sample rate */
        DWORD   nAvgBytesPerSec;   /* for buffer estimation */
        WORD    nBlockAlign;       /* block size of data */
        WORD    wBitsPerSample;

        // the last 2 fields are not present in PCM format
        WORD    cbSize;
        BYTE    extra[80];         /* Possible extra data */
    } waveinfo = {0};

    /* Beginninng of any WAVE file */
    struct WAVE_FILE_HEADER
    {
        DWORD	riff ;             /* Must contain 'RIFF' */
        DWORD   riffsize ;         /* size of rest of RIFF chunk (entire file?) */
        DWORD   wave;              /* Must contain 'WAVE' */
    } filehdr;

	struct CHUNK_HEADER
	{
        DWORD   id ;          /* 4-char chunk id */
        DWORD   size ;        /* chunk size */
    } chunkhdr ;

    FILE *fp = fopen(file, "rb");
    if(fp == NULL) return -1;

    /* Expect the following structure at the beginning of the file
       Position         contents
       --------         --------
       0-3              'RIFF'
       4-7              size of the data that follows
       8-11             'WAVE'

       followed by a sequence of "chunks" containing:
       n-n+3             4 char chunk id
       n+4 - n+7         size of data
       n+8 - n+8+size    data

       A 'fmt' chunk is manadatory

       0-3             'fmt '
       4-7              size of the format block (16)
       8-23             wave header - see typedef above
       24 ..            option data for non-PCM formats

       A subsequent 'data' chunk is also mandatory

       0-3              'data'
       4-7               size of the data that follows
       8-size+8          data
     */

    if(fread (&filehdr, 1, sizeof(filehdr), fp) != sizeof(filehdr)
        || strncmp ((char *)&filehdr.riff, "RIFF", 4) != 0
        || strncmp ((char *)&filehdr.wave, "WAVE", 4) != 0 )
    {
        fclose(fp);
        return -1;
    }
    
	BOOL err = FALSE;
	for (;;)
    {
        if (fread (&chunkhdr, 1, sizeof(chunkhdr), fp) != sizeof(chunkhdr)) 
		{
			err = TRUE;
			break;
		}
        if (strncmp ((char *)&chunkhdr.id, "fmt ", 4) == 0) break ;
        else
        {
            long size = ((chunkhdr.size +1) & ~1) ;   /* round up to even */
            if (fseek(fp, size, SEEK_CUR ) !=0)
			{
				err = TRUE;
                break;
			}
        }
    }
	if(err)
	{
		fclose(fp);
		return -2;
	}

	size_t fmtwords = (size_t)((chunkhdr.size+1)/2);
	if (chunkhdr.size > sizeof(waveinfo) || fread(&waveinfo, 2, fmtwords, fp) != fmtwords)
    {
		fclose(fp);
		return -3;
	}
	fclose(fp);

	*data_encoding = waveinfo.wFormatTag;
	*samples_per_sec = waveinfo.nSamplesPerSec;
	*bits_per_sample = waveinfo.wBitsPerSample;

	return 0;
}

FOOL_BOOL fool_istiffformatfile(const char* file)
{
    /* Beginninng of any TIFF file */
    struct TIFF_FILE_HEADER
    {
        BYTE byte1;
		BYTE byte2;
		BYTE byte3;
		BYTE byte4;
		unsigned offset;
    } filehdr;
	
    FILE *fp = fopen(file, "rb");
    if(fp == NULL) return FOOL_FALSE;
	
    if(fread (&filehdr, 1, sizeof(filehdr), fp) == sizeof(filehdr) &&
		(0x49 == filehdr.byte1 || 0x4D == filehdr.byte1) &&
		(0x49 == filehdr.byte2 || 0x4D == filehdr.byte2) &&
		0x2A == filehdr.byte3 &&
		0x00 == filehdr.byte4 &&
		filehdr.offset >= 8) {
        fclose(fp);
        return FOOL_TRUE;
    }
	fclose(fp);
	return FOOL_FALSE;
}

int fool_beginthread(FoolThreadFunc func, void *arg, FOOL_THREAD_T* ptid)
{
    FOOL_THREAD_T tid;
#ifdef WIN32
    unsigned long thd = _beginthreadex(NULL, 4096, func, arg, 0, (ptid==NULL)?&tid:ptid);
    if(0 == thd) return -1;
    CloseHandle((HANDLE)thd); //detach
#else
	if(0 != pthread_create((ptid==NULL)?&tid:ptid, NULL, func, arg)) return -1;
    pthread_detach(ptid == NULL ? tid : *ptid);
#endif
    return 0;
}

void fool_wait(int msec)
{
#ifdef WIN32
	Sleep(msec);
#else
	struct timeval timeout;
	if (msec>1000) {
		timeout.tv_sec = msec/1000;
		timeout.tv_usec = msec%1000 * 1000;
	}
	else {
		timeout.tv_sec = 0;
		timeout.tv_usec = msec * 1000;
	}
	select(0, NULL, NULL, NULL, &timeout);
#endif
}

//define global value "fool_time_base"
struct FOOL_TIME_BASE{
#ifndef WIN32
    struct timeval tv;		//from 0:0:0 01/01/1970, second+microsencond
#else
    unsigned long tick;		//from windows begin running, millisecond
#endif
    FOOL_TIME_BASE() {
#ifndef WIN32
        gettimeofday(&tv, NULL);
#else
        tick = GetTickCount();
#endif
    }
}fool_time_base;

//get elapsed time(in a ms unit) from beginning
unsigned long fool_getms()
{
#ifndef WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec-fool_time_base.tv.tv_sec)*1000+(tv.tv_usec-fool_time_base.tv.tv_usec)/1000;
#else
    unsigned long tick = GetTickCount();
    return tick - fool_time_base.tick;
#endif
}

char *fool_gettimestring(char* buf)
{
	if (NULL == buf) return NULL;
	int size = strlen(buf);
#ifndef WIN32
	struct tm now_t;
	struct timeval cur_t;
	gettimeofday(&cur_t, NULL);
	//now_t = localtime(&cur_t.tv_sec);	
	localtime_r(&cur_t.tv_sec, &now_t);//NOTE:_REENTRANT in makefile
	_snprintf(buf, size-1, "%04d/%02d/%02d %02d:%02d:%02d.%06d",
		now->tm_year+1900, now->tm_mon+1, now->tm_mday, 
		now_t.tm_hour, now_t.tm_min, now_t.tm_sec, cur_t.tv_usec);
#else
	SYSTEMTIME now;
	GetLocalTime(&now);
	_snprintf(buf, size-1, "%04d/%02d/%02d %02d:%02d:%02d.%03d",
		now.wYear, now.wMonth, now.wDay, 
		now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
#endif
	return buf;
}

char *fool_strupr(char *str)
{
#ifndef WIN32
    char *p = str;
    while(*p) {
        if(*p>='a' && *p<='z') *p = *p-'a'+'A';
        p++;
    }
    return str;
#else
	return strupr(str);
#endif
}

char *fool_strlwr(char *str)
{	
#ifndef WIN32
    char *p = str;
    while(*p) {
        if(*p>='A' && *p<='Z') *p = *p-'A'+'a';
        p++;
    }
    return str;
#else
	return strlwr(str);
#endif
}

char *fool_strltrim(char *str)
{
    unsigned char *p = (unsigned char*)str; // for chinese text
    while(*p && *p <= ' ') p++;
    memmove(str,p,strlen((char*)p)+1);
    return str;
}

char *fool_strrtrim(char *str)
{
    unsigned char *p = (unsigned char*)str+strlen(str)-1;
    while((char*)p != (str-1) && *p <= ' ') p--;
    *(p+1) = '\0';
    return str;
}

char *fool_strtrim(char *str)
{
    return fool_strltrim(str), fool_strrtrim(str);
}

char *fool_substr(char *sub, const char *str, int count)
{
    int i;
    for(i=0; str[i] && i<count; i++) sub[i]=str[i];
    sub[i] = '\0';
    return sub;
}

char *fool_replacechar(const char* in, char* out, char oldchar, char newchar)
{	
	int len = 0;
	if (NULL == in) sprintf(out, "NULL");
	else {
		len = strlen(in);		
		for (int i=0; i<len; i++) {
			if (oldchar == in[i]) out[i] = newchar;
			else out[i] = in[i];
		}
		out[len] = '\0';
	}
	return out;
}

char *fool_get_element_string(char *value, const char *key, const char *cbuf, char delimiter)
{
    char *buf = (char*)cbuf;
	
    if(NULL==buf) return NULL;
    while(*buf && *buf==delimiter) buf++;
    if(0==*buf) return NULL;
	
    char *ep, *dp, ch, NAME[128], KEY[128];
    strcpy(KEY,key); strupr(KEY);
    while(*buf)
    {
        ep=strchr(buf,'=');
        if(ep)
        {
            for(dp=ep+1; *dp && *dp!=delimiter; dp++);
            *ep='\0'; strcpy(NAME,buf); *ep='=';
            fool_strtrim(NAME); strupr(NAME);
			if(0==strcmp(KEY,NAME))
			{
                ch=*dp; *dp='\0';
                strcpy(value,ep+1); *dp=ch;
                fool_strtrim(value);
                return value;
			} else buf = *dp?dp+1:dp;
        } else { *value='\0'; return NULL; }
    }
	
    *value='\0';
    return NULL;
}

int fool_get_element_int(int *value, const char *key, const char *cbuf, char delimiter)
{
    char tmp[64];
    return NULL==fool_get_element_string(tmp,key,cbuf,delimiter)?(*value=0,-1):(*value=atoi(tmp),0);
}

FOOL_BOOL fool_extract_path(const char* fname, char* path)
{
    if(fname == NULL || path == NULL) return FOOL_FALSE;
    const char *p = fname+strlen(fname);
    while(p != fname && *p != '\\') p--;
    if(*p != '\\') return FOOL_FALSE;
    fool_substr(path, fname, (int)(p-fname));
    return FOOL_TRUE;
}

FOOL_BOOL fool_make_path(const char* path)
{
    if(NULL == path || 0 == path[0] || 0 == access(path,0)) return FOOL_TRUE;
    char ppath[256];
    if(FOOL_FALSE == fool_extract_path(path, ppath)) return (0 == _mkdir(path)?FOOL_TRUE:FOOL_FALSE);
    else return fool_make_path(ppath) ? (0 == _mkdir(path)?FOOL_TRUE:FOOL_FALSE) : FOOL_FALSE;
}