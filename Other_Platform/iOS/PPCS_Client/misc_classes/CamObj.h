//
//  CamObj.h
//

#import <Foundation/Foundation.h>
#import "av_fifo.h"

#define MAXSIZE_IMG_BUFFER  2764800

#define NOTDONE             0
#define DONE                1

#define CONN_MODE_UNKNOWN  -1
#define CONN_MODE_P2P       0
#define CONN_MODE_RLY       1

#define AV_TYPE_REALAV      1
#define AV_TYPE_PLAYBACK    2

@protocol DelegateCamera;
typedef enum {
    CONN_INFO_UNKNOWN=5000,
    CONN_INFO_CONNECTING,        CONN_INFO_NO_NETWORK,
    CONN_INFO_CONNECT_WRONG_DID, CONN_INFO_CONNECT_WRONG_PWD,
    CONN_INFO_CONNECT_FAIL,      STATUS_INFO_SESSION_CLOSED,
    CONN_INFO_CONNECTED,         STATUS_INFO_PPPP_CHECK_OK,

}E_CAM_STATE;

extern int  gAPIVer;

@interface CamObj : NSObject
{
    NSUInteger nRowID;
    NSString  *nsCamName;
    NSString  *nsDID;
    //-------------------

    E_CAM_STATE   mCamState;
    volatile int  mConnMode;
    volatile int  m_handle;
    volatile char m_bConnecting;
    
    av_fifo_t *m_fifoVideo, *m_fifoAudio;
    
    volatile BOOL m_bRunning, m_bVideoPlaying, m_bAudioDecording;
    NSThread *mThreadPlayVideo;
    NSThread *mThreadDecordAudio;
    NSThread *mThreadRecvAVData;
    NSConditionLock *mLockPlayVideo;
    NSConditionLock *mLockDecordAudio;
    NSConditionLock *mLockRecvAVData;
    
    unsigned long m_nTickUpdateInfo;
    unsigned long m_nFirstTickLocal_video, m_nTick2_video, m_nFirstTimestampDevice_video;
	unsigned long m_nFirstTickLocal_audio, m_nTick2_audio, m_nFirstTimestampDevice_audio;
    BOOL  m_bFirstFrame;
    int   m_nInitH264Decoder;
	int   m_framePara[4];
	unsigned char *m_pBufBmp24;
    NSInteger  mVideoHeight, mVideoWidth;
    NSUInteger mTotalFrame;
    char mOutAudio[640], mTmp[40];
    unsigned long mWaitTime_ms;
}

@property(assign)  NSUInteger   nRowID;
@property(assign)  E_CAM_STATE  mCamState;
@property(assign)  NSInteger mVideoHeight, mVideoWidth;

@property(assign)  av_fifo_t *m_fifoVideo, *m_fifoAudio;
@property(nonatomic, retain) NSLock  *mLockConnecting;

@property(assign) BOOL m_bRunning, m_bVideoPlaying, m_bAudioDecording;
@property(nonatomic, retain) NSString *nsCamName;
@property(nonatomic, retain) NSString *nsDID;
@property(nonatomic, retain) id<DelegateCamera> m_delegateCam;

- (void) releaseObj;

+ (void) initAPI;
+ (void) deinitAPI;
+ (NSString *) infoCode2Str:(int)infoCode;
+ (unsigned long) getTickCount;

- (int) readDataFromRemote:(int) handleSession withChannel:(unsigned char) Channel withBuf:(char *)DataBuf
withDataSize: (int *)pDataSize withTimeout:(int)TimeOut_ms;

- (NSInteger)sendIOCtrl:(int) handleSession withIOType:(int) nIOCtrlType withIOData:(char *)pIOData withIODataSize:(int)nIODataSize;
- (BOOL) isConnected;
- (void) stopAll;
- (NSInteger) startConnect:(unsigned long)waitTime_sec;
- (void) stopConnect;
- (NSInteger) openAudio;
- (void) closeAudio;
- (NSInteger) startVideo;
- (void) stopVideo;

@end

