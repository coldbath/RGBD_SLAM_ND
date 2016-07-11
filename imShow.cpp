#include "StdAfx.h"
#include "imShow.h"

const HBRUSH _ghcyBrhBack	= CreateHatchBrush(HS_DIAGCROSS, RGB(204, 204, 204));
const HBRUSH _ghcyBrhStatus = CreateSolidBrush(RGB(236, 233, 216));

imgWnd::imgWnd()
{
	mW4 = 0;
	mbStatus = true;
	memset(&mViewRc, 0, sizeof(RECT));
	memset(&mStatusBar, 0, sizeof(mStatusBar));
	m_title.Empty();
	mpRaw	= NULL;
	mhMenu	= NULL;
	m_hWnd	= NULL;
	memset(&mbmpInfo, 0, sizeof(BITMAPINFO));

	for(long i = 0; i < 256; i++)
	{
		mrgbTable[i].rgbBlue = mrgbTable[i].rgbGreen = mrgbTable[i].rgbRed = (unsigned char)(i & 0xFF);
		mrgbTable[i].rgbReserved = 0;
	}
}
imgWnd::~imgWnd()	{	Destroy();	}

void imgWnd::Destroy()
{
	mvRect.clear();	mvEllipse.clear();	mvLine.clear();	mvPoint.clear(); mvCurve.clear();

	mW4 = 0;
	memset(&mViewRc, 0, sizeof(RECT));
	m_title.Empty();
	memset(&mbmpInfo, 0, sizeof(BITMAPINFO));
	if(mpRaw) { delete[] mpRaw; mpRaw	= NULL; }

	if(mhMenu)	DestroyMenu(mhMenu);					mhMenu = NULL;
	if(m_hWnd) 	::SendMessage(m_hWnd, WM_CLOSE, 0, 0);	m_hWnd	= NULL;
}
bool imgWnd::Create(CString strTitle)
{
	if(strTitle.IsEmpty())
		return false;
	if(strTitle == m_title && m_hWnd)
		return true;

	mvRect.clear();	mvEllipse.clear();	mvPoint.clear(); mvCurve.clear();

	Destroy();
	m_title = strTitle;
	m_hWnd = CreateWindow(_T("_hcy_gerneral_data_show_window_"),
		m_title,
		WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_OVERLAPPED | WS_SIZEBOX,
		CW_USEDEFAULT,	CW_USEDEFAULT,	10, 10,
		NULL,	NULL,	NULL,	NULL);
	if(!m_hWnd)
	{
		MessageBox(NULL, _T("Can not create Window!"), _T("Information!"), MB_ICONERROR);
		return false;
	}
	SetWindowLong(m_hWnd, -21, (LONG)this);	//GWL_USERDATA

	mhMenu = CreatePopupMenu();
	if(mhMenu)	InsertMenu(mhMenu, 0, MF_BYPOSITION, WM_USER+1234, _T("Save Bitmap..."));

	return true;
}
void imgWnd::AddGrid(long xNum, long yNum, COLORREF color)
{
	if(xNum > 1)
	{
		float step = (float)(mbmpInfo.bmiHeader.biWidth-0.5f)/xNum, w = step;
		for(int i = 1; i < xNum; i++, w+=step)
		{
			int x = (int)(w+0.5f);
			AddRoi(gpLine, CRect(x, 0, x, mbmpInfo.bmiHeader.biHeight), color);
		}
	}
	if(yNum > 1)
	{
		float step = (float)(mbmpInfo.bmiHeader.biHeight-0.5f)/yNum, h = step;
		for(int i = 1; i < yNum; i++, h+=step)
		{
			int y = (int)(h+0.5f);
			AddRoi(gpLine, CRect(0, y, mbmpInfo.bmiHeader.biWidth, y), color);
		}
	}
}
void imgWnd::AddRoi(long type, RECT rc, COLORREF color, long penWidth)
{
	if(mbmpInfo.bmiHeader.biWidth <= 0 || mbmpInfo.bmiHeader.biHeight <= 0)
		return;
	if(rc.left < 0)								rc.left = 0;
	if(rc.left > mbmpInfo.bmiHeader.biWidth)	rc.left = mbmpInfo.bmiHeader.biWidth;
	if(rc.top < 0)								rc.top = 0;
	if(rc.top > mbmpInfo.bmiHeader.biHeight)	rc.top = mbmpInfo.bmiHeader.biHeight;
	if(rc.right < 0)							rc.right = 0;
	if(rc.right > mbmpInfo.bmiHeader.biWidth)	rc.right = mbmpInfo.bmiHeader.biWidth;
	if(rc.bottom < 0)							rc.bottom = 0;
	if(rc.bottom > mbmpInfo.bmiHeader.biHeight)	rc.bottom = mbmpInfo.bmiHeader.biHeight;

	tsRect ts;
	ts.rc = rc; ts.color = color; ts.width = penWidth;
	switch(type)
	{
	case gpLine:	mvLine.push_back(ts);		break;
	case gpRect:	mvRect.push_back(ts);		break;
	case gpEllipse:	mvEllipse.push_back(ts);	break;
	}
}
void imgWnd::SubRoi(long type, long index)
{
	switch(type)
	{
	case gpLine:	(index < 0 || index >= (long)mvLine.size()) ? mvLine.clear() : mvLine.erase(mvLine.begin()+index);				break;
	case gpRect:	(index < 0 || index >= (long)mvRect.size()) ? mvRect.clear() : mvRect.erase(mvRect.begin()+index);				break;
	case gpEllipse:	(index < 0 || index >= (long)mvEllipse.size()) ? mvEllipse.clear() : mvEllipse.erase(mvEllipse.begin()+index);	break;
	}
}
void imgWnd::AddPoint(long x, long y,  bool bCurve, COLORREF color, long penWidth)
{
	if(mbmpInfo.bmiHeader.biWidth <= 0 || mbmpInfo.bmiHeader.biHeight <= 0)
		return;

	if(x < 0)	x = 0;	if(x > mbmpInfo.bmiHeader.biWidth)	x = mbmpInfo.bmiHeader.biWidth;
	if(y < 0)	y = 0;	if(y > mbmpInfo.bmiHeader.biHeight) y = mbmpInfo.bmiHeader.biHeight;

	if(bCurve)
	{
		tsPoint ts;
		ts.pt.x = x; ts.pt.y = y; ts.width = penWidth; ts.color = color;
		mvCurve.push_back(ts);
	}
	else
	{
		tsPoint ts;
		ts.pt.x = x; ts.pt.y = y; ts.width = penWidth; ts.color = color;
		mvPoint.push_back(ts);
	}
}
void imgWnd::SubPoint(long index, bool bCurve)
{
	if(bCurve)
	{
		if(index >= 0 && index < (long)mvCurve.size())
			mvCurve.erase(mvCurve.begin()+index);
		else
			mvCurve.clear();
	}
	else
	{
		if(index >= 0 && index < (long)mvPoint.size())
			mvPoint.erase(mvPoint.begin()+index);
		else
			mvPoint.clear();
	}
}
void imgWnd::Draw()
{
	if(!m_hWnd)	return;

	PAINTSTRUCT pt;
	HDC hdc = BeginPaint(m_hWnd, &pt);
	if(mpRaw)
	{
		int saveBltMode = SetStretchBltMode(hdc, COLORONCOLOR);

		if(mbmpInfo.bmiHeader.biBitCount == 8)
			SetDIBColorTable(hdc, 0, 256, mrgbTable);

		StretchDIBits(hdc, mViewRc.left, mViewRc.top, mViewRc.right, mViewRc.bottom, 0, 0, mbmpInfo.bmiHeader.biWidth, 
			mbmpInfo.bmiHeader.biHeight, (void *)mpRaw, &mbmpInfo, DIB_RGB_COLORS, SRCCOPY);

		SetStretchBltMode(hdc, saveBltMode);

		drawGraph(hdc);
		drawStatus(hdc);
	}
	EndPaint(m_hWnd, &pt);
}
void imgWnd::EraseBkgnd()
{
	if(!m_hWnd)
		return;

	PAINTSTRUCT pt;
	HDC hdc = BeginPaint(m_hWnd, &pt);

	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	rcClient.bottom -= TIP_HEIGHT;

	HRGN rgn0 = CreateRectRgnIndirect(&rcClient);
	HRGN rgn1 = CreateRectRgn(mViewRc.left, mViewRc.top, mViewRc.left+mViewRc.right, mViewRc.top+mViewRc.bottom);
	CombineRgn(rgn0, rgn0, rgn1, RGN_XOR);

	FillRgn(hdc, rgn0, _ghcyBrhBack);

	DeleteObject(rgn0);
	DeleteObject(rgn1);

	drawStatus(hdc);

	EndPaint(m_hWnd, &pt);
}
void imgWnd::Size()
{
	updateRoi();
	InvalidateRect(m_hWnd, NULL, TRUE);
}
void imgWnd::Sizing()
{
	updateRoi();
	InvalidateRect(m_hWnd, NULL, TRUE);
	UpdateWindow(m_hWnd);
}
void imgWnd::RButtonDown(POINT pt)
{
	if(!PtInRoi(pt, mViewRc))	return;
	ClientToScreen(m_hWnd, &pt);
	if(mhMenu)	TrackPopupMenu(mhMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
}
void imgWnd::MouseMove()
{
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	rcClient.top = rcClient.bottom - TIP_HEIGHT;
	rcClient.left += 130;
	InvalidateRect(m_hWnd, &rcClient, FALSE);
}
void imgWnd::SaveBitmap()
{
	if(!mpRaw || mbmpInfo.bmiHeader.biWidth <= 0 || mbmpInfo.bmiHeader.biHeight <= 0)
		return;

	CFileDialog fdlg(FALSE, _T("bmp"), _T(""), OFN_HIDEREADONLY, _T("Bitmap file (*.bmp)|*.bmp|"), CWnd::FromHandle(m_hWnd));
	if(fdlg.DoModal() == IDCANCEL)
		return;
	CString strFileName = fdlg.GetFileName();
	//write file
	CFile file;
	if(!file.Open(strFileName, CFile::modeCreate | CFile::modeWrite))
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
bool imgWnd::allocBmp(long W, long H)
{
	long bpp = 24;//Channel*8;
	if(W != mbmpInfo.bmiHeader.biWidth || H != mbmpInfo.bmiHeader.biHeight || mbmpInfo.bmiHeader.biBitCount != bpp)
	{
		if(mpRaw) { delete[] mpRaw; mpRaw	= NULL; }
		fillBitmapInfo(W, H, bpp);
		mpRaw = new char [mbmpInfo.bmiHeader.biSizeImage];
	}
	return mpRaw != NULL;
}
void imgWnd::drawGraph(HDC hdc)
{
	if(!hdc || mvRect.empty() && mvEllipse.empty() && mvLine.empty() && mvPoint.empty() && mvCurve.empty())
		return;

	size_t i;
	long left, top, right, bottom;
	HGDIOBJ hOld = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	HPEN hPen;
	//rect
	for(i = 0; i < mvRect.size(); i++)
	{
		left	= convertToView(mvRect[i].rc.left, mViewRc.left);
		top		= convertToView(mvRect[i].rc.top, mViewRc.top);
		right	= convertToView(mvRect[i].rc.right, mViewRc.left);
		bottom	= convertToView(mvRect[i].rc.bottom, mViewRc.top);

		hPen = CreatePen(PS_SOLID, mvRect[i].width, mvRect[i].color);
		SelectObject(hdc, (HGDIOBJ)hPen);
		Rectangle(hdc, left, top, right, bottom);
		DeleteObject(hPen);
	}
	//ellipse
	for(i = 0; i < mvEllipse.size(); i++)
	{
		left	= convertToView(mvEllipse[i].rc.left, mViewRc.left);
		top		= convertToView(mvEllipse[i].rc.top, mViewRc.top);
		right	= convertToView(mvEllipse[i].rc.right, mViewRc.left);
		bottom	= convertToView(mvEllipse[i].rc.bottom, mViewRc.top);

		hPen = CreatePen(PS_SOLID, mvEllipse[i].width, mvEllipse[i].color);
		SelectObject(hdc, (HGDIOBJ)hPen);
		Ellipse(hdc, left, top, right, bottom);
		DeleteObject(hPen);
	}
	//line
	for(i = 0; i < mvLine.size(); i++)
	{
		left	= convertToView(mvLine[i].rc.left, mViewRc.left);
		top		= convertToView(mvLine[i].rc.top, mViewRc.top);
		right	= convertToView(mvLine[i].rc.right, mViewRc.left);
		bottom	= convertToView(mvLine[i].rc.bottom, mViewRc.top);

		hPen = CreatePen(PS_SOLID, mvLine[i].width, mvLine[i].color);
		SelectObject(hdc, (HGDIOBJ)hPen);
		MoveToEx(hdc, left, top, NULL);
		LineTo(hdc, right, bottom);
		DeleteObject(hPen);
	}
	//point
	for(i = 0; i < mvPoint.size(); i++)
	{
		left	= convertToView(mvPoint[i].pt.x, mViewRc.left);
		top		= convertToView(mvPoint[i].pt.y, mViewRc.top);

		hPen = CreatePen(PS_SOLID, mvPoint[i].width, mvPoint[i].color);
		SelectObject(hdc, (HGDIOBJ)hPen);
		MoveToEx(hdc, left-1, top-1, NULL);
		LineTo(hdc, left+2, top+2);
		MoveToEx(hdc, left+1, top-1, NULL);
		LineTo(hdc, left-2, top+2);
		DeleteObject(hPen);
	}
	//curve
	if(mvCurve.size() > 0)
	{
		left	= convertToView(mvCurve[0].pt.x, mViewRc.left);
		top		= convertToView(mvCurve[0].pt.y, mViewRc.top);
		MoveToEx(hdc, left, top, NULL);
		for(i = 1; i < mvCurve.size(); i++)
		{
			left	= convertToView(mvCurve[i].pt.x, mViewRc.left);
			top		= convertToView(mvCurve[i].pt.y, mViewRc.top);

			hPen = CreatePen(PS_SOLID, mvCurve[i-1].width, mvCurve[i-1].color);
			SelectObject(hdc, (HGDIOBJ)hPen);
			LineTo(hdc, left, top);
			DeleteObject(hPen);
		}
	}
	SelectObject(hdc, hOld);
}
SIZE imgWnd::calcFrameSize()
{
	SIZE sz;
	sz.cx = 50;
	sz.cy = 50;
	if(!mpRaw)
		return sz;

	int Width  = GetSystemMetrics(SM_CXFULLSCREEN)*4/5; 
	int Height = GetSystemMetrics(SM_CYFULLSCREEN)*4/5; 
	float zoom = 1.0f;
	if(mbmpInfo.bmiHeader.biWidth > Width || mbmpInfo.bmiHeader.biHeight > Height)
	{
		float zx = (float)Width/mbmpInfo.bmiHeader.biWidth;
		float zy = (float)Height/mbmpInfo.bmiHeader.biHeight;
		zoom = zx < zy ? zx : zy;
	}
	RECT rect;
	rect.top	= 0;
	rect.left	= 0;
	rect.right	= (long)(mbmpInfo.bmiHeader.biWidth*zoom);
	rect.bottom = (long)(mbmpInfo.bmiHeader.biHeight*zoom);
	::AdjustWindowRectEx(&rect, WS_CAPTION | WS_SIZEBOX, TRUE, 0);
	sz.cx = rect.right - rect.left;
	sz.cy = rect.bottom - rect.top;

	return sz;
}
void imgWnd::drawStatus(HDC hdc)
{
	if(!mbStatus || !hdc)
		return;

	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	rcClient.top = rcClient.bottom - TIP_HEIGHT;
	FillRect(hdc, &rcClient, _ghcyBrhStatus);

	HPEN hPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
	HGDIOBJ hOld;
	hOld = SelectObject(hdc, (HGDIOBJ)hPen);
	MoveToEx(hdc, rcClient.left, rcClient.top, NULL);
	LineTo(hdc, rcClient.right, rcClient.top);
	SelectObject(hdc, hOld);
	DeleteObject(hPen);

	rcClient.top	+= 1;
	hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	hOld = SelectObject(hdc, (HGDIOBJ)hPen);
	MoveToEx(hdc, rcClient.left, rcClient.top, NULL);
	LineTo(hdc, rcClient.right, rcClient.top);
	SelectObject(hdc, hOld);
	DeleteObject(hPen);

	rcClient.top    += 2;
	rcClient.bottom -= 1;
	hPen = CreatePen(PS_SOLID, 1, RGB(172, 168, 153));
	hOld = SelectObject(hdc, (HGDIOBJ)hPen);

	long of = 4;
	mStatusBar.Zoom.left	= 2;
	mStatusBar.Zoom.top		= rcClient.top;
	mStatusBar.Zoom.right	= mStatusBar.Zoom.left + 60;
	mStatusBar.Zoom.bottom	= rcClient.bottom;
	MoveToEx(hdc, mStatusBar.Zoom.right+1, mStatusBar.Zoom.top, NULL);	
	LineTo(hdc, mStatusBar.Zoom.right+1, mStatusBar.Zoom.bottom);

	mStatusBar.Size.left	= mStatusBar.Zoom.right+of;
	mStatusBar.Size.top		= rcClient.top;
	mStatusBar.Size.right	= mStatusBar.Size.left + 70;
	mStatusBar.Size.bottom	= rcClient.bottom;
	MoveToEx(hdc, mStatusBar.Size.right+1, mStatusBar.Size.top, NULL);	
	LineTo(hdc, mStatusBar.Size.right+1, mStatusBar.Size.bottom);

	mStatusBar.Info.left	= mStatusBar.Size.right+of;
	mStatusBar.Info.top		= rcClient.top;
	mStatusBar.Info.right	= rcClient.right;
	mStatusBar.Info.bottom	= rcClient.bottom;

	SelectObject(hdc, hOld);
	DeleteObject(hPen);

	CString strItem;
	int nBkMode = SetBkMode(hdc, TRANSPARENT);
	hOld = SelectObject(hdc, GetStockObject(ANSI_VAR_FONT));

	if(mZoom > 0.0f)	strItem.Format(_T("%.2f%%"), mZoom*100);
	else				strItem = _T("zoom");
	DrawText(hdc, strItem, strItem.GetLength(), &mStatusBar.Zoom, DT_LEFT);

	if(mbmpInfo.bmiHeader.biWidth > 0 && mbmpInfo.bmiHeader.biHeight > 0)
		strItem.Format(_T("%d x %d"), mbmpInfo.bmiHeader.biWidth, mbmpInfo.bmiHeader.biHeight);	
	else
		strItem = _T("w x h");							
	DrawText(hdc, strItem, strItem.GetLength(), &mStatusBar.Size, DT_LEFT);

	POINT pt;
	::GetCursorPos(&pt);
	ScreenToClient(m_hWnd, &pt);

	long x = -1, y = -1;
	if(PtInRoi(pt, mViewRc) && mZoom > 0.0f)
	{
		x = (long)((pt.x - mViewRc.left)/mZoom);
		y = (long)((pt.y - mViewRc.top)/mZoom);
	}
	if(!mpRaw || x < 0 || y < 0 || x >= mbmpInfo.bmiHeader.biWidth || y >= mbmpInfo.bmiHeader.biHeight)
		strItem = _T("[x, y]  <pixel value>");
	else
	{
		long C = mbmpInfo.bmiHeader.biBitCount>>3;
		unsigned char *ptr = (unsigned char*)mpRaw + (mbmpInfo.bmiHeader.biHeight-1-y)*mW4 + x*C;
		switch(C)
		{
		case 1:
			strItem.Format(_T("[%d, %d] Gray:%d"), x, y, ptr[0]);
			break;
		case 3:
			strItem.Format(_T("[%d, %d] RGB:%d, %d, %d"), x, y, ptr[2], ptr[1], ptr[0]);
			break;
		case 4:
			strItem.Format(_T("[%d, %d] ARGB:%d, %d, %d, %d"), x, y, ptr[3], ptr[2], ptr[1], ptr[0]);
			break;
		}
	}

	DrawText(hdc, strItem, strItem.GetLength(), &mStatusBar.Info, DT_CENTER);

	SelectObject(hdc, hOld);
	SetBkMode(hdc, nBkMode);
}
void imgWnd::fillBitmapInfo(long Width, long Height, long bpp)
{
	mW4 = ((((bpp*Width) + 31)/32)*4);
	memset(&mbmpInfo, 0, sizeof(BITMAPINFO));
	mbmpInfo.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	mbmpInfo.bmiHeader.biWidth			= Width;
	mbmpInfo.bmiHeader.biHeight			= Height;
	mbmpInfo.bmiHeader.biPlanes			= 1;
	mbmpInfo.bmiHeader.biBitCount		= (unsigned short)bpp;
	mbmpInfo.bmiHeader.biCompression	= BI_RGB;
	mbmpInfo.bmiHeader.biXPelsPerMeter	= 100;
	mbmpInfo.bmiHeader.biYPelsPerMeter	= 100;
	mbmpInfo.bmiHeader.biClrUsed		= bpp > 8 ? 0  : 256;
	mbmpInfo.bmiHeader.biSizeImage		=  mW4 * Height;
}
void imgWnd::updateRoi()
{
	if(!m_hWnd || !mpRaw)
		return;

	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);

	rcClient.bottom -= TIP_HEIGHT;
	float fzx = (float)rcClient.right/mbmpInfo.bmiHeader.biWidth;
	float fzy = (float)rcClient.bottom/mbmpInfo.bmiHeader.biHeight;
	mZoom			= fzx < fzy ? fzx : fzy;
	mViewRc.right	= (long)(mZoom * mbmpInfo.bmiHeader.biWidth);
	mViewRc.bottom	= (long)(mZoom * mbmpInfo.bmiHeader.biHeight);
	mViewRc.left	= rcClient.left + ((rcClient.right - mViewRc.right)>>1);
	mViewRc.top		= rcClient.top + ((rcClient.bottom - mViewRc.bottom)>>1);
}
bool imgWnd::PtInRoi(POINT pt, RECT Roi)
{
	if(pt.x < Roi.left || pt.x >= Roi.left+Roi.right
	|| pt.y < Roi.top || pt.y >= Roi.top + Roi.bottom)
		return false;
	return true; 
}
long imgWnd::convertToView(long ic, long vc)
{
	if(mZoom <= 0.0f)
		return 0;
	return mZoom <= 0.0f ? 0 : vc + (long)(ic*mZoom);
}
//================================================//
static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	imgWnd *pWnd = (imgWnd*)(GetWindowLong(hwnd, -21));//GWL_USERDATA
	if(pWnd)
	{
		switch(msg)
		{
		case WM_DESTROY:		pWnd->Destroy();							break;
		case WM_ERASEBKGND:		pWnd->EraseBkgnd();							return TRUE;
		case WM_PAINT:			pWnd->Draw();								break;
		case WM_SIZING:			pWnd->Sizing();								break;
		case WM_SIZE:			pWnd->Size();								break;
		case WM_MOUSEMOVE:		pWnd->MouseMove();							break;
		case WM_ACTIVATE:		InvalidateRect(pWnd->m_hWnd, NULL, TRUE);	break;
		case WM_NCPAINT:		UpdateWindow(pWnd->m_hWnd);					
								InvalidateRect(pWnd->m_hWnd, NULL, TRUE);	break;
		case WM_RBUTTONDOWN:	
			{	POINT pt; pt.x = LOWORD(lParam);	pt.y = HIWORD(lParam);
			pWnd->RButtonDown(pt);											break;	}
		case WM_COMMAND:
			{	if(WM_USER+1234 == LOWORD(wParam))
				pWnd->SaveBitmap();											break;	}
		default:															break;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

imShow::imShow(void)
{
	mppWnd = NULL;
	mppWnd = (imgWnd **)malloc(DEFAULT_WND_NUM*sizeof(imgWnd*));
	mWndId = mWndNum = 0;
	if(mppWnd)
		mWndNum	= DEFAULT_WND_NUM;

	static int wasInitialized = 0;
	if(mppWnd && !wasInitialized)
	{
		WNDCLASS wc;
		wc.style			= CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
		wc.lpfnWndProc		= wndProc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= NULL;
		wc.lpszClassName	= _T("_hcy_gerneral_data_show_window_");
		wc.lpszMenuName		= NULL;
		wc.hIcon			= NULL;//LoadIcon(0, IDI_APPLICATION);
		wc.hCursor			= LoadCursor(NULL, IDC_CROSS);
		wc.hbrBackground	= NULL; //CreateHatchBrush(HS_DIAGCROSS, RGB(204, 204, 204));
		if(!RegisterClass(&wc))
		{
			MessageBox(NULL, _T("Can not register view!"), _T("Information!"), MB_ICONERROR);
			return;
		}
		wasInitialized = 1;
	}
}

imShow::~imShow(void)
{
	Destroy(-1);	free(mppWnd);
}

imgWnd* imShow::Create(CString szWndTitle)
{
	if(szWndTitle.IsEmpty())
	{
		MessageBox(NULL, _T("Please give a name for window!"), _T("Information!"), MB_ICONERROR);
		return NULL;
	}
	long id = GetWindowIndex(szWndTitle);
	if(id >= 0)
		return mppWnd[id];

	if(mWndId >= mWndNum)
	{
		if((mppWnd = (imgWnd **)realloc(mppWnd, (mWndNum+DEFAULT_WND_NUM)*sizeof(imgWnd*))) != NULL)
			mWndNum += DEFAULT_WND_NUM;
	}
	if(!mppWnd)		return NULL;

	imgWnd *pWnd = NULL;
	pWnd = new imgWnd;
	if(!pWnd)	return NULL;

	if(pWnd->Create(szWndTitle))
		mppWnd[mWndId++] = pWnd;
	else
	{	delete pWnd; pWnd = NULL;	}
	return pWnd;
}
long imShow::GetWindowIndex(CString strTitle)
{
	if(strTitle.IsEmpty())
		return -1;

	long id;
	bool bFound = false;
	for(id = 0; id < mWndId; id++)
	{
		bFound = mppWnd[id]->m_title == strTitle;
		if(bFound)
			break;
	}
	return bFound ? (long)id : -1;
}
void imShow::Destroy(long id)
{
	long i;
	if(id >= 0 && id < mWndId)
	{
		delete mppWnd[id];
		mWndId--;
		for(i = id; i < mWndId; i++)
			*mppWnd[id] = *mppWnd[id+1];
	}
	else
	{
		for(i = 0; i < mWndId; i++)
			delete mppWnd[i];
		mWndId = 0;
	}
}
void imShow::DrawRect(CString szWndTitle, RECT Rect, bool bEllipse, COLORREF color, long penWidth)
{
	long id = GetWindowIndex(szWndTitle);
	if(id < 0)	return;
	mppWnd[id]->AddRoi(bEllipse ? imgWnd::gpEllipse : imgWnd::gpRect, Rect, color, penWidth);
}
void imShow::ClearRect(CString szWndTitle)
{
	long id = GetWindowIndex(szWndTitle);
	if(id < 0)	return;
	mppWnd[id]->SubRoi(imgWnd::gpEllipse, -1);
	mppWnd[id]->SubRoi(imgWnd::gpRect, -1);
}

void imShow::DrawLine(CString szWndTitle, long x0, long y0, long x1, long y1, COLORREF color, long penWidth)
{
	long id = GetWindowIndex(szWndTitle);
	if(id < 0)	return;
	mppWnd[id]->AddRoi(imgWnd::gpLine, CRect(x0, y0, x1, y1), color, penWidth);
}
void imShow::DrawGrid(CString szWndTitle, long xNum, long yNum, COLORREF color)
{
	long id = GetWindowIndex(szWndTitle);
	if(id < 0)	return;
	mppWnd[id]->AddGrid(xNum, yNum, color);
}
void imShow::ClearLine(CString szWndTitle)
{
	long id = GetWindowIndex(szWndTitle);
	if(id < 0)	return;
	mppWnd[id]->SubRoi(imgWnd::gpLine, -1);
}
void imShow::DrawPoint(CString szWndTitle, long x, long y, COLORREF color, long penWidth)
{
	long id = GetWindowIndex(szWndTitle);
	if(id < 0)	return;
	mppWnd[id]->AddPoint(x, y, false, color, penWidth);
}
void imShow::ClearPoint(CString szWndTitle)
{
	long id = GetWindowIndex(szWndTitle);
	if(id < 0)	return;
	mppWnd[id]->SubPoint(-1, false);
}

void imShow::DrawProjection(CString szWndTitle, float *pProj, long Length, bool bHor, RECT Rect, COLORREF color)
{
	if(!pProj || Length <= 0 || Rect.left < 0 || Rect.top < 0 || Rect.right <= Rect.left || Rect.bottom <= Rect.top)
		return;

	long id = GetWindowIndex(szWndTitle);
	if(id < 0)	return;

	long i;
	float *pp = pProj;
	float dMin = *pp, dMax = *pp;
	for(i = 0; i < Length; i++, pp++)
	{
		if(*pp < dMin)	dMin = (*pp);
		if(*pp > dMax)	dMax = (*pp);
	}
	if(dMin == dMax)
		return;

	long nw = Rect.right - Rect.left;
	long nh = Rect.bottom - Rect.top;
	float fPeak = (float)(0.9f * (bHor ? nh : nw) / (dMax - dMin+1.0f));
	if(bHor)
	{
		for(pp = pProj, i = 0; i < Length; i++, pp++)
			mppWnd[id]->AddPoint(i+Rect.left, Rect.top + nh - (long)(((*pp) - dMin) * fPeak), true, color);
	}
	else
	{
		for(pp = pProj, i = 0; i < Length; i++, pp++)
			mppWnd[id]->AddPoint(Rect.left + nw - (long)(((*pp) - dMin) * fPeak), i+Rect.top, true, color);
	}
}
void imShow::ClearProjection(CString szWndTitle)
{
	long id = GetWindowIndex(szWndTitle);
	if(id < 0)	return;
	mppWnd[id]->SubPoint(-1, true);
}
long imShow::waitKey(long delay)
{
	long time0 = GetTickCount();
	for(;;)
	{
		MSG message;
        long is_processed = 0;
		 if((delay > 0 && abs((int)(GetTickCount() - time0)) >= delay) || mWndId <= 0)
            return -1;

		 if(delay <= 0)  GetMessage(&message, 0, 0, 0);
		 else if(PeekMessage(&message, 0, 0, 0, PM_REMOVE) == FALSE)
		 {	 Sleep(1);	continue;	 }

		 for(long id = 0; id < mWndId; id++)
		 {
			 if(message.hwnd != mppWnd[id]->m_hWnd)	continue;

			 is_processed = 1;
			 switch(message.message)
			 {
			 case WM_DESTROY:
			 case WM_CHAR:
				 DispatchMessage(&message);
				 return (long)message.wParam;
			 case WM_SYSKEYDOWN:
				 if( message.wParam == VK_F10 )
				 {
					 is_processed = 1;
					 return (long)(message.wParam << 16);
				 }
				 break;
			 case WM_KEYDOWN:
				 TranslateMessage(&message);
				 if((message.wParam >= VK_F1 && message.wParam <= VK_F24) ||
					 message.wParam == VK_HOME || message.wParam == VK_END ||
					 message.wParam == VK_UP || message.wParam == VK_DOWN ||
					 message.wParam == VK_LEFT || message.wParam == VK_RIGHT ||
					 message.wParam == VK_INSERT || message.wParam == VK_DELETE ||
					 message.wParam == VK_PRIOR || message.wParam == VK_NEXT )
				 {
					 DispatchMessage(&message);
					 is_processed = 1;
					 return (long)(message.wParam << 16);
				 }
			 default:
				 DispatchMessage(&message);
				 is_processed = 1;
				 break;
			 }
		 }
		if( !is_processed )
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
	}
	return -1;
}
//================================================//
imShow _ghshow_window_;
//================================================//
long hWaitKey(long delay)
{
	return _ghshow_window_.waitKey(delay);
}
void hShow(char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<char>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(unsigned char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<unsigned char>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<short>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(unsigned short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<unsigned short>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(int *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<int>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(unsigned int *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<unsigned int>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<long>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(unsigned long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<unsigned long>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(float *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<float>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(double *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.show<double>(pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hShow(hData<char> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<char>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}
void hShow(hData<unsigned char> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<unsigned char>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}
void hShow(hData<short> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<short>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}
void hShow(hData<unsigned short> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<unsigned short>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}
void hShow(hData<int> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<int>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}
void hShow(hData<unsigned int> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<unsigned int>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}
void hShow(hData<long> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<long>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}
void hShow(hData<unsigned long> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<unsigned long>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}
void hShow(hData<float> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<float>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}
void hShow(hData<double> *pData, double Normalized, CString szWndTitle)
{
	if(pData && pData->valid())	_ghshow_window_.show<double>(pData->pD, pData->W, pData->H, pData->C, pData->Stride, Normalized, szWndTitle);
}

void hColormap(char *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<char>(pData, W, H, Stride, szWndTitle);
}
void hColormap(unsigned char *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<unsigned char>(pData, W, H, Stride, szWndTitle);
}
void hColormap(short *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<short>(pData, W, H, Stride, szWndTitle);
}
void hColormap(unsigned short *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<unsigned short>(pData, W, H, Stride, szWndTitle);
}
void hColormap(int *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<int>(pData, W, H, Stride, szWndTitle);
}
void hColormap(unsigned int *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<unsigned int>(pData, W, H, Stride, szWndTitle);
}
void hColormap(long *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<long>(pData, W, H, Stride, szWndTitle);
}
void hColormap(unsigned long *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<unsigned long>(pData, W, H, Stride, szWndTitle);
}
void hColormap(float *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<float>(pData, W, H, Stride, szWndTitle);
}
void hColormap(double *pData, long W, long H, long Stride, CString szWndTitle)
{
	_ghshow_window_.colormap<double>(pData, W, H, Stride, szWndTitle);
}

void hColormap(hData<char> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<char>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}
void hColormap(hData<unsigned char> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<unsigned char>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}
void hColormap(hData<short> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<short>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}
void hColormap(hData<unsigned short> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<unsigned short>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}
void hColormap(hData<int> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<int>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}
void hColormap(hData<unsigned int> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<unsigned int>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}
void hColormap(hData<long> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<long>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}
void hColormap(hData<unsigned long> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<unsigned long>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}
void hColormap(hData<float> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<float>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}
void hColormap(hData<double> *pData, CString szWndTitle)
{
	if(!pData || !pData->valid())	return;
	if(pData->C == 1)		_ghshow_window_.colormap<double>(pData->pD, pData->W, pData->H, pData->Stride, szWndTitle);
	else
	{
		hData<double> gd(pData->W, pData->H, 1);
		if(pData->gray(gd))	_ghshow_window_.colormap<double>(gd.pD, gd.W, gd.H, gd.Stride, szWndTitle);
	}
}

void hChannel(long ic, char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.channel<char>(ic, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hChannel(long ic, unsigned char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.channel<unsigned char>(ic, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hChannel(long ic, short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.channel<short>(ic, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hChannel(long ic, unsigned short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.channel<unsigned short>(ic, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hChannel(long ic, long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.channel<long>(ic, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hChannel(long ic, unsigned long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.channel<unsigned long>(ic, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hChannel(long ic, float *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.channel<float>(ic, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hChannel(long ic, double *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.channel<double>(ic, pData, W, H, C, Stride, Normalized, szWndTitle);
}

void hSave(CString strFile, char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.save<char>(strFile, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hSave(CString strFile, unsigned char *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.save<unsigned char>(strFile, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hSave(CString strFile, short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.save<short>(strFile, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hSave(CString strFile, unsigned short *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.save<unsigned short>(strFile, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hSave(CString strFile, long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.save<long>(strFile, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hSave(CString strFile, unsigned long *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.save<unsigned long>(strFile, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hSave(CString strFile, float *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.save<float>(strFile, pData, W, H, C, Stride, Normalized, szWndTitle);
}
void hSave(CString strFile, double *pData, long W, long H, long C, long Stride, double Normalized, CString szWndTitle)
{
	_ghshow_window_.save<double>(strFile, pData, W, H, C, Stride, Normalized, szWndTitle);
}

void hClear(CString szWndTitle)
{
	_ghshow_window_.ClearRect(szWndTitle);
	_ghshow_window_.ClearLine(szWndTitle);
	_ghshow_window_.ClearPoint(szWndTitle);
	_ghshow_window_.ClearProjection(szWndTitle);
}
void hDrawRect(CString szWndTitle, RECT Rect, bool bEllipse, COLORREF color, long penWidth)
{
	_ghshow_window_.DrawRect(szWndTitle, Rect, bEllipse, color, penWidth);
}
void hClearRect(CString szWndTitle)
{
	_ghshow_window_.ClearRect(szWndTitle);
}
void hDrawLine(CString szWndTitle, long x0, long y0, long x1, long y1, COLORREF color, long penWidth)
{
	_ghshow_window_.DrawLine(szWndTitle, x0, y0, x1, y1, color, penWidth);
}
void hClearLine(CString szWndTitle)
{
	_ghshow_window_.ClearLine(szWndTitle);
}
void hDrawPoint(CString szWndTitle, long x, long y, COLORREF color, long penWidth)
{
	_ghshow_window_.DrawPoint(szWndTitle, x, y, color, penWidth);
}
void hClearPoint(CString szWndTitle)
{
	_ghshow_window_.ClearPoint(szWndTitle);
}
void hDrawProject(CString szWndTitle, float *pProj, long Length, bool bHor, RECT Rect, COLORREF color)
{
	_ghshow_window_.DrawProjection(szWndTitle, pProj, Length, bHor, Rect, color);
}
void hClearProjection(CString szWndTitle)
{
	_ghshow_window_.ClearProjection(szWndTitle);
}