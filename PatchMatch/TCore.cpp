#include "stdafx.h"
#include "TCore.h"

#include "tpatchmatch.h"


TCore::TCore()
{

}



void TCore::synthesizeLargerTexture_forwardOnly(double sizeRate)
{
	const int newW = (int)(m_imgIn.getWidth () * sizeRate);
	const int newH = (int)(m_imgIn.getHeight() * sizeRate);
	m_imgOut.Allocate( newW, newH );

	patchmatch_TexSynth_multiLv_coherence( 3, 25, m_imgIn, m_imgOut);

}


//todo down/up sampling