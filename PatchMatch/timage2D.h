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
		m_rgba = 0;
		copy(src);
	}

	Image2D &operator=(const Image2D &src)
	{
		copy(src);
		return *this;
	}



	inline byte& operator[](int i4)       { return m_rgba[i4]; }
	inline byte  operator[](int i4) const { return m_rgba[i4]; }
	inline int getWidth () const { return m_W; }
	inline int getHeight() const { return m_H; }
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

	void SaveAs   (const char *fname)
	{
		t_saveImage(fname, m_W, m_H, m_rgba);
	}


	void FlipInY()
	{
		byte *tmp = new byte[ m_W * m_H * 4 ];
		for( int y = 0; y < m_H; ++y) memcpy( &tmp[ 4*(m_H-1-y)*m_W ], &m_rgba[ 4*y*m_W ], sizeof( byte ) * 4 * m_W ); 
		memcpy( m_rgba, tmp, sizeof( byte ) * m_W * m_H * 4 );
		delete[] tmp;
	}

	inline byte get(int x, int y, int ch)
	{
		return m_rgba[ 4 * (x+y*m_W) + ch];
	}

	void gaussianFilter33()
	{
		byte *rgba = new byte[4*m_W*m_H];
		memcpy( rgba, m_rgba, sizeof( byte) * 4*m_W*m_H);

		for( int y = 1; y < m_H - 1; ++y)
		for( int x = 1; x < m_W - 1; ++x)
		{
			for( int ch = 0; ch < 3; ++ch)
			{
				float v = ( 1 * get(x-1,y-1,ch) + 2 * get(x,y-1,ch) + 1 * get(x+1,y-1,ch) 
					      + 2 * get(x-1,y  ,ch) + 4 * get(x,y  ,ch) + 2 * get(x+1,y  ,ch)
					      + 1 * get(x-1,y+1,ch) + 2 * get(x,y+1,ch) + 1 * get(x+1,y+1,ch) ) / 16.0f;
				rgba[ (x+y*m_W) * 4 + ch ] = (byte) v;
			}
		}
	}

	void HalfDownSample()
	{
		this->gaussianFilter33();
		const int W = m_W / 2;
		const int H = m_H / 2;
		byte *rgba = new byte[W*H*4];
		
		for( int y = 0; y < H; ++y)
		for( int x = 0; x < W; ++x)
			memcpy(&rgba[ 4 * ( x + y * W) ],&m_rgba[4 * (2*x + 2*y*m_W)],sizeof(byte)*4);	

		delete[] m_rgba;
		m_rgba = rgba;
		m_W = W;
		m_H = H;
	}

	void doubleUpSample()
	{
		const int W = m_W * 2;
		const int H = m_H * 2;
		byte *rgba = new byte[W*H*4];
		
		for( int y = 0; y < H; ++y)
		for( int x = 0; x < W; ++x)
			memcpy( &rgba[ 4 * ( x + y * W) ], &m_rgba[4 * (x/2 + y/2*m_W)], sizeof(byte)*4);	

		delete[] m_rgba;
		m_rgba = rgba;
		m_W = W;
		m_H = H;
		gaussianFilter33(); //necessary?
	}



};


