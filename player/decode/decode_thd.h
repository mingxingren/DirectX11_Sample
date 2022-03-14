//
// Created by MMK on 2021/8/30.
//

#ifndef DECODE_THD_H
#define DECODE_THD_H

#include <array>
#include <thread>
#include <vector>
#include <string>
#include <d3d11.h>
extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class CDecodeThd {
public:
    CDecodeThd(const std::string &file_path, ID3D11Device* d3d_device);
    ~CDecodeThd() = default;
    CDecodeThd(const CDecodeThd&) = delete;
    CDecodeThd& operator=(const CDecodeThd&) = delete;
    void StartThd();

protected:
    void run();

private:
    /**
     * @brief _InitVideoFormat
     * @param _pAVFormatContext
     * @param _iStreamIndex
     * @param _pFrameOutRGB 给转码的目标图像分配内存
     * @param _pCodec 编码器
     */
    void _InitVideoFormat(AVFormatContext *_pAVFormatContext, int _iStreamIndex, AVFrame * _pFrameOutRGB, AVCodec * _pCodec);

    std::vector<std::string> _GetViodeSupportHWDevices();

    static AVPixelFormat _GetHwFormat(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);

private:
    AVCodecContext *m_pCodeContext = nullptr;

private:
    std::thread *m_thread = nullptr;
    AVPixelFormat m_eDstPixelFormat = AV_PIX_FMT_BGRA;

    // 硬件加速相关变量
    AVBufferRef *m_pHwDeviceCtx = nullptr;
    static AVPixelFormat m_eHwPixFmt;
    AVHWDeviceType m_eHWDeviceType = AV_HWDEVICE_TYPE_D3D11VA;

    std::string m_sFileName;
    ID3D11Device* m_pD3DDevice = nullptr;    // d3ddevice        
};


#endif //DECODE_THD_H
