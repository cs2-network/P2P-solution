// PPCS_ClientUIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PPCS_ClientUI.h"
#include "PPCS_ClientUIDlg.h"
#include <WinSock2.h>

#include "../../../../Include/PPCS/PPCS_API.h"
#include "Picture.h"
#include "wave_out.h"
#include "H264CodecLib.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//// This InitString is CS2 PPCS InitString, you must Todo: Modify this for your own InitString 
//// 用户需改为自己平台的 InitString
const char *g_DefaultInitString = "EBGAEIBIKHJJGFJKEOGCFAEPHPMAHONDGJFPBKCPAJJMLFKBDBAGCJPBGOLKIKLKAJMJKFDOOFMOBECEJIMM";

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

//=====================================================






//--define---------------------------------------------
#define MAX_SIZE_BUF	64*1024

#define CHANNEL_DATA	1
#define CHANNEL_IOCTRL	2
#define TIME_SPAN_IDLE	200 //in ms

#define OMSG_UPDATE_LOG		WM_USER+10
#define WPARAM_LOGIN_OK		1
#define WPARAM_LOGIN_FAIL	2


CPPCS_ClientUIDlg::CPPCS_ClientUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPPCS_ClientUIDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_handleSession		 =-1;
	m_threadID_RecvAVData=0L;

	m_bStartVideo	=false;
	m_bStartAudio	=false;
	m_bRunning		=false;
	
	m_rectVideo.SetRect(0,0,0,0);
	m_framePara[0]=m_framePara[1]=0;
	m_framePara[2]=m_framePara[3]=0;
	m_nInitH264Decoder=-1;
	m_pBufBmp24=new BYTE[1024*1024*1.5];

	m_bmiHead.biSize  = sizeof(BITMAPINFOHEADER);
	m_bmiHead.biWidth = 0;
	m_bmiHead.biHeight= 0;
	m_bmiHead.biPlanes= 1;
	m_bmiHead.biBitCount = 24;
	m_bmiHead.biCompression = BI_RGB;
	m_bmiHead.biSizeImage   = 0;
	m_bmiHead.biXPelsPerMeter = 0;
	m_bmiHead.biYPelsPerMeter = 0;
	m_bmiHead.biClrUsed       = 0;
	m_bmiHead.biClrImportant  = 0;

	m_pic	  =NULL;
}

void CPPCS_ClientUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_ctrlLog);
	DDX_Control(pDX, IDC_VIDEO, m_ctlVideo);
	DDX_Control(pDX, IDC_START, m_btnStart);
	DDX_Control(pDX, IDC_STOP, m_btnStop);
	DDX_Control(pDX, IDC_CHECK1, m_ctlChkAudio);
	DDX_Control(pDX, IDC_CHECK2, m_ctlChkVideo);
}

BEGIN_MESSAGE_MAP(CPPCS_ClientUIDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_START, &CPPCS_ClientUIDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, &CPPCS_ClientUIDlg::OnBnClickedStop)
	ON_WM_DESTROY()
	ON_MESSAGE(OMSG_UPDATE_LOG, OnUpdateLog)

	ON_BN_CLICKED(IDC_CHECK1, &CPPCS_ClientUIDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_CHECK2, &CPPCS_ClientUIDlg::OnBnClickedCheck2)
END_MESSAGE_MAP()


// CPPCS_ClientUIDlg message handlers

BOOL CPPCS_ClientUIDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	INT32 nRet = PPCS_Initialize((CHAR*)g_DefaultInitString);	

	CWnd *pWnd=GetDlgItem(IDC_VIDEO);
	if(pWnd) m_pVideoDC=pWnd->GetDC();
	m_ctlVideo.GetClientRect(&m_rectVideo);
	m_rectVideo.right=3*m_rectVideo.Height()/4;

	Set_WIN_Params(INVALID_FILEDESC, 8000.0f, 16, 1);
	m_nInitH264Decoder=InitCodec(1);

	InitUI();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPPCS_ClientUIDlg::OnDestroy()
{
	CDialog::OnDestroy();
	Logout();
	PPCS_DeInitialize();
	
	if(m_pic!=NULL) {
		delete m_pic;
		m_pic=NULL;
	}

	WIN_Audio_close();
	if(m_nInitH264Decoder>=0) {
		UninitCodec();
		m_nInitH264Decoder=-1;
	}
	if(m_pBufBmp24) {
		delete []m_pBufBmp24;
		m_pBufBmp24=NULL;
	}
}



