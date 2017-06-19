// Picture.h: interface for the CPicture class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PICTURE_H__86578881_D242_4CD6_AF57_723581F6EB40__INCLUDED_)
#define AFX_PICTURE_H__86578881_D242_4CD6_AF57_723581F6EB40__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define JPG_MAXSIZE	1024*1024*1.5
#define ERROR_TITLE "CPicture Error" 

class CPicture  
{
public:
	CPicture();
	virtual ~CPicture();
	
	BOOL PushData(BYTE *pData, BITMAPINFOHEADER *pBmiHead);
	BOOL PushData(BYTE *pData, int nSize);
	BOOL Show(CDC *pDC, CRect DrawRect);
	BOOL LoadPictureData(BYTE *pBuffer, BITMAPINFOHEADER *pBmiHead);
	BOOL LoadPictureData(BYTE *pBuffer, int nSize);
	void FreePictureData();

protected:
	HGLOBAL   m_hGlobal;	
	IPicture  *m_IPicture;
	LONG      m_Width, m_Height;

};

#endif // !defined(AFX_PICTURE_H__86578881_D242_4CD6_AF57_723581F6EB40__INCLUDED_)
