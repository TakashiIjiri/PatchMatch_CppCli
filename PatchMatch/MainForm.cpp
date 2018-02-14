#include "stdafx.h"
#include "MainForm.h"
#include "timageloader.h"
#include "TCore.h"

#include <string>

using namespace System;
using namespace System::IO;
using namespace System::Runtime::InteropServices;

using namespace PatchMatch;
using namespace std;






MainForm::MainForm()
{
	InitializeComponent();

	
	OpenFileDialog^ dlg = gcnew OpenFileDialog; 
	dlg->Filter = "sample texture (*.bmp,*.png)|*.bmp;*.png;"; 
	if (dlg->ShowDialog() == System::Windows::Forms::DialogResult::Cancel) return;

	
	IntPtr mptr = Marshal::StringToHGlobalAnsi(dlg->FileName);
	string fname = static_cast<const char*>(mptr.ToPointer());
	
	TCore::getInst()->initInputImage(fname.c_str());

	RepaintForm();
}




void MainForm::RepaintForm()
{
	Bitmap^ bmpPicBox = gcnew Bitmap(pictureBox1->Width, pictureBox1->Height);
	pictureBox1->Image = bmpPicBox;
	Graphics^ g = Graphics::FromImage(pictureBox1->Image);


	System::Drawing::Bitmap ^bmp1, ^bmp2;

	//draw input image
	t_RgbaToBmp( TCore::getInst()->m_imgIn.getWidth(), 
		         TCore::getInst()->m_imgIn.getHeight(), 
		         TCore::getInst()->m_imgIn.getRGBA  (), bmp1);

		//draw input image
	t_RgbaToBmp( TCore::getInst()->m_imgOut.getWidth(), 
		         TCore::getInst()->m_imgOut.getHeight(), 
		         TCore::getInst()->m_imgOut.getRGBA  (), bmp2);


	//draw output image

	g->DrawImage( bmp1, 0,0);
	g->DrawImage( bmp2, TCore::getInst()->m_imgIn.getWidth() + 10,0);

}
