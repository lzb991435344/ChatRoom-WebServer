/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef __CAPTURE_VIDEO_H__2009__
#define __CAPTURE_VIDEO_H__2009__

#if _MSC_VER > 1000
#pragma once
#endif

#include <atlbase.h>
#include <Windows.h>
#include <DShow.h>
//#include <streams.h>
#include <qedit.h>
#include "../../kms/km_head.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x){if(x != NULL) x->Release();x = NULL ;}
#endif

class SampleGrabberCallback ;

class CCaptureVideo
{
	friend class SampleGrabberCallback;

public:
	CCaptureVideo();
	virtual ~CCaptureVideo();
	void     GrabOneFrame(BOOL bGrab);
	HRESULT  Init(int iDeviceID, pio_socket i_socket, HWND hWnd, HWND hRoot,int width, int heigth);
	int      EnumDevices(char *buffer);
	void     stop();

private:
	HWND             m_hWnd;
	CComPtr<ISampleGrabber> m_pGrabber;

	CComPtr< IGraphBuilder > m_pGB;
	CComPtr< ICaptureGraphBuilder2 > m_pCapture;
	CComPtr< IBaseFilter > m_pBF;
	CComPtr< IMediaControl > m_pMC;
	CComPtr< IVideoWindow > m_pVW;

protected:
	void  FreeMediaType(AM_MEDIA_TYPE& mt);
	bool  BindFilter(int deviceId, IBaseFilter** pFilter);
	void  ResizeVideoWindow();

	HRESULT SetupVideoWindow();
	HRESULT InitCaptureGraphBuilder();
};
#endif