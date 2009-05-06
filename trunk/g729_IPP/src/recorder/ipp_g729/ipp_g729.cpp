#include "g729_intf.h"
#include "owng729.h"
#include "g729api.h"

CODEC_API CODER_HANDLE g729_init_decoder()
{
	Ipp32s CodecSize;
	apiG729Decoder_Alloc(G729A_CODEC, &CodecSize);
	G729Decoder_Obj* decoderObj = (G729Decoder_Obj*)ippsMalloc_8u(CodecSize);
	Ipp8s *buff = ippsMalloc_8s(G729_ENCODER_SCRATCH_MEMORY_SIZE);
	apiG729Decoder_InitBuff(decoderObj, buff);
	apiG729Decoder_Init(decoderObj, G729A_CODEC);
	return decoderObj;
}

CODEC_API bool g729_decode(CODER_HANDLE hDecoder, unsigned char *bitstream, short *synth_short)
{
	G729Decoder_Obj* decoderObj = (G729Decoder_Obj*)hDecoder;
	return apiG729Decode(decoderObj, bitstream, G729_Voice_Frame, synth_short) == APIG729_StsNoErr;
}

CODEC_API bool g729_release_decoder(CODER_HANDLE hDecoder)
{
	G729Decoder_Obj* decoderObj = (G729Decoder_Obj*)hDecoder;
	ippsFree(decoderObj->Mem.base);
	ippsFree(decoderObj);
	return true;
}
