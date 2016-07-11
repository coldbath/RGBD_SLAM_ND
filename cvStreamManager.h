#pragma once

#include <vector>
#include "cv.h"
#include "highgui.h"
#include "CameraDS.h"

class cvImage
{
	IplImage		mCopy;
	IplImage		*mpLoad;
	int				mx, my, mw, mh, mp;
	bool			mbFlip;
public:
	cvImage()	{	mpLoad = iplData = NULL;	mx = my = mw = mh = mp = 0; mbFlip = false;	}
	~cvImage()	{	if(mpLoad)	{	cvReleaseImage(&mpLoad); mpLoad = NULL; }			}

	IplImage *iplData;
	//---------------------------//
	void clear()			{	iplData = NULL; mx = my = mw = mh = mp = 0;		}
	bool valid()			{	return iplData != NULL && mw > 0 && mh > 0;		}
	int width()				{	return mw;										}
	int height()			{	return mh;										}
	int channel()			{	return iplData->nChannels;						}
	int stride()			{	return iplData->widthStep;						}
	unsigned char *data()	{	return (unsigned char*)iplData->imageData+mp;	}
	void flip(bool flag)	{	mbFlip = flag;									}
	CString information(int x, int y, double zoom = -1.0f)
	{
		if(!valid())	return _T("size  |  zoom  |  x,y  |  RGBA");

		CString str, str1;
		str.Format(_T("%dx%dx%d  "), iplData->width, iplData->height, iplData->nChannels*iplData->depth);
		if(zoom > 0.0f)
		{
			str1.Empty();	str1.Format(_T("|  %.2f%%  "), zoom*100);
			str += str1;
		}
		if(x >= 0 && x < iplData->width && y >= 0 && y < iplData->height)
		{
			str1.Empty();
			unsigned char *pi = (unsigned char *)iplData->imageData + y*iplData->widthStep + x*iplData->nChannels;
			switch(iplData->nChannels)
			{
			case 1:	str1.Format(_T("|  %d,%d  |  %d"), x, y, pi[0]);								break;
			case 3: str1.Format(_T("|  %d,%d  |  %d,%d,%d"), x, y, pi[2], pi[1], pi[0]);			break;
			case 4: str1.Format(_T("|  %d,%d  |  %d,%d,%d,%d"), x, y, pi[2], pi[1], pi[0], pi[3]);	break;
			}
			str += str1;
		}
		return str;
	}
	//---------------------------//
	void set(IplImage *frame, bool flip = false)
	{
		iplData = NULL;
		if(!frame)	return;
		iplData = frame;	mbFlip = flip;
		mx = my = mp = 0;	mw = frame->width;	mh = frame->height;
	}
	void set(cv::Mat *frame, bool flip = false)
	{
		iplData = NULL;
		if(!frame)	return;

		mCopy = *frame;
		iplData = &mCopy;

		mbFlip = flip;
		mx = my = mp = 0;	mw = frame->cols;	mh = frame->rows;
	}
	bool set(int Width, int Height, int Channel, unsigned char *val = NULL)
	{
		iplData = NULL;
		if(Width <= 0 || Height <= 0 || Channel <= 0)	return false;

		if(!mpLoad || mpLoad->width != Width || mpLoad->height != Height || mpLoad->nChannels != Channel)
		{
			if(mpLoad)	{	cvReleaseImage(&mpLoad); mpLoad = NULL; }	
			mpLoad = cvCreateImage(cvSize(Width, Height), IPL_DEPTH_8U, Channel);
		}
		if(!mpLoad)	return false;
		mx = my = mp = 0;	mw = Width;	mh = Height;
		if(val)	memset(mpLoad->imageData, *val, mpLoad->widthStep*mpLoad->height);
		return (iplData = mpLoad) != NULL;
	}
	template <typename dType>
	bool set(dType *pIn, int Width, int Height, int Channel, int Stride, double low = 0.0f, double high = 0.0f, bool flip = false)
	{
		iplData = NULL;
		if(!pIn || !set(Width, Height, Channel))	return false;
		//------------------------------//
		mbFlip = flip;
		normalize(Width, Height, Channel, iplData->imageData, iplData->widthStep, pIn, Stride, low, high);
		//------------------------------//
		return true;
	}
	void lock(int x, int y, int w, int h)
	{
		if(iplData)	
		{
			if(setROI(x, y, w, h))	cvSetImageROI(iplData, cvRect(mx, my, mw, mh));
		}
	}
	void unlock()
	{
		if(iplData)	
		{
			mx = my = mp = 0; mw = iplData->width; mh = iplData->height;
			cvResetImageROI(iplData);
		}
	}
	void save(char *filename)
	{
		if(iplData)	cvSaveImage(filename, iplData);
	}
	//---------------------------//
	void blend(int x, int y, int w, int h, cvImage *in)
	{
		if(!valid() || !in || !in->valid() || channel() > in->channel())
			return;

		bool alpha = in->channel() == 4;
		lock(x, y, w, h);
		cvImage sin;
		sin.resize(in, width(), height());
		unsigned char *pD = data(), *pI = sin.data(), *pd, *pi;
		for(int j = 0; j < height(); j++, pD+=stride(), pI+=sin.stride())
		{
			pi = pI; pd = pD;
			for(int i = 0; i < width(); i++, pd+=channel(), pi+=sin.channel())
			{
				for(int k = 0; k < channel(); k++)
				{
					pd[k] = alpha ? (pd[k]*(255-pi[3]) + pi[k]*pi[3])/255 : pi[k];
				}
			}
		}
		unlock();
	}
	//---------------------------//
	bool load(char *filename, bool flip = false, int type = CV_LOAD_IMAGE_UNCHANGED)
	{
		iplData = NULL;	mbFlip = flip;
		if(mpLoad)	{	cvReleaseImage(&mpLoad); mpLoad = NULL; }
		mpLoad = cvLoadImage(filename, type);
		if(mpLoad)	{	mx = my = mp = 0; mw = mpLoad->width; mh = mpLoad->height;	}

		return (iplData = mpLoad) != NULL;
	}
	bool load(CString strFile, bool flip = false, int type = CV_LOAD_IMAGE_UNCHANGED)
	{
		char filename[260];
		WideCharToMultiByte(CP_ACP, 0, strFile, -1, filename, sizeof(filename), NULL, NULL);
		return load(filename, flip, type);
	}
	void save(CString strFile)
	{
		char filename[260];
		WideCharToMultiByte(CP_ACP, 0, strFile, -1, filename, sizeof(filename), NULL, NULL);
		save(filename);
	}
	void draw(HDC hdc, long xDes, long yDes, long wDes, long hDes, DWORD rop = SRCCOPY)
	{
		if(!valid())	return;

		char buf[1064];
		BITMAPINFO *bmpInfo = (BITMAPINFO *)buf;
		int bpp = channel()*8;
		memset(bmpInfo, 0, sizeof(BITMAPINFO));
		bmpInfo->bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
		bmpInfo->bmiHeader.biWidth			= iplData->width;
		bmpInfo->bmiHeader.biHeight			= mbFlip ? iplData->height : -iplData->height;
		bmpInfo->bmiHeader.biPlanes			= 1;
		bmpInfo->bmiHeader.biBitCount		= (unsigned short)bpp;
		bmpInfo->bmiHeader.biCompression	= BI_RGB;
		bmpInfo->bmiHeader.biXPelsPerMeter	= 100;
		bmpInfo->bmiHeader.biYPelsPerMeter	= 100;
		bmpInfo->bmiHeader.biClrUsed		= bpp > 8 ? 0  : 256;
		bmpInfo->bmiHeader.biSizeImage		= stride() * height();

		if(channel() == 1)
		{
			for(int i = 0; i < 256; i++)
			{	
				bmpInfo->bmiColors[i].rgbReserved = 0;
				bmpInfo->bmiColors[i].rgbBlue = bmpInfo->bmiColors[i].rgbGreen = bmpInfo->bmiColors[i].rgbRed = (unsigned char)i;
			}
		}

		int saveBltMode = SetStretchBltMode(hdc, COLORONCOLOR);
		StretchDIBits(hdc, xDes, yDes, wDes, hDes, mx, my, mw, mh, (void *)iplData->imageData, bmpInfo, DIB_RGB_COLORS, SRCCOPY);
		SetStretchBltMode(hdc, saveBltMode);
	}
	//---------------------------//
	bool clone(cvImage *in)
	{
		if(!in || !in->valid() || !set(in->width(), in->height(), in->channel()))
			return false;

		cvCopy(in->iplData, iplData);
		return true;
	}
	bool flip(cvImage *in, int flag = 0)	// 0 up-bottom, 1 left - right,  -1 both
	{
		if(!in || !in->valid() || !set(in->width(), in->height(), in->channel()))
			return false;
		cvFlip(in->iplData, iplData, flag);
		return true;
	}
	bool resize(cvImage *in, int w, int h)
	{
		if(!in || !in->valid() || !set(w, h, in->channel()))
			return false;
		cvResize(in->iplData, iplData);
		return true;
	}
	bool gray(cvImage *in)
	{
		if(!in || !in->valid() || !set(in->width(), in->height(), 1))
			return false;
		if(in->channel() == channel())
			clone(in);
		else
			cvCvtColor(in->iplData, iplData, CV_BGR2GRAY);
		return true;
	}
	bool color(cvImage *in)
	{
		if(!in || !in->valid() || !set(in->width(), in->height(), 3))
			return false;
		if(in->channel() == channel())
			clone(in);
		else
			cvCvtColor(in->iplData, iplData, CV_GRAY2BGR);
		return true;
	}
	//---------------------------//
	template<typename xyType>
	void line(xyType x0, xyType y0, xyType x1, xyType y1, CvScalar color, int thickness = 1)
	{
		cvLine(iplData, cvPoint((int)(x0+0.5f), (int)(y0+0.5f)), cvPoint((int)(x1+0.5f), (int)(y1+0.5f)), color, thickness);
	}
	template<typename xyType>
	void circle(xyType x, xyType y, int radius, CvScalar color, int thickness = 1)
	{
		cvCircle(iplData, cvPoint((int)(x+0.5f), (int)(y+0.5f)), radius, color, thickness);
	}
	template<typename xyType>
	void rectangle(xyType x, xyType y, xyType w, xyType h, CvScalar color, int thickness = 1)
	{
		cvRectangle(iplData, cvPoint((int)(x+0.5f), (int)(y+0.5f)), cvPoint((int)(x+w+0.5f), (int)(y+h+0.5f)), color, thickness);
	}
	void textout(char *str, int x, int y, CvScalar color, float scale = 0.4f, int thickness = 1)
	{
		if(!str || !valid())	return;
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, scale, scale, 1, thickness);
		cvPutText(iplData, str, cvPoint(x, y), &font, color);
	}
	//---------------------------//
