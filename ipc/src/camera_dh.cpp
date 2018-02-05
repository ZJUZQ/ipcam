#include <string.h>
#include <stdio.h>

#include "camera_dh.hpp"
#include "dhnetsdk.h"
#include "dhplay.h"

typedef struct _DHIPC
{
    // Image info
    int width;
    int height;
    cv::Mat  mat;
    unsigned char  *image_data;
    
    // Login info 
    int load_flag;
    int init_flag;
    int loginHandle;
    int channel;
    int channel_num;
    int channel_real_num;
    char ip[1<<4];
    char username[1<<6];
    char password[1<<6];    
    unsigned short channel_port;
    unsigned short ip_port;

    // Other info
    DHDEV_SYSTEM_ATTR_CFG sys_info;
    DH_VIDEOENC_OPT stream_info;
    DHDEV_CHANNEL_CFG *channel_info;
    DHDEV_DSP_ENCODECAP *dsp_encap_info;
    DHDEV_CAMERA_CFG  *camera_info;
    DH_RealPlayType rtype;
    int proto_version_num;
} DHIPC;

// Conver yuv420 to rgb
#if (defined __GNUC__) || (defined __PGI) || (defined __IBMCPP__) || (defined __ARMCC_VERSION)
#define CV_ALIGN_TO_BOUNDARY(n) __attribute__((aligned(n)))
#elif (defined _MSC_VER)
#define CV_ALIGN_TO_BOUNDARY(n) __declspec(align(n))
#elif (defined __SUNPRO_CC)
// FIXME not sure about this one:
#define CV_ALIGN_TO_BOUNDARY(n) __attribute__((aligned(n)))
#else
#error Please tell me what is the equivalent of __attribute__((aligned(n))) for your compiler
#endif

#define CV_ALIGN16 CV_ALIGN_TO_BOUNDARY(16)

