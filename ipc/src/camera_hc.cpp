#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "camera_hc.hpp"

void CALLBACK DecCBFun(LONG nPort, char *pBuf, LONG nSize, FRAME_INFO *pFrameInfo, void* nUser, LONG nReserved2)  
{  
    USERDATA *userData = (USERDATA *)(nUser);
    long lFrameType = pFrameInfo->nType;  
    if (lFrameType == T_YV12)  
    {  
        userData->matRGB.create(pFrameInfo->nHeight, pFrameInfo->nWidth,  
                                CV_8UC3);  
      
        cv::Mat src(pFrameInfo->nHeight + pFrameInfo->nHeight / 2, 
                    pFrameInfo->nWidth, CV_8UC1, (uchar *)pBuf);  
        cv::cvtColor(src, userData->matRGB, CV_YUV2BGR_YV12);  
        pthread_mutex_lock(&userData->mutex);  
        userData->frameList.push_back(userData->matRGB);  
        pthread_mutex_unlock(&userData->mutex);  
    }  
}  
      
void CALLBACK g_RealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize,void* dwUser)  
{  
    USERDATA *userData = (USERDATA *)(dwUser);
    DWORD dRet;  
    switch (dwDataType)  
    {  
    case NET_DVR_SYSHEAD:           //set system head  
        if (!PlayM4_GetPort(&(userData->nPort)))// get the nPort
        {  
            break;  
        }  
        if (dwBufSize > 0) {  
            if (!PlayM4_SetStreamOpenMode(userData->nPort, STREAME_REALTIME)) {  
                dRet = PlayM4_GetLastError(userData->nPort);  
                break;  
            }  
            if (!PlayM4_OpenStream(userData->nPort, pBuffer, dwBufSize, 1024 * 1024)) {  
                dRet = PlayM4_GetLastError(userData->nPort);  
                break;  
            }  
            //set decode callback without display
            if (!PlayM4_SetDecCallBackMend(userData->nPort, 
                                           DecCBFun, dwUser))
            {  
                dRet = PlayM4_GetLastError(userData->nPort);  
                break;  
            }  
            ////set decode callback without display
            //if (!PlayM4_SetDecCallBackEx(nPort, DecCBFun, NULL, NULL))  
            //{  
            //    dRet = PlayM4_GetLastError(nPort);  
            //    break;  
            //}  
            // Enable video decode
            if (!PlayM4_Play(userData->nPort, NULL))  
            {  
                dRet = PlayM4_GetLastError(userData->nPort);  
                break;  
            }  
            // Enable audio
            //if (!PlayM4_PlaySound(userData->nPort)) {  
            //    dRet = PlayM4_GetLastError(userData->nPort);  
            //    break;  
            //}  
        }  
        break;  
    case NET_DVR_STREAMDATA:
        if (dwBufSize > 0 && userData->nPort != -1) {  
            BOOL inData = PlayM4_InputData(userData->nPort, pBuffer, 
                                           dwBufSize);  
            while (!inData) {  
                inData = PlayM4_InputData(userData->nPort, pBuffer, 
                                          dwBufSize);  
                std::cerr << "PlayM4_InputData failed \n" << std::endl;  
            }  
        }  
        break;  
    }  
}  
      
void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, 
                                  LONG lHandle, void *pUser)  
{  
    char tempbuf[256] = {0};  
    std::cout << "EXCEPTION_RECONNECT = " << EXCEPTION_RECONNECT << std::endl;  
    switch(dwType)  
    {  
    case EXCEPTION_RECONNECT:  
        printf("pyd----------reconnect--------%d\n", time(NULL));  
        break;  
    default:  
        break;  
    }  
}  


CAMERA_HC::CAMERA_HC()
{
    init();
}
         
CAMERA_HC::~CAMERA_HC()
{
    uninit();
}
    
int CAMERA_HC::init()
{
    // Init Client
    NET_DVR_Init();  
    NET_DVR_SetConnectTime(2000, 1);  
    NET_DVR_SetReconnect(1000, true);  
    NET_DVR_SetLogToFile(3, "./hc_camera_log");  
    
    // Init User data
    m_userData.nPort = -1;
    pthread_mutex_init(&m_userData.mutex, NULL);  
    return 0;
}

int CAMERA_HC::uninit()
{
    NET_DVR_Cleanup();  
    return 0;
}


int CAMERA_HC::createIPC(const char *IP, int ip_port, int ch_port)
{
    NET_DVR_DEVICEINFO_V30 struDeviceInfo = {0};  
    NET_DVR_SetRecvTimeOut(5000);  
    
    m_userData.lUserID = NET_DVR_Login_V30((char*)(IP), ip_port, 
                                  (char*)("admin"), 
                                  (char*)("vortex2014"), 
                                  &struDeviceInfo);  
    NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL); 
    
    printf("debug2\n");
    NET_DVR_CLIENTINFO ClientInfo = {0};  
    ClientInfo.lChannel       = 1;  
    ClientInfo.lLinkMode     = 0;  
    ClientInfo.hPlayWnd     = 0;  
    ClientInfo.sMultiCastIP = NULL;  

    m_userData.lRealPlayHandle=NET_DVR_RealPlay_V30(m_userData.lUserID, 
                                                    &ClientInfo, 
                                                    g_RealDataCallBack,
                                                    (void*)(&m_userData),
                                                    0);
    if (m_userData.lRealPlayHandle < 0)  
        return -1;
    return 0;
}

int CAMERA_HC::getImage(cv::Mat &img)
{
    int count=0;
    while (1)
    {
        count ++;
        pthread_mutex_lock(&m_userData.mutex);  
        if(m_userData.frameList.size()>0)  
        {  
            std::list<cv::Mat>::iterator it=m_userData.frameList.end();  
            img = (*(--it));  
            m_userData.frameList.pop_front();  
        }  
        m_userData.frameList.clear(); // drop old frame
        pthread_mutex_unlock(&m_userData.mutex);
        if (!img.empty())
            break;
        if (count>100)
            return -1;
    }
    return 0;
}