private:
	bool setROI(int x, int y, int w, int h)
	{
		if(w <= 0 || h <= 0)	return false;

		if(x < 0)					x = 0;
		if(x >= iplData->width)		x = iplData->width-1;
		if(y < 0)					y = 0;
		if(y >= iplData->height)	y = iplData->height-1;
		if(x+w > iplData->width)	w = iplData->width-x;
		if(y+h > iplData->height)	h = iplData->height-y;
		if(w <= 0 || h <= 0)	return false;

		mx = x; my = y; mw = w; mh = h;	mp = my*iplData->widthStep + mx*iplData->nChannels;
		return true;
	}
	template<typename dType>
	void normalize(int W, int H, int C, char *pOut, int oStep, dType *pIn, int iStep, double low = 0.0f, double high = 0.0f)
	{
		double level = high - low;
		if(level < 1e-6f)	getLevel(low, high, pIn, W, H, C, iStep);
		level = low >= high ? 1.0f : high-low;

		int i, j, k; double dv, dNorm = 255.0f/level;
		dType *pI = pIn, *pi;	unsigned char *pO = (unsigned char *)pOut, *po;
		for(j = 0; j < H; j++, pO += oStep, pI += iStep)
		{
			for(po = pO, pi = pI, i = 0; i < W; i++)
			{
				for(k = 0; k < C; k++, po++, pi++)
				{
					if((*pi) <= low)		dv = 0.0f;
					else if((*pi) >= high)	dv = 255.0f;
					else					dv = ((*pi)-low)*dNorm;
					*po = (unsigned char)dv;
				}
			}
		}
	}
	template<typename dType>
	void getLevel(double &dMin, double &dMax, dType *pData, int W, int H, int C, int Stride)
	{
		int i, j, Line = W*C;
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
};
class cvStream
{
public:
	cvStream()	{	clear();	}
	virtual ~cvStream()	{}

