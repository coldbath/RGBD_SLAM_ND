#pragma once

class Rclock
{
public:
	Rclock(void)	{	mt = 0.0;		}
	~Rclock(void)	{	}

	void start()	{	mt = counter();	}
	double stop()	{	return 1000.0f*(counter()-mt)/frequency();	}

	void box()
	{
		CString str;
		str.Format(_T("%.6f"), stop());
		AfxMessageBox(str);
	}
	void print(char *comment)	{	printf("%s - %.6f\n", comment, stop());	}

	double counter()
	{
#if defined WIN32 || defined WIN64 || defined _WIN64 || defined WINCE
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		return (double)counter.QuadPart;
#elif defined __linux || defined __linux__
		struct timespec tp;
		clock_gettime(CLOCK_MONOTONIC, &tp);
		return (double)tp.tv_sec*1000000000 + tp.tv_nsec;
#else
		struct timeval tv;
		struct timezone tz;
		gettimeofday( &tv, &tz );
		return (double)tv.tv_sec*1000000 + tv.tv_usec;
#endif
	}

	double frequency()
	{
#if defined WIN32 || defined WIN64 || defined _WIN64 || defined WINCE
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		return (double)freq.QuadPart;
#elif defined __linux || defined __linux__
		return 1e9;
#else
		return 1e6;
#endif
	}

private:
	double mt;
};
