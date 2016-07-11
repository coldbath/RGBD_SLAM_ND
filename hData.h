#pragma once
#include <math.h>

template<typename dType>
class hData
{
public:
	long W, H, C, Stride;
	dType *pD;

	hData()			{	mAlignSize = 16;	clear();	}
	~hData()		{	release();	}

	void release()			{	if(mpBuf)	{	delete[] mpBuf; mpBuf = NULL;}		mpD = pD = NULL;	}
	bool valid()			{	return pD != NULL;	}
	void align(long size)	{	mAlignSize = size < 0 ? 0 | (size>>3)<<3;	}

	template<typename tType>
	inline bool check(hData<tType> &dt)	{	return valid() && dt.valid() && W == dt.W && H == dt.H;	}
	template<typename tType>
	inline bool match(hData<tType> &dt)	{	return check(dt) && C == dt.C;	}
	
	hData(dType *pData, long Width, long Height, long Channel, long widthStep)
	{
		initialize(pData, Width, Height, Channel, widthStep);
	}
	hData(long Width, long Height, long Channel)
	{
		clear();
		create(Width, Height, Channel);
	}
	bool initialize(dType *pData, long Width, long Height, long Channel, long widthStep)
	{
		clear();
		if(!pData || Width <= 0 || Height <= 0 || Channel <= 0 || abs(widthStep) < Width)
			return false;

		mpD = pData;	mW	= Width;	mH	= Height;	C	= Channel;	Stride = widthStep;
		pD = mpD;		W = mW;			H = mH;
		return true;
	}
	bool create(long Width, long Height, long Channel)
	{
		release();
		if(Width <= 0 || Height <= 0 || Channel <= 0 
		|| (mpBuf = new dType [Width*Height*Channel]) == NULL)
			return false;

		mpD = mpBuf;
		mW	= Width;	mH	= Height;	C	= Channel;	Stride = mW*C;
		pD = mpD;		W = mW;			H = mH;
		return true;
	}

	bool lock(RECT *pRoi = NULL)
	{
		if(!mpD || mbLock)	return false;

		long sx = 0, sy = 0, ex = mW, ey = mH;
		if(pRoi)
		{
			sx = pRoi->left < 0 ? 0 : pRoi->left;
			sy = pRoi->top < 0 ? 0 : pRoi->top;
			ex = pRoi->right > mW ? mW : pRoi->right;
			ey = pRoi->bottom > mH ? mH : pRoi->bottom;
		}
		if(ex <= sx || ey <= sy)
			return (mbLock = false);

		W = ex - sx;
		H = ey - sy;
		pD = mpD + sy*Stride + sx*C;

		return (mbLock = true);
	}
	void unlock()	{	mbLock = false;	pD = mpD; W = mW; H = mH;	}

	template<typename oType>
	bool integral(hData<oType> *pOut)
	{
		if(!valid() || C > 64 || !pOut || !pOut->valid() || pOut->W != W+1 || pOut->H != H+1 || pOut->C != C)
			return false;

		double s[64];
		dType *pI = pD, *pi;
		oType *S0 = pOut->pD, *S1 = s0+pOut->Stride, *s0, *s1;
		memset(s1, 0, pOut->W*pOut->C*sizeof(oType));

		long i, j, k;
		for(j = 0; j < H; j++, pI+=Stride)
		{
			for(s0 = S0, s1 = S1, k = 0; k < C; k++, s1++)	s[k] = *s1 = 0.0f;
			for(pi = pI, s0+=C, i = 0; i < W; i++)
			{
				for(k = 0; k < C; k++, pi++, s0++, s1++)
				{
					s[k]  += (*pi);		*s1	= (oType)(s[k]+(*s0));
				}	
			}
			S1 = S0; S1+=pOut->Stride;
		}
		return true;
	}