	double id, total, fps;
	cvImage *pData;

	virtual bool valid()								{	return false;	}
	virtual bool validFile(CString strFile)				{	return false;	}
	virtual bool setFile(char fileName[])				{	return false;	}
	virtual void dropFile(CWnd *pWnd = NULL)			{}
	virtual bool open(CWnd *pWnd = NULL, int ic = -1, int frameWidth = 0, int frameHeight = 0)	{	return false;	}
	virtual void capture(bool flip = false)				{}
	virtual void setExposure(int val)					{}
	virtual void autoExposure()							{}
	virtual	void close()								{}
	//--------------------------------//
	void clear()			{	id = total = fps = 0.0f; pData = NULL;	}
	void flip(bool flag)	{	if(pData)	pData->flip(flag);			}
	bool previous()			{	return total > 0.0f && id > 0.0f;		}
	bool next()				{	return total > 0.0f && id < total-1.0f;	}
	bool terminal()			{	return id >= total;						}
	void setIndex(double index)
	{
		id = index;
		if(id < 0.0f)			id = 0.0f;
		else if(id > total)		id = total-1.0f;
	}
	void skip(int skip, bool loop = false)
	{
		id += skip;
		if(!loop)	setIndex(id);
		else if(total > 0.0f)
		{
			while(id < 0.0f)	id += total;
			while(id >= total)	id -= total;
		}
	}
	int cycleTime()	{	return fps <= 0.0f ? 33 : (int)(1000.0f/fps);	}
	double setRatio(double ratio)
	{
		if(ratio <= 0.0f)		id = 0.0f;
		else if(ratio >= 1.0f)	id = total-1.0f;
		else					id = (unsigned int)((total-1.0f)*ratio);
		return id;
	}
	double getRatio()	{	return total > 0.0f ? (id+1.0f)/total : 0.0f;	}
	//--------------------------------//
	CString infoImage(int x = -1, int y = -1, double zoom = -1.0f)	{	return pData != NULL ? pData->information(x, y, zoom) : _T("");	}
	CString infoFile()
	{
		CString str = _T("fps    index/total");
		if(valid())
		{
			if(fps <= 0.0f)	str.Format(_T("%d/%d"), (int)id+1, (int)total);
			else			str.Format(_T("fps - %.0f    %.0f/%.0f"), fps, id+1, total);
		}
		return str;
	}
	virtual CString getFullName()	{	return _T("");	}
	//--------------------------------//
	void playBar(CvScalar crBack, CvScalar crFore, CvScalar crFont, double ifps)
	{
		if(!valid())	return;

		double ratio = (id+1.0f)/total;
		int sp = 24, ep = pData->width()-24, len = ep-sp;
		int dx = sp+(int)(ratio*len+0.5f), y = pData->height() - 24, thick = 1;

		cvLine(pData->iplData, cvPoint(sp, y), cvPoint(ep, y), crBack, 5);
		cvLine(pData->iplData, cvPoint(sp, y), cvPoint(dx, y), crFore, 3);

		char str[128];	CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.35f, 0.35f, 1, 1);