#if 1//CV_SSE3
#include <immintrin.h>
/* Useful macro for masks used in _mm_shuffle_epi8() */
#define CV_128BIT_MASK(_name, B0,B1,B2,B3,B4,B5,B6,B7,B8,B9,B10,B11,B12,B13,B14,B15) \
	CV_ALIGN16 const unsigned long long _name[2] =                      \
    { 0x##B7##B6##B5##B4##B3##B2##B1##B0##ull,                          \
      0x##B15##B14##B13##B12##B11##B10##B9##B8##ull }

static void icvCvtColorYUV420P2RGB(unsigned char *src, unsigned char *dst,
                                   int width, int height)
{
    int i, j;
    int numpix = width * height;
    unsigned char *pu = src + numpix;
    unsigned char *pv = pu +  (numpix>>2);
    
    CV_128BIT_MASK(mask_r, 00,80,80, 04,80,80, 08,80,80, 0C,80,80, 80,80,80, 80);
    CV_128BIT_MASK(mask_g, 80,00,80, 80,04,80, 80,08,80, 80,0C,80, 80,80,80, 80);
    CV_128BIT_MASK(mask_b, 80,80,00, 80,80,04, 80,80,08, 80,80,0C, 80,80,80, 80);
                                                                                 

    CV_128BIT_MASK(mask_y,00,80, 80,80, 01,80, 80,80, 02,80, 80,80, 03,80, 80,80); 
    CV_128BIT_MASK(mask_u,00,80, 80,80, 00,80, 80,80, 01,80, 80,80, 01,80, 80,80); 
    CV_128BIT_MASK(mask_v,00,80, 80,80, 00,80, 80,80, 01,80, 80,80, 01,80, 80,80);

    const __m128i m0 = _mm_load_si128((const __m128i*)mask_r);
    const __m128i m1 = _mm_load_si128((const __m128i*)mask_g);
    const __m128i m2 = _mm_load_si128((const __m128i*)mask_b);
    
    const __m128i m3 = _mm_load_si128((const __m128i*)mask_u);
    const __m128i m4 = _mm_load_si128((const __m128i*)mask_v);
    const __m128i m5 = _mm_load_si128((const __m128i*)mask_y);

    const __m128i mc_128 = _mm_set1_epi32(128);
    const __m128i mc_262143 = _mm_set1_epi32(262143);
    const __m128i mc_1192 = _mm_set1_epi32(1192);
    const __m128i mc_16 = _mm_set1_epi32(16);
    const __m128i mc_2066 = _mm_set1_epi32(2066);
    const __m128i mc_1634 = _mm_set1_epi32(1634);
    const __m128i mc_833 = _mm_set1_epi32(833);
    const __m128i mc_400 = _mm_set1_epi32(400);
    const __m128i mc_255 = _mm_set1_epi32(255);
    const __m128i mc_0 = _mm_setzero_si128();
    // YUV 4:2:0
    for (i = 0; i < height; i += 2)
    {
        for (j = 0; j < width; j += 4)
        {
            __m128i my0 = _mm_loadl_epi64((const __m128i*)(src + (i + 0)*width + j));
            __m128i my1 = _mm_loadl_epi64((const __m128i*)(src + (i + 1)*width + j));
            __m128i mv = _mm_loadl_epi64((const __m128i*)(pu + (i/4)*width + (j/4)*2));
            __m128i mu = _mm_loadl_epi64((const __m128i*)(pv + (i/4)*width + (j/4)*2));
            
            __m128i my0i = _mm_shuffle_epi8(my0, m5);
            __m128i my1i = _mm_shuffle_epi8(my1, m5);
            __m128i mui = _mm_shuffle_epi8(mu, m3);
            __m128i mvi = _mm_shuffle_epi8(mv, m4);
            
            __m128i my0i_c = _mm_mullo_epi32(mc_1192, _mm_max_epi32(_mm_sub_epi32(my0i, mc_16), mc_0));
            __m128i my1i_c = _mm_mullo_epi32(mc_1192, _mm_max_epi32(_mm_sub_epi32(my1i, mc_16), mc_0));
            __m128i mvi_c = _mm_sub_epi32(mvi, mc_128);
            __m128i mui_c = _mm_sub_epi32(mui, mc_128);

            __m128i mvi_1634 = _mm_mullo_epi32(mc_1634, mvi_c);
            __m128i mui_400 = _mm_mullo_epi32(mc_400, mui_c);
            __m128i mvi_833 = _mm_mullo_epi32(mc_833, mvi_c);
            __m128i mui_2066 = _mm_mullo_epi32(mc_2066, mui_c);
            
            __m128i m_r0  =_mm_add_epi32(my0i_c, mvi_1634);
            __m128i m_g0 =_mm_sub_epi32(_mm_sub_epi32(my0i_c, mvi_833), mui_400);
            __m128i m_b0  =_mm_add_epi32(my0i_c, mui_2066);

            __m128i m_r1  =_mm_add_epi32(my1i_c, mvi_1634);
            __m128i m_g1 =_mm_sub_epi32(_mm_sub_epi32(my1i_c, mvi_833), mui_400);
            __m128i m_b1  =_mm_add_epi32(my1i_c, mui_2066);
            
            
            m_r0 = _mm_min_epi32(mc_262143, _mm_max_epi32(mc_0, m_r0));
            m_g0 = _mm_min_epi32(mc_262143, _mm_max_epi32(mc_0, m_g0));
            m_b0 = _mm_min_epi32(mc_262143, _mm_max_epi32(mc_0, m_b0));
            
            m_r0 = _mm_and_si128(_mm_srli_epi32(m_r0, 10), mc_255);
            m_g0 = _mm_and_si128(_mm_srli_epi32(m_g0, 10), mc_255);
            m_b0 = _mm_and_si128(_mm_srli_epi32(m_b0, 10), mc_255);
            
            m_r0 = _mm_shuffle_epi8(m_r0, m0);
            m_g0 = _mm_shuffle_epi8(m_g0, m1);
            m_b0 = _mm_shuffle_epi8(m_b0, m2);

            m_r1 = _mm_min_epi32(mc_262143, _mm_max_epi32(mc_0, m_r1));
            m_g1 = _mm_min_epi32(mc_262143, _mm_max_epi32(mc_0, m_g1));
            m_b1 = _mm_min_epi32(mc_262143, _mm_max_epi32(mc_0, m_b1));
            
            m_r1 = _mm_and_si128(_mm_srli_epi32(m_r1, 10), mc_255);
            m_g1 = _mm_and_si128(_mm_srli_epi32(m_g1, 10), mc_255);
            m_b1 = _mm_and_si128(_mm_srli_epi32(m_b1, 10), mc_255);
            
            m_r1 = _mm_shuffle_epi8(m_r1, m0);
            m_g1 = _mm_shuffle_epi8(m_g1, m1);
            m_b1 = _mm_shuffle_epi8(m_b1, m2);
            _mm_storeu_si128((__m128i *)(dst + (i + 0)*width*3 + j*3),
                             _mm_or_si128(_mm_or_si128(m_r0, m_g0), m_b0));
            _mm_storeu_si128((__m128i *)(dst + (i + 1)*width*3 + j*3),
                             _mm_or_si128(_mm_or_si128(m_r1, m_g1), m_b1));
        }
    }
}
#else

#define LIMIT(x) ((x)>0xffffff?0xff: ((x)<=0xffff?0:((x)>>16)))

static void move_420_block(int yTL, int yTR, int yBL, int yBR, int u,
                           int v, int rowPixels, unsigned char * rgb)
{
    const int rvScale = 91881;
    const int guScale = -22553;
    const int gvScale = -46801;
    const int buScale = 116129;
    const int yScale  = 65536;
    int r, g, b;

    g = guScale * u + gvScale * v;
    //  if (force_rgb) {
    //      r = buScale * u;
    //      b = rvScale * v;
    //  } else {
    r = rvScale * v;
    b = buScale * u;
    //  }

    yTL *= yScale; yTR *= yScale;
    yBL *= yScale; yBR *= yScale;

    /* Write out top two pixels */
    rgb[0] = LIMIT(b+yTL); rgb[1] = LIMIT(g+yTL);
    rgb[2] = LIMIT(r+yTL);

    rgb[3] = LIMIT(b+yTR); rgb[4] = LIMIT(g+yTR);
    rgb[5] = LIMIT(r+yTR);

    /* Skip down to next line to write out bottom two pixels */
    rgb += 3 * rowPixels;
    rgb[0] = LIMIT(b+yBL); rgb[1] = LIMIT(g+yBL);
    rgb[2] = LIMIT(r+yBL);

    rgb[3] = LIMIT(b+yBR); rgb[4] = LIMIT(g+yBR);
    rgb[5] = LIMIT(r+yBR);
}

static void icvCvtColorYUV420P2RGB(const unsigned char *src,
                                   unsigned char *dst,
                                   int width, int height)
{
    const int numpix = width * height;
    const int bytes = 24 >> 3;
    int i, j, y00, y01, y10, y11, u, v;
    const unsigned char *pY = src;
    const unsigned char *pU = pY + numpix;
    const unsigned char *pV = pU + numpix / 4;
    unsigned char *pOut = dst;
	
    for (j = 0; j <= height - 2; j += 2) {
        for (i = 0; i <= width - 2; i += 2) {
            y00 = *pY;
            y01 = *(pY + 1);
            y10 = *(pY + width);
            y11 = *(pY + width + 1);
            u = (*pU++) - 128;
            v = (*pV++) - 128;
			
            move_420_block(y00, y01, y10, y11, u, v, width, pOut);
			
            pY += 2;
            pOut += 2 * bytes;
			
        }
        pY += width;
        pOut += width * bytes;
    }
}
#endif


static const char* _connectErrorStr(int status)
{
    static char buf[1<<8];
    switch (status)
    {
        case 1 :           return "Invalid password!";
        case 2 :           return "Invalid account!";
        case 3 :           return "Timeout!";
        case 4 :           return "The user has logged in!";
        case 5 :           return "The user has been locked!";
        case 6 :           return "The user has listed into illegal!";
        case 7 :           return "The system is busy!";
        case 8 :           return "Can not find the network server!";
        case 9 :           return "Can not open config file!";
            
    };
    sprintf(buf, "Unknown %s code %d", status >= 0 ? "status":"error", status);
    return buf;
}

static void CALLBACK reconnFunc(LLONG lLoginID, char *pchDVRIP, LONG nDVRPort, LDWORD dwUser)
{
    fprintf(stderr, "%d, %s: try to connect!\n", lLoginID, pchDVRIP);
    fflush(stderr);
	return;
}

static void CALLBACK
realDataCallbackEx(LLONG handle, DWORD type,
                   BYTE *buf, DWORD buf_size,
                   LONG param, LDWORD  user)
{
    DHIPC *cap = (DHIPC *)user;
    PLAY_InputData(cap->channel_port, (unsigned char *)buf, buf_size);
}

static void CALLBACK
decodeCallback(LONG port, char * buf, LONG size,
               FRAME_INFO * frame_info, void *dev1, int dev2)
{
 	DHIPC *cap = (DHIPC*)dev1;
    size_t mem_size;
    int rows, cols;
    
	if (!cap || (frame_info->nType != T_IYUV))
	{
        return;
	}

    
    if (cap->init_flag == 0)
    {
        rows = frame_info->nHeight;
        cols =  frame_info->nWidth;
        mem_size = sizeof(unsigned char)*rows*cols*3;
        cap->mat.create(rows, cols, CV_8UC3);
        cap->image_data = new unsigned char[mem_size];
        cap->init_flag = 1;
    }
    else
    {
        cap->width = frame_info->nWidth;
        cap->height = frame_info->nHeight;                
        icvCvtColorYUV420P2RGB((unsigned char*)buf,
                               (unsigned char*)cap->image_data,
                               cap->width, cap->height);
        cap->load_flag = 1;
    }
}

// Camera class API
CAMERA_DH::CAMERA_DH()
{
    init();
}

CAMERA_DH::~CAMERA_DH()
{
    uninit();
}
    
int CAMERA_DH::init()
{
    // Init Client
    CLIENT_Init(NULL,NULL);
    NET_PARAM net_param = {0};
    net_param.nConnectTryNum = 2;
	CLIENT_SetNetworkParam(&net_param);
    CLIENT_SetAutoReconnect(reconnFunc, (DWORD)0);
    return 0;
}

int CAMERA_DH::uninit()
{
    CLIENT_Cleanup();
    return 0;
}


void* CAMERA_DH::createIPC(const char *IP, int ip_port, int ch_port)
{
    NET_DEVICEINFO dev_info;
    memset(&dev_info, 0, sizeof(NET_DEVICEINFO));

    //Init IPC
    DHIPC * ipc = new DHIPC();
    ipc->channel_real_num = 0;
    ipc->proto_version_num = 4;
    ipc->init_flag = 0;
    ipc->load_flag = 0;
    ipc->loginHandle = 0;
    strcpy(ipc->ip, IP);
    strcpy(ipc->username, "admin");
    strcpy(ipc->password, "admin");
    ipc->channel_port=ch_port;
    ipc->ip_port = ip_port;

    // Set the play sdk to decode the data from netsdk
    PLAY_SetStreamOpenMode(ipc->channel_port, STREAME_REALTIME);
	if (!(PLAY_OpenStream(ipc->channel_port, 0, 0, 1024*1024*1)))
		return NULL;
        
    if (!(PLAY_SetDecCallBackEx(ipc->channel_port, decodeCallback,
                                (void*)ipc)))
        return NULL;
	
	PLAY_SetDecCBStream(ipc->channel_port, 1);
	if (!(PLAY_Play(ipc->channel_port, NULL)))
		return NULL;
    
    // connect the net camera
    int error;
    ipc->loginHandle = CLIENT_Login(ipc->ip, ipc->ip_port,
                                    ipc->username, ipc->password,
                                    &dev_info, &error);
    ipc->channel_num = dev_info.byChanNum;
    if (error > 0)
    {
        fprintf(stderr, "can not connect this device: %s\n" , _connectErrorStr(error));
        return NULL;
    }

    ipc->rtype = DH_RType_Realplay_0;
	ipc->channel = CLIENT_RealPlayEx(ipc->loginHandle, 0, NULL,
                                     ipc->rtype); 
	if (ipc->channel == 0)
        return 0;
    
    if (!(CLIENT_SetRealDataCallBackEx(ipc->channel,
                                       realDataCallbackEx,
                                       (LDWORD)ipc, 1)))
        return 0;

    return ipc;
}

cv::Mat CAMERA_DH::getImage(void *ipc)
{
    DHIPC *_ipc = (DHIPC *)ipc;
    if (_ipc->load_flag)
        memcpy(_ipc->mat.data, _ipc->image_data,
               (size_t)(_ipc->width*_ipc->height*3));
    return _ipc->mat;
}

void CAMERA_DH::releaseIPC(void *ipc)
{
    DHIPC *_ipc = (DHIPC *)ipc;

    PLAY_SetDecCallBackEx(_ipc->channel_port, NULL, NULL); 
    PLAY_Stop(_ipc->channel_port); 
    PLAY_CloseStream(_ipc->channel_port);

    CLIENT_StopRealPlay(_ipc->channel);
	if(_ipc->loginHandle != 0)
	{
		CLIENT_Logout(_ipc->loginHandle);
	}
    if (_ipc->image_data != 0)
        delete []_ipc->image_data;
}
