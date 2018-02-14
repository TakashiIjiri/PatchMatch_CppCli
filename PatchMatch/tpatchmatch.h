#pragma once

#pragma unmanaged


#include "stdio.h"
#include "stdlib.h"
#include "timage2D.h"



//compute patch matching 
//  from all patches of imgS (source) to 
//  to       patches of imgT (target)

// data representation
// imgS : Image2D (rgba)
// imgT : Image2D (rgba)
// R    : size of patch

// matching is represented in the form of NNF (nearest neighbor field)
// resolution of NNF : same as imgS
// pixel of NNF      : contain 2D offset value (encoded in 1D array whose size is W * H * 2 )
//    + NNF[y,x] = (dy, dx) when a patch imgS[y:y+R, x:x+R] corresponds to imgT[y+dy:y+dy+R, x+dx:x+dx+R]
//    + NNF[y,x] = 0 for (y > imgS.H - R) or (x > imgS.W - R) !! 
//
//
//


class OfstPix
{
public:
	int x,y;
	OfstPix(int _x = 0, int _y = 0)
	{
		x = _x;
		y = _y;
	}

	OfstPix(const OfstPix &src)
	{
		x = src.x;
		y = src.y;
	}

	OfstPix &operator=(const OfstPix &src)
	{
		x = src.x;
		y = src.y;
		return *this;
	}

	void setZero(){ x= y=0;}
	void set(int _x, int _y){ 
		x = _x; 
		y = _y;
	}
};


inline void patchmatch_initNNF
(
	const Image2D &imgS,
	const Image2D &imgT,
	const int     &winR,
	OfstPix *nnf //size is [imgS.W * imgS.H * 2], should be allocated
)
{
	const int sW = imgS.getWidth ();
	const int sH = imgS.getHeight();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	for( int i = 0, s = sW*sH; i < s; ++i) nnf[i].setZero();

	for( int y = 0; y <= sH - winR; ++y)
	{
		for( int x = 0; x <= sW - winR; ++x)
		{
			int targetX = (int)( rand() / (double) RAND_MAX * (tW - winR) );
			int targetY = (int)( rand() / (double) RAND_MAX * (tH - winR) );
			nnf[x + y*sW] .set( targetX - x, targetY - y);
		}
	}
}




static double diffOfPatches
(
	const int     &winR,
	const Image2D &imgS, const int sX, const int sY,
	const Image2D &imgT, const int tX, const int tY
)
{
	const int sW = imgS.getWidth ();
	const int tW = imgT.getWidth ();


	double diff = 0;

	for( int y = 0; y < winR; ++y)
	{
		for( int x = 0; x < winR; ++x)
		{
			const int si = 4*( sX + x + sW*(sY + y) );
			const int ti = 4*( tX + x + tW*(tY + y) );
			double dx =  imgS[ si + 0] - imgT[ ti + 0];
			double dy =  imgS[ si + 1] - imgT[ ti + 1];
			double dz =  imgS[ si + 2] - imgT[ ti + 2];
			diff += dx*dx + dy*dy + dz*dz;
		}	
	}

	return diff;
}


//
//
//前方向RasterizationによるNNFの更新
//
//
void patchmatch_updateForeRaster
(
	const int     &winR,
	const Image2D &imgS,
	const Image2D &imgT,

	OfstPix *nnf //initialized (or already updated several times)
)
{
	const int sW = imgS.getWidth ();
	const int sH = imgS.getHeight();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	for( int y = 0; y <= sH - winR; ++y)
	{
		for( int x = 0; x <= sW - winR; ++x)
		{
			const int idx = y * sW + x;
			double diff = diffOfPatches( winR, imgS,x,y, imgT, x+nnf[idx].x, y+nnf[idx].y);

			if( x > 0) //check left
			{
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT, x+nnf[idx-1].x, y+nnf[idx-1].y);
				if( tmpDiff < diff)
				{
					diff = tmpDiff;
					nnf[idx] = nnf[idx-1];
				}
			}
			
			if( y > 0)//check up
			{
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT, x+nnf[idx-sW].x, y+nnf[idx-sW].y);
				if( tmpDiff < diff)
				{
					diff = tmpDiff;
					nnf[idx] = nnf[idx-sW];
				}
			}

			//random search
			for( int i=0; i<10; ++i)
			{
				int tX = (int)( rand() / (double) RAND_MAX * (tW - winR) );
				int tY = (int)( rand() / (double) RAND_MAX * (tH - winR) );
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT,tX,tY );
				if( tmpDiff < diff)
				{
					diff = tmpDiff;
					nnf[idx].set(tX - x, tY - y);
				}
			}
		}
	}
}




