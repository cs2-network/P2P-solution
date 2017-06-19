
#import <Foundation/Foundation.h>
#import <OpenAL/al.h>
#import <OpenAL/alc.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/ExtendedAudioFile.h>

@interface OpenALPlayer : NSObject{
    ALCcontext *mContext;
    ALCdevice *mDevice;
    ALuint outSourceID;
    ALuint buff;
    
    ALenum audioFormat;
    int sampleRate;
}
@property (nonatomic) ALenum audioFormat;
@property (nonatomic) ALCcontext *mContext;
@property (nonatomic) ALCdevice  *mDevice;

- (void)initOpenAL:(int)format :(int)sampleRate;
- (void)openAudioFromQueue:(NSData *)data;
- (void)playSound;
- (void)stopSound;
- (void)cleanUpOpenAL;
@end
