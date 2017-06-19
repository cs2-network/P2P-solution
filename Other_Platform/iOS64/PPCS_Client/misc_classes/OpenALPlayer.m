
#import "OpenALPlayer.h"

@implementation OpenALPlayer

@synthesize audioFormat;
@synthesize mDevice;
@synthesize mContext;

#pragma mark - openal function
-(void)initOpenAL:(int)format :(int)sampleRate_
{
    audioFormat = format;
    sampleRate = sampleRate_;
    
    //init the device and context
    mDevice=alcOpenDevice(NULL);
    if (mDevice) {
        mContext=alcCreateContext(mDevice, NULL);
        alcMakeContextCurrent(mContext);
    }
    
    alGenSources(1, &outSourceID);
    alSpeedOfSound(1.0);
    alDopplerVelocity(1.0);
    alDopplerFactor(1.0);
    alSourcef(outSourceID, AL_PITCH, 1.0f);
    alSourcef(outSourceID, AL_GAIN, 1.0f);
    alSourcei(outSourceID, AL_LOOPING, AL_FALSE);
    alSourcef(outSourceID, AL_SOURCE_TYPE, AL_STREAMING);
    //alSourcef(outSourceID, AL_BUFFERS_QUEUED, 29);
}


- (BOOL) updateQueueBuffer
{
    ALint stateVaue;
    int processed, queued;
    
    alGetSourcei(outSourceID, AL_SOURCE_STATE, &stateVaue);
    
    if (stateVaue == AL_STOPPED /*||
        stateVaue == AL_PAUSED || 
        stateVaue == AL_INITIAL*/) 
    {
        //[self playSound];
        return NO;
    }    
    
    alGetSourcei(outSourceID, AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(outSourceID, AL_BUFFERS_QUEUED, &queued);
    while(processed--)
    {
        alSourceUnqueueBuffers(outSourceID, 1, &buff);
        alDeleteBuffers(1, &buff);
    }
    
    return YES;
}

- (void)openAudioFromQueue:(NSData *)data
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSCondition* ticketCondition= [[NSCondition alloc] init];
    [ticketCondition lock];
    
    [self updateQueueBuffer];
    
    ALuint bufferID = 0;
    alGenBuffers(1, &bufferID);
    
    alBufferData(bufferID, audioFormat, [data bytes], [data length], sampleRate);
    alSourceQueueBuffers(outSourceID, 1, &bufferID);
        
    ALint stateVaue;
    alGetSourcei(outSourceID, AL_SOURCE_STATE, &stateVaue);
    if(stateVaue != AL_PLAYING) alSourcePlay(outSourceID);
    
    [ticketCondition unlock];
    [ticketCondition release];
    ticketCondition = nil;
    
    [pool release];
    pool = nil;
}

#pragma mark - play/stop/clean function
-(void)playSound
{
    //alSourcePlay(outSourceID);   
}

-(void)stopSound
{
    alSourceStop(outSourceID);
}

-(void)cleanUpOpenAL
{
    int processed = 0;
    alGetSourcei(outSourceID, AL_BUFFERS_PROCESSED, &processed);

    while(processed--) {
        alSourceUnqueueBuffers(outSourceID, 1, &buff);
        alDeleteBuffers(1, &buff);
    }

    alDeleteSources(1, &outSourceID);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(mContext);       
    alcCloseDevice(mDevice);
}

-(void)dealloc
{
    // NSLog(@"openal sound dealloc");

    [super dealloc];
}

@end
