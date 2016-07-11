#pragma once 
#include "ORBextractor.h"
#include "Frame.h"

namespace ORB_SLAM
{
	class Tracking
	{
	public:
		Tracking(){}
		~Tracking(){}

		void excute(unsigned char *pIn, int W, int H, int C, int Stride)
		{
			if(NULL == pIn || C !=1 ) return;
			//1.extract ORB 
			mCurrentFrame = Frame(mpORBextractor);
			mCurrentFrame.ExtractORB(pIn, W, H, C, Stride);
			//2.last frame PnP or relocation
			//3.track local map
			//4.Optimize Pose
			//5.new KeyFrame decision
		}
	private:
		ORBextractor *mpORBextractor;
		Frame mCurrentFrame;//current frame
	};
}
