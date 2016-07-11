#pragma once

#include "cvStreamManager.h"

class tUserRect
{
	int mx0, my0, mx1, my1;
public:
	tUserRect()	
	{	status = 0;  mx0 = my0 = mx1 = my1 = -1; roi = cv::Rect(0, 0, 0, 0); pShow = NULL;	}
	~tUserRect()	{}

	cv::Rect	roi;
	cvImage		*pShow;
	cvImage		backup;
	int			status;

	void clear()	{	status = 0; mx0 = my0 = mx1 = my1 = -1; roi = cv::Rect(0, 0, 0, 0);	}
	void update(int x, int y)	
	{	
		if(roi.width <= 0 || roi.height <= 0)
		{	mx0 = mx1 = x; my0 = my1 = y;	}
		else	
		{	mx1 = x; my1 = y;	}

		int x0, x1, y0, y1;
		if(mx0 > mx1)	{	x0 = mx1; x1 = mx0;	}
		else			{	x0 = mx0; x1 = mx1;	}
		if(my0 > my1)	{	y0 = my1; y1 = my0;	}
		else			{	y0 = my0; y1 = my1;	}
		roi = cv::Rect(x0, y0, x1-x0+1, y1-y0+1);
	}
};

static void _gmanualSelectRect(int event,int x, int y, int flags, void* out)
{
	tUserRect *ur = (tUserRect *)out;
	if(!ur->pShow)	return;

	ur->backup.clone(ur->pShow);

	if(event == CV_EVENT_LBUTTONDOWN)		{	ur->update(x, y);	ur->status = 1;	}
	else if(event == CV_EVENT_LBUTTONUP)	{	ur->update(x, y);	ur->status = 2;	}
	else if(ur->status == 1)				{	ur->update(x, y);	}
}

class uiShow
{
	char mTitle[260];
public:
	uiShow()	
	{	zoomFactor = 1.0f;	strcpy(mTitle, "display");	cvNamedWindow(mTitle, 1);	}
	~uiShow()	
	{	if(cvGetWindowHandle(mTitle)) cvDestroyWindow(mTitle);	}

	float	zoomFactor;
	cvImage imShow;

	void setTitle(char *title)
	{
		if(!title || strcmp(mTitle, title) == 0)	return;

		if(cvGetWindowHandle(mTitle)) cvDestroyWindow(mTitle);
		strcpy(mTitle, title);
		cvNamedWindow(mTitle, 1);
	}

	void update(cvImage *in)
	{
		int wLimit = ::GetSystemMetrics(SM_CXSCREEN)*85/100;
		int hLimit = ::GetSystemMetrics(SM_CYSCREEN)*85/100;

		int sW = in->width(), sH = in->height();
		zoomFactor = 1.0f;
		if(in->width() > wLimit || in->height() > hLimit)
		{
			zoomFactor = std::max((float)in->width()/wLimit, (float)in->height()/hLimit);
			image2ui(sW, sH);
		}
		imShow.resize(in, sW, sH);
	}
	void show()
	{
		cvShowImage(mTitle, imShow.iplData); 
	}
	cv::Rect userRoi(CvScalar color, int thickness CV_DEFAULT(1))
	{
		tUserRect selRoi;
		selRoi.pShow = &imShow;

		cvSetMouseCallback(mTitle, _gmanualSelectRect, (void*)(&selRoi));
		selRoi.clear();
		while(selRoi.status != 2)
		{
			cvRectangle(selRoi.backup.iplData, cvPoint(selRoi.roi.x, selRoi.roi.y), cvPoint(selRoi.roi.x+selRoi.roi.width, selRoi.roi.y+selRoi.roi.height), color, thickness);
			cvShowImage(mTitle, selRoi.backup.iplData); 
			if(27 == cv::waitKey(1))	break;
		}
		cv::setMouseCallback(mTitle, NULL);

		int rx = selRoi.roi.x;
		int ry = selRoi.roi.y;
		int rw = selRoi.roi.width;
		int rh = selRoi.roi.height;
		ui2image(rx,ry);
		ui2image(rw,rh);

		return cv::Rect(rx, ry, rw, rh);
	}
	void drawLine(CvPoint pt1, CvPoint pt2, CvScalar color, int thickness CV_DEFAULT(1))
	{
		cvLine(imShow.iplData, image2ui(pt1), image2ui(pt2), color, thickness);
	}
	void drawCircle(CvPoint center, int radius, CvScalar color, int thickness CV_DEFAULT(1))
	{
		cvCircle(imShow.iplData, image2ui(center), (int)(radius/zoomFactor+0.5f), color, thickness);
	}
	void drawEllipse(CvPoint center, float a, float b, float theta, CvScalar color, int thickness CV_DEFAULT(1))
	{
		cvEllipse(imShow.iplData, image2ui(center), cvSize((int)(a/zoomFactor+0.5f), (int)(b/zoomFactor+0.5f)), theta*180/CV_PI, 0, 360, color, thickness); 
	}
	void drawRect(CvPoint pt1, CvPoint pt2, CvScalar color, int thickness CV_DEFAULT(1))
	{
		cvRectangle(imShow.iplData, image2ui(pt1), image2ui(pt2), color, thickness);
	}
	void textout(char *str, CvPoint pt, CvScalar color, float scale = 0.4f, int thickness CV_DEFAULT(1), bool imagePoint = false)
	{
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, scale, scale, 1, thickness);
		cvPutText(imShow.iplData, str, imagePoint ? image2ui(pt) : pt, &font, color);
	}
	void drawPlayBar(CvScalar crBack, CvScalar crFore, CvScalar crFont, double ifps, double id, double total)
	{
		double ratio = (id+1.0f)/total;
		int sp = 24, ep = imShow.width()-24, len = ep-sp;
		int dx = sp+(int)(ratio*len+0.5f), y = imShow.height() - 24, thick = 1;

		cvLine(imShow.iplData, cvPoint(sp, y), cvPoint(ep, y), crBack, 5);
		cvLine(imShow.iplData, cvPoint(sp, y), cvPoint(dx, y), crFore, 3);

		char str[128];	CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.35f, 0.35f, 1, 1);

		sprintf(str, "%.2f%%", ratio*100);
		cvPutText(imShow.iplData, str, cvPoint(dx-16, y-5), &font, crFont);

		int bpp = imShow.iplData->depth*imShow.iplData->nChannels;
		if(ifps > 0)	sprintf(str, "%dx%dx%d  %.0f/%.0f fps %.0f", imShow.width(), imShow.height(), bpp, id+1, total, ifps);
		else			sprintf(str, "%dx%dx%d  %.0f/%.0f", imShow.width(), imShow.height(), bpp, id+1, total);
		cvPutText(imShow.iplData, str, cvPoint(sp, y+16), &font, crFont);
	}
	void image2ui(int &x, int &y)
	{
		x = (int)(x/zoomFactor+0.5f);
		y = (int)(y/zoomFactor+0.5f);
	}
	CvPoint image2ui(CvPoint &in)
	{
		return cvPoint((int)(in.x/zoomFactor+0.5f), (int)(in.y/zoomFactor+0.5f));
	}
	void ui2image(int &x, int &y)
	{
		x = (int)(x*zoomFactor+0.5f);
		y = (int)(y*zoomFactor+0.5f);
	}
	CvPoint ui2image(CvPoint &in)
	{
		return cvPoint((int)(in.x*zoomFactor+0.5f), (int)(in.y*zoomFactor+0.5f));
	}
};