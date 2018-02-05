#include "opencv2/opencv.hpp"  
#include <sstream>      
#include <fstream>  
#include <unistd.h>  
#include "camera_hc.hpp"      

#include "log.h"
#include "comdef.h"
int usage()
{
    printf("Usage::");
    printf("\t./demo_camera_hc [Paras] camera_IP intrinsic.yaml\n");
    printf("Paras::\n");
    printf("\ts: save the video\n");
    printf("\tu: Up dimention to 1920x1080, default 960x540\n");
    printf("\tn: save video name, default res_video.avi\n");
    printf("\th: Print the help information\n");
    return 0;
}

int main(int argc, char *argv[])  
{  
       // Parse the options
    char opts[] = "hsun:";
    char oc;
    bool isSave = false;
    bool isUp = false;
    std::string saveN = "res_video.avi";
    while((oc = getopt_t(argc, argv, opts)) != -1)
    {
        switch(oc)
        {
        case 'h':
            return usage();
        case 's':
            isSave = true;
            break;
        case 'u':
            isUp = true;
            break;
        case 'n':
            saveN = getarg_t();
            break;
        }
    }
    argv += getpos_t();
    argc -= getpos_t();
    if (argc<1)
        return usage();
    
    //Read the undistort params
    cv::FileStorage intrinsicF(argv[1], 
                               cv::FileStorage::READ);
    if (!intrinsicF.isOpened())
        return -1;
    
    cv::Mat K, D;
    intrinsicF["Camera_Matrix"]>>K;
    intrinsicF["Distortion_Coefficients"]>>D;
    
    //
    std::string cameraIP = argv[0];
    int cameraPORT = 8000;
    // Create Camera handle
    CAMERA_HC *camera = new CAMERA_HC();
    if (0 !=camera->createIPC(cameraIP.c_str(), cameraPORT, 0))
    {
        printf("Create IPC failed: %s:%d\n",cameraIP.c_str(), cameraPORT);
        return -1;
    }
    
    // Wait 1s to init camera configuration
    usleep(1e6);

    cv::VideoWriter outputV;
    if (isSave)
    {
        cv::Mat tmpF;
        int count=0;
        if (0 != camera->getImage(tmpF))
        {
            printf("Get data failed for long time\n");
            return -1;
        }
        int w = isUp?tmpF.cols:tmpF.cols/2;
        int h = isUp?tmpF.rows:tmpF.rows/2;
        outputV.open(saveN.c_str(),
                     CV_FOURCC('M','J','P','G'),
                     25,
                     cv::Size(w, h), true);
        if (!outputV.isOpened())
        {
            fprintf(stderr, "Write video open failed\n");
            return -1;
        }
    }

    cv::Mat map1, map2;
    cv::Mat tmpF;
    if (0 != camera->getImage(tmpF))
    {
        printf("Get data failed for long time\n");
        return -1;
    }
    cv::initUndistortRectifyMap(K, D,
                                cv::noArray(),
                                cv::noArray(),
                                cv::Size(tmpF.cols, tmpF.rows),
                                CV_32FC1,
                                map1, map2);
    
    cv::Mat oriF, undistortF, saveF, showF;
    double beg, time_cap=0, time_enc=0;
    int64_t frame_num = 0;
    cv::namedWindow("camera_hc", 0);
    while (true)
    {
        frame_num += 1;
        beg = timeStamp();
        //get camera 
        if (0 != camera->getImage(oriF))
            continue;

        // undistort
        cv::remap(oriF, undistortF,
                  map1, map2, cv::INTER_CUBIC);
        if (isUp)
            saveF = undistortF;
        else
            cv::resize(undistortF, saveF, cv::Size(0,0), 0.5, 0.5);
        time_cap += (timeStamp() - beg);
        
        //encode the video
        beg = timeStamp();
        if(isSave)
            outputV << saveF;
        time_enc += (timeStamp() - beg);
        char str[1024]={0};
        
        if (saveF.cols>=1920)
            cv::resize(saveF, showF, cv::Size(0,0), 0.5, 0.5);
        else
            showF = saveF;
        
        sprintf(str,"cap: %0.1ffps, enc: %0.1ffps", 
                (1000.0f*1000*frame_num)/time_cap,
                (1000.0f*1000*frame_num)/time_enc);
        
        cv::putText(showF, str,
                    cv::Point(10, 40), 
                    cv::FONT_HERSHEY_SIMPLEX, 1,
                    cv::Scalar(0,0,255), 1, 4);
        cv::imshow( "camera_hc", showF);
        char c = (char)cv::waitKey(2);
        std::stringstream outfile;
        outfile << "image_" << frame_num << ".png";
        if(c == 's')
            cv::imwrite(outfile.str(), undistortF);
        else if(27==c || 'q'==c)
            break;
    }
    
    delete camera;
    return 0;
}  