	bool average(double Avg[], long ic = -1)
	{
		if(!valid())	return false;

		long i, j, k, sz = W*H;
		dType *pI = pD, *pi;	
		if(ic < 0 || ic > C)
		{
			double sum = 0.0f;
			for(j = 0; j < H; j++, pI+=Stride)
			{
				for(pi = pI, i = 0; i < W; i++)
				{
					for(k = 0; k < C; k++, pi++)
						sum += (*pi);
				}
			}
			Avg[0] = iv; sum /= sz;
		}
		else if(ic == C)
		{
			for(k = 0; k < C; k++)	Avg[k] = 0.0f;	
			for(j = 0; j < H; j++, pI+=Stride)
			{
				for(pi = pI, i = 0; i < W; i++)
				{
					for(k = 0; k < C; k++, pi++)
						Avg[k] += (*pi);
				}
			}
			for(k = 0; k < C; k++)	Avg[k] /= sz;	
		}
		else
		{
			dType iv = *pI, xv = *pI;
			for(pI += ic, j = 0; j < H; j++, pI+=Stride)
			{
				for(pi = pI, i = 0; i < W; i++, pi+=C)
					sum += (*pi);
			}
			Avg[0] = sum/sz;
		}
		return true;
	}
	bool range(dType Low[], dType High[], long ic = -1)
	{
		if(!valid())	return false;

		long i, j, k;
		dType *pI = pD, *pi;	
		if(ic < 0 || ic > C)
		{
			dType iv = *pI, xv = *pI;
			for(j = 0; j < H; j++, pI+=Stride)
			{
				for(pi = pI, i = 0; i < W; i++)
				{
					for(k = 0; k < C; k++, pi++)
					{
						if(iv > (*pi)) iv = *pi;
						if(xv < (*pi)) xv = *pi;
					}
				}
			}
			Low[0] = iv; High[0] = xv;
		}
		else if(ic == C)
		{
			for(k = 0; k < C; k++)
			{	Low[k] = High[k] = pI[k];	}
			for(j = 0; j < H; j++, pI+=Stride)
			{
				for(pi = pI, i = 0; i < W; i++)
				{
					for(k = 0; k < C; k++, pi++)
					{
						if(Low[k] > (*pi))	Low[k] = *pi;
						if(High[k] < (*pi)) High[k] = *pi;
					}
				}
			}
		}
		else
		{
			dType iv = *pI, xv = *pI;
			for(pI += ic, j = 0; j < H; j++, pI+=Stride)
			{
				for(pi = pI, i = 0; i < W; i++, pi+=C)
				{
					if(iv > (*pi)) iv = *pi;
					if(xv < (*pi)) xv = *pi;
				}
			}
			Low[0] = iv; High[0] = xv;
		}
		return true;
	}
	bool normalize(double Nlow[], double Nhigh[], long ic = -1)
	{
		if(!valid())	return false;

		long i, j, k;
		dType *pI = pD, *pi;	
		if(ic < 0 || ic > C)
		{
			if(Nhigh[0] == Nlow[0])	return false;
			dType Low, High;
			range(&Low, &High, ic);
			double Nl = Nlow[0], dn = Low == High ? 0.0f : (Nhigh[0]-Nl)/(High-Low);
			for(j = 0; j < H; j++, pI+=Stride)
			{
				for(pi = pI, i = 0; i < W; i++)
				{
					for(k = 0; k < C; k++, pi++)
						*pi = (dType)(Nl+((*pi)-Low)*dn);
				}
			}
		}
		else if(ic == C)
		{
			char *pBuf = NULL;
			if((pBuf = new char [C*sizeof(double)+2*C*sizeof(dType)]) == NULL)
				return false;
			double *dn = (double *)pBuf;
			dType *Low = (dType *)(pBuf+C*sizeof(double)), *High = Low+C;
			range(Low, High, ic);
			for(k = 0; k < C; k++)
				dn[k] = Low[k] == High[k] ? 0.0f : (Nhigh[k]-Nlow[k])/(High[k]-Low[k]);
			for(j = 0; j < H; j++, pI+=Stride)
			{
				for(pi = pI, i = 0; i < W; i++)
				{
					for(k = 0; k < C; k++, pi++)
						*pi = (dType)(Nlow[k]+((*pi)-Low[k])*dn[k]);
				}
			}
			delete[] pBuf;
		}
		else
		{
			if(Nhigh[0] == Nlow[0])	return false;
			dType Low, High;
			range(&Low, &High, ic);
			double Nl = Nlow[0], dn = Low == High ? 0.0f : (Nhigh[0]-Nl)/(High-Low);
			for(pI+=ic, j = 0; j < H; j++, pI+=Stride)
			{
				for(pi = pI, i = 0; i < W; i++, pi+=C)
					*pi = (dType)(Nl+((*pi)-Low)*dn);
			}
		}
		return true;
	}