		sprintf(str, "%.2f%%", ratio*100);
		cvPutText(pData->iplData, str, cvPoint(dx-16, y-5), &font, crFont);

		int bpp = pData->iplData->depth*pData->iplData->nChannels;
		if(ifps > 0)	sprintf(str, "%dx%dx%d  %.0f/%.0f fps %.0f", pData->width(), pData->height(), bpp, id+1, total, ifps);
		else			sprintf(str, "%dx%dx%d  %.0f/%.0f", pData->width(), pData->height(), bpp, id+1, total);
		cvPutText(pData->iplData, str, cvPoint(sp, y+16), &font, crFont);
	}
};
class cvImageList : public cvStream
{
	cvImage					mImage;
	double					mid;	
public:
	std::vector<CString>	mvFile;

	cvImageList() : cvStream()	{	mvFile.clear();	mid = -1.0f;	}
	~cvImageList()				{	pData = NULL;					}

	bool valid()	{	return mImage.valid() && pData != NULL;	}
	CString getFullName()	
	{	return (mvFile.empty() || id < 0.0f || id >= total) ? _T("") : mvFile[(unsigned int)id];	}
	bool validFile(CString strFile)	
	{
		CString strExt = strFile.Mid(strFile.ReverseFind('.')+1);
		return isValid(strExt);
	}
	bool setFile(char fileName[])
	{
		if(!fileName)			return false;
		WCHAR wcTmp[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, fileName, -1, wcTmp, sizeof(wcTmp));
		if(!validFile(wcTmp))	return false;
		append(wcTmp);
		return true;
	}
	bool open(CWnd *pWnd = NULL, int ic = -1, int frameWidth = 0, int frameHeight = 0)
	{
		bool bUpdate = false;
		CString strFilt = _T("Support format|*.jpg;*.jpeg;*jpe;*.bmp;*dib;*.png;*.pbm;*pgm;*ppm;*tif;*tiff;|	\
							jpg file (*.jpg;*.jpeg;*jpe)|*.jpg;*jpeg;*jpe|	\
							bmp file (*.bmp)|*.bmp|dib file (*.dib)|*.dib|png file (*.png)|*.png|	\
							pbm file (*.pbm)|*.pbm|pgm file (*.pgm)|*.pgm|ppm file (*.ppm)|*.ppm|	\
							tif file (*.tif;*.tiff)|*.tif;*tiff|other file(*.*)|*.*|");
		CString strExt = _T("jpg|jpeg|jpe|bmp|dib|png|pbm|pgm|ppm|tif|tiff");
		CFileDialog dlg(TRUE, strExt, _T(""), OFN_ALLOWMULTISELECT, strFilt, pWnd); 

		int MAXFILE = 8192, bufsz = MAXFILE*(MAX_PATH+1)+1;
		TCHAR *pBuf = NULL;
		pBuf = new TCHAR [bufsz];
		if(pBuf)
		{
			dlg.m_ofn.nMaxFile = bufsz; 
			dlg.m_ofn.lpstrFile = pBuf; 
			dlg.m_ofn.lpstrFile[0] = _T('\0');
		}
		AfxGetApp()->BeginWaitCursor();
		if(dlg.DoModal() == IDOK)
		{
			reset();
			POSITION ps = dlg.GetStartPosition();	
			DWORD nCnt = 0;
			while(ps != NULL && nCnt < dlg.m_ofn.nMaxFile)
				append(dlg.GetNextPathName(ps));
			total = mvFile.size();
			bUpdate = true;
		}
		AfxGetApp()->EndWaitCursor();

		if(pBuf) delete[] pBuf;
		return bUpdate;
	}
	void dropFile(HDROP hDrop)
	{
		if(!hDrop)	return;

		AfxGetApp()->BeginWaitCursor();

		reset();
		unsigned int i, numFile = ::DragQueryFile(hDrop, 0xFFFFFFFF, NULL, MAX_PATH);
		TCHAR szPath[MAX_PATH];
		for(i = 0; i < numFile; i++)
		{
			::DragQueryFile(hDrop, i, szPath, MAX_PATH);
			append(szPath);
		}
		total = mvFile.size();
		::DragFinish(hDrop);

		AfxGetApp()->EndWaitCursor();
	}
	bool browseFolder(CWnd *pWnd = NULL, bool bSubFolder = false)
	{
		bool bUpdate = false;
		TCHAR szDisplayName[MAX_PATH]	= _T("");
		TCHAR szPath[MAX_PATH]			= _T("");
		BROWSEINFO bi;
		bi.hwndOwner      = pWnd ? pWnd->GetSafeHwnd() : NULL;
		bi.pidlRoot       = /*NULL*/CSIDL_DESKTOP;
		bi.lpszTitle      = _T("Select Folder");
		bi.pszDisplayName = szDisplayName;
		bi.ulFlags        = BIF_RETURNONLYFSDIRS;
		bi.lpfn           = NULL;
		bi.lParam         = (LPARAM)pWnd;

		LPITEMIDLIST pItemIDList = SHBrowseForFolder(&bi);
		if(!pItemIDList || !SHGetPathFromIDList(pItemIDList, szPath))		
			return bUpdate;
		IMalloc *pMalloc;
		if(SHGetMalloc(&pMalloc) != NOERROR)
		{	TRACE( "Failed to get pointer to shells task allocator" );		}
		pMalloc->Free(pItemIDList);
		if(pMalloc)	pMalloc->Release();

		AfxGetApp()->BeginWaitCursor();

		scanFolder(szPath, bSubFolder);
		bUpdate = true;

		AfxGetApp()->EndWaitCursor();
		return bUpdate;
	}
	void capture(bool flip = false)	
	{
		if(total <= 0.0f)	return;
		if(mid == id)		return;

		mImage.iplData = NULL;
		if(id < total)	
		{
			mid = id;
			mImage.load(mvFile[(unsigned int)id], flip);
		}
		pData = mImage.valid() ? &mImage : NULL;
	}
	void scanFolder(CString strPath, bool bSubFolder)
	{
		reset();
		scanning(strPath, bSubFolder);
		total = (double)mvFile.size();
	}
	void setExposure(int val)	{}
	void autoExposure()			{}
	void close()				{	reset();	}
private:
	void scanning(CString strPath, bool bSubFolder)
	{
		if(strPath.IsEmpty())	return;

		scanFileFromFolder(strPath);
		if(!bSubFolder)			return;

		CFileFind ff;
		CString strDir = strPath;
		if(strDir.Right(1) != _T("\\"))	strDir += _T("\\");

		strDir += _T("*.*");	
		CString strChild;
		BOOL bFind = ff.FindFile(strDir);
		while(bFind)
		{
			bFind = ff.FindNextFile();
			strChild = ff.GetFilePath();
			if(ff.IsDots())			continue;
			if(ff.IsDirectory())	scanFolder(strChild, bSubFolder);	
		}
		ff.Close();

	//	std::sort(mvFile.begin(), mvFile.end());
	}
	void scanFileFromFolder(CString strFolder)
	{
		if(strFolder.IsEmpty())	return;

		if(strFolder[strFolder.GetLength()-1] != _T('\\'))
			strFolder += _T('\\');

		CFileFind Finder;
		CString strSearch = strFolder + _T("*.*");
		BOOL bFound = Finder.FindFile(strSearch);
		while(bFound)
		{
			bFound = Finder.FindNextFile();
			if(Finder.IsDots() || Finder.IsDirectory())	continue;

			append(Finder.GetFilePath());
		}
		Finder.Close();
	}
	void append(CString strFile)
	{
		if(validFile(strFile) && !isExist(strFile))
		{
			mvFile.push_back(strFile);
			printf("[%06d]\r", (int)mvFile.size());
		}
	}
	bool isExist(CString strFile)
	{
		int i, num = (int)mvFile.size();
		for(i = 0; i < num; i++)
		{	if(strFile == mvFile[i])	return true;	}
		return false;
	}
	bool isValid(CString strExt)
	{
		return strExt.CompareNoCase(_T("jpg"))	== 0	|| strExt.CompareNoCase(_T("jpeg")) == 0	|| strExt.CompareNoCase(_T("jpe")) == 0
			|| strExt.CompareNoCase(_T("bmp"))  == 0	|| strExt.CompareNoCase(_T("dib"))  == 0	|| strExt.CompareNoCase(_T("png")) == 0
			|| strExt.CompareNoCase(_T("pbm"))  == 0	|| strExt.CompareNoCase(_T("pgm"))  == 0	|| strExt.CompareNoCase(_T("ppm")) == 0
			|| strExt.CompareNoCase(_T("tif"))  == 0	|| strExt.CompareNoCase(_T("tiff")) == 0;
	}
	void reset()	{	clear();	mvFile.clear();	mid = -1.0f;	}
};
class cvVideo : public cvStream
{
	CString		mstrPathName;
	cvImage		mImage;
	double		mid;
	IplImage	*mpFrame;
	CvCapture	*mCap;
public:
	cvVideo() : cvStream()	{	mCap = NULL; mImage.iplData = NULL; mid = -1.0f; mstrPathName = _T("");	}
	~cvVideo()				{	close();	}

