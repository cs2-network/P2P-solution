#ifdef LINUX
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h> 
#include <sys/types.h>
#include <sys/time.h> 
#include <signal.h> 
#include <netinet/in.h>
#include <netdb.h> 
#include <net/if.h>
#include <string.h>
#include <sched.h>
#include <stdarg.h>
#include <dirent.h>
#include <arpa/inet.h>  // inet_ntoa
#endif
#ifdef WIN32DLL
#include <windows.h>
#include <stdio.h>
#include <direct.h>
#endif

#ifdef WIN32DLL
#include "../../../../Include/PPCS/PPCS_API.h"
#include "../../../../Include/AVSTREAM_IO_Proto.h"
#else
#include "../../../Include/PPCS/PPCS_API.h"
#include "../../../Include/AVSTREAM_IO_Proto.h"
#endif

//#define CONNECT_TEST  		// 连接测试
//#define RW_TEST				// 读写测试
//#define FT_TEST				// 文件传输测试
//#define RW_TEST1				// 读写测试1
#define AVSTREAM_TEST			// AV视频测试 即 蝴蝶飞舞测试
//#define MULTI_CHANNEL_TEST	// 多通道测试
//#define RW_TEST2				// 读写测试2
//#define RW_TEST3				// 读写测试3
//#define PktSR_TEST			// 即时传输测试（丢包不重传，不保证送达）

//// This InitString is CS2 PPCS InitString, you must Todo: Modify this for your own InitString 
//// 用户需改为自己平台的 InitString
const char *g_DefaultInitString = "EBGAEIBIKHJJGFJKEOGCFAEPHPMAHONDGJFPBKCPAJJMLFKBDBAGCJPBGOLKIKLKAJMJKFDOOFMOBECEJIMM";

#ifdef WIN32DLL
UINT32 MyGetTickCount() 
{
	return GetTickCount();
}
#endif
#ifdef LINUX
CHAR bFlagGetTickCount= 0;
struct timeval gTime_Begin;
UINT32 MyGetTickCount()
{
	if (!bFlagGetTickCount)
	{
		bFlagGetTickCount = 1;
		gettimeofday(&gTime_Begin, NULL);
		return 0;
	}
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//printf("%d %d %d %d\n",tv.tv_sec , gTime_Begin.tv_sec, tv.tv_usec ,gTime_Begin.tv_usec);
	return (tv.tv_sec - gTime_Begin.tv_sec)*1000 + (tv.tv_usec - gTime_Begin.tv_usec)/1000;
}
#endif

void mSecSleep(UINT32 ms)
{
#ifdef WIN32DLL
	Sleep(ms);
#endif //// WIN32DLL
#ifdef LINUX
	usleep(ms * 1000);
#endif //// LINUX
}

// 获取错误信息
const char *getP2PErrorCodeInfo(int err)
{
    if (0 < err)
        return "NoError";
    switch (err)
    {
        case 0: return "ERROR_P2P_SUCCESSFUL";
        case -1: return "ERROR_P2P_NOT_INITIALIZED";
        case -2: return "ERROR_P2P_ALREADY_INITIALIZED";
        case -3: return "ERROR_P2P_TIME_OUT";
        case -4: return "ERROR_P2P_INVALID_ID";
        case -5: return "ERROR_P2P_INVALID_PARAMETER";
        case -6: return "ERROR_P2P_DEVICE_NOT_ONLINE";
        case -7: return "ERROR_P2P_FAIL_TO_RESOLVE_NAME";
        case -8: return "ERROR_P2P_INVALID_PREFIX";
        case -9: return "ERROR_P2P_ID_OUT_OF_DATE";
        case -10: return "ERROR_P2P_NO_RELAY_SERVER_AVAILABLE";
        case -11: return "ERROR_P2P_INVALID_SESSION_HANDLE";
        case -12: return "ERROR_P2P_SESSION_CLOSED_REMOTE";
        case -13: return "ERROR_P2P_SESSION_CLOSED_TIMEOUT";
        case -14: return "ERROR_P2P_SESSION_CLOSED_CALLED";
        case -15: return "ERROR_P2P_REMOTE_SITE_BUFFER_FULL";
        case -16: return "ERROR_P2P_USER_LISTEN_BREAK";
        case -17: return "ERROR_P2P_MAX_SESSION";
        case -18: return "ERROR_P2P_UDP_PORT_BIND_FAILED";
        case -19: return "ERROR_P2P_USER_CONNECT_BREAK";
        case -20: return "ERROR_P2P_SESSION_CLOSED_INSUFFICIENT_MEMORY";
        case -21: return "ERROR_P2P_INVALID_APILICENSE";
        case -22: return "ERROR_P2P_FAIL_TO_CREATE_THREAD";
        default:
            return "Unknow, something is wrong!";
    }
}

#ifdef AVSTREAM_TEST
#define MAX_SIZE_BUF	64*1024
#define CHANNEL_DATA	1
#define CHANNEL_IOCTRL	2
#define TIME_SPAN_IDLE	20 //in ms
#define MAX_USER		8

typedef struct {
	CHAR bUsed;
	CHAR bVideoRequested;
	CHAR bAudioRequested;
	CHAR bVideoGo;
	INT32 SessionHandle;
} st_User;
st_User gUser[MAX_USER];
INT32	gRunning = 1;

void myReleaseUser(st_User *pstUser)
{
	if (!pstUser) 
		return;
	if (pstUser->SessionHandle >= 0)
	{
		printf("PPCS_Close(%d)\n", pstUser->SessionHandle);
		PPCS_Close(pstUser->SessionHandle);
	}		
		
	memset(pstUser, 0, sizeof(st_User));
}

#if defined(WIN32DLL)
#define mSecSleep(ms)	Sleep(ms)
	typedef	 DWORD			PPCS_threadid;
#elif defined(LINUX)
#define mSecSleep(ms)	usleep(ms * 1000)
	typedef pthread_t		PPCS_threadid;
#endif

ULONG myGetTickCount()
{
#if defined(WIN32DLL)
	return GetTickCount();
#elif defined(LINUX)
	struct timeval current;
	gettimeofday(&current, NULL);
	return current.tv_sec*1000 + current.tv_usec/1000;
#endif
}