	template<typename oType>
	bool gray(hData<oType> &out, long oc = 0, double *wgt = NULL)
	{
		if(!check(out))	return false;
		
		if(C == 1)	return getChannel(0, out, oc);

		if(oc < 0)	oc = 0;	if(oc >= out.C)	oc = out.C-1;
		long i, j, k;
		oType *pO = out.pD+oc, *po;	dType *pI = pD, *pi;
		double val;

		if(wgt == NULL)
		{
			for(j = 0; j < H; j++, pO+=out.Stride, pI+=Stride)
			{
				for(po = pO, pi = pI, i = 0; i < W; i++, po+=out.C)
				{
					for(val = 0.0f, k = 0; k < C; k++, pi++)	val += (*pi);
					*po = (oType)val;
				}
			}
		}
		else
		{
			for(j = 0; j < H; j++, pO+=out.Stride, pI+=Stride)
			{
				for(po = pO, pi = pI, i = 0; i < W; i++, po+=out.C)
				{
					for(val = 0.0f, k = 0; k < C; k++, pi++)	val += wgt[k]*(*pi);
					*po = (oType)val;
				}
			}
		}
		return true;
	}
	
	template<typename oType>
	bool getChannel(long ic, hData<oType> &out, long oc = 0)
	{
		if(!check(out))	return false;
		if(ic < 0)	ic = 0;	if(ic >= C)		ic = C-1;
		if(oc < 0)	oc = 0;	if(oc >= out.C)	oc = out.C-1;

		oType *pO = out.pD+oc, *po;	dType *pI = pD+ic, *pi;
		long i, j;
		for(j = 0; j < H; j++, pO+=out.Stride, pI+=Stride)
		{
			for(po = pO, pi = pI, i = 0; i < W; i++, po+=out.C, pi+=C)
				*po = (oType)(*pi);
		}
		return true;
	}
	template<typename iType>
	bool setChannel(long oc, hData<iType> &in, long ic = 0)
	{
		if(!check(In))	return false;
		if(oc < 0)	oc = 0;	if(oc >= C)		oc = C-1;
		if(ic < 0)	ic = 0;	if(ic >= in.C)	ic = in.C-1;

		dType *pO = pD+oc, *po; iType *pI = in.pD+ic, *pi;	
		long i, j;
		for(j = 0; j < H; j++, pI+=in.Stride, pO+=Stride)
		{
			for(po = pO, pi = pI, i = 0; i < W; i++, po+=C, pi+=in.C)
				*po = (dType)(*pi);
		}
		return true;
	}

	template<typename iType>
	bool set(hData<iType> &in, double norm = 0.0f)
	{
		if(!match(in))	return false;
		long i, j, line = W*C;
		dType *pO = pD, *po;	iType *pI = in.pD, *pi;
		if(norm == 0.0f)
		{
			for(j = 0; j < H; j++, pO+=Stride, pI+=in.Stride)
			{
				for(po = pO, pi = pI, i = 0; i < line; i++, po++, pi++)
					*po = (dType)(*pi);
			}
		}
		else
		{
			for(j = 0; j < H; j++, pO+=Stride, pI+=in.Stride)
			{
				for(po = pO, pi = pI, i = 0; i < line; i++, po++, pi++)
					*po = (dType)((*pi)*norm);
			}
		}
		return true;
	}
	void set(dType value)
	{
		if(!valid())	return;
		long i, j, line = W*C;
		dType *pO = pD, *po;
		for(j = 0; j < H; j++, pO+=Stride)
		{
			for(po = pO, i = 0; i < line; i++, po++)
				*po = value;
		}
	}

