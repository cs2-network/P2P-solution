//
//  ViewController.h
//  PPCS_Client
//

#import <UIKit/UIKit.h>

@protocol DelegateCamera;
@class CamObj;

@interface ViewController : UIViewController <UITextFieldDelegate, DelegateCamera>
{
    NSInteger m_nWidth, m_nHeight;
    NSInteger m_nImgDataSize;
    CGColorSpaceRef m_colorSpaceRGB;
	size_t			m_bytesPerRow;
}

@property(nonatomic, retain) CamObj    *m_camObj;

@property(nonatomic, retain) IBOutlet UIImageView *imageview;
@property(nonatomic, retain) IBOutlet UILabel     *uiStatus;
@property(nonatomic, retain) IBOutlet UITextField *uiDID;


@property(nonatomic, retain) IBOutlet UIButton *btnConnect;
@property(nonatomic, retain) IBOutlet UIButton *btnDisconnect;
@property(nonatomic, retain) IBOutlet UIButton *btnStartVideo;
@property(nonatomic, retain) IBOutlet UIButton *btnStopVideo;
@property(nonatomic, retain) IBOutlet UIButton *btnStartAudio;
@property(nonatomic, retain) IBOutlet UIButton *btnStopAudio;

- (IBAction)BtnConnect:(id)sender;
- (IBAction)BtnDisconnect:(id)sender;
- (IBAction)BtnStartVideo:(id)sender;
- (IBAction)BtnStopVideo:(id)sender;
- (IBAction)BtnStartAudio:(id)sender;
- (IBAction)BtnStopAudio:(id)sender;

@end
