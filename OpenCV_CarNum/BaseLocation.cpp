#define _CRT_SECURE_NO_WARNINGS  //����������#include<stdio.h>֮ǰ�����򻹻ᱨ��  
#include "stdafx.h"
#include "BaseLocation.h"


BaseLocation::BaseLocation()
{
}


BaseLocation::~BaseLocation()
{
}

//����Ϳ�߱��Ƿ����Ԥ�ڵ�Ҫ��
int BaseLocation::verifySizes(RotatedRect rotated_rect)
{
	//�ݴ���
	float error = .75f;
	//�й����Ʊ�׼440mm*140mm
	//136 * 32 // ����ͼƬ�Ĵ�С
	float aspect = float(WIDTH) / float(HEIGHT);

	//��С ������ �����ϵĶ���
	//������ž��� ��ʱ����
	int min = 20 * aspect * 20;
	int max = 180 * aspect * 180;

	//�������� error��ΪҲ����
	float rmin = aspect - aspect * error;
	float rmax = aspect + aspect * error;
	//���
	float area = rotated_rect.size.height * rotated_rect.size.width;
	//���������ĳ��� ��ȸ�С���� �߿��
	float r = (float)rotated_rect.size.width / (float)rotated_rect.size.height;
	if (r < 1) r = (float)rotated_rect.size.height / (float)rotated_rect.size.width;
	if ((area < min || area > max) || (r < rmin || r > rmax))
		return 0;
	return 1;
}

// ���ƺ�����ת
void BaseLocation::tortuosity(Mat src, vector<RotatedRect>& rects, vector<Mat>& dst_plates)
{
	for (auto roi_rect : rects) {
		float r = (float)roi_rect.size.width / (float)roi_rect.size.height;
		float roi_angle = roi_rect.angle;
		Size roi_rect_size = roi_rect.size;
		int isVer = 0; // ͼƬ�Ƿ�����ֱ��
					   //�������
		if (r < 1) {
			//roi_angle = 90 + roi_angle;
			isVer = 1;
			swap(roi_rect_size.width, roi_rect_size.height);
		}
		Rect2f rect;
		safeRect(src, roi_rect, rect);// ��֤������ͼƬ��Χ�ڣ��ڷ�Χ�ڵĲ��ֵĴ���rect
		Mat src_rect = src(rect);  // �õ�rect ��ͼƬ
								   //�����roi�����ĵ� ����ȥ���Ͻ����������������ͼ��
		Point2f roi_ref_center = roi_rect.center - rect.tl();
		Mat deskew_mat;
		//����Ҫ��ת�� ��ת�Ƕ�Сû��Ҫ��ת��
		//
		if (isVer) {
			transpose(src, deskew_mat);
			flip(deskew_mat, deskew_mat, 1);
		}
		else {
			if ((roi_angle - 5 < 0 && roi_angle + 5 > 0)) {
				deskew_mat = src_rect.clone();
			}
			else {
				Mat rotated_mat;
				rotation(src_rect, rotated_mat, roi_rect_size, roi_ref_center, roi_angle);
				deskew_mat = rotated_mat;
			}
		}

		//һ�����¿�߱ȷ�Χ��������ݿ��Ը���ʵ��������е���
		if (deskew_mat.cols * 1.0 / deskew_mat.rows > 2.3 &&
			deskew_mat.cols * 1.0 / deskew_mat.rows < 6) {
			Mat plate_mat;
			plate_mat.create(HEIGHT, WIDTH, CV_8UC3);
			resize(deskew_mat, plate_mat, plate_mat.size());
			dst_plates.push_back(plate_mat);
		}
		deskew_mat.release();
	}
}

/*
��ȡһ��RotatedRect��ͼƬ���������ȡ��ͼƬ�ǰ���RotatedRect����С��ͼƬ����
���ܳ���src��ͼƬ�ķ�Χ
*/
void BaseLocation::safeRect(Mat src, RotatedRect & rect, Rect2f & dst_rect)
{
	Rect2f boudRect = rect.boundingRect2f();
	float tl_x = boudRect.x > 0 ? boudRect.x : 0;
	float tl_y = boudRect.y > 0 ? boudRect.y : 0;

	float br_x = boudRect.x + boudRect.width < src.cols
		? boudRect.x + boudRect.width - 1
		: src.cols - 1;
	float br_y = boudRect.y + boudRect.height < src.rows
		? boudRect.y + boudRect.height - 1
		: src.rows - 1;

	float roi_width = br_x - tl_x;
	float roi_height = br_y - tl_y;

	if (roi_width <= 0 || roi_height <= 0) return;
	dst_rect = Rect2f(tl_x, tl_y, roi_width, roi_height);
}

void BaseLocation::rotation(Mat src, Mat & dst, Size rect_size, Point2f center, double angle)
{

	//�����ת����
	Mat rot_mat = getRotationMatrix2D(center, angle, 1);

	//���÷���任
	Mat mat_rotated;
	//��ֹ������ת��ʱ��ͼƬ������
	warpAffine(src, mat_rotated, rot_mat, Size(sqrt(pow(src.cols, 2) + pow(src.rows, 2)), sqrt(pow(src.cols, 2) + pow(src.rows, 2))),
		CV_INTER_CUBIC);
	//��ȡ
	getRectSubPix(mat_rotated, Size(rect_size.width, rect_size.height),
		center, dst);
	mat_rotated.release();
	rot_mat.release();
}