//
//
//逆方向RasterizationによるNNFの更新
//
//
void patchmatch_updateBackRaster
(
	const int     &winR,
	const Image2D &imgS,
	const Image2D &imgT,

	OfstPix *nnf //initialized (or already updated several times)
)
{
	const int sW = imgS.getWidth ();
	const int sH = imgS.getHeight();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	for( int y = sH - winR; y >= 0; --y)
	{
		for( int x = sW - winR; x >= 0; --x)
		{
			const int idx = y * sW + x;
			double diff = diffOfPatches( winR, imgS,x,y, imgT, x+nnf[idx].x, y+nnf[idx].y);

			if( x < sW - winR) //check right
			{
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT, x+nnf[idx+1].x, y+nnf[idx+1].y);
				if( tmpDiff < diff)
				{
					diff = tmpDiff;
					nnf[idx] = nnf[idx+1];
				}
			}
			
			if( y < sH - winR )//check bottom
			{
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT, x+nnf[idx+sW].x, y+nnf[idx+sW].y);
				if( tmpDiff < diff)
				{
					diff = tmpDiff;
					nnf[idx] = nnf[idx+sW];
				}
			}

			//random search
			for( int i=0; i<10; ++i)
			{
				int tX = (int)( rand() / (double) RAND_MAX * (tW - winR) );
				int tY = (int)( rand() / (double) RAND_MAX * (tH - winR) );
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT,tX,tY );
				if( tmpDiff < diff)
				{
					diff = tmpDiff;
					nnf[idx].set(tX - x, tY - y);
				}
			}
		}
	}
}






//update Image T by copying patches from imgS to imgT
// 
// nnf_StoT represent patch match from S to T
// nnf_TtoS represent patch match from T to S
//
// (*) note:  if nnf_StoT or nnf_TtoS is null, it will be ingnored
void patchmatch_updateImageByVoting
(
	const int     &winR,
	const Image2D &imgS,

	Image2D &imgT,

	const OfstPix *nnf_StoT,
	const OfstPix *nnf_TtoS
)
{
	const int sW = imgS.getWidth ();
	const int sH = imgS.getHeight();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	//(r,g,b,votenum)
	int *rgba = new int[tW*tH*4];
	memset( rgba, 0, sizeof(int) * tW*tH*4);


	if( nnf_TtoS != 0)
	{
		for( int ty = 0; ty <= tH - winR; ++ty)
		for( int tx = 0; tx <= tW - winR; ++tx)
		{
			int sx = tx + nnf_TtoS[ty*tW +tx].x;
			int sy = ty + nnf_TtoS[ty*tW +tx].y;

			for( int yy = 0; yy < winR; ++yy)
			for( int xx = 0; xx < winR; ++xx)
			{
				int ti = 4*(tx+xx + (ty+yy) * tW);
				int si = 4*(sx+xx + (sy+yy) * sW);
				rgba[ti+0] += imgS[si+0];
				rgba[ti+1] += imgS[si+1];
				rgba[ti+2] += imgS[si+2];
				rgba[ti+3] += 1;
			}
		}
	}

	
	if( nnf_StoT != 0)
	{
		for( int sy = 0; sy <= sH - winR; ++sy)
		for( int sx = 0; sx <= sW - winR; ++sx)
		{
			int tx = sx + nnf_StoT[sy*sW +sx].x;
			int ty = sy + nnf_StoT[sy*sW +sx].y;

			for( int yy = 0; yy < winR; ++yy)
			for( int xx = 0; xx < winR; ++xx)
			{
				int ti = 4*(tx+xx + (ty+yy) * tW);
				int si = 4*(sx+xx + (sy+yy) * sW);
				rgba[ti+0] += imgS[si+0];
				rgba[ti+1] += imgS[si+1];
				rgba[ti+2] += imgS[si+2];
				rgba[ti+3] += 1;
			}
		}
	}


	for( int i=0, s=tW*tH; i<s; ++i)
	{
		imgT[4*i+0] = (byte)(rgba[4*i + 0] / rgba[4*i + 3]);
		imgT[4*i+1] = (byte)(rgba[4*i + 1] / rgba[4*i + 3]);
		imgT[4*i+2] = (byte)(rgba[4*i + 2] / rgba[4*i + 3]);
	}
}
























#pragma managed
