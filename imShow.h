#pragma once
#include "hData.h"

#include <vector>
class imgWnd
{
	struct	tsRect{
		COLORREF	color;
		long		width;
		RECT		rc;	
	};
	struct tsPoint{
		COLORREF	color;
		long		width;
		POINT		pt;
	};
public:
	imgWnd();
	~imgWnd();

	enum { gpLine = 1, gpRect = 2, gpEllipse = 3 };

	void Destroy();
	bool Create(CString strTitle);
	
	template <typename dType>
	void Input(dType *pData, long Width, long Height, long Channel, long Stride, double Normalized = 0.0f)
	{
		memset(&mViewRc, 0, sizeof(RECT));
		if(!m_hWnd || !pData || Width <= 0 || Height <= 0 || Channel <= 0)
			return;
		if(abs(Stride) < Width*Channel)		Stride = Width*Channel;
		//------------------------//
		if(!allocBmp(Width, Height))	return;
		//------------------------//
		normalize(pData, Width, Height, Channel, Stride, Normalized);
		SIZE sz = calcFrameSize();
		SetWindowPos(m_hWnd, NULL, CW_USEDEFAULT, CW_USEDEFAULT, sz.cx, sz.cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
		updateRoi();
		InvalidateRect(m_hWnd, NULL, TRUE);
	}
	template <typename dType>
	void Input(long ic, dType *pData, long Width, long Height, long Channel, long Stride, double Normalized = 0.0f)
	{
		if(!m_hWnd || !pData || Width <= 0 || Height <= 0 || Channel <= 0)
			return;
		if(abs(Stride) < Width*Channel)		Stride = Width*Channel;
		if(ic < 0)			ic = 0;
		if(ic >= Channel)	ic = Channel-1;
		//------------------------//
		if(Channel == 1)
			Input(pData, Width, Height, 1, Width, Normalized);
		else
		{
			long i, j;
			dType *pC = NULL;
			if((pC = new dType [Width*Height]) == NULL)	return;
			dType *pc = pC, *pD = pData, *pd;
			for(j = 0; j < Height; j++, pD+=Stride)
			{
				for(pd = pD+ic, i = 0; i < Width; i++, pd+=Channel)
					*pc++ = *pd;
			}
			Input(pC, Width, Height, 1, Width, Normalized);
			delete[] pC;
		}
	}
	template <typename dType>
	void Input(dType *pData, long Width, long Height, long Stride)
	{
		memset(&mViewRc, 0, sizeof(RECT));
		if(!m_hWnd || !pData || Width <= 0 || Height <= 0)
			return;
		if(abs(Stride) < Width)	Stride = Width;
		//------------------------//
		if(!allocBmp(Width, Height))	return;
		//------------------------//
		dType dMin, dMax;
		getLevel(dMin, dMax, pData, Width, Height, 1, Stride);
		double dNorm = dMin == dMax ? 1.0f : 255.0f/(dMax-dMin);
		//------------------------//
		unsigned char Jr[256], Jg[256], Jb[256];
		long i, j, r = 0, g = 0, b = 131;
		for(i = 0; i < 32; i++, b += 4)
		{	Jr[i] = (unsigned char)r;	Jg[i] = (unsigned char)g;	Jb[i] = (unsigned char)b;	}
		r = 0; g = 3; b = 255;
		for(i = 32; i < 96; i++, g+=4)
		{	Jr[i] = (unsigned char)r;	Jg[i] = (unsigned char)g;	Jb[i] = (unsigned char)b;	}
		r = 3; g = 255; b = 252;
		for(i = 96; i < 160; i++, r+=4, b-=4)
		{	Jr[i] = (unsigned char)r;	Jg[i] = (unsigned char)g;	Jb[i] = (unsigned char)b;	}
		r = 255; g = 252; b = 0;
		for(i = 160; i < 224; i++, g-=4)
		{	Jr[i] = (unsigned char)r;	Jg[i] = (unsigned char)g;	Jb[i] = (unsigned char)b;	}
		r = 252; g = 0; b = 0;
		for(i = 224; i < 256; i++, r-=4)
		{	Jr[i] = (unsigned char)r;	Jg[i] = (unsigned char)g;	Jb[i] = (unsigned char)b;	}
		//------------------------//
		dType *pD = pData, *pd;
		unsigned char *pB = (unsigned char *)mpRaw + mbmpInfo.bmiHeader.biSizeImage-mW4, *pb;
		for(j = 0; j < Height; j++, pB -= mW4, pD+=Stride)
		{
			for(pd = pD, pb = pB, i = 0; i < Width; i++, pd++)
			{
				long idx = (long)(((*pd)-dMin)*dNorm);
				if(idx < 0)		idx = 0;
				if(idx > 255)	idx = 255;
				*pb++ = Jb[idx];	*pb++ = Jg[idx];	*pb++ = Jr[idx];
			}
		}
		//------------------------//
		SIZE sz = calcFrameSize();
		SetWindowPos(m_hWnd, NULL, CW_USEDEFAULT, CW_USEDEFAULT, sz.cx, sz.cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
		updateRoi();
		InvalidateRect(m_hWnd, NULL, TRUE);
	}

	template <typename dType>
	void save(CString strFile, dType *pData, long Width, long Height, long Channel, long Stride, double Normalized = 0.0f)
	{
		if(strFile.IsEmpty() || !pData || Width <= 0 || Height <= 0 || Channel <= 0)
			return;
		if(abs(Stride) < Width*Channel)		Stride = Width*Channel;
		//------------------------//
		if(!allocBmp(Width, Height))	return;
		//------------------------//
		normalize(pData, Width, Height, Channel, Stride, Normalized);
		//------------------------//
		if(!mpRaw || mbmpInfo.bmiHeader.biWidth <= 0 || mbmpInfo.bmiHeader.biHeight <= 0)
			return;
		//write file
		CFile file;
		if(!file.Open(strFile, CFile::modeCreate | CFile::modeWrite))
			return;

		long PaletteSize = mbmpInfo.bmiHeader.biClrUsed*sizeof(RGBQUAD);
		BITMAPFILEHEADER hdr;
		hdr.bfType		= 0x4d42;	//'BM'
		hdr.bfOffBits	= sizeof(BITMAPFILEHEADER) + mbmpInfo.bmiHeader.biSize + PaletteSize;
		hdr.bfSize		= hdr.bfOffBits + mbmpInfo.bmiHeader.biSizeImage;
		hdr.bfReserved1 = 0;
		hdr.bfReserved2 = 0;

		file.Write(&hdr, sizeof(BITMAPFILEHEADER));
		file.Write(&mbmpInfo.bmiHeader, mbmpInfo.bmiHeader.biSize);
		if(PaletteSize != 0)
			file.Write(mrgbTable, PaletteSize);

		file.Write(mpRaw, mbmpInfo.bmiHeader.biSizeImage);

		file.Flush();
		file.Close();
	}
	
	void AddRoi(long type, RECT Roi,COLORREF color = RGB(255, 255, 0), long penWidth = 1);
	void SubRoi(long type, long index);
	void AddPoint(long x, long y, bool bCurve = false, COLORREF color = RGB(255, 255, 0), long penWidth = 1);
	void SubPoint(long index,  bool bCurve);
	void AddGrid(long xNum, long yNum, COLORREF color = RGB(255, 255, 0));
	//----------------------//
	//message
	void Draw();
	void EraseBkgnd();
	void Size();
	void Sizing();
	void RButtonDown(POINT pt);
	void MouseMove();
	void SaveBitmap();
	//----------------------//
	CString		m_title;
	HWND		m_hWnd;
	//----------------------//
private:
	bool allocBmp(long W, long H);
	template<typename dType>
	void normalize(dType *pData, long W, long H, long C, long Stride, double level = 0.0f)
	{
		double dMin = 0.0f, dNorm = level;
		if(level <= 0.0f)
		{
			dType din, dax;
			getLevel(din, dax, pData, W, H, C, Stride);
			dMin = din;
			dNorm = din == dax ? 1.0f : dax-din;
		}
		
		dNorm = 255.0f/dNorm;
		long i, j, k;
		dType *pD = pData, *pd;
		double dv;
		unsigned char *pB = (unsigned char *)mpRaw + mbmpInfo.bmiHeader.biSizeImage-mW4, *pb;
		for(pD = pData, j = 0; j < H; j++, pB -= mW4, pD += Stride)
		{
			for(pd = pD, pb = pB, i = 0; i < W; i++)
			{
				long nC = C > 3 ? 3 : C;
				for(k = 0; k < nC; k++, pb++, pd++)
				{
					dv = ((*pd)-dMin)*dNorm;
					if(dv > 255.0f)	dv = 255;
					if(dv < 0.0f)	dv = 0;
					*pb = (unsigned char)dv;
				}
				pd += (C - nC);
				for(; k < 3; k++, pb++)
					*pb = (unsigned char)dv;
			}
		}
	}
	template<typename dType>
	void getLevel(dType &dMin, dType &dMax, dType *pData, long W, long H, long C, long Stride)
	{
		long i, j, Line = W*C;
		dType *pD = pData, *pd = pD;
		dMin = *pd, dMax = *pd;
		for(j = 0; j < H; j++, pD+=Stride)
		{
			for(pd = pD, i = 0; i < Line; i++, pd++)
			{
				if((*pd) < dMin)	dMin = *pd;
				if((*pd) > dMax)	dMax = *pd;
			}
		}
	}
	void drawGraph(HDC hdc);
	SIZE calcFrameSize();
	void drawStatus(HDC hdc);
	void fillBitmapInfo(long Width, long Height, long bpp);
	void updateRoi();
	bool PtInRoi(POINT pt, RECT Roi);
	long convertToView(long ic, long vc);
	//----------------------//
	struct{	RECT Zoom, Size, Info;	}mStatusBar;

	HMENU		mhMenu;

	char		*mpRaw;
	BITMAPINFO	mbmpInfo;
	long		mW4;
	RGBQUAD		mrgbTable[256];

	bool		mbStatus;
	RECT		mViewRc;
	float		mZoom;

	std::vector<tsRect>		mvRect;
	std::vector<tsRect>		mvEllipse;
	std::vector<tsRect>		mvLine;
	std::vector<tsPoint>	mvPoint;
	std::vector<tsPoint>	mvCurve;

	#define		TIP_HEIGHT		20
};
//=============================================//
class imShow
{
public:
	imShow(void);
	~imShow(void);

	template<typename dType>
	void show(dType *pData, long Width, long Height, long Channel, long Stride, double Normalized, CString szWndTitle)
	{
		if(szWndTitle.IsEmpty())
			szWndTitle.Format(_T("Display %d"), mWndNum);

		imgWnd *pWnd = Create(szWndTitle);
		if(!pWnd)	return;

		pWnd->Input<dType>(pData, Width, Height, Channel, Stride, Normalized);
	}
	template<typename dType>
	void channel(long ic, dType *pData, long Width, long Height, long Channel, long Stride, double Normalized, CString szWndTitle)
	{
		if(szWndTitle.IsEmpty())
			szWndTitle.Format(_T("Display %d"), mWndNum);

		imgWnd	 *pWnd = Create(szWndTitle);
		if(!pWnd)	return;
		pWnd->Input<dType>(ic, pData, Width, Height, Channel, Stride, Normalized);
	}
	template<typename dType>
	void colormap(dType *pData, long Width, long Height, long Stride, CString szWndTitle)
	{
		if(szWndTitle.IsEmpty())
			szWndTitle.Format(_T("Display %d"), mWndNum);

		imgWnd	 *pWnd = Create(szWndTitle);
		if(!pWnd)	return;
		pWnd->Input<dType>(pData, Width, Height, Stride);
	}

	template<typename dType>
	void save(CString strFile, dType *pData, long Width, long Height, long Channel, long Stride, double Normalized, CString szWndTitle)
	{
		if(szWndTitle.IsEmpty())
			szWndTitle.Format(_T("Display %d"), mWndNum);

		imgWnd	 *pWnd = Create(szWndTitle);
		if(!pWnd)	return;
		pWnd->save<dType>(strFile, pData, Width, Height, Channel, Stride, Normalized);
	}

	void DrawRect(CString szWndTitle, RECT Rect, bool bEllipse = false, COLORREF color = RGB(255, 255, 0), long penWidth = 1);
	void ClearRect(CString szWndTitle);

	void DrawLine(CString szWndTitle, long x0, long y0, long x1, long y1, COLORREF color = RGB(255, 255, 0), long penWidth = 1);
	void DrawGrid(CString szWndTitle, long xNum, long yNum, COLORREF color = RGB(255, 255, 0));
	void ClearLine(CString szWndTitle);

	void DrawPoint(CString szWndTitle, long x, long y, COLORREF color = RGB(255, 255, 0), long penWidth = 1);
	void ClearPoint(CString szWndTitle);

	void DrawProjection(CString szWndTitle, float *pProj, long Length, bool bHor, RECT Rect, COLORREF color = RGB(255, 255, 0));
	void ClearProjection(CString szWndTitle);

	long waitKey(long delay);

private:
	imgWnd* Create(CString szWndTitle);
	long GetWindowIndex(CString strTitle);
	void Destroy(long id);

	imgWnd **mppWnd;
	long	mWndId;
	long	mWndNum;

	#define	DEFAULT_WND_NUM		32
};

long hWaitKey(long delay = 0);

void hShow(char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hShow(unsigned char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hShow(short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hShow(unsigned short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hShow(int *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hShow(unsigned int *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hShow(long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hShow(unsigned long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hShow(float *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hShow(double *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);

void hShow(hData<char> *pData, double Normalized, CString szWndTitle);
void hShow(hData<unsigned char> *pData, double Normalized, CString szWndTitle);
void hShow(hData<short> *pData, double Normalized, CString szWndTitle);
void hShow(hData<unsigned short> *pData, double Normalized, CString szWndTitle);
void hShow(hData<int> *pData, double Normalized, CString szWndTitle);
void hShow(hData<unsigned int> *pData, double Normalized, CString szWndTitle);
void hShow(hData<long> *pData, double Normalized, CString szWndTitle);
void hShow(hData<unsigned long> *pData, double Normalized, CString szWndTitle);
void hShow(hData<float> *pData, double Normalized, CString szWndTitle);
void hShow(hData<double> *pData, double Normalized, CString szWndTitle);

void hColormap(char *pData, long W, long H, long Stride, CString szWndTitle);
void hColormap(unsigned char *pData, long W, long H, long Stride, CString szWndTitle);
void hColormap(short *pData, long W, long H, long Stride, CString szWndTitle);
void hColormap(unsigned short *pData, long W, long H, long Stride, CString szWndTitle);
void hColormap(int *pData, long W, long H, long Stride, CString szWndTitle);
void hColormap(unsigned int *pData, long W, long H, long Stride, CString szWndTitle);
void hColormap(long *pData, long W, long H, long Stride, CString szWndTitle);
void hColormap(unsigned long *pData, long W, long H, long Stride, CString szWndTitle);
void hColormap(float *pData, long W, long H, long Stride, CString szWndTitle);
void hColormap(double *pData, long W, long H, long Stride, CString szWndTitle);

void hColormap(hData<char> *pData, CString szWndTitle);
void hColormap(hData<unsigned char> *pData, CString szWndTitle);
void hColormap(hData<short> *pData, CString szWndTitle);
void hColormap(hData<unsigned short> *pData, CString szWndTitle);
void hColormap(hData<int> *pData, CString szWndTitle);
void hColormap(hData<unsigned int> *pData, CString szWndTitle);
void hColormap(hData<long> *pData, CString szWndTitle);
void hColormap(hData<unsigned long> *pData, CString szWndTitle);
void hColormap(hData<float> *pData, CString szWndTitle);
void hColormap(hData<double> *pData, CString szWndTitle);

void hChannel(long ic, char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hChannel(long ic, unsigned char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hChannel(long ic, short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hChannel(long ic, unsigned short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hChannel(long ic, long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hChannel(long ic, unsigned long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hChannel(long ic, float *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hChannel(long ic, double *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);

void hSave(CString strFile, char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hSave(CString strFile, unsigned char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hSave(CString strFile, short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hSave(CString strFile, unsigned short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hSave(CString strFile, long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hSave(CString strFile, unsigned long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hSave(CString strFile, float *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);
void hSave(CString strFile, double *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle);

void hClear(CString szWndTitle);
void hDrawRect(CString szWndTitle, RECT Rect, bool bEllipse = false, COLORREF color = RGB(255, 255, 0), long penWidth = 1);
void hClearRect(CString szWndTitle);
void hDrawLine(CString szWndTitle, long x0, long y0, long x1, long y1, COLORREF color = RGB(255, 255, 0), long penWidth = 1);
void hClearLine(CString szWndTitle);
void hDrawPoint(CString szWndTitle, long x, long y, COLORREF color = RGB(255, 255, 0), long penWidth = 1);
void hClearPoint(CString szWndTitle);
void hDrawProject(CString szWndTitle, float *pProj, long Length, bool bHor, RECT Rect, COLORREF color = RGB(255, 255, 0));
void hClearProjection(CString szWndTitle);