	bool valid()			{	return mCap != NULL && mImage.valid();		}
	CString getFullName()	{	return mstrPathName;						}
	bool validFile(CString strFile)	
	{
		CString strExt = strFile.Mid(strFile.ReverseFind('.')+1);
		return isValid(strExt);
	}
	bool setFile(char fileName[])
	{
		if(!fileName)				return false;
		WCHAR wcTmp[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, fileName, -1, wcTmp, sizeof(wcTmp));
		if(!validFile(wcTmp))		return false;
		load(wcTmp);
		return true;
	}
	bool open(CWnd *pWnd = NULL, int ic = -1, int frameWidth = 0, int frameHeight = 0)
	{
		bool bUpdate = false;
		CString strFilt = _T("Support format|*avi;*mp4;*mov;*3gp;*mpg;*mpeg;*wmv|avi file (*.avi)|*.avi|	\
							mp4 file (*.mp4)|*.mp4|mov file (*.mov)|*.mov|3gp file (*.3gp)|*.3gp|	\
							mpeg file (*.mpg;*mpeg)|*.mpg;*mpeg |wmv file (*.wmv)|*.wmv| \
							other file(*.*)|*.*|");
		CString strExt = _T("avi|mp4|mov|3gp|mpg|mpeg|wmv");
		CFileDialog dlg(TRUE, strExt, _T(""),  OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_READONLY, strFilt, pWnd); 

		AfxGetApp()->BeginWaitCursor();

		if(dlg.DoModal() == IDOK)	
		{
			load(dlg.GetPathName());
			bUpdate = true;
		}
		
		AfxGetApp()->EndWaitCursor();
		return bUpdate;
	}
	void dropFile(HDROP hDrop)
	{
		if(!hDrop)	return;

		AfxGetApp()->BeginWaitCursor();

		TCHAR szPath[MAX_PATH];
		::DragQueryFile(hDrop, 0, szPath, MAX_PATH);
		load(szPath);
		::DragFinish(hDrop);

		AfxGetApp()->EndWaitCursor();
	}
	void capture(bool flip = false)	
	{
		if(total <= 0.0f)	return;
		if(mid == id)		return;

		mImage.iplData = NULL;
		if(id < total)
		{
			if(id - mid == 1.0f)
			{
				mid = id;
				mpFrame = cvQueryFrame(mCap);
				mImage.set(mpFrame, flip);
			}
			else
			{
				double td = id;
				while(true)
				{
					cvSetCaptureProperty(mCap, CV_CAP_PROP_POS_FRAMES, td);
					mid = cvGetCaptureProperty(mCap, CV_CAP_PROP_POS_FRAMES);
					if(mid <= id)	break;
					td -= 8;
				}
				for(; mid < id; mid++)	mpFrame = cvQueryFrame(mCap);
				mpFrame = cvQueryFrame(mCap);
				mImage.set(mpFrame, flip);
			}
		}
		pData = mImage.valid() ? &mImage : NULL;
	}
	void setExposure(int val)	{}
	void autoExposure()			{}
	void close()				{	if(mCap) cvReleaseCapture(&mCap); mCap = NULL; 	pData = NULL;	}
private:
	void load(CString strFile)
	{
		reset();
	//	if(!validFile(strFile))	return;

		mstrPathName = strFile;
		char filename[260];
		WideCharToMultiByte(CP_ACP, 0, mstrPathName, -1, filename, sizeof(filename), NULL, NULL);

		if((mCap = cvCaptureFromAVI(filename)) != NULL)
		{
			fps		= cvGetCaptureProperty(mCap, CV_CAP_PROP_FPS);
			total	= cvGetCaptureProperty(mCap, CV_CAP_PROP_FRAME_COUNT);
		}
	}
	bool isValid(CString strExt)
	{
		return strExt.CompareNoCase(_T("avi"))	== 0	|| strExt.CompareNoCase(_T("mp4")) == 0		
			|| strExt.CompareNoCase(_T("mov")) == 0		|| strExt.CompareNoCase(_T("mpg")) == 0
			|| strExt.CompareNoCase(_T("mpeg")) == 0;
	}
	void reset()
	{	
		clear();	mid = -1.0f;	mstrPathName = _T("");
		if(mCap) cvReleaseCapture(&mCap); mCap = NULL; mImage.iplData = NULL;	
	}
};
class cvCamera : public cvStream
{
	cvImage		mImage;
	IplImage	*mpFrame;
	CvCapture	*mCap;
public:
	cvCamera() : cvStream()	{	mCap = NULL; mImage.iplData = NULL; pData = NULL;		}
	~cvCamera()				{	close();	}

