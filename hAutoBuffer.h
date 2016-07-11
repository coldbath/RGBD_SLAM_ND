#pragma once

template<typename bType>
class hAutoBuffer
{
	char	*mpBuf;	
	int		mSize;
public:
	hAutoBuffer()	{	mpBuf = NULL; mSize = 0;	}
	~hAutoBuffer()	{	destroy();	}

	void destroy()		{	if(mpBuf)	{	free(mpBuf); mpBuf = NULL; }	}
	inline bool valid()	{	return mpBuf != NULL;	}

	bool create(int size, bool keepPoint = false)
	{
		if(size <= 0)	return false;
		if(!mpBuf)
		{
			if((mpBuf = (char *)malloc(size*sizeof(bType))) == NULL)
				return false;
			mSize = size;
		}
		if(size > mSize)
		{
			if(keepPoint)
			{
				if((mpBuf = (char *)realloc(mpBuf, size*sizeof(bType))) == NULL)	
					return false;
			}
			else
			{
				free(mpBuf); mpBuf = NULL; 
				if((mpBuf = (char *)malloc(size*sizeof(bType))) == NULL)
					return false;
			}
			mSize = size;
		}
		return true;
	}
	inline bType *ptr()								{	return (bType *)mpBuf;	}
	inline bType *ptr(char *p, int align = 16)		{	return (bType *)(((size_t)p + align-1) & -align);	}
	inline bType *ptr(int offset, int align = 16)	{	return ptr(mpBuf+offset, align);	}
};