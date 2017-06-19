//
//  CamObj.m
//
#import <sys/time.h>
#import "CamObj.h"
#import "H264iPhone.h"
#import "DelegateCamera.h"
#import "PPCS_API.h"
#import "AVStream_IO_proto.h"
#import "H264iPhone.h"
#import "OpenALPlayer.h"

#define CHANNEL_DATA	1
#define CHANNEL_IOCTRL	2


#define MAX_SIZE_IOCTRL_BUF   5120    //5K
#define MAX_SIZE_AV_BUF       262144  //256K
#define MAX_SIZE_AUDIO_PCM    3200
#define MAX_SIZE_AUDIO_SAMPLE 640

#define MAX_AUDIO_BUF_NUM     25
#define MIN_AUDIO_BUF_NUM     1

int  gAPIVer   =0;

@implementation CamObj
@synthesize nRowID, mCamState;
@synthesize m_bRunning, m_bVideoPlaying, m_bAudioDecording;
@synthesize mVideoHeight, mVideoWidth;
@synthesize m_fifoVideo, m_fifoAudio;
@synthesize mLockConnecting;
@synthesize nsCamName, nsDID;
@synthesize m_delegateCam;

#pragma mark -
#pragma mark init and release
- (void) initValue
{
    nRowID    =-1;
    nsCamName =@"";
    nsDID     =@"";

    mConnMode=CONN_MODE_UNKNOWN;
    mCamState=CONN_INFO_UNKNOWN;
    
    m_nTickUpdateInfo=0L;
    m_bVideoPlaying  =NO;
    m_bAudioDecording=NO;
    m_bRunning=NO;
    
    mThreadPlayVideo  =nil;
    mThreadDecordAudio=nil;
    mThreadRecvAVData =nil;
    mLockPlayVideo   =nil;
    mLockDecordAudio =nil;
    mLockRecvAVData  =nil;
    
    m_handle    =-1;
    mWaitTime_ms=0L;

    mVideoHeight=0;
    mVideoWidth =0;
    
    m_fifoAudio=av_FifoNew();
    m_fifoVideo=av_FifoNew();
    m_nInitH264Decoder=-1;

    m_bConnecting=0;
    self.mLockConnecting=[[NSLock alloc] init];
    
    m_delegateCam=nil;
}

- (id)init
{
    if((self = [super init])) [self initValue];
    return self;
}

- (void) releaseObj
{
    [nsCamName release];
    [nsDID release];
    nsCamName=nil;
    nsDID=nil;
    
    if(m_fifoVideo){
        av_FifoRelease(m_fifoVideo);
        m_fifoVideo=NULL;
    }
    if(m_fifoAudio){
        av_FifoRelease(m_fifoAudio);
        m_fifoAudio=NULL;
    }
    
    [mLockConnecting release];
    mLockConnecting=nil;
}

+ (void) initAPI
{
    char Para[]={"EBGAEIBIKHJJGFJKEOGCFAEPHPMAHONDGJFPBKCPAJJMLFKBDBAGCJPBGOLKIKLKAJMJKFDOOFMOBECEJIMM"};
    PPCS_Initialize(Para);
    gAPIVer=PPCS_GetAPIVersion();
    NSLog(@"gAPIVer=0x%X", gAPIVer);
    [self NetworkDetect];
}

+ (void)NetworkDetect{
    
    st_PPCS_NetInfo NetInfo;
    PPCS_NetworkDetect(&NetInfo, 0);
    
    printf("-------------- NetInfo: -------------------\n");
    printf("My Lan IP:%s\n", NetInfo.MyLanIP);
    printf("My Wan IP:%s\n", NetInfo.MyWanIP);
    printf("Internet Reachable: %s\n", 1 == NetInfo.bFlagInternet ? "Yes":"No");
    printf("P2P Server IP resolved: %s\n", 1 == NetInfo.bFlagHostResolved ? "Yes":"No");
    printf("Server Hello Ack: %s\n", 1 == NetInfo.bFlagServerHello ? "Yes":"No");
    
    printf("Local NAT Type         :");
    
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
    
    printf("---------------------------------\n");
}


