#define _CRT_SECURE_NO_WARNINGS  //����������#include<stdio.h>֮ǰ�����򻹻ᱨ��  
#include "stdafx.h"
#include "CarPlateRecongize.h"
#define TAG

char CHARS[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','J','K','L','M','N','P','Q','R','S','T','U','V','W','X','Y','Z' };

string ZHCHARS[] = { "��","��","��","��","��","��","��","��","��","��","��","��","��","³","��","��","��","��","��","��","��","��","��","��","��","ԥ","��","��","��","��","��" };

CarPlateRecongize::CarPlateRecongize(const char* svm_path, const char * ann_path, const char * ann_zh_path)
{
	// ��ʼ��
	svm = SVM::load(svm_path);
	svm_hog = new HOGDescriptor(Size(128, 64), Size(16, 16), Size(8, 8), Size(8, 8), 3);
	ann_hog = new HOGDescriptor(Size(32, 32), Size(16, 16), Size(8, 8), Size(8, 8), 3);

	ann = ANN_MLP::load(ann_path);
	ann_zh = ANN_MLP::load(ann_zh_path);
}


CarPlateRecongize::~CarPlateRecongize()
{
}

string CarPlateRecongize::plateRecongize(Mat src, Mat & plate)
{
	findPlateLocation(src, plate);
	if (plate.empty()) {
		return "δ�ҵ�����";
	}

	Mat plate_gray;
	Mat plate_threshold;
	cvtColor(plate, plate_gray, CV_BGR2GRAY);
	threshold(plate_gray, plate_threshold, 0, 255, THRESH_OTSU + THRESH_BINARY);
#ifdef TAG
	imshow("plate_threshold", plate_threshold);

#endif
	clearFixPoint(plate_threshold);
	vector<vector<Point>> contours;
	vector<Rect>	vec_rect;
	findContours(plate_threshold, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
	for (auto cnt : contours)
	{
		Rect rect = boundingRect(cnt);
		Mat aux_roi = plate_threshold(rect);
		if (verifyCharSizes(aux_roi))
		{
			vec_rect.push_back(rect);
		}
		aux_roi.release();
	}
	if (vec_rect.empty()) {
		return "δ�ҵ����ƺ����ַ�";
	}
	//����
	vector<Rect> sorted_rect(vec_rect);
	sort(sorted_rect.begin(), sorted_rect.end(),
		[](const Rect &r1, const Rect &r2) {return r1.x < r2.x; });
	int city_index = getCityIndex(sorted_rect);

	Rect chinese_rect;
	getChineseRect(sorted_rect[city_index], chinese_rect);
	vector<Mat> vec_chars;
	Mat dst = plate_threshold(chinese_rect);
	vec_chars.push_back(dst);

	int count = 6;
	for (int i = city_index; i < sorted_rect.size() && count; i++) {
		Rect rect = sorted_rect[i];
		Mat dst = plate_threshold(rect);
		vec_chars.push_back(dst);
		--count;
	}

	string plate_result;
	predict(vec_chars, plate_result);
	return plate_result;
}

//���ҳ������ڵ�����
void CarPlateRecongize::findPlateLocation(Mat src, Mat & dst)
{
	vector<Mat> plates;// ��������ͼ��
	vector<Mat> solel_plates;// ��������ͼ��

							 //��һ���׶Σ�����ͼƬ�����е�����
	plateLocate(src, solel_plates);
	plates.insert(plates.end(), solel_plates.begin(), solel_plates.end());


	//�ڶ����׶Σ��������е��������ĸ������ƣ�����ѧϰ��ʹ��SVM
	int index = -1;
	// ��ѯ�����ҵ�������
	float minScore = 3;
	for (int i = 0; i < plates.size(); i++)
	{
		auto plate = plates[i];
		Mat p;
#ifdef TAG
		char * string = (char *)malloc(sizeof(char) * 128);
#endif
		cvtColor(plate, src, CV_BGR2GRAY);
		threshold(src, src, 0, 255, THRESH_BINARY + THRESH_OTSU);
#ifdef TAG

#endif
		getHOGFeatures(svm_hog, src, p);

		Mat sample = p.reshape(1, 1);
		sample.convertTo(p, CV_32FC1);
		float score = svm->predict(sample, noArray(), StatModel::Flags::RAW_OUTPUT);
		if (score < minScore) {
			minScore = score;
			index = i;
		}
#ifdef TAG
		sprintf_s(string, 32, " i %d �� score�� %f", i, score);
		imshow(string, src);
		cout << string;
#endif
		src.release();
		p.release();
		sample.release();
	}
	if (index >= 0) {
		dst = plates[index].clone();
	}
	for (auto p : plates)
	{
		p.release();
	}
}

void CarPlateRecongize::plateLocate(Mat src, vector<Mat>& plates)
{
	Mat src_threshold;
	// ��ͼ����л�������ȥ����������ǿͼƬ�г���λ�õ�Ч��
	processMat(src, src_threshold, 5, 15, 3);

	vector<vector<Point>> contours;
	findContours(src_threshold, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
	src_threshold.release();
	vector<RotatedRect> vec_sobel_roi;

	for (auto cnt : contours) {
		//�õ�����cnt�����������С�ı�Ե���ɵ�rotateRect
		RotatedRect rotate_rect = minAreaRect(cnt);
		if (BaseLocation::verifySizes(rotate_rect)) {
			vec_sobel_roi.push_back(rotate_rect);
			CvPoint2D32f point[4];
			Point pt[4];
			cvBoxPoints(rotate_rect, point);
			for (int i = 0; i<4; i++)
			{
				pt[i].x = (int)point[i].x;
				pt[i].y = (int)point[i].y;
			}
			Scalar color = Scalar(255, 0, 255);
			line(src, pt[0], pt[1], color, 1);
			line(src, pt[1], pt[2], color, 1);
			line(src, pt[2], pt[3], color, 1);
			line(src, pt[3], pt[0], color, 1);
		}

	}
#ifdef TAG
	imshow("contours", src);
	//waitKey(0);
#endif
	//У׼ͼƬ��ͼƬ�п�����б��
	tortuosity(src, vec_sobel_roi, plates);
}

void CarPlateRecongize::processMat(Mat src, Mat & dst, int blur_size, int close_w, int close_h)
{
	//ͼ��Ԥ���� �������� ����================================================================
	//��˹�˲� Ҳ���Ǹ�˹ģ�� ����
	// imwrite("/sdcard/car/src.jpg",src);
	Mat blur;
	GaussianBlur(src, blur, Size(blur_size, blur_size), 0);
#ifdef TAG
	// imwrite("/sdcard/car/blur.jpg",blur);
	imshow("gaussian", blur);
#endif
	//�Ҷ�
	Mat gray;
	cvtColor(blur, gray, CV_BGR2GRAY);
	// imwrite("/sdcard/car/gray.jpg",gray);
#ifdef TAG
	imshow("g_gray", gray);
#endif
	//��Ե����˲��� ��Ե��� �������ֳ���
	Mat sobel, abs_sobel;
	Sobel(gray, sobel, CV_16S, 1, 0);
	// imwrite("/sdcard/car/sobel.jpg",sobel);
#ifdef TAG
	imshow("Sobel", sobel);
#endif
	//������Ԫ����ȡ����ֵ
	convertScaleAbs(sobel, abs_sobel);
	// imwrite("/sdcard/car/abs_sobel.jpg",abs_sobel);
#ifdef TAG
	imshow("convertScaleAbs", abs_sobel);
#endif
	//��Ȩ
	Mat weighted;
	addWeighted(abs_sobel, 1, 0, 0, 0, weighted);
	// imwrite("/sdcard/car/weighted.jpg",weighted);
#ifdef TAG
	imshow("weighted", weighted);
#endif
	//��ֵ
	Mat thresholds;
	threshold(weighted, thresholds, 0, 255, THRESH_OTSU + THRESH_BINARY);
	// imwrite("/sdcard/car/thresholds.jpg",thresholds);
#ifdef TAG
	imshow("thresholds", thresholds);
#endif
	//�ղ��� �����͡���ʴ
	//�Ѱ�ɫ�����������������������κκ�ɫ�������С�ڽṹԪ�صĴ�С���ᱻ����
	//���ڽṹ��С �����й����Ʊ��� ��A 12345 �жϲ� ���� width��С����,����������Ӳ���Ҫ������
	//sobel�ķ�ʽ��λ����100%ƥ��
	Mat element = getStructuringElement(MORPH_RECT, Size(close_w, close_h));
	morphologyEx(thresholds, dst, MORPH_CLOSE, element);
#ifdef TAG
	imshow("close", dst);
#endif
	//waitKey(0);
	// imwrite("/sdcard/car/dst.jpg",dst);
	blur.release();
	gray.release();
	sobel.release();
	abs_sobel.release();
	weighted.release();
	thresholds.release();
	element.release();

	//    imshow("g", dst);
}

void CarPlateRecongize::getHOGFeatures(HOGDescriptor * hog, Mat image, Mat & features)
{
	vector<float> descriptor;

	Mat trainImg = Mat(hog->winSize, CV_32S);
	resize(image, trainImg, hog->winSize);

	//���㣬��ȡHOG ����
	hog->compute(trainImg, descriptor, Size(8, 8));

	Mat mat_featrue(descriptor);
	mat_featrue.copyTo(features);
	mat_featrue.release();
	trainImg.release();
}

void CarPlateRecongize::clearFixPoint(Mat & img)
{
	const int maxChange = 6;
	vector<int > vec;
	for (int i = 0; i < img.rows; i++) {
		int change = 0;
		for (int j = 0; j < img.cols - 1; j++) {
			//�����һ����+1
			if (img.at<char>(i, j) != img.at<char>(i, j + 1)) change++;
		}
		vec.push_back(change);
	}
	for (int i = 0; i < img.rows; i++) {
		int change = vec[i];
		//��Ծ������maxChange ������Ĩ��
		if (change <= maxChange) {
			for (int j = 0; j < img.cols; j++) {
				img.at<char>(i, j) = 0;
			}
		}
	}
}

int CarPlateRecongize::verifyCharSizes(Mat &src)
{
	float aspect = .9f;
	float charAspect = (float)src.cols / (float)src.rows;
	float error = 0.7f;
	float minHeight = 10.f;
	float maxHeight = 33.f;
	float minAspect = 0.05f;
	float maxAspect = aspect + aspect * error;
	if (charAspect > minAspect && charAspect < maxAspect &&
		src.rows >= minHeight && src.rows < maxHeight)
		return 1;
	return 0;
}

int CarPlateRecongize::getCityIndex(vector<Rect>& rects)
{
	int specIndex = 0;
	for (int i = 0; i < rects.size(); i++) {
		Rect mr = rects[i];
		int midx = mr.x + mr.width / 2;
		//����ƽ����Ϊ7�� �����ǰ�±��Ӧ�ľ����ұߴ���1/7 ��С��2/7
		if (midx < WIDTH / 7 * 2 && midx > WIDTH / 7) {
			specIndex = i;
		}
	}
	return specIndex;
	return 0;
}

void CarPlateRecongize::getChineseRect(Rect & src, Rect & dst)
{
	//�������һ�� ��ֵ�Ǹ���ž��� ���Բ鿴�س����������ַ�����
	float width = src.width  * 1.15f;
	int x = src.x;

	//x���ȥһ���(���ĺͺ�������΢��һ���)
	int newx = x - int(width * 1.15f);
	dst.x = newx > 0 ? newx : 0;
	dst.y = src.y;
	dst.width = int(width);
	dst.height = src.height;
}

string CarPlateRecongize::predict(vector<Mat> vec_chars, string & plate_result)
{

	for (int i = 0; i < vec_chars.size(); ++i) {
		Mat mat = vec_chars[i];
		Point maxLoc;
		Mat features;
		getHOGFeatures(ann_hog, mat, features);
		Mat sample = features.reshape(1, 1);
		sample.convertTo(sample, CV_32FC1);
		Mat response;
		if (i) {
			//Ԥ��
			ann->predict(sample, response);
			//������������ƥ���
			minMaxLoc(response, 0, 0, 0, &maxLoc);
			plate_result += CHARS[maxLoc.x];
		}
		else {
			ann_zh->predict(sample, response);
			minMaxLoc(response, 0, 0, 0, &maxLoc);
			plate_result += ZHCHARS[maxLoc.x];
		}
		response.release();
		sample.release();
		features.release();
		mat.release();
	}
	//ann_zh->predict(sample, response);

	return plate_result;
}

