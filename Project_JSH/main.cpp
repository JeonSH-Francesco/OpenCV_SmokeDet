#include <opencv2/opencv.hpp>   
#include <Windows.h>      
#include <iostream>
#include <stdio.h>
#include <thread>  // C++ thread library

#pragma comment(lib,"winmm.lib")  //PlaySound를 사용하기 위해 winmm 라이브러리 사용
#define CRT_SECURE_NO_WARNINGS

using namespace std;
using namespace cv;

#define CAM_WIDTH 480
#define CAM_HEIGHT 240
CascadeClassifier face_cascade;

int cnt = 0;   //연기 탐지 횟수
vector<Rect> detectFace(Mat fram0);  // 얼굴 탐지 함수 
vector<vector<Rect>> setRoi(vector<Rect> faces);   // 얼굴 주변의 Roi 설정
void drawFace(vector<Rect> faces, Mat frame0);  //탐지된 얼굴 반복 탐지 함수
void detectSmoke(vector<vector<Rect>> roiArray, Mat frame2, Mat frame0);  // 연기 탐지 함수
void saveSmoke(vector<Rect> faces, Mat frame0); //흡연시 흡연행위 저장함수
void saveFace(vector<Rect> faces, Mat frame0); //흡연시 흡연자 얼굴 저장함수
void playSoundAsync(); // 비동기적으로 사운드를 재생하는 함수

int main() {
    Mat frame0, frame1, frame2, result; // 0: raw, 1: binary(MOG2), 2:binary(MOG2)에서 추가 노이즈제거
    Ptr<BackgroundSubtractor> pMOG2; //MOG2 배경 삭제 객체
    vector<Rect> faces; // 얼굴 좌표.
    vector<vector<Rect>> roiArray; //roi

    int start = 0;
    int tr = 0;

    clock_t timer_s, timer_e;
    double duration;
    timer_s = clock();  //현재 프로세스가 실행되고 지금까지 진행되어 온 ms

    pMOG2 = createBackgroundSubtractorMOG2();
    VideoCapture capture("d:/6-2.mp4");  //영상 경로
    if (!capture.isOpened()) {
        exit(EXIT_FAILURE);
    }

    while (capture.read(frame0)) {

        faces = detectFace(frame0); // 얼굴s 좌표 찾기.
        pMOG2->apply(frame0, frame1); // 차 영상. binary(frame1).
        medianBlur(frame1, frame2, 3); // 노이즈제거한 binary(frame2)
        result = frame0.clone();    //통합본 화면

        drawFace(faces, result); // 결과창에 얼굴s 영역 그리기.
        if (faces.size() > 0) {
            start = 1;  //얼굴이 인식되는 경우의 변수
            roiArray = setRoi(faces);
            if (cnt >= 8) {
                saveFace(faces, frame0);
                saveSmoke(faces, frame0);
            }
        }

        timer_e = clock();
        duration = (double)(timer_e - timer_s) / CLOCKS_PER_SEC;     //시간 차를 통해 정확도를 높임.

        if ((start == 1) && (duration > 0.2)) {     //얼굴이 탐지 되고 연기를 뿜는 속도를 통해 탐지
            detectSmoke(roiArray, frame2, result);
            timer_s = clock();
        }

        if (cnt >= 8) {
            putText(result, "Violators are subject to a fine of 100,000won.", Point(50, 400), FONT_HERSHEY_PLAIN, 2.0, Scalar(0, 0, 255), 2);
        }

        // PlaySound가 실행 중일 때에도 영상이 계속 진행되도록 별도의 스레드에서 실행
        if (cnt == 8 && tr == 0) {
            tr++;   //PlaySound와 관련된 알림 제어 변수
            thread soundThread(playSoundAsync);  // 별도의 스레드에서 안내 방송 실행
            soundThread.detach();  // 스레드를 분리하여 비동기적으로 실행되게 함
        }

        imshow("frame2", frame2);
        imshow("result", result);

        if (waitKey(1) == 27) break; // esc 입력시 종료.
    }
}