void CPPCS_ClientUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPPCS_ClientUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPPCS_ClientUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPPCS_ClientUIDlg::SetLog(LPCTSTR lpText)
{
	CString csText;
	GetDlgItemText(IDC_EDIT1, csText);
	csText+=lpText;
	SetDlgItemText(IDC_EDIT1, csText);
	m_ctrlLog.LineScroll(m_ctrlLog.GetLineCount(), 0);
}

LRESULT CPPCS_ClientUIDlg::OnUpdateLog(WPARAM wParam, LPARAM lParam)
{

	return 0L;
}

void CPPCS_ClientUIDlg::InitUI()
{
	UINT32 APIVersion = PPCS_GetAPIVersion();
	CString csText;
	INT32 nRet=0;

	csText.Format("PPCS_API Version: %d.%d.%d.%d\n", (APIVersion & 0xFF000000)>>24, (APIVersion & 0x00FF0000)>>16, (APIVersion & 0x0000FF00)>>8, (APIVersion & 0x000000FF) >> 0 );
	SetDlgItemText(IDC_APIVER, csText);
	
	st_PPCS_NetInfo NetInfo;
	nRet = PPCS_NetworkDetect(&NetInfo,0);
	
	switch(NetInfo.NAT_Type)
	{
		case 0:
			csText="Unknow";
			break;
		case 1:
			csText="IP-Restricted Cone";
			break;
		case 2:
			csText="Port-Restricted Cone";
			break;
		case 3:
			csText="Symmetric";
			break;
		default:;
	}

	TRACE("  -------------- NetInfo: -------------------\n");
	TRACE("  Internet Reachable     : %s\n", (NetInfo.bFlagInternet == 1) ? "YES":"NO");
	TRACE("  P2P Server IP resolved : %s\n", (NetInfo.bFlagHostResolved == 1) ? "YES":"NO");
	TRACE("  P2P Server Hello Ack   : %s\n", (NetInfo.bFlagServerHello == 1) ? "YES":"NO");
	TRACE("  Local NAT Type         : %s\n", csText);
	TRACE("  My Wan IP : %s\n", NetInfo.MyWanIP);
	TRACE("  My Lan IP : %s\n", NetInfo.MyLanIP);

	CString csTmp;
	csTmp.Format("  My Wan IP : %s\r\n", NetInfo.MyWanIP); SetLog(csTmp);
	csTmp.Format("  My Lan IP : %s\r\n", NetInfo.MyLanIP); SetLog(csTmp);
	csTmp.Format("  Local NAT Type: %d: %s\r\n", NetInfo.NAT_Type, csText); SetLog(csTmp);

	BtnSwitch();
}


void CPPCS_ClientUIDlg::DispPic(BYTE *pData, BITMAPINFOHEADER *pBmiHead)
{
	if(m_pic==NULL) m_pic=new CPicture();

	if(m_pic->PushData(pData, pBmiHead)) {
		m_pic->Show(m_pVideoDC, m_rectVideo);
	}
}