+ (void) deinitAPI
{
    PPCS_DeInitialize();
}

+ (NSString *) infoCode2Str:(int)infoCode
{
    NSString *result=@"";    
    switch(infoCode) {
        case CONN_INFO_NO_NETWORK:
            result=NSLocalizedString(@"Network is not reachable",nil);
            break;
            
        case CONN_INFO_CONNECTING:
            result=NSLocalizedString(@"Connecting...",nil);
            break;
            
        case CONN_INFO_CONNECT_WRONG_DID:
            result=NSLocalizedString(@"Wrong DID",nil);
            break;
            
        case CONN_INFO_CONNECT_WRONG_PWD:
            result=NSLocalizedString(@"Wrong password",nil);
            break;
            
        case CONN_INFO_CONNECT_FAIL:
            result=NSLocalizedString(@"Failed to connect",nil);
            break;
            
        case CONN_INFO_CONNECTED:
            result=NSLocalizedString(@"Connected",nil);
            break;
            
        case STATUS_INFO_SESSION_CLOSED:
            result=NSLocalizedString(@"Disconnected",nil);
            break;
            
        default:
            break;
    }
    return result;
}

#pragma mark - misc function
+ (unsigned long) getTickCount
{
	struct timeval tv;
	if(gettimeofday(&tv, NULL)!=0) return 0;
	return (tv.tv_sec*1000 +tv.tv_usec/1000);
}

-(void) ResetAudioVar
{
	m_nFirstTickLocal_audio=0L;
	m_nTick2_audio=0L;
	m_nFirstTimestampDevice_audio=0L;
    
	av_FifoEmpty(m_fifoAudio);
}

-(void) ResetVideoVar
{
	m_nFirstTickLocal_video=0L;
	m_nTick2_video=0L;
	m_nFirstTimestampDevice_video=0L;

	av_FifoEmpty(m_fifoVideo);
    m_bFirstFrame=TRUE;
}

- (BOOL) mayContinue
{
    if(nsDID==nil || [nsDID length]<=0) return NO;
    else return YES;
}