void playSoundAsync() {
    PlaySound(TEXT("D:\\Alarm.wav"), NULL, SND_SYNC);  // 안내 방송을 동기적으로 실행하되 메인 스레드는 영상 처리를 계속
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
    for (int i = 0; i < faces.size(); i++) { //탐지된 얼굴만큼 반복. 얼굴 하나면 한번만 수행됨.
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
    for (i = 0; i < faces.size(); i++) {                    //탐지된 얼굴만큼 반복. 얼굴 하나면 한번만 수행됨
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
        imwrite("C:/Users/82107/source/repos/Project_JSH/result.jpg", result); //사진이 저장되는 경로
    }
}
void saveFace(vector<Rect> faces, Mat frame0) {
    int i;
    for (i = 0; i < faces.size(); i++) {
        Rect select(faces[i].x, faces[i].y, faces[i].width, faces[i].height);
        Mat faceROI = frame0(select);
        imwrite("C:/Users/82107/source/repos/Project_JSH/WhoAreYou.jpg", frame0); //사진이 저장되는 경로
    }
}
/*
#include <opencv2/opencv.hpp>   
#include <Windows.h>      
#include <iostream>
#include <stdio.h>

#pragma comment(lib,"winmm.lib")  //PlaySound를 사용하기 위해 winmm 라이브러리 사용
#define CRT_SECURE_NO_WARNINGS

using namespace std;
using namespace cv;

#define CAM_WIDTH 480
#define CAM_HEIGHT 240
CascadeClassifier face_cascade;

int cnt = 0;   //연기 탐지 횟수
vector<Rect> detectFace(Mat fram0);  // 얼굴 탐지 함수 
vector<vector<Rect>> setRoi(vector<Rect> faces);   // 얼굴 주변의 Roi 설정
void drawFace(vector<Rect> faces, Mat frame0);  //탐지된 얼굴 반복 탐지 함수
void detectSmoke(vector<vector<Rect>> roiArray, Mat frame2, Mat frame0);  // 연기 탐지 함수
void saveSmoke(vector<Rect> faces, Mat frame0); //흡연시 흡연행위 저장함수
void saveFace(vector<Rect> faces, Mat frame0); //흡연시 흡연자 얼굴 저장함수

int main() {
    Mat frame0, frame1, frame2, result; // 0: raw, 1: binary(MOG2), 2:binary(MOG2)에서 추가 노이즈제거
    Ptr<BackgroundSubtractor> pMOG2; //MOG2 배경 삭제 객체
    vector<Rect> faces; // 얼굴 좌표.
    vector<vector<Rect>> roiArray; //roi

    int start = 0;
    int tr = 0; 

    clock_t timer_s, timer_e;
    double duration;
    timer_s = clock();  //현재 프로세스가 실행되고 지금까지 진행되어 온 ms


    pMOG2 = createBackgroundSubtractorMOG2();
    VideoCapture capture("d:/6-2.mp4");  //영상 경로
    if (!capture.isOpened()) {
        exit(EXIT_FAILURE);
    }

    while (capture.read(frame0)) {

        faces = detectFace(frame0); // 얼굴s 좌표 찾기.
        pMOG2->apply(frame0, frame1); // 차 영상. binary(frame1).
        medianBlur(frame1, frame2, 3); // 노이즈제거한 binary(frame2)
        result = frame0.clone();    //통합본 화면

        drawFace(faces, result); // 결과창에 얼굴s 영역 그리기.
        if (faces.size() > 0) {
            start = 1;  //얼굴이 인식되는 경우의 변수
            roiArray = setRoi(faces);
            if (cnt >= 8) {
                saveFace(faces, frame0);
                saveSmoke(faces, frame0);
            }
        }

        timer_e = clock();
        duration = (double)(timer_e - timer_s) / CLOCKS_PER_SEC;     //시간 차를 통해 정확도를 높임.

        if ((start == 1) && (duration > 0.2)) {     //얼굴이 탐지 되고 연기를 뿜는 속도를 통해 탐지
            detectSmoke(roiArray, frame2, result);
            timer_s = clock();

        }
        if (cnt >= 8) {
            putText(result, "Violators are subject to a fine of 100,000won.", Point(50, 400), FONT_HERSHEY_PLAIN, 2.0, Scalar(0, 0, 255), 2);
        }
        if (cnt == 8 && tr == 0) {
            PlaySound(TEXT("D:\\Alarm.wav"), NULL, SND_SYNC);
            tr++;   //PlaySound와 관련된 알림 제어 변수 
        }

        imshow("frame2", frame2);
        imshow("result", result);

        if (waitKey(1) == 27)break; // esc 입력시 종료.
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
    for (int i = 0; i < faces.size(); i++) { //탐지된 얼굴만큼 반복. 얼굴 하나면 한번만 수행됨.
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
    for (i = 0; i < faces.size(); i++) {                    //탐지된 얼굴만큼 반복. 얼굴 하나면 한번만 수행됨
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
        imwrite("C:/Users/82107/source/repos/Project_JSH/result.jpg", result); //사진이 저장되는 경로
    }
}

void saveFace(vector<Rect> faces, Mat frame0) {
    int i;
    for (i = 0; i < faces.size(); i++) {
        Rect select(faces[i].x, faces[i].y, faces[i].width, faces[i].height);
        Mat faceROI = frame0(select);
        imwrite("C:/Users/82107/source/repos/Project_JSH/WhoAreYou.jpg", frame0); //사진이 저장되는 경로
    }
}
*/
