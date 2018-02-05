#ifndef CAMERA_HC_HPP
#define CAMERA_HC_HPP

#include "opencv2/opencv.hpp"
#include "HCNetSDK.h"  
#include "PlayM4.h"  
#include "LinuxPlayM4.h"  
#include "pthread.h"
#include <list>

typedef struct __TAG_UserData
{
    cv::Mat matRGB;
    pthread_mutex_t mutex;  
    std::list<cv::Mat> frameList;  
    long lRealPlayHandle;  
    long lUserID;
    int  nPort;
}USERDATA;

class CAMERA_HC
{
public:
    CAMERA_HC();
    ~CAMERA_HC();

public:
    int init();
    int uninit();
    
public:
    int createIPC(const char *IP, int ip_port, int ch_port);
    int getImage(cv::Mat &img);
    
private:
    USERDATA m_userData;
};

#endif

