// PPCS_ClientUIDlg.h : header file
//

#pragma once

#include "../../../../Include/AVSTREAM_IO_Proto.h"
#include "afxwin.h"


// CPPCS_ClientUIDlg dialog
class CPicture;
class CPPCS_ClientUIDlg : public CDialog
{
// Construction
public:
	CPPCS_ClientUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PPCS_CLIENTUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	INT32 m_handleSession;
	DWORD m_threadID_RecvAVData;
	bool  m_bRunning;
	bool  m_bStartAudio, m_bStartVideo;

	int   m_nInitH264Decoder;
	int   m_framePara[4];
	BITMAPINFOHEADER m_bmiHead;
	BYTE  *m_pBufBmp24;

	CPicture *m_pic;
	CDC		 *m_pVideoDC;
	CRect	 m_rectVideo;

	void InitUI();
	void DispPic(BYTE *pData, BITMAPINFOHEADER *pBmiHead);
	void DispPic(BYTE *pData, int nSize);
	void PlayAudio_pcm(BYTE *pData, int nDataSize);
	void PlayAudio_adpcm(BYTE *pData, int nDataSize);

	bool CreateThreadRecv(INT32 SessionHandle);
	DWORD static WINAPI myThreadRecvIOCtrl(void* arg);
	DWORD static WINAPI myThreadRecvAVData(void* arg);
	void  myDoIOCtrl(INT32 SessionHandle, CHAR *pData);
	void  myDoAudioData(INT32 SessionHandle, CHAR *pData);
	void  myDoVideoData(INT32 SessionHandle, CHAR *pData);
	INT32 myGetDataSizeFrom(st_AVStreamIOHead *pStreamIOHead);

	void SendIOCtrl(INT32 handleSession, UINT16 nIOCtrlType, char *pIOData, INT32 nIODataSize);

	void SetLog(LPCTSTR lpText);
	void Logout();
	void BtnSwitch();

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnUpdateLog(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()


public:
	CEdit m_ctrlLog;
	CStatic m_ctlVideo;
	CButton m_btnStart;
	CButton m_btnStop;
	CButton m_ctlChkAudio;
	CButton m_ctlChkVideo;

	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	afx_msg void OnDestroy();
	
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedCheck2();
	
};
