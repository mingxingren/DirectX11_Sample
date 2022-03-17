//
// Created by MMK on 2021/8/30.
//

#include "decode_thd.h"
#include <memory>
extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext_d3d11va.h>
}
#include "../time.h"

AVPixelFormat CDecodeThd::m_eHwPixFmt = AV_PIX_FMT_NONE;

CDecodeThd::CDecodeThd(const std::string &file_path, CD3DRender* render) 
: m_sFileName(file_path), m_pRenderDevice(render) {

}

void CDecodeThd::StartThd(){
    if (this->m_thread == nullptr) {
        this->m_thread = new std::thread(&CDecodeThd::run, this);
    }
}

void CDecodeThd::run(){
    _GetViodeSupportHWDevices();

    AVFormatContext *pFormatContex = nullptr;
    std::string sFileName = m_sFileName;
    int iRet = ::avformat_open_input(&pFormatContex, sFileName.c_str(), NULL, NULL);
    if (!pFormatContex)
    {
       printf("avformat_open_input fail \n", "");
       return;
    }
    printf("avformat_open_input success \n");

    if (avformat_find_stream_info(pFormatContex, NULL) < 0)
    {
        printf("could not find stream information \n", "");
        return;
    }

    printf("avformat_find_stream_info success \n", "");
    av_dump_format(pFormatContex, 0, sFileName.c_str(), 0);

    int iAudioStreamIndex = -1;
    int iVideoStreamIndex = -1;

    double dAudioUnit = 0, dVieoUnit = 0;
    AVCodec *pVideoDecoder = nullptr;

    // 硬件加速
    iRet = av_find_best_stream(pFormatContex, AVMEDIA_TYPE_VIDEO, -1, -1, (const AVCodec**)&pVideoDecoder, 0);
    if (iRet < 0){
        printf("Cannot find a video stream in the input file");
    }
    iVideoStreamIndex = iRet;
    dVieoUnit = av_q2d(pFormatContex->streams[iRet]->time_base) * 1000;
    printf("Find a video stream, index : %d, Time Unit: %f", iVideoStreamIndex, dVieoUnit);

    for (int i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(pVideoDecoder, i);
        if (!config) {
            fprintf(stderr, "Decoder %s does not support device type %s.\n",
                    pVideoDecoder->name, av_hwdevice_get_type_name(m_eHWDeviceType));
            return;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == m_eHWDeviceType) {
            m_eHwPixFmt = config->pix_fmt;
            printf("######################m_eHwPixFmt: %d\n", m_eHwPixFmt);
            break;
        }
    }
    // 硬件加速

    for (uint32_t i = 0; i < pFormatContex->nb_streams; i++)
    {
       if (pFormatContex->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
       {
            iAudioStreamIndex = i;
            dAudioUnit = av_q2d(pFormatContex->streams[i]->time_base) * 1000;
            printf("Find a audio stream, index : %d, Time Unit: %f", iAudioStreamIndex, dAudioUnit);
       }
    }

    AVFrame * pVideoFrameRGB = av_frame_alloc();
    if (iVideoStreamIndex == -1)
    {
        printf("Cant find video stream \n");
    }
    else{
        this->_InitVideoFormat(pFormatContex, iVideoStreamIndex, pVideoFrameRGB, pVideoDecoder);
    }

    AVPacket *pPack = av_packet_alloc();

    // 硬解改动
    AVFrame *pHwFrame = av_frame_alloc();
    AVFrame *pResultFrame = nullptr;

    int64_t frame_count = 0;
    int64_t total_time = 0;
    while (true) {
        Timer count;
        if (::av_read_frame(pFormatContex, pPack) != 0){
            break;
        }

        // 解析出视频格式
        if(pPack->stream_index == iVideoStreamIndex)
        {
            iRet = ::avcodec_send_packet(m_pCodeContext, pPack);
            if (iRet != 0) {
                printf("avcodec_receive_frame() failed: %d \n", iRet);
                return;
            }

            std::shared_ptr<AVFrame> pFramePtr(av_frame_alloc(), [](AVFrame* p){
                av_frame_free(&p);
            });

            std::shared_ptr<AVFrame> pRecieveFramePtr(av_frame_alloc(), [](AVFrame* p){
                av_frame_free(&p);
            });
            while (0 == ::avcodec_receive_frame(m_pCodeContext, pFramePtr.get())){
                // 硬解改动
                if (pFramePtr->format == m_eHwPixFmt) {
                    /* retrieve data from GPU to CPU */
                    printf("###############frame format is: %s \n", av_get_pix_fmt_name(static_cast<AVPixelFormat>(pFramePtr->format)));
                    this->m_pRenderDevice->RenderFrame(pFramePtr.get());
                    
                    frame_count += 1;
                    total_time += count.elapsed();
                    printf("############### decode and render cost milliseconds: %d \n", count.elapsed());
                } else{
                   printf("###################software decode a frame success \n");
//                    pResultFrame = pFrameOrg;
                }
                // 硬解改动

            }
        }
        ::av_packet_unref(pPack);
    }

    float average_time = (float)total_time / (float)frame_count;
    printf("total frame count: %d , total_time: %d average cost time every frame: %f \n", frame_count, total_time, average_time);
    ::av_packet_free(&pPack);
}

void CDecodeThd::_InitVideoFormat(AVFormatContext *_pAVFormatContext, int _iStreamIndex,
                                  AVFrame *pFrameOutRGB, AVCodec * _pCodec) {
    AVCodecParameters *pCodeParam = _pAVFormatContext->streams[_iStreamIndex]->codecpar;

    const AVCodec *pCodec = ::avcodec_find_decoder(pCodeParam->codec_id);
    if (pCodec == NULL) {
        printf("Cannt find video codec!\n");
        return;
    }

    m_pCodeContext = ::avcodec_alloc_context3(_pCodec);
    int iRet = avcodec_parameters_to_context(m_pCodeContext, _pAVFormatContext->streams[_iStreamIndex]->codecpar);
    if (iRet < 0){
        printf("#######################avcodec_parameters_to_context fail \n");
    }
    // m_pCodeContext->get_format  = _GetHwFormat;

    // 创建硬件解码器
    AVBufferRef* hw_device_ctx =  av_hwdevice_ctx_alloc(m_eHWDeviceType);
    AVHWDeviceContext* device_ctx = reinterpret_cast<AVHWDeviceContext*>(hw_device_ctx->data);
    AVD3D11VADeviceContext* d3d11va_device = reinterpret_cast<AVD3D11VADeviceContext*>(device_ctx->hwctx);
    d3d11va_device->device = m_pRenderDevice->GetD3DDevice();
    iRet = av_hwdevice_ctx_init(hw_device_ctx);
    if (iRet != 0) {
        printf("#######################av_hwdevice_ctx_init fail \n");
    }
    m_pCodeContext->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    m_pHwDeviceCtx = hw_device_ctx;

    iRet = ::avcodec_open2(m_pCodeContext, pCodec, NULL);
    if (iRet < 0) {
        printf("avcodec_open2() failed, ret: %d", iRet);
        return;
    }
}

std::vector<std::string> CDecodeThd::_GetViodeSupportHWDevices() {
    enum AVHWDeviceType print_type = AV_HWDEVICE_TYPE_NONE;
    AVBufferRef *hw_device_ctx = NULL;

    while ((print_type = av_hwdevice_iterate_types(print_type)) != AV_HWDEVICE_TYPE_NONE){
        printf("#####################support: %s \n", av_hwdevice_get_type_name(print_type));
    }
    return std::vector<std::string>();
}

AVPixelFormat CDecodeThd::_GetHwFormat(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == m_eHwPixFmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}