	bool valid()			{	return mCap != NULL && mImage.valid();		}
	CString getFullName()	{	return _T("cvCamera");						}
	bool open(CWnd *pWnd = NULL, int ic = -1, int frameWidth = 0, int frameHeight = 0)
	{
		if(ic < 0)	ic = 0;
		mCap = cvCreateCameraCapture(CV_CAP_DSHOW+ic);
		fps = 30.0f; id = 0.0f; total = 1.7976931348623158e+308;

		if(mCap && frameWidth > 0 && frameHeight > 0)	
		{
			cvSetCaptureProperty(mCap, CV_CAP_PROP_FRAME_WIDTH, frameWidth);
			cvSetCaptureProperty(mCap, CV_CAP_PROP_FRAME_HEIGHT, frameHeight);
		}
		return mCap != NULL;
	}
	void capture(bool flip = false)	
	{
		if(!mCap)	return;

		mImage.iplData = NULL;
		mpFrame = cvQueryFrame(mCap);
		if(mpFrame != NULL)	mImage.set(mpFrame, flip);

		pData = mImage.valid() ? &mImage : NULL;
	}
	void setFps(float val)
	{
		if(!mCap)	return;
		cvSetCaptureProperty(mCap, CV_CAP_PROP_FPS, val);
	}
	void setExposure(int val)
	{
		if(!mCap)	return;
		cvSetCaptureProperty(mCap, CV_CAP_PROP_EXPOSURE, val);
	}
	void autoExposure()
	{
		if(!mCap)	return;
		double val = cvGetCaptureProperty(mCap, CV_CAP_PROP_AUTO_EXPOSURE);
		cvSetCaptureProperty(mCap, CV_CAP_PROP_EXPOSURE, val);
	}
	void close()	{	if(mCap) cvReleaseCapture(&mCap); mCap = NULL; 	pData = NULL;	}
};
class dxCamera : public cvStream
{
	cvImage		mImage;
	IplImage	*mpFrame;
	int			mic;
	CCameraDS	mCds;
public:
	dxCamera() : cvStream()	{	mic = -1; mImage.iplData = NULL; pData = NULL;	}
	~dxCamera()				{	close();	pData = NULL;	}

