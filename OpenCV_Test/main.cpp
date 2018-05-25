#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/stitching.hpp>
#include <iostream>
#include <fstream>

using namespace cv;
using namespace std;

//����ƴ�ӵ�ԭʼͼ������
vector<Mat> imgs;

//��������ԭʼƴ��ͼ����
void parseCmdArgs(int argc, char** argv);

//  stitcherȫ��ͼƴ��
int main(int argc, char** argv) {
	//����ƴ��ͼ��
	parseCmdArgs(argc, argv);
	//���ƴ��ͼƬ
	Mat pano;
	Stitcher stitcher = Stitcher::createDefault(false);
	Stitcher::Status status = stitcher.stitch(imgs, pano);//ƴ��
	if (status != Stitcher::OK) //�ж�ƴ���Ƿ�ɹ�
	{
		cout << "Can't stitch images, error code = " << int(status) << endl;
		return -1;
	}
	namedWindow("ȫ��ƴ��", 0);
	imshow("ȫ��ƴ��", pano);
	imwrite("D:\\ȫ��ƴ��.jpg", pano);
	waitKey(0);
	return 0;
}

//��������ԭʼƴ��ͼ����
void parseCmdArgs(int argc, char** argv) 
{
	for (int i = 1; i < argc; i++)
	{
		Mat img = imread(argv[i]);
		if (img.empty())
		{
			cout << "Can't read image '" << argv[i] << "'\n";
		}
		imgs.push_back(img);
	}

}