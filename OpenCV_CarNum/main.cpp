// OpenCV_carPlate.cpp : �������̨Ӧ�ó������ڵ㡣
//
#define _CRT_SECURE_NO_WARNINGS  //����������#include<stdio.h>֮ǰ�����򻹻ᱨ��  
#include "stdafx.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>


CarPlateRecongize * carPlateRecongize = 0;


void init() {
	carPlateRecongize = new CarPlateRecongize("HOG_SVM_DATA.xml",
		"HOG_ANN_DATA.xml",
		"HOG_ANN_ZH_DATA.xml");
}
int main()
{
	init();
	Mat img = imread("C:/Users/Administrator/Desktop/DL/benchi.jpg");
	Mat plate;
	cout << carPlateRecongize->plateRecongize(img, plate);

	waitKey(0);
	system("pause");
	return 0;
}

