#include <opencv2/opencv.hpp>   
#include <Windows.h>      
#include <iostream>
#include <stdio.h>

#pragma comment(lib,"winmm.lib")  //PlaySound�� ����ϱ� ���� winmm ���̺귯�� ���
#define CRT_SECURE_NO_WARNINGS

using namespace std;
using namespace cv;

#define CAM_WIDTH 480
#define CAM_HEIGHT 240
CascadeClassifier face_cascade;

int cnt = 0;   //���� Ž�� Ƚ��
vector<Rect> detectFace(Mat fram0);  // �� Ž�� �Լ� 
vector<vector<Rect>> setRoi(vector<Rect> faces);   // �� �ֺ��� Roi ����
void drawFace(vector<Rect> faces, Mat frame0);  //Ž���� �� �ݺ� Ž�� �Լ�
void detectSmoke(vector<vector<Rect>> roiArray, Mat frame2, Mat frame0);  // ���� Ž�� �Լ�
void saveSmoke(vector<Rect> faces, Mat frame0); //���� ������ �����Լ�
void saveFace(vector<Rect> faces, Mat frame0); //���� ���� �� �����Լ�

int main() {
    Mat frame0, frame1, frame2, result; // 0: raw, 1: binary(MOG2), 2:binary(MOG2)���� �߰� ����������
    Ptr<BackgroundSubtractor> pMOG2; //MOG2 ��� ���� ��ü
    vector<Rect> faces; // �� ��ǥ.
    vector<vector<Rect>> roiArray; //roi

    int start = 0;
    int tr = 0; 

    clock_t timer_s, timer_e;
    double duration;
    timer_s = clock();  //���� ���μ����� ����ǰ� ���ݱ��� ����Ǿ� �� ms


    pMOG2 = createBackgroundSubtractorMOG2();
    VideoCapture capture("d:/6-2.mp4");  //���� ���
    if (!capture.isOpened()) {
        exit(EXIT_FAILURE);
    }

    while (capture.read(frame0)) {

        faces = detectFace(frame0); // ��s ��ǥ ã��.
        pMOG2->apply(frame0, frame1); // �� ����. binary(frame1).
        medianBlur(frame1, frame2, 3); // ������������ binary(frame2)
        result = frame0.clone();    //���պ� ȭ��

        drawFace(faces, result); // ���â�� ��s ���� �׸���.
        if (faces.size() > 0) {
            start = 1;  //���� �νĵǴ� ����� ����
            roiArray = setRoi(faces);
            if (cnt >= 8) {
                saveFace(faces, frame0);
                saveSmoke(faces, frame0);
            }
        }

        timer_e = clock();
        duration = (double)(timer_e - timer_s) / CLOCKS_PER_SEC;     //�ð� ���� ���� ��Ȯ���� ����.

        if ((start == 1) && (duration > 0.2)) {     //���� Ž�� �ǰ� ���⸦ �մ� �ӵ��� ���� Ž��
            detectSmoke(roiArray, frame2, result);
            timer_s = clock();

        }
        if (cnt >= 8) {
            putText(result, "Violators are subject to a fine of 100,000won.", Point(50, 400), FONT_HERSHEY_PLAIN, 2.0, Scalar(0, 0, 255), 2);
        }
        if (cnt == 8 && tr == 0) {
            PlaySound(TEXT("D:\\Alarm.wav"), NULL, SND_SYNC);
            tr++;   //PlaySound�� ���õ� �˸� ���� ���� 
        }

        imshow("frame2", frame2);
        imshow("result", result);

        if (waitKey(1) == 27)break; // esc �Է½� ����.
    }
}

vector<Rect> detectFace(Mat frame0) {
    CascadeClassifier face_classifier;
    vector <Rect> faces;
    Mat grayframe;

    face_classifier.load("C:/opencv/sources/data/haarcascades/haarcascade_frontalface_alt.xml");
    cvtColor(frame0, grayframe, COLOR_BGR2GRAY);
    face_classifier.detectMultiScale(grayframe, faces, 1.1, 3, 0, Size(30, 30));
    return faces;
}