INT32 myGetDataSizeFrom(st_AVStreamIOHead *pStreamIOHead)
{
	INT32 nDataSize=pStreamIOHead->nStreamIOHead;
	nDataSize &=0x00FFFFFF;
	return nDataSize;
}

void myDoIOCtrl(INT32 iIndex, CHAR *pData)
{
	st_AVIOCtrlHead stIOCtrlHead;
	memcpy(&stIOCtrlHead, pData, sizeof(st_AVIOCtrlHead));
	switch (stIOCtrlHead.nIOCtrlType)
	{
	case IOCTRL_TYPE_VIDEO_START:
		gUser[iIndex].bVideoRequested=1;
		printf("myDoIOCtrl(..): %d, IOCTRL_TYPE_VIDEO_START\n", iIndex);
		break;

	case IOCTRL_TYPE_VIDEO_STOP:
		gUser[iIndex].bVideoRequested=0;
		printf("myDoIOCtrl(..): %d, IOCTRL_TYPE_VIDEO_STOP\n", iIndex);
		break;

	case IOCTRL_TYPE_AUDIO_START:
		gUser[iIndex].bAudioRequested=1;
		printf("myDoIOCtrl(..): %d, IOCTRL_TYPE_AUDIO_START\n", iIndex);
		break;

	case IOCTRL_TYPE_AUDIO_STOP:
		gUser[iIndex].bAudioRequested=0;
		printf("myDoIOCtrl(..): %d, IOCTRL_TYPE_AUDIO_STOP\n", iIndex);
		break;
	default:;
	}
}


#ifdef WIN32DLL
DWORD WINAPI myThreadRecvIOCtrl(void* arg)
#endif
#ifdef LINUX
void *myThreadRecvIOCtrl(void *arg)
#endif
{
	INT32 i = (INT32)arg, nRet = -1;
	CHAR  buf[1024];
	INT32 nRecvSize = 4;
	st_AVStreamIOHead *pStreamIOHead = NULL;
	
	do{
		nRecvSize=sizeof(st_AVStreamIOHead);
		nRet=PPCS_Read(gUser[i].SessionHandle, CHANNEL_IOCTRL,buf, &nRecvSize, 0xFFFFFFFF);
		if(!(nRet== ERROR_PPCS_TIME_OUT || nRet==ERROR_PPCS_SUCCESSFUL)){
			myReleaseUser(&gUser[i]);
			printf("myThreadRecvIOCtrl: failed %d !!\n", nRet);
			break;
		}

		if(nRecvSize>0){
			pStreamIOHead=(st_AVStreamIOHead *)buf;
			nRecvSize=myGetDataSizeFrom(pStreamIOHead);
			nRet=PPCS_Read(gUser[i].SessionHandle, CHANNEL_IOCTRL,buf, &nRecvSize, 0xFFFFFFFF);
			if(!(nRet == ERROR_PPCS_TIME_OUT || nRet==ERROR_PPCS_SUCCESSFUL)){
				myReleaseUser(&gUser[i]);
				printf("myThreadRecvIOCtrl: failed %d !!\n", nRet);
				break;
			}
			if(nRecvSize>0) myDoIOCtrl(i, buf);
		}
	}while(gRunning);

	printf("---%d, myThreadRecvIOCtrl exit!!\n", i);

#ifdef WIN32DLL
	return 0L;
#endif
#ifdef LINUX
	pthread_exit(0);
#endif
}

void AudioFrameReady(CHAR *Buf, INT32 nSize, st_AVFrameHead *pFreamHead)
{
	INT32  UserCount = 0, nRet=0, wsize=0;

	for(INT32 i = 0 ; i < MAX_USER; i++) { if(gUser[i].bUsed) UserCount++;}
	pFreamHead->nOnlineNum=UserCount;

	for(INT32 i = 0 ; i < MAX_USER; i++)
	{
		if(!gUser[i].bUsed || !gUser[i].bAudioRequested) continue;

		nRet=PPCS_Check_Buffer(gUser[i].SessionHandle, 1, (UINT32 *)&wsize, NULL);
		if(nRet==ERROR_PPCS_SESSION_CLOSED_TIMEOUT){
			myReleaseUser(&gUser[i]);				
			printf("AudioFrameReady: %d, Session TimeOUT!!\n", i);

		}else if(nRet==ERROR_PPCS_SESSION_CLOSED_REMOTE){
			myReleaseUser(&gUser[i]);				
			printf("AudioFrameReady: %d, Session Remote Close!!\n", i);

		}else if(nRet==ERROR_PPCS_INVALID_SESSION_HANDLE){
			myReleaseUser(&gUser[i]);				
			printf("AudioFrameReady: %d, invalid session handle!!\n", i);
		}
		if(wsize> 65535) continue; //64*1024
		nRet=PPCS_Write(gUser[i].SessionHandle, CHANNEL_DATA, Buf, nSize);
	}
}

void VideoFrameReady(CHAR *Buf, INT32 nSize, st_AVFrameHead *pFreamHead)
{
	INT32 UserCount = 0, nRet=0, wsize=0;
	for(INT32 i = 0 ; i < MAX_USER; i++) { if(gUser[i].bUsed) UserCount++;}
	pFreamHead->nOnlineNum=UserCount;

	for(INT32 i = 0 ; i < MAX_USER; i++)
	{
		if(!gUser[i].bUsed || !gUser[i].bVideoRequested) continue;

		if(gUser[i].bVideoGo == 0) continue;

		nRet=PPCS_Check_Buffer(gUser[i].SessionHandle, 1,(UINT32 *)&wsize, NULL);
		if(nRet==ERROR_PPCS_SESSION_CLOSED_TIMEOUT){
			myReleaseUser(&gUser[i]);
			printf("VideoFrameReady: %d, Session TimeOUT!!\n", i);

		}else if(nRet==ERROR_PPCS_SESSION_CLOSED_REMOTE){
			myReleaseUser(&gUser[i]);				
			printf("VideoFrameReady: %d, Session Remote Close!!\n", i);

		}else if(nRet==ERROR_PPCS_INVALID_SESSION_HANDLE){
			myReleaseUser(&gUser[i]);				
			printf("VideoFrameReady: %d, invalid session handle!!\n", i);
		}
		if(wsize> 65535)
		{
			gUser[i].bVideoGo = 0;
			continue; //64*1024
		}
		nRet=PPCS_Write(gUser[i].SessionHandle, CHANNEL_DATA, Buf, nSize);
	}
}