	bool valid()			{	return  mic >= 0 && mImage.valid();		}
	CString getFullName()	{	return _T("dxCamera");					}
	bool open(CWnd *pWnd = NULL, int ic = -1, int frameWidth = 0, int frameHeight = 0)
	{
		mic = -1;
		if(ic < 0)	ic = 0;
		fps = 30; id = 0.0f; total = 1.7976931348623158e+308;
		if(mCds.OpenCamera(ic, false, frameWidth, frameHeight))	mic = ic;
		return mic >= 0;
	}
	void capture(bool flip = false)	
	{
		if(mic < 0)	return;

		mImage.iplData = NULL;
		mpFrame = mCds.QueryFrame();
		if(mpFrame != NULL)	mImage.set(mpFrame, flip);

		pData = mImage.valid() ? &mImage : NULL;
	}
	void setExposure(int val)
	{
		if(mic < 0)	return;
		//long minExp, maxExp, step, defaultExp, capsFlags;
		//if(mCds.GetProperty(CameraControl_Exposure, &minExp, &maxExp, &step, &defaultExp, &capsFlags))
			mCds.SetProperty(CameraControl_Exposure, val, CameraControl_Flags_Manual);
	}
	void autoExposure()
	{
		if(mic < 0)	return;
		mCds.SetProperty(CameraControl_Exposure, -1, CameraControl_Flags_Auto);
	}
	void close()	{	mCds.CloseCamera();	}
};
class cvRecorder
{
	#define BUF_NUM		8
	cvImage				mData[BUF_NUM];
	char				mLock[BUF_NUM];
	std::vector<int>	mvList;
	int					mW, mH, mC;
	CvVideoWriter		*mWriter;
public:
	cvRecorder()	
	{	
		mWriter = NULL;	mvList.clear(); mW = mH = mC = 0; 
		memset(mLock, 0, BUF_NUM);
	}
	~cvRecorder()	{	quit();	}

