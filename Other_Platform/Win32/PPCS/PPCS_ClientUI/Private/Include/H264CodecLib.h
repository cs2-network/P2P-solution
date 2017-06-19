
#ifndef _H264CODEC_LIB_
#define _H264CODEC_LIB_

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

long GetApiVer();
int  InitCodec(char bInOneFrameOnce);
void UninitCodec();
int  H264Decode(unsigned char* out_bmp24, unsigned char* in_pData, int nDataSize, int* in_out_pPara, char in_bFlip);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif