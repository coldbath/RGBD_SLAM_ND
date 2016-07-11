#pragma once
#include <opencv2/opencv.hpp>
#include <iostream>

namespace ORB_SLAM
{
	class ORBextractor
	{
	public:
		ORBextractor(int _nfeatures=2000, float _scaleFactor=1.2, int _nlevels=8, int _iniThFAST=20, int _minThFAST=7)
		{
			nfeatures = _nfeatures;	scaleFactor = _scaleFactor;	nlevels = _nlevels;	
			iniThFAST = _iniThFAST;	minThFAST = _minThFAST;

		}
		~ORBextractor(){}

		void excute(unsigned char *pIn, int W, int H, int C, int Stride)
		{
			if(NULL == pIn || C !=1 ) return;
			std::cout << "ss" << std::endl;
		}

	private:
		int nfeatures;//Number of features per image
		double scaleFactor;//Scale factor between levels in the scale pyramid 
		int nlevels;//Number of levels in the scale pyramid	
		int iniThFAST;// Fast threshold
		int minThFAST;// Image is divided in a grid. At each cell FAST are extracted imposing a minimum response.
					 // Firstly we impose iniThFAST. If no corners are detected we impose a lower value minThFAST
				     // You can lower these values if your images have low contrast	
		
	};
}