	bool valid()	{	return mWriter != NULL; }
	void quit()	
	{	
		write(-1);	mW = mH = mC = 0; mvList.clear(); memset(mLock, 0, BUF_NUM);
		if(mWriter)	{	cvReleaseVideoWriter(&mWriter);	mWriter = NULL;	}
	}
	bool create(char *filename, int Width, int Height, int Channel, double fps = 0.0f)
	{
		if(!filename || Width <= 0 || Height <= 0 || Channel <= 0)	
			return false;

		if(fps <= 0.0f)		fps = 30.0f;
		if(fps > 90.0f)	fps = 90.0f;
		if(!mWriter || mW != Width || mH != Height || mC != Channel)
		{
			mvList.clear(); memset(mLock, 0, BUF_NUM);
			if(mWriter)	{	cvReleaseVideoWriter(&mWriter);	mWriter = NULL;	}
			mWriter = cvCreateVideoWriter(filename, CV_FOURCC('D', 'I', 'V', 'X'), fps, cvSize(Width, Height), Channel >= 3);
			mW = Width; mH = Height; mC = Channel;
		}
		return mWriter != NULL;
	}
	bool create(CString strFile, int Width, int Height, int Channel, double fps = 0.0f)
	{
		char filename[260];
		WideCharToMultiByte(CP_ACP, 0, strFile, -1, filename, sizeof(filename), NULL, NULL);
		return create(filename, Width, Height, Channel, fps);
	}
	void append(cvImage *in, bool flip)
	{
		if(!mWriter || !in || !in->valid() || mvList.size() >= BUF_NUM
		|| in->width() != mW || in->height() != mH || in->channel() != mC)	
			return;
		int id = getFreeId();
		if(id < 0)	return;

		bool bOk = false;
		if(flip)	bOk = mData[id].flip(in, flip);
		else		bOk = mData[id].clone(in);
		if(bOk)		{	mLock[id] = 1;	mvList.push_back(id);	}
	}
	void write(int limitCnt = -1)
	{
		if(!mWriter)		return;
		int k, num = 0, sz = (int)mvList.size();
		for(k = 0; k < sz; k++)
		{
			cvWriteFrame(mWriter, mData[mvList[k]].iplData);
			mLock[mvList[k]] = 0;	num++;
			if(limitCnt > 0 && num >= limitCnt)
				break;
		}
		if(num > 0)	mvList.erase(mvList.begin(), mvList.begin()+num);
	}
private:
	int getFreeId()
	{
		int k, id = -1;
		for(k = 0; k < BUF_NUM; k++)
		{	if(mLock[k] == 0)	{	id = k; break; }	}
		return id;
	}
};