int h264_GetOneFrameFromFile(FILE* fp, CHAR* pBufArg)
{
	int  nPos1=0, nPos2=0;
	bool bEOF=false;
	CHAR buf2[8]={0};

	while(1){ //find 1st start code
		if(feof(fp)) { 
			bEOF=true;
			break;
		}
		pBufArg[nPos1]=fgetc(fp);
		if(pBufArg[nPos1]==0) nPos1++;
		else nPos1=0;
		if(nPos1==3){
			if(feof(fp)) { 
				bEOF=true;
				break;
			}
			pBufArg[nPos1]=fgetc(fp);
			if(pBufArg[nPos1]==1) {
				nPos1++;
				break;
			}else nPos1=0;
		}
	}
	if (bEOF && nPos1<4) 
		return -1;

	while (1)
	{ //find 2nd start code
		if (feof(fp)) 
		{
			bEOF = true;
			break;
		}
		buf2[nPos2] = fgetc(fp);
		pBufArg[nPos1++] = buf2[nPos2]; //important
		if (buf2[nPos2] == 0) 
			nPos2++;
		else 
			nPos2 = 0;
		if (nPos2 == 3)
		{
			if(feof(fp)) 
			{
				bEOF = true;
				break;
			}
			buf2[nPos2] = fgetc(fp);
			pBufArg[nPos1++] = buf2[nPos2]; //important
			if (buf2[nPos2] == 1) 
			{
				nPos2++;
				break;
			}
			else 
				nPos2=0;
		}
	}

	if (bEOF) 
	{
		fseek(fp, 0, SEEK_SET);
		//printf("  h264_GetOneFrameFromFile(..):reach the tail of file\n");
	}
	else 
		fseek(fp, -4, SEEK_CUR);

	return (nPos1-4);
}

//@return	>=0 success
//			<0  fail
//				-1: non-h264 file
//
INT32 myGetOneFrame_video(FILE *fp, CHAR *out_pBuf, INT32 nBufMaxSize, CHAR *out_pchFrameType)
{
	CHAR  *bufTmp = (CHAR *)malloc(MAX_SIZE_BUF);
	INT32 nSize = 0, nTotalSize = 0;
	bool  bIFrame = false;

	memset(bufTmp, 0, MAX_SIZE_BUF);
	do{
		nSize = h264_GetOneFrameFromFile(fp, bufTmp);
		if (nSize <= 0) 
		{
			nTotalSize=nSize; //-1, error code
			break; //break----------------
		}
		else
		{
			if (out_pBuf) 
				memcpy(out_pBuf+nTotalSize, bufTmp, nSize);
			nTotalSize += nSize;

			if ((bufTmp[4]&0x07) == 0x07) 
			{
				bIFrame = true;

				//read 2nd time
				nSize = h264_GetOneFrameFromFile(fp, bufTmp);
				if (nSize <= 0)
				{
					nTotalSize = nSize; //-1, error code
					break; //break----------------
				}
				else
				{
					if (out_pBuf) 
						memcpy(out_pBuf+nTotalSize, bufTmp, nSize);
					nTotalSize += nSize;
				}

				//read 3rd time
				nSize = h264_GetOneFrameFromFile(fp, bufTmp);
				if(nSize <= 0)
				{
					nTotalSize=nSize; //-1, error code
					break; //break----------------
				}
				else
				{
					if(out_pBuf)
						memcpy(out_pBuf+nTotalSize, bufTmp, nSize);
					nTotalSize += nSize;
				}
			}
			else  
				bIFrame = false;			
		}

		if (out_pchFrameType != NULL) 
		{
			if(bIFrame) 
				*out_pchFrameType=VFRAME_FLAG_I;
			else 
				*out_pchFrameType=VFRAME_FLAG_P;
		}
	} while(0);

	free(bufTmp);
	return nTotalSize;
}

//=={{audio: ADPCM codec==============================================================
INT32 g_nAudioPreSample=0;
INT32 g_nAudioIndex=0;

static int gs_index_adjust[8]= {-1,-1,-1,-1,2,4,6,8};
static int gs_step_table[89] = 
{
	7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,
	50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,
	408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,
	2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
	10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767
};

void Encode(unsigned char *pRaw, int nLenRaw, unsigned char *pBufEncoded)
{
	short *pcm = (short *)pRaw;
	int cur_sample;
	int i;
	int delta;
	int sb;
	int code;
	nLenRaw >>= 1;

	for (i = 0; i < nLenRaw; i++)
	{
		cur_sample = pcm[i]; 
		delta = cur_sample - g_nAudioPreSample;
		if (delta < 0)
		{
			delta = -delta;
			sb = 8;
		}
		else 
			sb = 0;

		code = 4 * delta / gs_step_table[g_nAudioIndex];	
		if (code>7)	
			code=7;

		delta = (gs_step_table[g_nAudioIndex] * code) / 4 + gs_step_table[g_nAudioIndex] / 8;
		if (sb) 
			delta = -delta;

		g_nAudioPreSample += delta;
		if (g_nAudioPreSample > 32767) 
			g_nAudioPreSample = 32767;
		else if (g_nAudioPreSample < -32768) 
			g_nAudioPreSample = -32768;

		g_nAudioIndex += gs_index_adjust[code];
		if (g_nAudioIndex < 0) 
			g_nAudioIndex = 0;
		else if (g_nAudioIndex > 88) 
			g_nAudioIndex = 88;

		if (i & 0x01) 
			pBufEncoded[i>>1] |= code | sb;
		else 
			pBufEncoded[i>>1] = (code | sb) << 4;
	}
}