vector<vector<Rect>> setRoi(vector<Rect> faces) {
    vector<vector<Rect>> roiArray;
    for (int i = 0; i < faces.size(); i++) { //Ž���� �󱼸�ŭ �ݺ�. �� �ϳ��� �ѹ��� �����.
        vector<Rect> roi;

        roi.push_back(Rect(faces[i].x - faces[i].width, faces[i].y, faces[i].width, faces[i].height));
        roi.push_back(Rect(faces[i].x - faces[i].width, faces[i].y - faces[i].height, faces[i].width, faces[i].height));
        roi.push_back(Rect(faces[i].x, faces[i].y - faces[i].height, faces[i].width, faces[i].height));
        roi.push_back(Rect(faces[i].x + faces[i].width, faces[i].y - faces[i].height, faces[i].width, faces[i].height));
        roi.push_back(Rect(faces[i].x + faces[i].width, faces[i].y, faces[i].width, faces[i].height));

        roiArray.push_back(roi);
    }
    return roiArray;
}

void detectSmoke(vector<vector<Rect>> roiArray, Mat frame2, Mat frame0) {
    for (int i = 0; i < roiArray.size(); i++) {
        for (int j = 0; j < roiArray[i].size(); j++) {
            Scalar s = sum(frame2(roiArray[i][j]) / 255);
            cout << ((s[0] / roiArray[i][j].width) / roiArray[i][j].height) * 100 << "%" << endl;
            cout << "cnt : " << cnt << endl;

            if ((((s[0] / roiArray[i][j].width) / roiArray[i][j].height) * 100) > 20) {
                rectangle(frame0, roiArray[i][0], Scalar(255, 0, 0), 3, 4, 0);
                rectangle(frame0, roiArray[i][1], Scalar(255, 0, 0), 3, 4, 0);
                rectangle(frame0, roiArray[i][2], Scalar(255, 0, 0), 3, 4, 0);
                rectangle(frame0, roiArray[i][3], Scalar(255, 0, 0), 3, 4, 0);
                rectangle(frame0, roiArray[i][4], Scalar(255, 0, 0), 3, 4, 0);
                rectangle(frame0, roiArray[i][j], Scalar(0, 0, 255), 3, 4, 0);
                putText(frame0, "Smoke detection", Point(300, 300), FONT_HERSHEY_PLAIN, 2.0, 255, 2);
                cnt++;
            }
        }
    }
}


void drawFace(vector<Rect> faces, Mat result) {
    int i;
    for (i = 0; i < faces.size(); i++) {                    //Ž���� �󱼸�ŭ �ݺ�. �� �ϳ��� �ѹ��� �����
        Point lb(faces[i].x + faces[i].width, faces[i].y + faces[i].height);
        Point tr(faces[i].x, faces[i].y);
        rectangle(result, lb, tr, Scalar(0, 255, 0), 3, 4, 0);
        putText(result, "Face detection", Point(200, 200), FONT_HERSHEY_PLAIN, 2.0, 255, 2);
    }
}

void saveSmoke(vector<Rect> faces, Mat result) {
    int i;
    for (i = 0; i < faces.size(); i++) {
        Rect select(faces[i].x, faces[i].y, faces[i].width, faces[i].height);
        Mat faceROI = result(select);
        imwrite("C:/Users/82107/source/repos/Project_JSH/result.jpg", result); //������ ����Ǵ� ���
    }
}

void saveFace(vector<Rect> faces, Mat frame0) {
    int i;
    for (i = 0; i < faces.size(); i++) {
        Rect select(faces[i].x, faces[i].y, faces[i].width, faces[i].height);
        Mat faceROI = frame0(select);
        imwrite("C:/Users/82107/source/repos/Project_JSH/WhoAreYou.jpg", frame0); //������ ����Ǵ� ���
    }
}