	template<typename tType>
	bool add(hData<tType> &dt)
	{
		if(!match(dt))	return false;
		long i, j, line = W*C;
		dType *pO = pD, *po;	tType *pI = dt.pD, *pi;
		for(j = 0; j < H; j++, pO+=Stride, pI+=dt.Stride)
		{
			for(po = pO, pi = pI, i = 0; i < line; i++, po++, pi++)
				*po = (dType)((*pi)+(*po));
		}
		return true;
	}
	//---------------------------------------//
	//color space
	bool rgb2yuv()
	{
		if(!valid() || C < 3)	return false;
		long i, j;
		dType *pO = pD, *po, r, g, b;
		for(j = 0; j < H; j++, pO+=Stride)
		{
			for(po = pO, i = 0; i < W; i++, po+=C)
			{
				b = po[0];	g = po[1];	r = po[2];
				po[0] = (dType)(0.299f*r + 0.587f*g + 0.114f*b);	//Y
				po[1] = (dType)(-0.169f*r - 0.331*g + 0.499f*b);	//U
				po[2] = (dType)(0.499*r - 0.418f*g - 0.0813f*b);	//V
			}
		}
		return true;
	}
	bool yuv2rgb()
	{
		if(!valid() || C < 3)	return false;
		long i, j;
		dType *pO = pD, *po, y, u, v;
		for(j = 0; j < H; j++, pO+=Stride)
		{
			for(po = pO, i = 0; i < W; i++, po+=C)
			{
				y = po[0];	u = po[1];	v = po[2];
				po[0] = (dType)(y + 1.772f*u);			//B
				po[1] = (dType)(y-0.344f*u - 0.714f*v);	//G
				po[2] = (dType)(y + 1.402f*v);			//R
			}
		}
		return true;
	}
	bool rgb2lab()
	{
		if(!valid() || C < 3)	return false;
		long i, j;
		dType *pO = pD, *po, r, g, b;
		double x, y, z, dy, dt = 1.0f/3.0f;
		for(j = 0; j < H; j++, pO+=Stride)
		{
			for(po = pO, i = 0; i < W; i++, po+=C)
			{
				b = po[0];	g = po[1];	r = po[2];
				x = 0.433890f*r + 0.376226f*g + 0.189884f*b;
				y = 0.212639f*r + 0.715169f*g + 0.072192f*b;
				z = 0.018625f*r + 0.109462f*g + 0.872913f*b;
				dy = pow(y, dt);
				po[0] = (dType)y;				//L
				po[1] = (dType)(100.0f*(x-dy));	//a
				po[2] = (dType)(40.0f*(dy-z));	//b
			}
		}
		return true;
	}
	bool lab2rgb()
	{
		if(!valid() || C < 3)	return false;
		long i, j;
		dType *pO = pD, *po;
		double y, a, b, x, z, dy, dt = 1.0f/3.0f;
		for(j = 0; j < H; j++, pO+=Stride)
		{
			for(po = pO, i = 0; i < W; i++, po+=C)
			{
				y = po[0];	a = po[1];	b = po[2];

				dy = pow(y, dt);
				x = dy+a*0.010f;
				z = dy-b*0.025f;
				po[0] = (dType)(0.052874f*x - 0.203977f*y +  1.150958f*z);	//b
				po[1] = (dType)(-0.921216f*x + 1.875965f*y +  0.045251f*z);	//g
				po[2] = (dType)(3.080376f*x - 1.537379f*y -  0.542949f*z);	//r
			}
		}
		return true;
	}
	bool rgb2lab(double range)
	{
		if(!valid() || C < 3 || range == 0.0f)	return false;

		long i, j;
		double Lrx[256], Lry[256], Lrz[256], Lgx[256], Lgy[256], Lgz[256], Lbx[256], Lby[256], Lbz[256];
		for(i = 0; i < 256; i++)
		{
			float di = i/255.0f;
			di = di > 0.04045f ? pow((di+0.055f)/1.055f, 2.4f) : di/12.92f;
			Lrx[i] = 0.4124f * di / 0.95047f;	Lgx[i] = 0.3576f * di / 0.95047f;	Lbx[i] = 0.1805f * di / 0.95047f;
			Lry[i] = 0.2126f * di;				Lgy[i] = 0.7152f * di;				Lby[i] = 0.0722f * di;
			Lrz[i] = 0.0193f * di / 1.08883f;	Lgz[i] = 0.1192f * di / 1.08883f;	Lbz[i] = 0.9505f * di / 1.08883f;
		}

		dType *pO = pD, *po;
		unsigned char r, g, b;
		double x, y, z, dn = 255.0f/range, dp = 1.0f/3.0f, dc = 16.0f/116.0f;
		for(j = 0; j < H; j++, pO+=Stride)
		{
			for(po = pO, i = 0; i < W; i++, po+=C)
			{
				b = (unsigned char)(po[0]*dn);	g = (unsigned char)(po[1]*dn);	r = (unsigned char)(po[2]*dn);
				x = Lrx[r] + Lgx[g] + Lbx[b];
				y = Lry[r] + Lgy[g] + Lby[b];
				z = Lrz[r] + Lgz[g] + Lbz[b];

				x = x > 0.008856f ? pow(x, dp) : 7.787f*x + dc;
				y = y > 0.008856f ? pow(y, dp) : 7.787f*y + dc;
				z = z > 0.008856f ? pow(z, dp) : 7.787f*z + dc;

				po[0] = (dType)(116.0f*y - 16.0f);	//L
				po[1] = (dType)(500.0f*(x - y));	//a
				po[2] = (dType)(200.0f*(y - z));	//b
			}
		}
		return true;
	}
	bool lab2rgb(double range)
	{
		if(!valid() || C < 3 || range == 0.0f)	return false;

		long i, j;
		dType *pO = pD, *po;
		double x, y, z, r, g, b, dt = 6.0f/29.0f, dc = 16.0f/116.0f, dp = 1.0f/2.4f;
		for(j = 0; j < H; j++, pO+=Stride)
		{
			for(po = pO, i = 0; i < W; i++, po+=C)
			{
				y = (po[0] + 16.0f) / 116.0f;
				x = po[1]/500.0f + y;
				z = y - po[2]/200.0f;

				y = y > dt ? pow(y, 3.0) : (y-dc)/7.787f;
				x = x > dt ? pow(x, 3.0) : (x-dc)/7.787f;
				z = z > dt ? pow(z, 3.0) : (z-dc)/7.787f;
				x *= 0.95047f;
				z *= 1.08883f;

				r =  3.2406f*x  - 1.5372f*y - 0.4986f*z; 
				g = -0.9689f*x  + 1.8758f*y + 0.0415f*z; 
				b =  0.0557f*x  - 0.2040f*y + 1.0570f*z; 

				po[0] = (dType)(range*(b > 0.0031308f ? 1.055f * pow(b, dp) - 0.055f : 12.92f * b));
				po[1] = (dType)(range*(g > 0.0031308f ? 1.055f * pow(g, dp) - 0.055f : 12.92f * g));
				po[2] = (dType)(range*(r > 0.0031308f ? 1.055f * pow(r, dp) - 0.055f : 12.92f * r));
			}
		}
		return true;
	}
	//---------------------------------------//
private:
	void clear()	{	mpBuf = mpD = pD = NULL;	W = H = C = Stride = 0;	 mbLock = false; 	}
	
private:
	dType	*mpBuf;

	long	mW, mH;
	dType	*mpD;
	bool	mbLock;
	long	mAlignSize;
};