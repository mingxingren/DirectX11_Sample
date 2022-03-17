#ifndef D3DRENDER_H
#define D3DRENDER_H

#include <iostream>
#include <d3d11.h>
#include <wrl/client.h>
extern "C" {
#include <libavcodec/avcodec.h>
}

using namespace Microsoft::WRL;

class CD3DRender {
public:
    CD3DRender();
    ~CD3DRender();
    CD3DRender(const CD3DRender&) = delete;
    CD3DRender& operator=(const CD3DRender&) = delete;

public:
    bool Init(HWND window);
    void SetViewport(UINT32 width, UINT32 height);
    void RenderFrame(AVFrame * frame);
    ID3D11Device * GetD3DDevice() { return this->m_pDevice.Get(); }

private:
    bool InitDevice(HWND window, int dialog_width, int dialog_height);  // 初始化 ID3DDevice
    bool InitShader();  // 初始化 shader
    bool ReinitTexture(DXGI_FORMAT img_format, int width, int height);  // 重新初始化纹理
    void SetShaderResViewDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* desc,
                            _In_ ID3D11Texture2D* pTex2D,
                            D3D11_SRV_DIMENSION viewDimension,
                            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
                            UINT mostDetailedMip = 0,
                            UINT mipLevels = -1,
                            UINT firstArraySlice = 0,
                            UINT arraySize = -1 );

private:
    HWND m_iWindow = nullptr;
    ComPtr<ID3D11Device> m_pDevice = nullptr;
    ComPtr<ID3D11DeviceContext> m_pDeviceContext = nullptr;
    ComPtr<IDXGISwapChain> m_pSwapChain = nullptr;     // 交换链
    ComPtr<ID3D11RenderTargetView> m_pRTV = nullptr;

    ComPtr<ID3D11Texture2D> m_pTexture = nullptr;
    ComPtr<ID3D11ShaderResourceView> m_pLuminanceView = nullptr;
    ComPtr<ID3D11ShaderResourceView> m_pChrominanceView = nullptr;

    ComPtr<IDXGIFactory> m_pFactory = nullptr;
    ComPtr<ID3D11VertexShader> m_pVertexShader = nullptr;  // 顶点着色器
    ComPtr<ID3D11PixelShader> m_pPixelShader = nullptr;    // 片段着色器
    ComPtr<ID3D11InputLayout> m_pInputLayout = nullptr;
    ComPtr<ID3D11SamplerState> m_pPSSamplerState = nullptr;    // 判断着色采样
    ComPtr<ID3D11Buffer> VertexBuffer = nullptr;
};

#endif //D3DRENDER_H