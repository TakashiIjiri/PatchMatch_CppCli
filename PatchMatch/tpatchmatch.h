#pragma once

#pragma unmanaged


#include "stdio.h"
#include "stdlib.h"
#include "timage2D.h"
#include <vector>
#include <string>
#include <time.h>

#include <omp.h>

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


class NffPix
{
public:
	int x, y;
	NffPix(int _x = 0, int _y = 0)
	{
		x = _x;
		y = _y;
	}

	NffPix(const NffPix &src)
	{
		x = src.x;
		y = src.y;
	}

	NffPix &operator=(const NffPix &src)
	{
		x = src.x;
		y = src.y;
		return *this;
	}

	void set(int _x, int _y){ 
		x = _x; 
		y = _y;
	}
};




static double diffOfPatches
(
	const int     &winR,
	const Image2D &imgS, const int &sX, const int &sY,
	const Image2D &imgT, const int &tX, const int &tY,
	const double maxDiff
)
{
	const int sW = imgS.getWidth ();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	double diff = 0;

	if(tX < 0          || tY < 0          ) return DBL_MAX;
	if(tX + winR >= tW || tY + winR >= tH ) return DBL_MAX;


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
		if( diff > maxDiff ) return DBL_MAX;
	}

	return diff;
}





//initialize matching imgS --> imgT 
inline void patchmatch_initNNF
(
	const int     &winR,
	const Image2D &imgS,
	const Image2D &imgT,
	NffPix *nnf //size is [imgS.W * imgS.H * 2], should be allocated
)
{
	const int sW = imgS.getWidth ();
	const int sH = imgS.getHeight();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	for( int i = 0, s = sW*sH; i < s; ++i) nnf[i].set(0,0);

	for( int y = 0; y <= sH - winR; ++y)
	{
		for( int x = 0; x <= sW - winR; ++x)
		{
			int targetX = (int)( rand() / (double) RAND_MAX * (tW - winR) );
			int targetY = (int)( rand() / (double) RAND_MAX * (tH - winR) );
			double diff = diffOfPatches(winR, imgS,x,y, imgT,targetX,targetY, DBL_MAX);
			nnf[x + y*sW] .set( targetX - x, targetY - y);
		}
	}
}




