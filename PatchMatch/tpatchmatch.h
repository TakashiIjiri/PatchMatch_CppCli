#pragma once

#pragma unmanaged


#include "stdio.h"
#include "stdlib.h"
#include "timage2D.h"
#include <vector>
#include <string>
using namespace std;



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


//initialize matching imgS --> imgT 
inline void patchmatch_initNNF
(
	const int     &winR,
	const Image2D &imgS,
	const Image2D &imgT,
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
	const Image2D &imgS, const int &sX, const int &sY,
	const Image2D &imgT, const int &tX, const int &tY
)
{
	const int sW = imgS.getWidth ();
	const int tW = imgT.getWidth ();

	double diff = 0;

	for( int y = 0; y < winR; ++y)
	{
		const int tmpSi = 4*( sX + sW*(sY + y) ); //高速化のため
		const int tmpTi = 4*( tX + tW*(tY + y) ); //高速化のため
		for( int x = 0; x < winR; ++x)
		{
			const int si = tmpSi + 4*x; //const int si = 4*( sX + x + sW*(sY + y) );
			const int ti = tmpTi + 4*x; //const int ti = 4*( tX + x + tW*(tY + y) );
			double dx =  imgS[ si + 0] - imgT[ ti + 0];
			double dy =  imgS[ si + 1] - imgT[ ti + 1];
			double dz =  imgS[ si + 2] - imgT[ ti + 2];
			diff += dx*dx + dy*dy + dz*dz;
		}	
	}

	return diff;
}


#define RAND_SEARCH_NUM 3


