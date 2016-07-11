#pragma once
#include "uiShow.h"
#include "Rclock.h"
#include "hData.h"
#include "imShow.h"

#include "ORBsystem.h"

enum {	inIMAGE = 0, inVIDEO, inCAMERA, dxCAMERA };

void funcProcessing(int typeInput)
{
	cvImageList	inImg;
	cvVideo		inVid;
	cvCamera	inCam;
	dxCamera	dxCam;
	cvStream	*pLoader = NULL;

	switch(typeInput)
	{
	case inIMAGE:	pLoader = (cvStream *)&inImg;	break;
	case inCAMERA:	pLoader = (cvStream *)&inCam;	break;
	case dxCAMERA:	pLoader = (cvStream *)&dxCam;	break;
	default:		pLoader = (cvStream *)&inVid;	break;
	}
	if(!pLoader->open(NULL, 0))	
	{
		printf("failed resource!\n");
		return;
	}
	//--------------------------//
	double fps = pLoader->fps, total = 0.0f;	
	int flag, skipTime = typeInput != inIMAGE ? 33 : 0;

	bool bSelect = false; 
	bool bBar = false, bPlay = typeInput != inIMAGE, bQuit = false;
	//--------------------------//
	uiShow		UI;
	cvImage		imGray;

	Rclock		ct;
	bool		bInit = false;	
	ORB_SLAM::ORBsystem orbsys;
	//--------------------------//
	while(!bQuit)
	{
		pLoader->capture();

		if(!pLoader->pData)	break;

		IplImage *frame = pLoader->pData->iplData;
		int W = pLoader->pData->width(), H = pLoader->pData->height(), C = pLoader->pData->channel();
		unsigned char *pColor = pLoader->pData->data();	int cStride = pLoader->pData->stride();
		imGray.gray(pLoader->pData);
		unsigned char *pGray = imGray.data();			int gStride = imGray.stride();

		UI.update(pLoader->pData);
		//--------------------------//
		cv::Rect roi = cv::Rect(0, 0, W, H);
		if(bSelect)
		{
			bSelect = false;
			cv::Rect sr = UI.userRoi(CV_RGB(0, 192, 192), 1);
			int rx = sr.x,	ex = rx + sr.width;
			int ry = sr.y,	ey = ry + sr.height;
			if(rx < 0)	rx = 0;
			if(ry < 0)	ry = 0;
			if(ex > W)	ex = W;
			if(ey > H)	ey = H;
			if(ex > rx && ey > ry)	roi = cv::Rect(rx, ry, ex-rx, ey-ry);
		}
		//--------------------------//
		ct.start();
		if(!bInit)
		{
			bInit = true;
		}
		if(bInit)
		{
			//function running
			orbsys.excute(pGray, W, H, 1,gStride);
			double dt = ct.stop();
			total += dt;	fps = 1000.0f/(dt+1e-5f);
		}
		//--------------------------//
		if(bBar && typeInput != inCAMERA)	
			UI.drawPlayBar(CV_RGB(240, 237, 224), CV_RGB(32, 157, 32), CV_RGB(255, 255, 32), fps, pLoader->id, pLoader->total);
		if(roi.width > 0 && roi.width != W && roi.height > 0 && roi.height != H)
			UI.drawRect(cvPoint(roi.x, roi.y), cvPoint(roi.x+roi.width, roi.y+roi.height), CV_RGB(0, 255, 255), 1);
		//	UI.update(pLoader->pData);
		UI.show();
		//--------------------------//
		flag = cvWaitKey(skipTime);
		switch(flag)
		{
		case ' ':	bPlay = !bPlay;		break;
		case 'r':
		case 'R':	bSelect = true;		break;
		case 'f':
		case 'F':	bPlay = false;	if(pLoader->previous())	pLoader->skip(-8);	break;
		case 'b':
		case 'B':	bPlay = false;	if(pLoader->next())		pLoader->skip(8);	break;
		case 's':
		case 'S':	
			{
				SYSTEMTIME st;		::GetSystemTime(&st);
				CString strFile;	char file[260];
				strFile.Format(_T("simg_%d_%d%d%d%d%d.bmp"), (int)pLoader->id, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
				WideCharToMultiByte(CP_ACP, 0, strFile, -1, file, sizeof(file), NULL, NULL);  // WideChar -> char
				cvSaveImage(file, pLoader->pData->iplData);
			}
			break;
		case 'q':
		case 'Q':
		case 27:	bQuit = true;				break;	//Esc
		case 'x':
		case 'X':	bBar = !bBar;				break;
		default:	pLoader->skip(1, true);		break;
		}
		skipTime = bPlay ? (int)(1000/fps+1) : 0;	
	}
	//--------------------------//
	printf("avg fps - %f\n", 1000.0f/(total/pLoader->id+1e-5f));
	cvWaitKey(500);
	printf("finish!\n");
}
//------------------------------------------//