#pragma once
#include <fstream>
#include "opencv2/core/core.hpp"   
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "tDepth.h"


inline short BilinearInterpolation(short q11, short q12, short q21, short q22,
	short x1, short x2, short y1, short y2, 
	short x, short y) 
{
	float x2x1, y2y1, x2x, y2y, yy1, xx1;
	x2x1 = x2 - x1;
	y2y1 = y2 - y1;
	x2x = x2 - x;
	y2y = y2 - y;
	yy1 = y - y1;
	xx1 = x - x1;
	return 1.0 / (x2x1 * y2y1) * (
		q11 * x2x * y2y +
		q21 * xx1 * y2y +
		q12 * x2x * yy1 +
		q22 * xx1 * yy1
		);
}

class tDepth
{
public:
	tDepth(){
		mW = 0;
		mH = 0;
		mData = NULL;
	}
	tDepth(int w , int h , short* data){
		mW = w;
		mH = h;
		mData = new short[w * h];
		memcpy(mData, data, w * h * sizeof(short));
		//mData = data;
	}
	~tDepth()		{	release();	}

	void release()			
	{	
		if(mData)	
		{	
			delete[] mData; 
			mData = NULL;
		}	
	}

	bool check(float &th)
	{
		if (mData == NULL || mH == 0 || mW == 0)
			return false;	
		
		short *pI = mData, *pi;		
		int i, j;
		float count = 0;
		for ( j=0; j<mH; j++, pI+=mW)
			for ( i=0, pi=pI; i<mW; i++, pi+=1)
		{
			if (*pi != 0){ count+=1; }
		}

		th = count/(float)(mW*mH);		
		return true;
	}

	bool save(char* filename){
		FILE* fp;

		if((fp=fopen(filename, "wb+")) == NULL){ return false; }

		if (mW != NULL || mH != NULL)
		{
			if (!fwrite(&mW, sizeof(int), 1, fp)){ return false; }
			if (!fwrite(&mH, sizeof(int), 1, fp)){ return false; }
		}
		else{ fclose(fp); return false; }

		if (mData != NULL){
			if (!fwrite(mData, sizeof(short), mW*mH, fp)){ return false; }		
		}
		else{ fclose(fp); return false; }
		fclose(fp);
		return true;		
	}

	bool load(const char* filename){
		FILE* fp;
		if((fp=fopen(filename,"rb")) == NULL){ return false; }

		if (!fread(&mW, sizeof(int), 1, fp) )
		{ fclose(fp); return false; }

		if (!fread(&mH, sizeof(int), 1, fp))
		{ fclose(fp); return false; }

		if (mData == NULL){ mData = new short[mW * mH]; }

		if(!fread(mData, sizeof(short), mW*mH, fp)){ fclose(fp); return false; }
		fclose(fp);
		return true;		
	}

	bool normlize(int w, int h)
	{
		if (mData == NULL || mH == 0 || mW == 0)
			return false;	
		cv::Mat dst(w, h, CV_16SC1);
		dst.setTo(0);
		cv::Mat src(mH, mW, CV_16SC1, (short*)mData);

		cv::resize(src, dst, cv::Size(w, h));
		release();

		mW = w;
		mH = h;
				
		mData = new short[w*h];
		memcpy(mData, (short*)dst.data, w * h * sizeof(short));

		return true;
	}

	bool histeq()
	{
		if (mData == NULL || mH == 0 || mW == 0)
			return false;	
		
		float imax=0.0f, imin=0.0f, itip=0.0f;

		double dstHist[1000];  
		memset(dstHist,0,1000 * sizeof(double));  

		int i,j;
		int iCount = 0;
		int iBin = 10;
		double dstArray[1000];
		memset(dstArray, 0, 1000*sizeof(double));

		long lSum = 0;
		float mean = 0;

		//统计直方图  
		for (i=0; i<mW; i++)
			for (j=0; j<mH; j++)
		{
			if (*(mData+j*mW+i) != 0)
			{
				dstHist[(int)(*(mData+j*mW+i))/iBin]++;
				iCount++;
				lSum += *(mData+j*mW+i);
			}
		}
		
		//均值
		mean = lSum / iCount;

		//计算直方图累积  
		dstArray[0] = dstHist[0];  
		for (i = 1;i < 1000;i++)  
		{  
			dstArray[i] = dstArray[i - 1] + dstHist[i];  
		}
		
		bool bmin = true;		
		bool bmax = true;
		bool btip = true;
		/*
		//version2
		for (i=0; i<1000; i++)
		{
			dstArray[i] /= iCount;
			if (bmin && dstArray[i] > 0.02) 
			{
				imin = i*iBin;
				bmin = false;
			}

			if (btip && (float)dstHist[i]/iCount > 0.1)
			{
				float nn = (float)abs(i*iBin - mean)/mean;

				if (nn<0.1)
				{
					itip = i*iBin;
					btip = false;
				}

			}
		}

		for (i=0; i<mW; i++)
			for (j=0; j<mH; j++)
			{
				if (*(mData+j*mW+i) < imin || *(mData+j*mW+i)  > imax)
					*(mData+j*mW+i) = 0;

				imax = itip + 200;
				float dn = 255/(imax-imin+0.1f);

				*(mData+j*mW+i)=(*(mData+j*mW+i)-imin)*dn;
			}
			*/
		
		//version1
		for (i=0; i<1000; i++)
		{
			dstArray[i] /= iCount;
			if (bmin && dstArray[i] > 0.02) 
			{
				imin = i*iBin;
				bmin = false;
			}

			if (bmax && dstArray[i] > 0.98) 
			{
				imax = i*iBin;
				bmax = false;
			}
		}

		for (i=0; i<mW; i++)
			for (j=0; j<mH; j++)
		{
			if (*(mData+j*mW+i) < imin || *(mData+j*mW+i)  > imax)
				*(mData+j*mW+i) = 0;

			//imax = itip + 200;
			float dn = 255/(imax-imin+0.1f);

			*(mData+j*mW+i)=(*(mData+j*mW+i)-imin)*dn;
		}
		
		return true;
	}

	bool interpolation()
	{
		if (mData == NULL || mH == 0 || mW == 0)
			return false;

		/*BilinearInterpolation(short q11, short q12, short q21, short q22,
		short x1, short x2, short y1, short y2, 
		short x, short y) */
		int i,j;
		for (i=1; i<mW-1; i++)
			for (j=1; j<mH-1; j++)
		{
			*(mData+j*mW+i) = BilinearInterpolation(*(mData+(j+1)*mW+i-1),//q11
				*(mData+(j-1)*mW+i-1),//q12
				*(mData+(j+1)*mW+i+1),//q21
				*(mData+(j-1)*mW+i+1),//q22
				i-1,//x1
				i+1,//x2
				j+1,//y1
				j-1,//y2
				i,//x
				j//y
				);
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	int mW,  mH;
	short *mData;
};
