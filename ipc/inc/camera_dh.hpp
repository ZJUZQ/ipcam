#ifndef CAMERA_DH_HPP
#define CAMERA_DH_HPP

#include "opencv2/opencv.hpp"
class CAMERA_DH
{
public:
    CAMERA_DH();
    ~CAMERA_DH();

public:
    int init();
    int uninit();
    
public:
    void* createIPC(const char *IP, int ip_port, int ch_port);
    cv::Mat getImage(void *ipc);
    void releaseIPC(void *ipc);
};

#endif

