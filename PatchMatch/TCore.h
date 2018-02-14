#pragma once



#pragma unmanaged

#include "timage2D.h"




class TCore
{


private:
	TCore();



public:
	static TCore *getInst(){
		static TCore p;
		return &p;
	}

	Image2D m_imgIn, m_imgOut;


	void initInputImage(const char* fname)
	{
		m_imgIn .Allocate(fname);
		m_imgOut = m_imgIn;
	}


};

#pragma managed
