/********************************************************************
	created:	2003/08/26
	created:	26:8:2003   14:20
	filename: 	foolbear.h
	file path:	-
	file base:	foolbear
	file ext:	h
	author:		FOOLBEAR
	
	purpose:	define common convenience function, structure and so on
*********************************************************************/

#ifndef __FOOLBEAR_H__
#define __FOOLBEAR_H__

#include <stdio.h>
#include <io.h>
#include <direct.h>

#ifdef WIN32
	#define FOOL_THREAD_T unsigned
	#define FOOL_STDCALL __stdcall
	#define FOOL_THREAD_RETURN_TYPE unsigned
	#define FOOL_THREAD_RETURN return(0)
#else 
	#define FOOL_THREAD_T pthread_t
	#define FOOL_STDCALL
	#define FOOL_THREAD_RETURN_TYPE void* 
	#define FOOL_THREAD_RETURN return(NULL)
#endif

#define FOOL_FOR_EVER	1
#define FOOL_MAX_PATH	260

typedef enum {
	FOOL_FALSE = 0,
	FOOL_TRUE
}FOOL_BOOL;

#define _FOOL_PRINT_CASE_(x) case x: sprintf(buf, "%s", #x); break
#define _FOOL_PRINT_BIT_(x) if (x == (value & x)) sprintf(buf, "%s(%s)", buf, #x)

typedef FOOL_THREAD_RETURN_TYPE (FOOL_STDCALL *FoolThreadFunc)(void *);

int fool_ulaw2linear(unsigned char ulawbyte);
int fool_getwavefileformat(const char* file, unsigned short* data_encoding, 
		unsigned long* samples_per_sec, unsigned short* bits_per_sample);
FOOL_BOOL fool_istiffformatfile(const char* file);

int fool_beginthread(FoolThreadFunc func, void *arg, FOOL_THREAD_T* ptid=NULL);

void fool_wait(int msec);
unsigned long fool_getms();
char *fool_gettimestring(char* buf); 

char *fool_strupr(char *str);
char *fool_strlwr(char *str);
char *fool_strltrim(char *str);
char *fool_strrtrim(char *str);
char *fool_strtrim(char *str);
char *fool_substr(char *sub, const char *str, int count);
char *fool_replacechar(const char* in, char* out, char oldchar, char newchar);

char *fool_get_element_string(char *value, const char *key, const char *cbuf, char delimiter);
int fool_get_element_int(int *value, const char *key, const char *cbuf, char delimiter);

FOOL_BOOL fool_extract_path(const char* fname, char* path);
FOOL_BOOL fool_make_path(const char* path);

#endif