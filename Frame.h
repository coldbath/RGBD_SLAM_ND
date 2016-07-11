#pragma once
#include <opencv2/opencv.hpp>

#include "ORBextractor.h"

namespace ORB_SLAM
{
	class Frame
	{
	public:
		Frame(ORBextractor *extractor){	mpORBextractor = extractor;	}
		~Frame(){}

		void ExtractORB(unsigned char *pIn, int W, int H, int C, int Stride)
		{
			mpORBextractor->excute(pIn, W, H, C, Stride);
		}
	private:
		ORBextractor *mpORBextractor;
		std::vector<cv::KeyPoint> mvKeys;//vector of keypoints and undistorted
		cv::Mat mDescriptors;//ORB descriptor, each row associated to a keypoint
	};
}