void CPPCS_ClientUIDlg::DispPic(BYTE *pData, int nSize)
{
	if(m_pic==NULL) m_pic=new CPicture();

	if(m_pic->PushData(pData, nSize)) {		
		m_pic->Show(m_pVideoDC, m_rectVideo);
	}
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

#define READSIZE_EVERY_TIME	2560
UCHAR g_out_pBufTmp[READSIZE_EVERY_TIME];

void CPPCS_ClientUIDlg::PlayAudio_adpcm(BYTE *pData, int nDataSize)
{
	int nSize=0;
	char bufTmp[640];
	for(int i=0; i<nDataSize/160; i++){
		Decode((char *)pData+i*160, 160, bufTmp);
		memcpy(g_out_pBufTmp+nSize, bufTmp, 640);
		nSize+=640;
	}

	WIN_Play_Samples(g_out_pBufTmp, nSize);
}

void CPPCS_ClientUIDlg::PlayAudio_pcm(BYTE *pData, int nDataSize)
{
	WIN_Play_Samples(pData, nDataSize);
}

INT32 CPPCS_ClientUIDlg::myGetDataSizeFrom(st_AVStreamIOHead *pStreamIOHead)
{
	INT32 nDataSize=pStreamIOHead->nStreamIOHead;
	nDataSize &=0x00FFFFFF;
	return nDataSize;
}

void CPPCS_ClientUIDlg::myDoAudioData(INT32 SessionHandle, CHAR *pData)
{
	st_AVFrameHead stFrameHead;
	int nLenFrameHead=sizeof(st_AVFrameHead);
	memcpy(&stFrameHead, pData, nLenFrameHead);
	switch(stFrameHead.nCodecID)
	{
		case CODECID_A_PCM:
			PlayAudio_pcm((BYTE *)&pData[nLenFrameHead], stFrameHead.nDataSize);
			break;

		case CODECID_A_ADPCM:{
				//TRACE("myThreadRecvAVData: stFrameHead.nDataSize=%d\n", stFrameHead.nDataSize);
				PlayAudio_adpcm((BYTE *)&pData[nLenFrameHead], stFrameHead.nDataSize);
			}
			break;
		default:;
	}

}
void CPPCS_ClientUIDlg::myDoVideoData(INT32 SessionHandle, CHAR *pData)
{
	static BOOL bFirstFrame=TRUE;
	st_AVFrameHead stFrameHead;
	int nLenFrameHead=sizeof(st_AVFrameHead);
	memcpy(&stFrameHead, pData, nLenFrameHead);
	switch(stFrameHead.nCodecID)
	{
		case CODECID_V_H264:
			//Decode
			if(m_nInitH264Decoder>=0){
				//TRACE("myDoVideoData(.): stFrameHead.flag=%d\n", stFrameHead.flag);
				if(bFirstFrame && stFrameHead.flag!=VFRAME_FLAG_I) break;
				bFirstFrame=false;

				bool bChgedWidth  =0;
				int consumed_bytes=0;
				int nFrameSize=stFrameHead.nDataSize;
				BYTE *pFrame=(BYTE *)(pData+nLenFrameHead);

				while(nFrameSize>0){
			AGAIN_DECODER_NAL:
					consumed_bytes=H264Decode(m_pBufBmp24, pFrame, nFrameSize, m_framePara, 1);
					if(consumed_bytes<0){
						nFrameSize=0;
						break;
					}
					if(!m_bRunning) break;
					
					if(m_framePara[0]>0){
						if(m_framePara[2]>0 && m_framePara[2]!=m_bmiHead.biWidth){
							if(m_bmiHead.biWidth!=0) bChgedWidth=1;

							m_bmiHead.biWidth		=m_framePara[2];
							m_bmiHead.biHeight		=m_framePara[3];
							m_bmiHead.biSizeImage	=m_framePara[2]*m_framePara[3]*3;

							TRACE("  myDoVideoData(..): DecoderNal(.)>=0, %dX%d, pFrame[2,3,4,5]=%X,%X,%X,%X\n",
									m_framePara[2], m_framePara[3], pFrame[2],pFrame[3],pFrame[4],pFrame[5]);
							if(bChgedWidth) goto AGAIN_DECODER_NAL; //goto-------------------------------------
						}
						DispPic(m_pBufBmp24, &m_bmiHead);
						//TRACE("DispPic=%d ms \n", (GetTickCount()-test3));
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

DWORD WINAPI CPPCS_ClientUIDlg::myThreadRecvAVData(void* arg)
{
	CPPCS_ClientUIDlg *pThis=(CPPCS_ClientUIDlg *)arg;
	INT32 SessionHandle=pThis->m_handleSession;
	INT32 nRet=-1;

	CHAR  *pAVData=(CHAR *)malloc(MAX_SIZE_BUF);
	INT32 nRecvSize=4;
	CHAR  nCurStreamIOType=0;
	st_AVStreamIOHead *pStreamIOHead=NULL;

	while(pThis->m_bRunning){
		nRecvSize=sizeof(st_AVStreamIOHead);
		nRet=PPCS_Read(SessionHandle, CHANNEL_DATA,pAVData, &nRecvSize, 0xFFFFFFFF);
		if(nRet == ERROR_PPCS_SESSION_CLOSED_TIMEOUT){
			TRACE("myThreadRecvAVData: Session TimeOUT!!\n");
			break;

		}else if(nRet == ERROR_PPCS_SESSION_CLOSED_REMOTE){
			TRACE("myThreadRecvAVData: Session Remote Close!!\n");
			break;

		}else if(nRet==ERROR_PPCS_SESSION_CLOSED_CALLED){
			TRACE("myThreadRecvAVData: myself called PPCS_Close!!\n");
			break;

		}//else TRACE("myThreadRecvAVData: errorCode=%d\n", nRet);

		if(nRecvSize>0){
			pStreamIOHead=(st_AVStreamIOHead *)pAVData;
			nCurStreamIOType=pStreamIOHead->uionStreamIOHead.nStreamIOType;

			//TRACE("  --myThreadRecvAVData: 1st nRecvSize=%d, nCurStreamIOType=%d\n", nRecvSize, nCurStreamIOType);
			nRecvSize=pThis->myGetDataSizeFrom(pStreamIOHead);
			nRet=PPCS_Read(SessionHandle,CHANNEL_DATA, pAVData, &nRecvSize, 0xFFFFFFFF);
			//TRACE("  --myThreadRecvAVData: 2nd nRecvSize=%d\n", nRecvSize);
			if(nRet == ERROR_PPCS_SESSION_CLOSED_TIMEOUT){
				TRACE("myThreadRecvAVData: Session TimeOUT!!\n");
				break;

			}else if(nRet == ERROR_PPCS_SESSION_CLOSED_REMOTE){
				TRACE("myThreadRecvAVData: Session Remote Close!!\n");
				break;

			}else if(nRet==ERROR_PPCS_SESSION_CLOSED_CALLED){
				TRACE("myThreadRecvAVData: myself called PPCS_Close!!\n");
				break;
			}

			if(nRecvSize>0){
				if(nRecvSize>=64*1024) TRACE("====nRecvSize>64*1024, nCurStreamIOType=%d\n", nCurStreamIOType);
				//TRACE("myThreadRecvAVData: nRecvSize=%d, nCurStreamIOType=%d\n", nRecvSize, nCurStreamIOType);

				if(nCurStreamIOType==SIO_TYPE_AUDIO) pThis->myDoAudioData(SessionHandle, pAVData);
				else if(nCurStreamIOType==SIO_TYPE_VIDEO) pThis->myDoVideoData(SessionHandle, pAVData);
			}
		}//if(nRecvSize>0)-end
	}

	free(pAVData);
	TRACE("....Thread Recv AVData exit.\n");
	pThis->m_threadID_RecvAVData=0L;
	return 0L;
}

bool CPPCS_ClientUIDlg::CreateThreadRecv(INT32 SessionHandle)
{
	void *hThread=NULL;
	if(m_threadID_RecvAVData!=0L) return false;
	
	m_bRunning=true;
	hThread = CreateThread(NULL, 0, myThreadRecvAVData, (LPVOID)this, 0, &m_threadID_RecvAVData);
	if(NULL!=hThread) CloseHandle(hThread);
	else {
		m_bRunning=false;
		return false;
	}

	BtnSwitch();
	return true;
}

void CPPCS_ClientUIDlg::SendIOCtrl(INT32 handleSession, UINT16 nIOCtrlType, char *pIOData, INT32 nIODataSize)
{
	int nLenHead=sizeof(st_AVStreamIOHead)+sizeof(st_AVIOCtrlHead);
	char *packet=new char[nLenHead+nIODataSize];
	st_AVStreamIOHead *pstStreamIOHead=(st_AVStreamIOHead *)packet;
	st_AVIOCtrlHead *pstIOCtrlHead	 =(st_AVIOCtrlHead *)(packet+sizeof(st_AVStreamIOHead));

	pstStreamIOHead->nStreamIOHead=sizeof(st_AVIOCtrlHead)+nIODataSize;
	pstStreamIOHead->uionStreamIOHead.nStreamIOType=SIO_TYPE_IOCTRL;

	pstIOCtrlHead->nIOCtrlType	  =nIOCtrlType;
	pstIOCtrlHead->nIOCtrlDataSize=nIODataSize;

	if(pIOData) memcpy(packet+nLenHead, pIOData, nIODataSize);

	int nRet=0, nSize=nLenHead+nIODataSize;
	nRet=PPCS_Write(handleSession, CHANNEL_IOCTRL, packet, nSize);
	delete []packet;
	TRACE("SendIOCtrl(..): PPCS_Write(..)=%d\n", nRet);
}

void CPPCS_ClientUIDlg::OnBnClickedStart()
{
	CString csUID, csTmp;
	GetDlgItemText(IDC_COMBO1, csUID);
	if(csUID.IsEmpty()){
		MessageBox("UID must be input.", "Tips", MB_ICONINFORMATION|MB_OK);
		return;
	}
	if(m_handleSession<0){
		m_handleSession = PPCS_Connect(csUID.GetBuffer(), 1, 0);
		csUID.ReleaseBuffer();

		csTmp.Format("PPCS_Connect(..)= %d\r\n", m_handleSession);
		SetLog(csTmp);
	}
	if(m_handleSession<0) return;//return=========================

	if(!m_bRunning){
		st_PPCS_Session Sinfo;
		if(PPCS_Check(m_handleSession, &Sinfo) == ERROR_PPCS_SUCCESSFUL)
		{
			TRACE("  -------------- Session Ready: -%s------------------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			TRACE("  Socket : %d\n", Sinfo.Skt);
			TRACE("  Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			TRACE("  My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			TRACE("  My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			TRACE("  Connection time : %d second before\n", Sinfo.ConnectTime);
			TRACE("  DID : %s\n", Sinfo.DID);
			TRACE("  I am %s\n", (Sinfo.bCorD ==0)? "Client":"Device");
			TRACE("  Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			TRACE("  ------------End of Session info : ---------------\n");
		}
		csTmp.Format("  Remote Addr: %s:%d\r\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port)); SetLog(csTmp);
		csTmp.Format("  Remote mode: %s\r\n", (Sinfo.bMode ==0)? "P2P":"RLY"); SetLog(csTmp);
	}

	bool b=CreateThreadRecv(m_handleSession);


}

void CPPCS_ClientUIDlg::OnBnClickedCheck1() //audio
{
	if(m_handleSession<0){
		MessageBox("Please first connect IPCam.", "Tips", MB_ICONINFORMATION|MB_OK);
		return;
	}
	if(m_ctlChkAudio.GetCheck()){
		SendIOCtrl(m_handleSession, IOCTRL_TYPE_AUDIO_START, NULL, 0);
		m_bStartAudio=true;
	}else{
		SendIOCtrl(m_handleSession, IOCTRL_TYPE_AUDIO_STOP, NULL, 0);
		m_bStartAudio=false;
	}
}

void CPPCS_ClientUIDlg::OnBnClickedCheck2()//video
{
	if(m_handleSession<0){
		MessageBox("Please first connect IPCam.", "Tips", MB_ICONINFORMATION|MB_OK);
		return;
	}

	if(m_ctlChkVideo.GetCheck()){
		SendIOCtrl(m_handleSession, IOCTRL_TYPE_VIDEO_START, NULL, 0);
		m_bStartVideo=true;
	}else{
		SendIOCtrl(m_handleSession, IOCTRL_TYPE_VIDEO_STOP, NULL, 0);
		m_bStartVideo=false;
	}
}

void CPPCS_ClientUIDlg::OnBnClickedStop()
{
	if(m_handleSession<0){
		MessageBox("Please first connect IPCam.", "Tips", MB_ICONINFORMATION|MB_OK);
		return;
	}
	Logout();
	BtnSwitch();
}

void CPPCS_ClientUIDlg::Logout()
{
	m_bRunning   =false;

	if(m_bStartAudio){
		SendIOCtrl(m_handleSession, IOCTRL_TYPE_AUDIO_STOP, NULL, 0);
		m_bStartAudio=false;
	}
	if(m_bStartVideo){
		SendIOCtrl(m_handleSession, IOCTRL_TYPE_VIDEO_STOP, NULL, 0);
		m_bStartVideo=false;
	}
		
	if(m_handleSession>=0) {
		PPCS_Close(m_handleSession);
		m_handleSession=-1;
	}
	
	//wait m_threadID_RecvAVData exit
	Sleep(2000);
}

void CPPCS_ClientUIDlg::BtnSwitch()
{
	if(m_bRunning){
		m_ctlChkAudio.EnableWindow(SW_SHOW);
		m_ctlChkVideo.EnableWindow(SW_SHOW);
		m_btnStop.EnableWindow(SW_SHOW);
	}else{
		m_ctlChkAudio.SetCheck(FALSE);
		m_ctlChkVideo.SetCheck(FALSE);
		m_ctlChkAudio.EnableWindow(SW_HIDE);
		m_ctlChkVideo.EnableWindow(SW_HIDE);
		m_btnStop.EnableWindow(SW_HIDE);
	}
}