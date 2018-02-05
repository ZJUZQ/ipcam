#include "opencv2/opencv.hpp"  
      
#include <fstream>  
#include <unistd.h>  
#include <sstream>
#include "camera_hc.hpp"      

#include "log.h"
#include "comdef.h"
int usage()
{
    printf("Usage::");
    printf("\t./demo_camera_rtsp [Paras] camera_IP\n");
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
    
    cv::VideoCapture vidCap;
    std::string cameraIP = argv[0];
    // Create Camera handle
    vidCap.open("rtsp://admin:admin123@192.168.200.102");
    if (!vidCap.isOpened())
    {
        printf("Create IPC failed: %s\n",cameraIP.c_str());
        return -1;
    }
    
    // Wait 1s to init camera configuration
    usleep(1e6);
    
    cv::VideoWriter outputV;
    int videoH=1080, videoW=1920;
    if (isSave)
    {
        int w = isUp?videoW:videoW/2;
        int h = isUp?videoH:videoH/2;
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
    
    cv::Mat oriF, saveF, showF;
    double beg, time_cap=0, time_enc=0;
    int64_t frame_num = 0;
    cv::namedWindow("camera_rtsp", 0);
    while (true)
    {
        frame_num += 1;
        beg = timeStamp();
        //get camera 
        vidCap>>oriF;
        if (isUp)
            saveF = oriF;
        else
            cv::resize(oriF, saveF, cv::Size(0,0), 0.5, 0.5);
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
            cv::imwrite(outfile.str(), oriF);

        if(27==c || 'q'==c)
            break;
    }
    
    return 0;
}  
