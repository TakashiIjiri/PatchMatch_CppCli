// PatchMatch.cpp : メイン プロジェクト ファイルです。

#include "stdafx.h"
#include "MainForm.h"

#include <stdio.h>

using namespace System;
using namespace PatchMatch;



[STAThreadAttribute]
int main()
{
   printf("Hello World");


	MainForm::getInst()->ShowDialog();

	return 0;
}