//
//前方向RasterizationによるNNFの更新
//
//update matching imgS --> imgT 

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



	
	printf( "\n\n\ncheck!! before");
	for( int sy = 0; sy <= sH - winR; ++sy)
	for( int sx = 0; sx <= sW - winR; ++sx)
	{
		int tx = sx + nnf[sy*sW +sx].x;
		int ty = sy + nnf[sy*sW +sx].y;

		if( tx < 0) printf( "a");
		if( ty < 0) printf( "b");
		if( tx + winR - 1 >= tW) printf("c");
		if( tx + winR - 1 >= tW) printf("d");
	}






	for( int y = 0; y <= sH - winR; ++y)
	{
		for( int x = 0; x <= sW - winR; ++x)
		{
			const int idx = y * sW + x;
			double diff = diffOfPatches( winR, imgS,x,y, imgT, x+nnf[idx].x, y+nnf[idx].y);

			if( x > 0) //check left
			{
				ただ入れるだけだと踏み越え可能性あり！！！！！！！！！！！

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
			for( int i=0; i<RAND_SEARCH_NUM; ++i)
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


	//check!!
	printf( "\ncheck!! after\n\n\n");

	for( int sy = 0; sy <= sH - winR; ++sy)
	for( int sx = 0; sx <= sW - winR; ++sx)
	{
		int tx = sx + nnf[sy*sW +sx].x;
		int ty = sy + nnf[sy*sW +sx].y;

		if( tx < 0) printf( "a");
		if( ty < 0) printf( "b");
		if( tx + winR - 1 >= tW) printf("c");
		if( tx + winR - 1 >= tW) printf("d");
	}



}




//
//
//逆方向RasterizationによるNNFの更新
//
//update matching imgS --> imgT 
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
			for( int i=0; i<RAND_SEARCH_NUM; ++i)
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

			//だめ、マイナスの値が出てる

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

			if( tx < 0 || ty < 0) printf( "*");
			if( tx + winR - 1 >= tW || ty + winR - 1 >= tH) printf( "@");



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





void patchmatch_TexSynth_coherence
(
const int winR     ,
const Image2D &imgS, //allocated 
      Image2D &imgT //allocated (without pixel color info)
)
{
	const int sW = imgS.getWidth ();
	const int sH = imgS.getHeight();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	//OfstPix *nnf_StoT,
	OfstPix *nnf_TtoS = new OfstPix[tW*tH];

	patchmatch_initNNF( winR, imgT, imgS,  nnf_TtoS );
	patchmatch_updateImageByVoting( winR, imgS, imgT, 0, nnf_TtoS);

	for( int i=0; i < 5; ++i)
	{
		patchmatch_updateForeRaster(winR, imgT, imgS, nnf_TtoS);
		patchmatch_updateBackRaster(winR, imgT, imgS, nnf_TtoS);
		patchmatch_updateImageByVoting( winR, imgS, imgT, 0, nnf_TtoS);	

		string fname = "res" + std::to_string(i) + string(".png");
		imgT.SaveAs(fname.c_str());
		printf( "%s\n", fname.c_str());

	}

	delete[] nnf_TtoS;
}




void patchmatch_TexSynth_coh_comp
(
const int winR     ,
const Image2D &imgS, //allocated 
      Image2D &imgT //allocated (without pixel color info)
)
{
	const int sW = imgS.getWidth ();
	const int sH = imgS.getHeight();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	//OfstPix *nnf_StoT,
	OfstPix *nnf_TtoS = new OfstPix[tW*tH];
	OfstPix *nnf_StoT = new OfstPix[sW*sH];

	patchmatch_initNNF( winR, imgT, imgS,  nnf_TtoS );
	patchmatch_initNNF( winR, imgS, imgT,  nnf_StoT );
	patchmatch_updateImageByVoting( winR, imgS, imgT, nnf_StoT, nnf_TtoS);
	imgT.SaveAs("00.png");

	for( int i=0; i < 5; ++i)
	{
		printf( "a");
		patchmatch_updateForeRaster(winR, imgT, imgS, nnf_TtoS);
		patchmatch_updateBackRaster(winR, imgT, imgS, nnf_TtoS);
		printf( "b");
		patchmatch_updateForeRaster(winR, imgS, imgT, nnf_StoT);
		patchmatch_updateBackRaster(winR, imgS, imgT, nnf_StoT);
		printf( "c");
		patchmatch_updateImageByVoting( winR, imgS, imgT, nnf_StoT, nnf_TtoS);	
		printf( "d");

		string fname = "res" + std::to_string(i) + string(".png");
		imgT.SaveAs(fname.c_str());
		printf( "e");

		printf( "%s\n", fname.c_str());
	}

	delete[] nnf_TtoS;
	delete[] nnf_StoT;
}







void patchmatch_TexSynth_multiLv_coherence
(
const int level,
const int _winR ,
const Image2D &_imgS, //allocated 
      Image2D &_imgT //allocated (without pixel color info)
)
{
	//prepare multi level imgS
	Image2D tmpS = _imgS;
	Image2D tmpT = _imgT;
	int     tmpR = _winR;
	vector<Image2D> imgS_multi(level);
	vector<Image2D> imgT_multi(level);
	vector<int>     winR_multi(level);
	for( int i=0; i < level; ++i) 
	{
		imgS_multi[level - 1 - i] = tmpS;
		imgT_multi[level - 1 - i] = tmpT;
		winR_multi[level - 1 - i] = tmpR;
		tmpS.HalfDownSample();
		tmpT.HalfDownSample();
		tmpR /= 2;
	}



	for( int lv = 0; lv < level; ++lv)
	{

		const int sW = imgS_multi[lv].getWidth ();
		const int sH = imgS_multi[lv].getHeight();
		const int tW = imgT_multi[lv].getWidth ();
		const int tH = imgT_multi[lv].getHeight();

		OfstPix *nnf_TtoS = new OfstPix[tW*tH];

		//init by random
		patchmatch_initNNF( winR_multi[lv], imgT_multi[lv], imgS_multi[lv],  nnf_TtoS );

		if( lv == 0 )
		{
			patchmatch_updateImageByVoting( winR_multi[lv], imgS_multi[lv], imgT_multi[lv], 0, nnf_TtoS);
			imgT_multi[lv].SaveAs("00.png");

		}
		else 
		{
			imgT_multi[lv] = imgT_multi[lv-1];
			imgT_multi[lv].doubleUpSample();
		}

		//update 
		for( int i=0; i < 5; ++i)
		{
			patchmatch_updateForeRaster    (winR_multi[lv], imgT_multi[lv], imgS_multi[lv],    nnf_TtoS);
			patchmatch_updateBackRaster    (winR_multi[lv], imgT_multi[lv], imgS_multi[lv],    nnf_TtoS);
			patchmatch_updateImageByVoting( winR_multi[lv], imgS_multi[lv], imgT_multi[lv], 0, nnf_TtoS);	

			string fname = "res" + std::to_string(lv) + string("_") + std::to_string(i) + string(".png");
			imgT_multi[lv].SaveAs(fname.c_str());
			printf( "%s\n", fname.c_str());
		}


		delete[] nnf_TtoS;
	}

	_imgT = imgT_multi.back();
}





















#pragma managed
