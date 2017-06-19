//
//  ViewController.m
//  PPCS_Client
//

#import "ViewController.h"
#import "CamObj.h"
#import "DelegateCamera.h"
#import "PPCS_API.h"

@interface ViewController ()

@end

@implementation ViewController

@synthesize m_camObj;
@synthesize imageview;
@synthesize uiStatus, uiDID;
@synthesize btnConnect, btnDisconnect;
@synthesize btnStartVideo, btnStopVideo;
@synthesize btnStartAudio, btnStopAudio;

- (void)viewDidLoad
{
    [super viewDidLoad];
    m_camObj=[[CamObj alloc] init];
    
    m_colorSpaceRGB   = CGColorSpaceCreateDeviceRGB();
	m_bytesPerRow	  =0;
    m_nWidth=m_nHeight=0;
    m_nImgDataSize    =0;
}

- (void)dealloc
{
    if(m_camObj!=nil){
        [m_camObj releaseObj];
        [m_camObj release];
        m_camObj=nil;
    }
    
    [imageview release];
    [uiStatus  release];
    [uiDID     release];

    [btnConnect     release];
    [btnDisconnect  release];
    [btnStartVideo  release];
    [btnStopVideo   release];
    [btnStartAudio  release];
    [btnStopAudio   release];
    
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    uiDID.text=@"";
    self.uiDID.delegate=self;
    
    
}

- (void)viewDidAppear:(BOOL)animated
{
    NSLog(@"ViewController %@\n", NSStringFromSelector(_cmd));
    
    m_camObj.nsCamName=@"154";
    m_camObj.nsDID=uiDID.text;
    m_camObj.m_delegateCam=self;
    
    [super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    NSLog(@"ViewController %@\n", NSStringFromSelector(_cmd));
    
    m_camObj.m_delegateCam=nil;
    self.uiDID.delegate=nil;
    
    [m_camObj closeAudio];
    [m_camObj stopVideo];
    [m_camObj stopConnect];
    [super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated
{
    NSLog(@"ViewController  %@\n", NSStringFromSelector(_cmd));
    
    [super viewDidDisappear:animated];
}

- (IBAction)BtnConnect:(id)sender
{
    m_camObj.nsDID=uiDID.text;
    NSInteger nRet=[m_camObj startConnect:10];
    NSLog(@"BtnConnect, =%d", nRet);
}

- (IBAction)BtnDisconnect:(id)sender
{
    [m_camObj closeAudio];
    [m_camObj stopVideo];
    [m_camObj stopConnect];
}

- (IBAction)BtnStartVideo:(id)sender
{
    NSInteger nRet=[m_camObj startVideo];
    NSLog(@"BtnStartVideo, =%d", nRet);
}

- (IBAction)BtnStopVideo:(id)sender
{
    [m_camObj stopVideo];
}

- (IBAction)BtnStartAudio:(id)sender
{
    NSInteger nRet=[m_camObj openAudio];
    NSLog(@"BtnStartAudio, =%d", nRet);
}

- (IBAction)BtnStopAudio:(id)sender
{
    [m_camObj closeAudio];
}

#pragma mark -
#pragma mark DelegateCamera
- (void) refreshFrame:(uint8_t *)imgData withVideoWidth:(NSInteger)width videoHeight:(NSInteger)height withObj:(NSObject *)obj
{
    if(m_nWidth!=width || m_nHeight!=height){
        m_nWidth =width;
        m_nHeight=height;
        m_bytesPerRow =m_nWidth*3;
        m_nImgDataSize=m_nWidth*m_nHeight*3;
        
        if(m_nWidth>0) uiStatus.text=[NSString stringWithFormat:@"%dx%d", m_nWidth, m_nHeight];
    }
    
    //memcpy(m_snapData, imgData, MAXSIZE_IMG_BUFFER);
    CGDataProviderRef provider=CGDataProviderCreateWithData(NULL, imgData, m_nImgDataSize, NULL);
    CGImageRef ImgRef=CGImageCreate(width,
                                    height,
                                    8,
                                    24,
                                    m_bytesPerRow,
                                    m_colorSpaceRGB, kCGBitmapByteOrderDefault,
                                    provider, NULL,true,kCGRenderingIntentDefault);
    if(provider!=nil) CGDataProviderRelease(provider);
    if(imageview.contentMode!=UIViewContentModeScaleToFill) imageview.contentMode=UIViewContentModeScaleToFill;
    imageview.image=[UIImage imageWithCGImage:ImgRef];
    if(ImgRef!=nil)   CGImageRelease(ImgRef);
}

- (void) refreshSessionInfo:(int)infoCode withObj:(NSObject *)obj withString:(NSString *)strValue;
{
    NSLog(@" ViewController] refreshSessionInfo: infoCode=%d\n", infoCode);
}

- (void) refreshSessionInfo:(NSInteger)mode
                   OnlineNm:(NSInteger)onlineNm
                 TotalFrame:(NSInteger)totalFrame
                       Time:(NSInteger)time_s
{
    float fFPS=0.0f;
    if(time_s>0)fFPS=totalFrame*1.0f/time_s;
    
    NSString *nsMode=@"Unknown";
    if(mode==CONN_MODE_P2P) nsMode=@"P2P";
    else if(mode==CONN_MODE_RLY) nsMode=@"Relay";
    uiStatus.text=[NSString stringWithFormat:@"%@, %dx%d, N=%d, %0.2fFPS", nsMode, m_nWidth, m_nHeight, onlineNm, fFPS];
}

- (void) updateRecvIOCtrl:(int)ioType withIOData:(char *)pIOData withSize:(int)nIODataSize withObj:(NSObject *)obj
{
    
}

#pragma mark -
#pragma mark Text Field Delegate Methods
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	[textField resignFirstResponder];
	return YES;
}

- (void)toNextResponder:(UITextField *)textField
{
    [textField becomeFirstResponder];
}


@end