-(int) readDataFromRemote:(int) handleSession withChannel:(unsigned char) Channel withBuf:(char *)DataBuf
                                          withDataSize: (int *)pDataSize withTimeout:(int)TimeOut_ms
{
	INT32 nRet=-1, nTotalRead=0, nRead=0;
	while(nTotalRead < *pDataSize){
		nRead=*pDataSize-nTotalRead;
		if(handleSession>=0) nRet=PPCS_Read(handleSession, Channel,DataBuf+nTotalRead, &nRead, TimeOut_ms);
		else break;
		nTotalRead+=nRead;
        
		if((nRet != ERROR_PPCS_SUCCESSFUL) && (nRet != ERROR_PPCS_TIME_OUT )) break;
        
		if(!m_bRunning) break;
	}
	//NSLog(@" readDataFromRemote(.)=%d, *pDataSize=%d\n", nRet, *pDataSize);
	if(nRet<0) *pDataSize=nTotalRead;
	return nRet;
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

	for(i = 0; i<nLenRaw; i++)
	{
		cur_sample = pcm[i]; 
		delta = cur_sample - g_nAudioPreSample;
		if (delta < 0){
			delta = -delta;
			sb = 8;
		}else sb = 0;

		code = 4 * delta / gs_step_table[g_nAudioIndex];	
		if (code>7)	code=7;

		delta = (gs_step_table[g_nAudioIndex] * code) / 4 + gs_step_table[g_nAudioIndex] / 8;
		if(sb) delta = -delta;

		g_nAudioPreSample += delta;
		if (g_nAudioPreSample > 32767) g_nAudioPreSample = 32767;
		else if (g_nAudioPreSample < -32768) g_nAudioPreSample = -32768;

		g_nAudioIndex += gs_index_adjust[code];
		if(g_nAudioIndex < 0) g_nAudioIndex = 0;
		else if(g_nAudioIndex > 88) g_nAudioIndex = 88;

		if(i & 0x01) pBufEncoded[i>>1] |= code | sb;
		else pBufEncoded[i>>1] = (code | sb) << 4;
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

	for(i=0; i<nLenData; i++)
	{
		if(i & 0x01) code = pDataCompressed[i>>1] & 0x0f;
		else code = pDataCompressed[i>>1] >> 4;

		if((code & 8) != 0) sb = 1;
		else sb = 0;
		code &= 7;

		delta = (gs_step_table[g_nAudioIndex] * code) / 4 + gs_step_table[g_nAudioIndex] / 8;
		if(sb) delta = -delta;

		g_nAudioPreSample += delta;
		if(g_nAudioPreSample > 32767) g_nAudioPreSample = 32767;
		else if (g_nAudioPreSample < -32768) g_nAudioPreSample = -32768;

		pcm[i] = g_nAudioPreSample;
		g_nAudioIndex+= gs_index_adjust[code];
		if(g_nAudioIndex < 0) g_nAudioIndex = 0;
		if(g_nAudioIndex > 88) g_nAudioIndex= 88;
	}
}
//==}}audio: ADPCM codec==============================================================

#pragma mark - interface of CamObj
- (NSInteger)sendIOCtrl:(int) handleSession withIOType:(int) nIOCtrlType withIOData:(char *)pIOData withIODataSize:(int)nIODataSize
{
    NSInteger nRet=0;
    
    int nLenHead=sizeof(st_AVStreamIOHead)+sizeof(st_AVIOCtrlHead);
	char *packet=new char[nLenHead+nIODataSize];
	st_AVStreamIOHead *pstStreamIOHead=(st_AVStreamIOHead *)packet;
	st_AVIOCtrlHead *pstIOCtrlHead	 =(st_AVIOCtrlHead *)(packet+sizeof(st_AVStreamIOHead));

	pstStreamIOHead->nStreamIOHead=sizeof(st_AVIOCtrlHead)+nIODataSize;
	pstStreamIOHead->uionStreamIOHead.nStreamIOType=SIO_TYPE_IOCTRL;

	pstIOCtrlHead->nIOCtrlType	  =nIOCtrlType;
	pstIOCtrlHead->nIOCtrlDataSize=nIODataSize;

	if(pIOData) memcpy(packet+nLenHead, pIOData, nIODataSize);

	int nSize=nLenHead+nIODataSize;
	nRet=PPCS_Write(handleSession, CHANNEL_IOCTRL, packet, nSize);
	delete []packet;
	NSLog(@"SendIOCtrl(..): PPCS_Write(..)=%d\n", nRet);

    return nRet;
}

- (BOOL) isConnected
{
    return (m_handle>=0 ? YES : NO);
}

- (void) stopAll
{
    [self closeAudio];
    [self stopVideo];
    [self stopConnect];
}

INT32 myGetDataSizeFrom(st_AVStreamIOHead *pStreamIOHead)
{
	INT32 nDataSize=pStreamIOHead->nStreamIOHead;
	nDataSize &=0x00FFFFFF;
	return nDataSize;
}

- (NSInteger) startConnect:(unsigned long)waitTime_sec
{
    if(![self mayContinue]) return -1;
    else if(m_bConnecting) return -2;
    else {
        m_bConnecting=1;
    [mLockConnecting lock];
        mConnMode=CONN_MODE_UNKNOWN;
        
        char *sDID=NULL;
        sDID=(char *)[nsDID cStringUsingEncoding:NSASCIIStringEncoding];
        m_handle=PPCS_Connect(sDID, 1, 0);
        if(m_handle>=0){
            st_PPCS_Session SInfo;
            memset(&SInfo, 0, sizeof(SInfo));
            PPCS_Check(m_handle, &SInfo);
            mConnMode=SInfo.bMode;
            
            //create receiving data thread
            if(mThreadRecvAVData==nil){
                m_bRunning=YES;
                mLockRecvAVData=[[NSConditionLock alloc] initWithCondition:NOTDONE];
                mThreadRecvAVData=[[NSThread alloc] initWithTarget:self selector:@selector(ThreadRecvAVData) object:nil];
                [mThreadRecvAVData start];
            }
        }
    [mLockConnecting unlock];
        m_bConnecting=0;
    }
    return m_handle;
}

- (void) stopConnect
{
    PPCS_Connect_Break();
    
    
    if(m_handle>=0) {
        PPCS_Close(m_handle);
        
        //stop receiving data thread
        m_bRunning=NO;
        if(mThreadRecvAVData!=nil){
            [mLockRecvAVData lockWhenCondition:DONE];
            [mLockRecvAVData unlock];
            
            [mLockRecvAVData release];
            mLockRecvAVData  =nil;
            [mThreadRecvAVData release];
            mThreadRecvAVData=nil;
        }
        
        m_handle=-1;
    }
}

- (NSInteger) openAudio
{
    NSInteger nRet=-1;
    nRet=[self sendIOCtrl:m_handle withIOType:IOCTRL_TYPE_AUDIO_START withIOData:NULL withIODataSize:0];
    
    if(nRet>=0 && mThreadDecordAudio==nil){
        [self ResetAudioVar];
        mLockDecordAudio=[[NSConditionLock alloc] initWithCondition:NOTDONE];
        mThreadDecordAudio=[[NSThread alloc] initWithTarget:self selector:@selector(ThreadDecordAudio) object:nil];
        [mThreadDecordAudio start];
    }
    return nRet;
}

- (void) closeAudio
{
    NSInteger nRet=-1;
    nRet=[self sendIOCtrl:m_handle withIOType:IOCTRL_TYPE_AUDIO_STOP withIOData:NULL withIODataSize:0];
    NSLog(@"closeAudio, nRet=%d", nRet);
    
    m_bAudioDecording=NO;
    if(mThreadDecordAudio!=nil){
        [mLockDecordAudio lockWhenCondition:DONE];
        [mLockDecordAudio unlock];
        
        [mLockDecordAudio release];
        mLockDecordAudio  =nil;
        [mThreadDecordAudio release];
        mThreadDecordAudio=nil;
    }
}

- (NSInteger) startVideo
{
    NSInteger nRet=-1;
    nRet=[self sendIOCtrl:m_handle withIOType:IOCTRL_TYPE_VIDEO_START withIOData:NULL withIODataSize:0];
    
    if(nRet>=0 && mThreadPlayVideo==nil){
        mLockPlayVideo=[[NSConditionLock alloc] initWithCondition:NOTDONE];
        mThreadPlayVideo=[[NSThread alloc] initWithTarget:self selector:@selector(ThreadPlayVideo) object:nil];
        [mThreadPlayVideo start];
    }
    return nRet;
}

- (void) stopVideo
{
    NSInteger nRet=-1;
    nRet=[self sendIOCtrl:m_handle withIOType:IOCTRL_TYPE_VIDEO_STOP withIOData:NULL withIODataSize:0];
    NSLog(@"stopVideo, nRet=%d", nRet);
    
    m_bVideoPlaying=NO;
    if(nRet>=0 && mThreadPlayVideo!=nil){
        [mLockPlayVideo lockWhenCondition:DONE];
        [mLockPlayVideo unlock];
        
        [mLockPlayVideo release];
        mLockPlayVideo  =nil;
        [mThreadPlayVideo release];
        mThreadPlayVideo=nil;
    }
}

-(void) myDoVideoData:(CHAR *)pData
{
	st_AVFrameHead stFrameHead;
	int nLenFrameHead=sizeof(stFrameHead);
	memcpy(&stFrameHead, pData, nLenFrameHead);
	long nDiffTimeStamp=0L;
    
    //update online num every 3s
    unsigned long nTick2=[CamObj getTickCount];
    NSUInteger nTimespan=nTick2-m_nTickUpdateInfo;
    if(nTimespan==0) nTimespan=1000;
    if(nTimespan>=3000 || m_bFirstFrame){
        m_nTickUpdateInfo=nTick2;
        NSUInteger totalFrame=mTotalFrame;
        mTotalFrame=0;
        nTimespan=nTimespan/1000;
        //NSLog(@"myDoVideoData, stFrameHead.status=%d, totalFrame=%d\n", stFrameHead.status, totalFrame);
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if(self.m_delegateCam && [self.m_delegateCam respondsToSelector:@selector(refreshSessionInfo:OnlineNm:TotalFrame:Time:)])
                [self.m_delegateCam refreshSessionInfo:mConnMode OnlineNm:stFrameHead.nOnlineNum TotalFrame:totalFrame Time:nTimespan];
        });
    }
    
	switch(stFrameHead.nCodecID)
	{
		case CODECID_V_H264:
			if(m_nInitH264Decoder>=0){
				if(m_bFirstFrame && stFrameHead.flag!=VFRAME_FLAG_I) break;
				m_bFirstFrame=FALSE;
                
				int consumed_bytes=0;
				int nFrameSize=stFrameHead.nDataSize;
				UCHAR *pFrame=(UCHAR *)(pData+nLenFrameHead);
                
				while(nFrameSize>0){
                AGAIN_DECODER_NAL:
					consumed_bytes=H264Decode(m_pBufBmp24, pFrame, nFrameSize, m_framePara);
					if(consumed_bytes<0){
						nFrameSize=0;
						break;
					}
					if(!m_bVideoPlaying) break;
					
					if(m_framePara[0]>0){
						if(m_framePara[2]>0 && m_framePara[2]!=mVideoWidth){
							mVideoWidth		=m_framePara[2];
							mVideoHeight	=m_framePara[3];
							NSLog(@"  myDoVideoData(..): DecoderNal(.)>=0, %dX%d, pFrame[2,3,4,5]=%X,%X,%X,%X\n",
                                  m_framePara[2], m_framePara[3], pFrame[2],pFrame[3],pFrame[4],pFrame[5]);
						}
						
						m_nTick2_video=[CamObj getTickCount];
						if(m_nFirstTimestampDevice_video==0 || m_nFirstTickLocal_video==0){
							m_nFirstTimestampDevice_video=stFrameHead.nTimeStamp;
							m_nFirstTickLocal_video		 =m_nTick2_video;
						}
						if(m_nTick2_video<m_nFirstTickLocal_video ||
                           stFrameHead.nTimeStamp<m_nFirstTimestampDevice_video)
                        {
							m_nFirstTimestampDevice_video=stFrameHead.nTimeStamp;
							m_nFirstTickLocal_video		 =m_nTick2_video;
						}
						
						nDiffTimeStamp=(stFrameHead.nTimeStamp-m_nFirstTimestampDevice_video) - (m_nTick2_video-m_nFirstTickLocal_video);
                        if(nDiffTimeStamp<3000){
                            for(int kk=0; kk<nDiffTimeStamp; kk++){
                                if(!m_bVideoPlaying) break;
                                usleep(1000);
                            }
                        }
                        
                        mTotalFrame++;
                        dispatch_async(dispatch_get_main_queue(), ^{
                            if(self.m_delegateCam &&
                               [self.m_delegateCam respondsToSelector:@selector(refreshFrame:withVideoWidth:videoHeight:withObj:)])
                                [self.m_delegateCam refreshFrame:m_pBufBmp24
                                                  withVideoWidth:mVideoWidth
                                                     videoHeight:mVideoHeight
                                                         withObj:self];
                        });
					}
					nFrameSize-=consumed_bytes;
					if(nFrameSize>0) memcpy(pFrame, pFrame+consumed_bytes, nFrameSize);
					else nFrameSize=0;
				}//while--end
			}
			break;
		default:;
	}
}
//video: decord and display it
- (void)ThreadPlayVideo
{
    block_t *pBlock=NULL;
    NSLog(@"    ThreadPlayVideo, nNumFiFo=%d", av_FifoCount(m_fifoVideo));
    
    m_nTickUpdateInfo =0L;
    mTotalFrame=0;

    [mLockPlayVideo lock];
    m_nInitH264Decoder=InitCodec(1);
    m_pBufBmp24=(unsigned char *)malloc(MAXSIZE_IMG_BUFFER);
    m_bVideoPlaying=YES;
    while(m_bVideoPlaying){
        pBlock=av_FifoGetAndRemove(m_fifoVideo);
        if(pBlock==NULL){
            usleep(8000);
            continue;
        }

        [self myDoVideoData:pBlock->p_buffer];
        block_Release(pBlock);
        pBlock=NULL;
    }
    free(m_pBufBmp24);
    UninitCodec();
    [mLockPlayVideo unlockWithCondition:DONE];
    
    NSLog(@"=== ThreadPlayVideo exit ===");
}


