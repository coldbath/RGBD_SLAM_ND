#pragma once
#include "Tracking.h"

namespace ORB_SLAM
{
	class ORBsystem
	{
	public:
		ORBsystem(){ }
		~ORBsystem(){ }

		void excute(unsigned char *pIn, int W, int H, int C, int Stride)
		{
			//1.initialize
			//2.tracking
			mptracking = new Tracking();
			mptracking->excute(pIn, W, H, C, Stride);
			//3.local mapping 
			//4.loop closer
		}
	private:
		Tracking *mptracking;
	};
}