void Decode(char *pDataCompressed, int nLenData, char *pDecoded)
{
	int i;
	int code;
	int sb;
	int delta;
	short *pcm = (short *)pDecoded;
	nLenData <<= 1;

	for (i = 0; i < nLenData; i++)
	{
		if (i & 0x01) 
			code = pDataCompressed[i>>1] & 0x0f;
		else 
			code = pDataCompressed[i>>1] >> 4;

		if ((code & 8) != 0) 
			sb = 1;
		else 
			sb = 0;
		code &= 7;

		delta = (gs_step_table[g_nAudioIndex] * code) / 4 + gs_step_table[g_nAudioIndex] / 8;
		if (sb) 
			delta = -delta;

		g_nAudioPreSample += delta;
		if (g_nAudioPreSample > 32767) 
			g_nAudioPreSample = 32767;
		else if (g_nAudioPreSample < -32768) 
			g_nAudioPreSample = -32768;

		pcm[i] = g_nAudioPreSample;
		g_nAudioIndex += gs_index_adjust[code];
		if (g_nAudioIndex < 0) 
			g_nAudioIndex = 0;
		if (g_nAudioIndex > 88) 
			g_nAudioIndex= 88;
	}
}
//==}}audio: ADPCM codec==============================================================

#define ADPCM_ENCODE_BYTE_SIZE	160
#define ADPCM_DECODE_BYTE_SIZE	640

#define READSIZE_EVERY_TIME		1920
	
UCHAR g_out_pBufTmp[READSIZE_EVERY_TIME];

// get 1920 bytes every time
INT32 myGetAudioData(FILE *fp, CHAR *out_pBuf, INT32 nBufMaxSize, CHAR *out_pchFrameType)
{
	INT32 nReadSize = 0, nSize = 0;

	nReadSize = fread(g_out_pBufTmp, 1, READSIZE_EVERY_TIME,fp);
	if (nReadSize < READSIZE_EVERY_TIME) 
	{
		fseek(fp, 0, SEEK_SET);
		//printf("  myGetAudioData(..):reach the tail of file\n");

		memset(out_pBuf, 0, nBufMaxSize);
		nSize = READSIZE_EVERY_TIME/ADPCM_DECODE_BYTE_SIZE *ADPCM_ENCODE_BYTE_SIZE;
	}
	else
	{
		UCHAR bufSmall[ADPCM_ENCODE_BYTE_SIZE];
		nSize = 0;
		for (int i = 0; i < nReadSize/ADPCM_DECODE_BYTE_SIZE; i++)
		{
			Encode(g_out_pBufTmp+i*ADPCM_DECODE_BYTE_SIZE, ADPCM_DECODE_BYTE_SIZE, bufSmall);
			memcpy(out_pBuf+nSize, bufSmall, ADPCM_ENCODE_BYTE_SIZE);
			nSize += ADPCM_ENCODE_BYTE_SIZE;
		}
	}

	return nSize;
}

#ifdef WIN32DLL
DWORD WINAPI myThreadGetDataFromFile(void* arg)
#endif
#ifdef LINUX
void *myThreadGetDataFromFile(void *arg)
#endif
{
	CHAR strVideoFile[300] = {0}, strAudioFile[300] = {0};
	CHAR sAppPath[256] = {0};
	int nLenPath = 0;
	if ( !(getcwd(sAppPath, sizeof(sAppPath))) )
	{
		printf("***Error: getcwd failed!!\n\n");
#ifdef WIN32DLL
		return 0;
#else
		pthread_exit(0);
#endif		
	}
	nLenPath = strlen(sAppPath);
	
	// 拼接蝴蝶飞舞视频数据文件路径
#ifdef WIN32DLL
	if (sAppPath[nLenPath-1] != '\\') 
		strcat(sAppPath, "\\");
	strcpy(strVideoFile, sAppPath); 
	strcat(strVideoFile, "..\\..\\..\\AVData\\V.h264");
	strcpy(strAudioFile, sAppPath); 
	strcat(strAudioFile, "..\\..\\..\\AVData\\8K_16_1.pcm");
#endif //// #ifdef WIN32DLL
#ifdef LINUX
	if (sAppPath[nLenPath-1] != '/') 
		strcat(sAppPath, "/");
	strcpy(strVideoFile, sAppPath); 
	strcat(strVideoFile, "../../AVData/V.h264");
	//strcat(strVideoFile, "./AVData/V.h264");
	strcpy(strAudioFile, sAppPath); 
	strcat(strAudioFile, "../../AVData/8K_16_1.pcm");
	//strcat(strAudioFile, "./AVData/8K_16_1.pcm");
#endif //// #ifdef LINUX
	
	INT32 size = 0, nHeadLen = sizeof(st_AVStreamIOHead)+sizeof(st_AVFrameHead);
	CHAR *pBuf = (CHAR *)malloc(MAX_SIZE_BUF);
	CHAR *pBufVideo=&pBuf[nHeadLen];
	CHAR *pBufAudio=pBufVideo;
	UINT32 TheCounter = 0;
	
	st_AVStreamIOHead *pstStreamIOHead = (st_AVStreamIOHead *)pBuf;
	st_AVFrameHead *pstFrameHead =(st_AVFrameHead *)&pBuf[sizeof(st_AVStreamIOHead)];
	pstFrameHead->nOnlineNum = 1;
	ULONG nCurTick = 0L;
	
	FILE *fpH264 = fopen(strVideoFile, "rb");
	FILE *fpAudio= fopen(strAudioFile, "rb");
	do{
		if (fpH264 == NULL || fpAudio == NULL)
		{
			printf("\n***Error: Failed to open audio or video file!!!\n");
			printf("***AudioFile Path: %s\n", strAudioFile);
			printf("***VideoFile Path: %s\n\n", strVideoFile);
			break;
		}
		
		while (gRunning)
		{
			nCurTick=myGetTickCount();
			if (TheCounter % 4 == 0) // for Audio 80 ms
			{
				size=myGetAudioData(fpAudio, pBufAudio, MAX_SIZE_BUF-nHeadLen, (CHAR *)&(pstFrameHead->flag));
				if (size>0) 
				{
					pstFrameHead->nCodecID  =CODECID_A_ADPCM;
					pstFrameHead->nTimeStamp=nCurTick;
					pstFrameHead->nDataSize =size;

					pstStreamIOHead->nStreamIOHead=sizeof(st_AVFrameHead)+size;
					pstStreamIOHead->uionStreamIOHead.nStreamIOType=SIO_TYPE_AUDIO;

					AudioFrameReady(pBuf, size+nHeadLen, pstFrameHead);
				}
			}
			if (TheCounter % 5 == 0) // Video 10 Hz
			{
				size = myGetOneFrame_video(fpH264, pBufVideo, MAX_SIZE_BUF-nHeadLen, (CHAR *)&(pstFrameHead->flag));
				if (size>0) 
				{
					pstFrameHead->nCodecID  =CODECID_V_H264;				
					pstFrameHead->nTimeStamp=nCurTick;
					pstFrameHead->nDataSize =size;

					pstStreamIOHead->nStreamIOHead=sizeof(st_AVFrameHead)+size;
					pstStreamIOHead->uionStreamIOHead.nStreamIOType=SIO_TYPE_VIDEO;
					
					for (INT32 i = 0 ; i < MAX_USER; i++)
					{
						if (!gUser[i].bUsed || !gUser[i].bVideoRequested) 
							continue;
						if (pstFrameHead->flag == 0)
							gUser[i].bVideoGo = 1;
					}	
					VideoFrameReady(pBuf, size+nHeadLen, pstFrameHead);
				}
			}

			mSecSleep(TIME_SPAN_IDLE);
			TheCounter ++;
		}//while-end
	} while(0);

	if (fpH264)  
		fclose(fpH264);
	if (fpAudio) 
		fclose(fpAudio);
	free(pBuf);

#ifdef WIN32DLL
	return 0L;
#endif
#ifdef LINUX
	pthread_exit(0);
#endif
}