//audio: decord and play it
- (void)ThreadDecordAudio
{
    NSLog(@"  --- ThreadDecordAudio, going...\n");
    int nAudioFIFONum=0, nAUDIO_BUF_NUM=MAX_AUDIO_BUF_NUM;
[mLockDecordAudio lock];
    block_t *pBlock=NULL;
    st_AVFrameHead stFrameHead;
    int  nLenFrameHead=sizeof(st_AVFrameHead), nFrameSize=0;
    char *pFrame=NULL;
    char *outPCM=(char *)malloc(MAX_SIZE_AUDIO_PCM);
    int  nTimeSleep=25000, nSizePCM=0;
    char bufTmp[640];
    
    OpenALPlayer *player = nil;
    player = [[OpenALPlayer alloc] init];
    int format=AL_FORMAT_MONO16, nSamplingRate=8000;
    [player initOpenAL:format :nSamplingRate];

    g_nAudioIndex=0;
    g_nAudioPreSample=0;
    m_bAudioDecording=YES;
    while(m_bAudioDecording){
        nAudioFIFONum=av_FifoCount(m_fifoAudio);
        if(nAudioFIFONum<nAUDIO_BUF_NUM){
            if(nAUDIO_BUF_NUM==MIN_AUDIO_BUF_NUM) nAUDIO_BUF_NUM=MAX_AUDIO_BUF_NUM;
            usleep(4000);
            continue;
        }else nAUDIO_BUF_NUM=MIN_AUDIO_BUF_NUM;
        
        pBlock=av_FifoGetAndRemove(m_fifoAudio);
        if(pBlock==NULL) continue;
        
        memcpy(&stFrameHead, pBlock->p_buffer, nLenFrameHead);        
        pFrame=(pBlock->p_buffer+nLenFrameHead);
        nFrameSize=stFrameHead.nDataSize;
        if(stFrameHead.nCodecID==CODECID_A_ADPCM){
            nSizePCM=0;
            for(int i=0; i<nFrameSize/160; i++){
                Decode((char *)pFrame+i*160, 160, bufTmp);
                memcpy(outPCM+nSizePCM, bufTmp, 640);
                nSizePCM+=640;
            }

            [player openAudioFromQueue:[NSData dataWithBytes:outPCM length:nSizePCM]];
            usleep(nTimeSleep);
            //NSLog(@"adpcm, nTimeSleep=%d nFrameSize=%d, nSizePCM=%d", nTimeSleep, nFrameSize, nSizePCM);
        }
        block_Release(pBlock);
        pBlock=NULL;
    }
    
    if(outPCM) {
        free(outPCM);
        outPCM=NULL;
    }
    if(player != nil) {
        [player stopSound];
        [player cleanUpOpenAL];
        [player release];
        player = nil;
    }
[mLockDecordAudio unlockWithCondition:DONE];
    NSLog(@"=== ThreadDecordAudio exit ===");
}