#define SKIP_I   3
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

	NffPix *nnf //initialized (or already updated several times)
)
{
	const int sW = imgS.getWidth ();
	const int sH = imgS.getHeight();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	/*
The PatchMatch paper said that
[To do so, we alternate between iterations of random
search and propagation, where each stage addresses the entire offset
field in parallel. Although propagation is inherently a serial operation,
we adapt the jump flood scheme of Rong and Tan [2006]
to perform propagation over several iterations.]

This code simply compute the update in parallel
	*/

#pragma omp parallel for schedule(static)
	for( int y = 0; y <= sH - winR; y += SKIP_I)
	{
		//printf("%d(%d)",y, omp_get_thread_num() );
		for( int x = 0; x <= sW - winR; x += SKIP_I)
		{
			const int idx = y * sW + x;
			double diff = diffOfPatches( winR, imgS,x,y, imgT, x+nnf[idx].x, y+nnf[idx].y, DBL_MAX);

			if( x >= SKIP_I) //check left
			{
				NffPix *n = &nnf[idx-SKIP_I];
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT, x+n->x, y+n->y, diff);
				if( tmpDiff < diff) 
				{
					nnf[idx].set(n->x, n->y);
					diff = tmpDiff;
				}
			}
			
			if( y >= SKIP_I)//check up
			{
				NffPix *n = &nnf[idx-sW*SKIP_I];

				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT, x+n->x, y+n->y, diff);
				if( tmpDiff < diff) 
				{
					nnf[idx].set(n->x, n->y);
					diff = tmpDiff;
				}

			}

			//random search
			for( int i=0; i<RAND_SEARCH_NUM; ++i)
			{
				int tX = (int)( rand() / (double) RAND_MAX * (tW - winR) );
				int tY = (int)( rand() / (double) RAND_MAX * (tH - winR) );
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT,tX,tY, diff );
				if( tmpDiff < diff){
					nnf[idx].set(tX - x, tY - y);
					diff = tmpDiff;
				}
			}
		}
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

	NffPix *nnf //initialized (or already updated several times)
)
{
	//W = 10, R = 3,        7,6,5,4,3,2,1,0
	//W = 10, R = 3, S = 2,   6,  4,  2,  0,   N = (W-R) - (W-R) % S



	const int sW = imgS.getWidth ();
	const int sH = imgS.getHeight();
	const int tW = imgT.getWidth ();
	const int tH = imgT.getHeight();

	const int yStart = (sH - winR) - (sH - winR)%SKIP_I;
	const int xStart = (sW - winR) - (sW - winR)%SKIP_I;

#pragma omp parallel for schedule(static)
	for( int y = yStart; y >= 0; y -= SKIP_I)
	{
		for( int x = (sW - winR) - (sW - winR)%SKIP_I; x >= 0; x -= SKIP_I)
		{
			const int idx = y * sW + x;
			double diff = diffOfPatches( winR, imgS,x,y, imgT, x+nnf[idx].x, y+nnf[idx].y, DBL_MAX);

			if( x != xStart) //check right
			{
				NffPix *n = &nnf[idx+SKIP_I];
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT, x+n->x, y+n->y, diff);
				if( tmpDiff < diff)
				{
					nnf[idx].set(n->x, n->y);
					diff = tmpDiff;
				}
			}
			
			if( y != yStart )//check bottom
			{
				NffPix *n = &nnf[idx+SKIP_I*sW];
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT, x+n->x, y+n->y, diff);
				if( tmpDiff < diff)
				{
					nnf[idx].set(n->x, n->y);
					diff = tmpDiff;
				}
			}
		
			//random search
			for( int i=0; i<RAND_SEARCH_NUM; ++i)
			{
				int tX = (int)( rand() / (double) RAND_MAX * (tW - winR) );
				int tY = (int)( rand() / (double) RAND_MAX * (tH - winR) );
				double tmpDiff = diffOfPatches( winR, imgS,x,y, imgT,tX,tY, diff);
				if( tmpDiff < diff){
					nnf[idx].set(tX - x, tY - y);
					diff = tmpDiff;
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

	const NffPix *nnf_StoT,
	const NffPix *nnf_TtoS
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
		for( int ty = 0; ty <= tH - winR; ty += SKIP_I)
		for( int tx = 0; tx <= tW - winR; tx += SKIP_I)
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
		for( int sy = 0; sy <= sH - winR; sy += SKIP_I)
		for( int sx = 0; sx <= sW - winR; sx += SKIP_I)
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
		if( rgba[4*i + 3] != 0)
		{
			imgT[4*i+0] = (byte)(rgba[4*i + 0] / rgba[4*i + 3]);
			imgT[4*i+1] = (byte)(rgba[4*i + 1] / rgba[4*i + 3]);
			imgT[4*i+2] = (byte)(rgba[4*i + 2] / rgba[4*i + 3]);		
		}
		else
		{
			//no vote exist (ignore)		
		}
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
	NffPix *nnf_TtoS = new NffPix[tW*tH];

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
	NffPix *nnf_TtoS = new NffPix[tW*tH];
	NffPix *nnf_StoT = new NffPix[sW*sH];

	patchmatch_initNNF( winR, imgT, imgS,  nnf_TtoS );
	patchmatch_initNNF( winR, imgS, imgT,  nnf_StoT );
	patchmatch_updateImageByVoting( winR, imgS, imgT, nnf_StoT, nnf_TtoS);
	imgT.SaveAs("00.png");

	for( int i=0; i < 5; ++i)
	{
		printf( "update T-->S");
		patchmatch_updateForeRaster(winR, imgT, imgS, nnf_TtoS);
		patchmatch_updateBackRaster(winR, imgT, imgS, nnf_TtoS);
		printf( "update S-->T");
		patchmatch_updateForeRaster(winR, imgS, imgT, nnf_StoT);
		patchmatch_updateBackRaster(winR, imgS, imgT, nnf_StoT);
		printf( "Voting");
		patchmatch_updateImageByVoting( winR, imgS, imgT, nnf_StoT, nnf_TtoS);	
		printf( "done\n");

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
		clock_t t0 = clock();
		const int sW = imgS_multi[lv].getWidth ();
		const int sH = imgS_multi[lv].getHeight();
		const int tW = imgT_multi[lv].getWidth ();
		const int tH = imgT_multi[lv].getHeight();

		NffPix *nnf_TtoS = new NffPix[tW*tH];

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
		for( int i=0; i < 10; ++i)
		{
			patchmatch_updateForeRaster    (winR_multi[lv], imgT_multi[lv], imgS_multi[lv],    nnf_TtoS);
			patchmatch_updateBackRaster    (winR_multi[lv], imgT_multi[lv], imgS_multi[lv],    nnf_TtoS);
			patchmatch_updateImageByVoting( winR_multi[lv], imgS_multi[lv], imgT_multi[lv], 0, nnf_TtoS);	

			string fname = "res" + std::to_string(lv) + string("_") + std::to_string(i) + string(".png");
			imgT_multi[lv].SaveAs(fname.c_str());
			printf( "%s\n", fname.c_str());
		}

		clock_t t1 = clock();
		printf( "%f\n", (t1-t0)/(double)CLOCKS_PER_SEC);

		delete[] nnf_TtoS;
	}

	_imgT = imgT_multi.back();
}





















#pragma managed
