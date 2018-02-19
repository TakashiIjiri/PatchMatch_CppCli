#include "stdafx.h"
#include "TCore.h"

#include "tpatchmatch.h"


TCore::TCore()
{

}



void TCore::synthesizeLargerTexture(double sizeRate)
{
	const int newW = (int)(m_imgIn.getWidth () * sizeRate);
	const int newH = (int)(m_imgIn.getHeight() * sizeRate);
	m_imgOut.Allocate( newW, newH );

	//1. multi level synthesis only with coherence term 
	patchmatch_TexSynth_multiLv_coherence( 4, 31, m_imgIn, m_imgOut);
	
	//2. single level synthesis with coherence and completness term
	//patchmatch_TexSynth_coh_comp(17, m_imgIn, m_imgOut);

}


//todo down/up sampling