#endif //// #ifdef AVSTREAM_TEST


#ifdef RW_TEST3
#define TEST_WRITE_SIZE 16064  // (251 * 64), 251 is a prime number
#define TOTAL_WRITE_SIZE (4*1024*TEST_WRITE_SIZE)
#define TEST_NUMBER_OF_CHANNEL 1

INT32 gTheSessionHandle;
CHAR gFlagWorking = 0;

#ifdef WIN32DLL
DWORD WINAPI ThreadWrite(void* arg)
#endif
#ifdef LINUX
void *ThreadWrite(void *arg)
#endif
{
	INT32 Channel = (INT32) arg;	
	UCHAR *Buffer = (UCHAR *)malloc(TEST_WRITE_SIZE);
	INT32 TotalSize = 0; 
	if (Buffer == NULL) 
	{
		printf("malloc Failed!!\n");
	}
	else
	{
		for (INT32 i = 0 ; i< TEST_WRITE_SIZE; i++)	
			Buffer[i] = i%251;
		printf("ThreadWrite %d running... \n",Channel);
	}
	UINT32 WriteSize;
	while (PPCS_Check_Buffer(gTheSessionHandle, Channel, &WriteSize, NULL) == ERROR_PPCS_SUCCESSFUL)
	{
		if ((WriteSize < 256*1024) && (TotalSize < TOTAL_WRITE_SIZE))
		{
			INT32 n1,n2,n3,n4;
			n1 = rand() % (TEST_WRITE_SIZE/3);
			n2 = rand() % (TEST_WRITE_SIZE/3);
			n3 = rand() % (TEST_WRITE_SIZE/3);
			n4 = TEST_WRITE_SIZE - n1 -n2 -n3;
			PPCS_Write(gTheSessionHandle, Channel, (CHAR*)Buffer, n1);
			PPCS_Write(gTheSessionHandle, Channel, (CHAR*)(Buffer+n1), n2);
			PPCS_Write(gTheSessionHandle, Channel, (CHAR*)(Buffer+n1+n2), n3);
			PPCS_Write(gTheSessionHandle, Channel, (CHAR*)(Buffer+n1+n2+n3), n4);
			TotalSize += TEST_WRITE_SIZE;
		}
		else if (TotalSize >= TOTAL_WRITE_SIZE)
			break;
		else
			mSecSleep(1);
		gFlagWorking = 1;
	}
	free(Buffer);
	printf("\nThreadWrite %d Exit. TotalSize = %d ",Channel,TotalSize);
#ifdef WIN32DLL
	return 0;
#endif
#ifdef LINUX
	pthread_exit(0);
#endif
}

#ifdef WIN32DLL
DWORD WINAPI ThreadRead(void* arg)
#endif
#ifdef LINUX
void *ThreadRead(void *arg)
#endif
{

	INT32 Channel = (INT32) arg;	
	INT32 i = 0;

	UINT32 tick = MyGetTickCount();
	printf("ThreadRead %d running... \n",Channel);
	while (1)
	{
		
		UCHAR zz;
		INT32 ReadSize=1;
		INT32 ret;
		ret = PPCS_Read(gTheSessionHandle, Channel, (CHAR*)&zz, &ReadSize, 100);
		if ((ret < 0) && (ret != ERROR_PPCS_TIME_OUT))
		{
			printf("Channel:%d PPCS_Read ret = %d , i=%d\n", Channel, ret,i);
			break;
		}
		if ((i >= TOTAL_WRITE_SIZE) && (ret == ERROR_PPCS_TIME_OUT))
			break;
		gFlagWorking = 1;
		if (ReadSize == 0)
			continue;
		if ((i%251) != zz)
		{
			printf("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Channel:%d Error!! i=%d zz= %d\n",Channel,  i,zz);
			break;
		}
		else
		{
			if (i%(5*1024*1024) == (5*1024*1024-1)) 
			{
				printf("%d",Channel); 
				fflush(NULL);
			}
		}
		i++;
	}
	tick = MyGetTickCount() - tick - 100;
	printf("\nThreadRead %d Exit - Channel:%d, Time: %d.%d sec, %d KByte/sec",Channel, Channel,tick/1000, tick%1000, i/tick);

#ifdef WIN32DLL
	return 0;
#endif
#ifdef LINUX
	pthread_exit(0);
#endif
}

#endif //// #ifdef RW_TEST3

