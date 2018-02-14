#pragma once


#include <string.h>
#include "timageloader.h"

#ifndef byte
typedef unsigned char byte;
#endif 








//2D image 
class Image2D
{
protected:
	byte *m_rgba;//rgba
	int m_W, m_H;

public:
	~Image2D()
	{
		if (m_rgba) delete[] m_rgba;
	}

	Image2D()
	{
		m_W = m_H = 0;
		m_rgba = 0;
	}

	Image2D(const Image2D &src)
	{
		copy(src);
	}

	Image2D &operator=(const Image2D &src)
	{
		copy(src);
		return *this;
	}



	inline byte& operator[](int i4)       { return m_rgba[i4]; }
	inline byte  operator[](int i4) const { return m_rgba[i4]; }
	int getWidth () const { return m_W; }
	int getHeight() const { return m_H; }
	byte* getRGBA  () const { return m_rgba; }

private:
	void copy(const Image2D &src)
	{
		if( m_rgba ) delete[] m_rgba;

		m_W = src.m_W;
		m_H = src.m_H;

		if ( src.m_rgba )
		{
			m_rgba = new byte[m_W * m_H * 4];
			memcpy(m_rgba, src.m_rgba, sizeof(byte) * m_W * m_H * 4);
		}
	}
public:

	void Allocate(const int W, const int H)
	{
		if (m_rgba) delete[] m_rgba;
		m_W = W;
		m_H = H;
		m_rgba = new byte [4 * W * H];
	}

	bool Allocate (const char *fname)
	{
		if (m_rgba != 0) delete[] m_rgba;
		return t_loadImage( fname, m_W, m_H, m_rgba);
	}

	bool SaveAs   (const char *fname, int flg_BmpJpgPngTiff);


	void FlipInY()
	{
		byte *tmp = new byte[ m_W * m_H * 4 ];
		for( int y = 0; y < m_H; ++y) memcpy( &tmp[ 4*(m_H-1-y)*m_W ], &m_rgba[ 4*y*m_W ], sizeof( byte ) * 4 * m_W ); 
		memcpy( m_rgba, tmp, sizeof( byte ) * m_W * m_H * 4 );
		delete[] tmp;
	}

};


