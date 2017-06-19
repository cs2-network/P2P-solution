#ifndef _H264IPHONE_H_
#define _H264IPHONE_H_

#define OS_IPHONE

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

    int  InitCodec(char bInOneFrameOnce);
    void UninitCodec();
    int  H264Decode(uint8_t *out, uint8_t *inData, int inDataLen, int *framePara);
    
#ifdef __cplusplus
}
#endif //__cplusplus

#endif