INT32 main(INT32 argc, CHAR **argv)
{
	if (argc != 3 && argc != 4)
	{
#ifdef WIN32DLL		
		printf("Usage: DID APILicense [InitString]\n");
		printf("\tDID: This is Device DID: ABCD-123456-ABCDEF\n");
		printf("\tAPILicense: If you set the CRCKey, This parameter must be input  APILicense:CRCKey\n");
		printf("\tInitString: The InitString, if this parameter is empty, program default InitString will be used.\n");
		printf("Example:\n");
		printf("\t PPCS_Device ABCD-123456-ABCDEF ABCDEF\n");
		printf("\t PPCS_Device ABCD-123456-ABCDEF ABCDEF %s\n", g_DefaultInitString);
		printf("\t PPCS_Device ABCD-123456-ABCDEF ABCDEF:ABC123\n");
		printf("\t PPCS_Device ABCD-123456-ABCDEF ABCDEF:ABC123 %s\n", g_DefaultInitString);
		printf("\nPlease press any key to exit the main thread...");
		getchar();
#else
		printf("Usage: ./PPCS_Device DID APILicense [InitString]\n");
		printf("\tDID: This is Device DID: ABCD-123456-ABCDEF\n");
		printf("\tAPILicense: If you set the CRCKey, This parameter must be input  APILicense:CRCKey\n");
		printf("\tInitString: The InitString, if this parameter is empty, program default InitString will be used.\n");
		printf("Example:\n");
		printf("\t ./PPCS_Device ABCD-123456-ABCDEF ABCDEF\n");
		printf("\t ./PPCS_Device ABCD-123456-ABCDEF ABCDEF %s\n", g_DefaultInitString);
		printf("\t ./PPCS_Device ABCD-123456-ABCDEF ABCDEF:ABC123\n");
		printf("\t ./PPCS_Device ABCD-123456-ABCDEF ABCDEF:ABC123 %s\n", g_DefaultInitString);
#endif	
		return 0;
	}
	
#ifdef AVSTREAM_TEST
	memset(gUser, 0, sizeof(st_User) * MAX_USER);

	// To Do ... Create a thread to read AV data from file and call FrameReady()
	PPCS_threadid threadIDrecv;
	memset(&threadIDrecv, 0, sizeof(threadIDrecv));

#ifdef WIN32DLL
	void *hThread = NULL;
	hThread = CreateThread(NULL, 0, myThreadGetDataFromFile, (LPVOID)NULL, 0, &threadIDrecv);
	if (NULL != hThread) 
		CloseHandle(hThread);
	else
#else
	if (0 == pthread_create(&threadIDrecv, NULL, &myThreadGetDataFromFile, (void *)NULL)) 
	{
		pthread_detach(threadIDrecv);
	}
	else
#endif
	{
		gRunning=0;
		return 0;
	}
#endif //// #ifdef AVSTREAM_TEST
	
	//1.获取P2P版本信息
	UINT32 APIVersion = PPCS_GetAPIVersion();
	printf("\nPPCS_API Version: %d.%d.%d.%d\n", 
				(APIVersion & 0xFF000000)>>24, 
				(APIVersion & 0x00FF0000)>>16, 
				(APIVersion & 0x0000FF00)>>8, 
				(APIVersion & 0x000000FF) >> 0 );
	
	INT32 ret;
	if (3 == argc) // use default initstring (CS2)
	{
		printf("PPCS_Initialize(%s)...\n", g_DefaultInitString);
		ret = PPCS_Initialize((CHAR*)g_DefaultInitString);
	}
	else if (4 == argc)
	{
		printf("PPCS_Initialize(%s)...\n", argv[3]);
		ret = PPCS_Initialize((CHAR*)argv[3]);
	}
	
	// 2.网络侦测
	st_PPCS_NetInfo NetInfo;
	printf("PPCS_NetworkDetect ...\n");
	ret = PPCS_NetworkDetect(&NetInfo, 10000);
	if (ret < 0)
		printf("PPCS_NetworkDetect() ret = %d\n", ret);
	
	printf("-------------- NetInfo: -------------------\n");
	printf("Internet Reachable     : %s\n", (NetInfo.bFlagInternet == 1) ? "YES":"NO");
	printf("P2P Server IP resolved : %s\n", (NetInfo.bFlagHostResolved == 1) ? "YES":"NO");
	printf("P2P Server Hello Ack   : %s\n", (NetInfo.bFlagServerHello == 1) ? "YES":"NO");
	printf("Local NAT Type         :");
	
	// 3.Open Device Relay(此功能可选)
	printf("PPCS_Share_Bandwidth(1)\n");
	PPCS_Share_Bandwidth(1);
	
	switch (NetInfo.NAT_Type)
	{
	case 0:
		printf(" Unknow\n");
		break;
	case 1:
		printf(" IP-Restricted Cone\n");
		break;
	case 2:
		printf(" Port-Restricted Cone\n");
		break;
	case 3:
		printf(" Symmetric\n");
		break;
	}
	printf("My Wan IP : %s\n", NetInfo.MyWanIP);
	printf("My Lan IP : %s\n", NetInfo.MyLanIP);
	printf("-------------------------------------------\n");
#ifdef CONNECT_TEST
	
	for (UINT16 i = 0; i < 10000; i++)
	{
		printf("CONNECT_TEST: PPCS_Listen(%s, 60, %d, 1, %s)...\n", argv[1], 10000+i, argv[2]);
		// 设备进入监听
		ret = PPCS_Listen(argv[1], 60, 10000+i, 1, argv[2]);
		
		if (ret < 0)
		{
			printf("CONNECT_TEST: PPCS_Listen failed : %d. [%s]\n\n", ret, getP2PErrorCodeInfo(ret));
			mSecSleep(200);
		}
		else
		{
			st_PPCS_Session Sinfo;	
			if (PPCS_Check(ret, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
			{
				printf("-------%d, Session Ready (%d): -%s------------------\n", i,ret,(Sinfo.bMode ==0)? "P2P":"RLY");
				//printf("Socket : %d\n", Sinfo.Skt);
				printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
				//printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
				//printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
				//printf("Connection time : %d second before\n", Sinfo.ConnectTime);
				//printf("DID : %s\n", Sinfo.DID);
				//printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
				//printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
				//printf("------------End of Session info (%d): ---------------\n", i);
#ifdef WIN32DLL
				Sleep(50);
#endif
#ifdef LINUX
				usleep(50000);
#endif
				printf("PPCS_Close(%d)\n", ret);
				PPCS_Close(ret);
			}
		}
	}  // for
#endif //// CONNECT_TEST

#ifdef PktSR_TEST
	INT32 SessionHandle;
	printf("PktSR_TEST: PPCS_Listen(%s, 600, 0, 1, %s)...\n", argv[1], argv[2]);
	SessionHandle = PPCS_Listen(argv[1], 600, 0, 1, argv[2]);
	
	if (SessionHandle < 0)
		printf("PktSR_TEST: PPCS_Listen() ret= %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("PktSR_TEST: P2P Connected!! SessionHandle= %d.\n\n", SessionHandle);
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(SessionHandle, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}
		
		for (INT32 i = 0 ; i < 1000000; i++)
		{
			CHAR PktBuf[1024];
			memset(PktBuf, (UCHAR)(i % 100), sizeof(PktBuf));
			
			ret = PPCS_PktSend(SessionHandle, 0, PktBuf, sizeof(PktBuf));
			
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			mSecSleep(20); //// sleep 20 ms, ie, 50 packet * 1KByte / sec --> 50KB / sec or 400kbps
			if (i%100 == 99) 
				printf("%d\n", i + 1);
		}
		printf("PPCS_Close(%d)\n", SessionHandle);
		PPCS_Close(SessionHandle);
	}
#endif //// #ifdef PktSR_TEST

#ifdef RW_TEST
	INT32 SessionHandle;
	UINT32 WriteByte;
	
	printf("RW_TEST: PPCS_Listen(%s, 600, 10000, 1, %s)...\n", argv[1], argv[2]);
	SessionHandle = PPCS_Listen(argv[1], 600, 10000, 1, argv[2]);
	
	if (SessionHandle < 0)
		printf("RW_TEST: PPCS_Listen() ret= %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("RW_TEST: P2P Connected!! SessionHandle= %d.\n\n", SessionHandle);
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(SessionHandle, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}
		
		for (INT32 i = 0 ; i< 1000000; i++)
		{
			ret = PPCS_Check_Buffer(SessionHandle, 0, &WriteByte, NULL);
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			if (WriteByte > (32 * 1024))
			{
				mSecSleep(50);
				i--;
				continue;
			}
			INT32 zz = i* 2;

			ret = PPCS_Write(SessionHandle, 0, (CHAR*)&zz, 4);
			
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			if (i%1000 == 999) 
				printf("i = %d\n", i);
		}
		printf("PPCS_Close(%d)\n", SessionHandle);
		PPCS_Close(SessionHandle);
	}
#endif //// RW_TEST

#ifdef FT_TEST
	INT32 SessionHandle;
	printf("FT_TEST: PPCS_Listen(%s, 600, 0, 1, %s)...\n", argv[1], argv[2]);
	SessionHandle = PPCS_Listen(argv[1], 600, 0, 1, argv[2]);
	
	if (0 > SessionHandle)
		printf("FT_TEST: PPCS_Listen() ret= %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("FT_TEST: P2P Connected!! SessionHandle= %d.\n\n", SessionHandle);
		UINT32 Counter = 0;
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(ret, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}
		FILE *fp = fopen("1.txt", "rb");
		if (!fp)
		{
			printf("***Error: failed to open file: %s\n", "1.txt");
			printf("PPCS_Close(%d)\n", SessionHandle);
			PPCS_Close(SessionHandle);
			PPCS_DeInitialize();
			printf("PPCS_DeInitialize done!!\n");
			return 0;
		}
		
		while (!feof(fp))
		{
			CHAR buf[2000];
			INT32 size;
			INT32 ret;
			size = (INT32)fread(buf, 1, sizeof(buf), fp);
			
			ret = PPCS_Write(SessionHandle, 1, buf, size);
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			Counter += size;
			if (Counter % 1048576 == 0) 
				printf("%d MB\n", Counter/1048576);
			
			UINT32 wsize;			
			PPCS_Check_Buffer(SessionHandle, 1, &wsize, NULL);
			if (wsize > 64 * 1024)
			{
				mSecSleep(1);
			}
			if (wsize > 128 * 1024)
			{
				mSecSleep(100);
			}
		}
		printf("File Transfer done!! file size = %d\n",Counter );
		fclose(fp);
		printf("PPCS_Close(%d)\n", SessionHandle);
		PPCS_Close(SessionHandle);
	}
#endif //// FT_TEST


#ifdef RW_TEST1
	INT32 SessionHandle;
	printf("RW_TEST1: PPCS_Listen(%s, 600, 0, 1, %s)...\n", argv[1], argv[2]);
	SessionHandle = PPCS_Listen(argv[1], 600, 0, 1, argv[2]);
	
	if (0 > SessionHandle)
		printf("RW_TEST1: PPCS_Listen() ret= %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("RW_TEST1: P2P Connected!! SessionHandle= %d.\n\n", SessionHandle);
		INT32 i = 0;
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(ret, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}
		
		printf("PPCS Reading ...\n\n");
		while (1)
		{
			//UINT32 ReadSize;
			//PPCS_Check_Buffer(SessionHandle, 0, NULL, &ReadSize);
			LONG zz;
			INT32 DataSize = 4;
			ret = PPCS_Read(SessionHandle, 2, (CHAR*)&zz, &DataSize, 0xFFFFFFFF);
			if (ret == ERROR_PPCS_SESSION_CLOSED_TIMEOUT)
			{
				printf("Session TimeOUT!!\n");
				break;
			}
			else if (ret == ERROR_PPCS_SESSION_CLOSED_REMOTE)
			{
				printf("Session Remote Close!!\n");
				break;
			}
			if ((i * 2) == zz) 
			{
				if (i % 10000000 == 999999)
					printf("i= %d, %ld\n", i, zz); 
			}
			else
			{
				printf("Error: i= %d, %ld\n",i, zz); 
				break;
			}
			i++;
		}
		printf("PPCS_Close(%d)\n", SessionHandle);
		PPCS_Close(SessionHandle);
	}
#endif //// RW_TEST1

#ifdef RW_TEST2
	INT32 SessionHandle;
	printf("RW_TEST2: PPCS_Listen(%s, 600, 10000, 1, %s)...\n", argv[1], argv[2]);
	SessionHandle = PPCS_Listen(argv[1], 600, 10000, 1, argv[2]);
	
	if (0 > SessionHandle)
		printf("RW_TEST2: PPCS_Listen() ret= %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
	else
	{
		printf("RW_TEST2: P2P Connected!! SessionHandle= %d.\n\n", SessionHandle);
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(SessionHandle, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("------------------ Session Ready: %s ------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("---------------- End of Session info ------------------\n");
		}
		INT32 i = 0;
		UINT32 tick = MyGetTickCount();
		
		while (1)
		{
			UCHAR zz;
			INT32 ReadSize = 1;
			ret = PPCS_Read(SessionHandle, 0, (CHAR*)&zz, &ReadSize, 5000);
			if (ret < 0)
			{
				printf("\nPPCS_Read ret= %d , i= %d\n", ret, i);
				break;
			}
			if ((i%256) != zz)
			{
				printf("Error!! i= %d zz= %d\n", i, zz);
				break;
			}
			else
			{
				if (i%102400 == 0)
				{
					printf(".");
					setbuf(stdout, NULL);
				}						
			}
			i++;
		}
		printf("\n");
		printf("Time: %d.%d sec, %d KByte/sec\n",(MyGetTickCount()-tick)/1000, (MyGetTickCount()-tick)%1000, i / (MyGetTickCount()-tick - 1000));
		printf("PPCS_Close(%d)\n", SessionHandle);
		PPCS_Close(SessionHandle);
	}
#endif //// RW_TEST2


#ifdef RW_TEST3
	printf("RW_TEST3: PPCS_Listen(%s, 60, 0, 1, %s)...\n", argv[1], argv[2]);
	gTheSessionHandle = PPCS_Listen(argv[1], 60, 0, 1, argv[2]);
	
	if (0 > gTheSessionHandle)
		printf("RW_TEST3: PPCS_Listen() ret= %d. [%s]\n", gTheSessionHandle, getP2PErrorCodeInfo(gTheSessionHandle));
	else
	{
		printf("RW_TEST3: P2P Connected!! SessionHandle= %d.\n\n", gTheSessionHandle);
		st_PPCS_Session Sinfo;	
		if (PPCS_Check(gTheSessionHandle, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			printf("-------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("Socket : %d\n", Sinfo.Skt);
			printf("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			printf("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			printf("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			printf("Connection time : %d second before\n", Sinfo.ConnectTime);
			printf("DID : %s\n", Sinfo.DID);
			printf("I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			printf("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			printf("------------End of Session info : ---------------\n");
		}

#ifdef WIN32DLL
		HANDLE hThreadWrite[TEST_NUMBER_OF_CHANNEL];
		HANDLE hThreadRead[TEST_NUMBER_OF_CHANNEL];
#endif
#ifdef LINUX
		pthread_t ThreadWriteID[TEST_NUMBER_OF_CHANNEL];
		pthread_t ThreadReadID[TEST_NUMBER_OF_CHANNEL];
#endif

		for (INT32 i = 0; i < TEST_NUMBER_OF_CHANNEL; i++)
		{
#ifdef WIN32DLL
			hThreadWrite[i] = CreateThread(NULL, 0, ThreadWrite, (void *) i, 0, NULL);
			mSecSleep(10);
			hThreadRead[i] = CreateThread(NULL, 0, ThreadRead, (void *) i, 0, NULL);
			mSecSleep(10);
#endif
#ifdef LINUX
			pthread_create(&ThreadWriteID[i], NULL, &ThreadWrite, (void *) i);
			mSecSleep(10);
			pthread_create(&ThreadReadID[i], NULL, &ThreadRead, (void *) i);
			mSecSleep(10);
#endif
		}
		gFlagWorking = 1;
		while (1 == gFlagWorking)
		{
			gFlagWorking = 0;
			mSecSleep(200);
		}
		printf("PPCS_Close(%d)\n", gTheSessionHandle);
		PPCS_Close(gTheSessionHandle);
		
		for (INT32 i = 0; i < TEST_NUMBER_OF_CHANNEL; i++)
		{	
#ifdef WIN32DLL
			WaitForSingleObject(hThreadRead[i], INFINITE);
			WaitForSingleObject(hThreadWrite[i], INFINITE);
#endif
#ifdef LINUX
			pthread_join(ThreadReadID[i], NULL);
			pthread_join(ThreadWriteID[i], NULL);
#endif
		}
	}
#endif

#ifdef AVSTREAM_TEST
	printf("\n");
	int pp = 0;
	while (1) 
	{
		INT32 SessionHandle = 0;
		//pp++;
		printf("AVSTREAM_TEST: PPCS_Listen(%s, 600, %d, 1, %s)...\n", argv[1], pp, argv[2]);
		SessionHandle = PPCS_Listen(argv[1], 600, pp, 1, argv[2]);
		
		if (SessionHandle < 0)
			printf("AVSTREAM_TEST: PPCS_Listen ret= %d. [%s]\n", SessionHandle, getP2PErrorCodeInfo(SessionHandle));
		if (SessionHandle >= 0)
		{
			printf("AVSTREAM_TEST: P2P Connected!! SessionHandle= %d\n", SessionHandle);
			INT32 i;
			for (i = 0; i < MAX_USER; i++)
			{
				if (!gUser[i].bUsed) 
					break;
			}
			if (MAX_USER == i) 
				continue;
			
			printf("i= %d connected!!\n", i);
			gUser[i].SessionHandle = SessionHandle;
			gUser[i].bUsed = 1;

#ifdef WIN32DLL
			void *hThread = NULL;
			hThread = CreateThread(NULL, 0, myThreadRecvIOCtrl, (LPVOID)i, 0, &threadIDrecv);
			if (NULL != hThread) 
				CloseHandle(hThread);
			else
#endif
#ifdef LINUX
			if (0 == pthread_create(&threadIDrecv, NULL, &myThreadRecvIOCtrl, (void *)i)) 
			{
				pthread_detach(threadIDrecv);
			}
			else
#endif
			{
				gRunning=0;
				return 0;
			}
		}
		else if (SessionHandle == ERROR_PPCS_INVALID_ID)
		{
			printf("Invalid DID \n");
			break;
		}
	}
#endif
	
	ret = PPCS_DeInitialize();
	printf("PPCS_DeInitialize done!!\n");
#ifdef WIN32DLL
	printf("Job Done!! press any key to exit ...\n");
	getchar();
#endif
	return 0;
}
