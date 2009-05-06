#ifndef G729_INTERFACE_H
#define G729_INTERFACE_H

#ifdef UNIX

	#define CODEC_API extern "C"
	#define __stdcall

#else

	#ifdef CODEC_EXPORTS
	#define CODEC_API extern "C" __declspec(dllexport)
	#else
	#define CODEC_API extern "C" __declspec(dllimport)
	#endif

#endif

#define L_G729A_FRAME_COMPRESSED 10
#define L_G729A_FRAME            80
typedef void* CODER_HANDLE;

CODEC_API CODER_HANDLE g729_init_decoder();
CODEC_API bool g729_decode(CODER_HANDLE hDecoder, unsigned char *bitstream, short *synth_short);
CODEC_API bool g729_release_decoder(CODER_HANDLE hDecoder);

#endif