- (void)ThreadRecvAVData
{
    NSLog(@"    ThreadRecvAVData going...");
    
	CHAR  *pAVData=(CHAR *)malloc(MAX_SIZE_AV_BUF);
	INT32 nRecvSize=4, nRet=0;
	CHAR  nCurStreamIOType=0;
	st_AVStreamIOHead *pStreamIOHead=NULL;
    block_t *pBlock=NULL;
    
    m_bRunning=YES;
    while(m_bRunning){
		nRecvSize=sizeof(st_AVStreamIOHead);
		nRet=[self readDataFromRemote:m_handle withChannel:CHANNEL_DATA withBuf:pAVData withDataSize:&nRecvSize withTimeout:100];
		if(nRet == ERROR_PPCS_SESSION_CLOSED_TIMEOUT){
			NSLog(@"ThreadRecvAVData: Session TimeOUT!!\n");
			break;
            
		}else if(nRet == ERROR_PPCS_SESSION_CLOSED_REMOTE){
			NSLog(@"ThreadRecvAVData: Session Remote Close!!\n");
			break;
            
		}else if(nRet==ERROR_PPCS_SESSION_CLOSED_CALLED){
			NSLog(@"ThreadRecvAVData: myself called PPCS_Close!!\n");
			break;
            
		}//else NSLog(@"ThreadRecvAVData: errorCode=%d\n", nRet);
        
		if(nRecvSize>0){
			pStreamIOHead=(st_AVStreamIOHead *)pAVData;
			nCurStreamIOType=pStreamIOHead->uionStreamIOHead.nStreamIOType;
            
			nRecvSize=myGetDataSizeFrom(pStreamIOHead);
			nRet=[self readDataFromRemote:m_handle withChannel:CHANNEL_DATA withBuf:pAVData withDataSize:&nRecvSize withTimeout:100];
			if(nRet == ERROR_PPCS_SESSION_CLOSED_TIMEOUT){
				NSLog(@"ThreadRecvAVData: Session TimeOUT!!\n");
				break;
                
			}else if(nRet == ERROR_PPCS_SESSION_CLOSED_REMOTE){
				NSLog(@"ThreadRecvAVData: Session Remote Close!!\n");
				break;
                
			}else if(nRet==ERROR_PPCS_SESSION_CLOSED_CALLED){
				NSLog(@"ThreadRecvAVData: myself called PPCS_Close!!\n");
				break;
			}
            
			if(nRecvSize>0){
				if(nRecvSize>=MAX_SIZE_AV_BUF) NSLog(@"====nRecvSize>256K, nCurStreamIOType=%d\n", nCurStreamIOType);
                else{
                    //NSLog(@"ThreadRecvAVData: nRecvSize=%d, nCurStreamIOType=%d\n", nRecvSize, nCurStreamIOType);
                    if(nCurStreamIOType==SIO_TYPE_AUDIO){
                        pBlock=(block_t *)malloc(sizeof(block_t));
						block_Alloc(pBlock, pAVData, nRecvSize+sizeof(st_AVStreamIOHead));
						av_FifoPut(m_fifoAudio, pBlock);
                        
                    }else if(nCurStreamIOType==SIO_TYPE_VIDEO){
                        pBlock=(block_t *)malloc(sizeof(block_t));
						block_Alloc(pBlock, pAVData, nRecvSize+sizeof(st_AVStreamIOHead));
						av_FifoPut(m_fifoVideo, pBlock);
                    }
                }
			}
		}//if(nRecvSize>0)-end
    }
    [mLockRecvAVData unlockWithCondition:DONE];
    
    NSLog(@"=== ThreadRecvAVData exit ===");
}

@end

