#ifndef D3DRENDER_H
#define D3DRENDER_H

#include <iostream>
#include <d3d11.h>

class CD3DRender {
public:
    CD3DRender();
    ~CD3DRender();
    CD3DRender(const CD3DRender&) = delete;
    CD3DRender& operator=(const CD3DRender&) = delete;

public:
    bool Init(HWND window);
    void SetViewport(UINT32 width, UINT32 height);
    void RenderFrame();

private:
    void SetShaderResViewDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* desc,
                            _In_ ID3D11Texture2D* pTex2D,
                            D3D11_SRV_DIMENSION viewDimension,
                            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN,
                            UINT mostDetailedMip = 0,
                            UINT mipLevels = -1,
                            UINT firstArraySlice = 0,
                            UINT arraySize = -1 );

private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pDeviceContext = nullptr;
    ID3D11Texture2D* m_pSharedSurf = nullptr;
    ID3D11Texture2D* m_pAccessibleSurf = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    
    ID3D11Texture2D* m_pTexture = nullptr;
    ID3D11ShaderResourceView* m_pLuminanceView = nullptr;
    ID3D11ShaderResourceView* m_pChrominanceView = nullptr;

    uint32_t m_width = 0;
    uint32_t m_height = 0;

    HWND m_iWindow = nullptr;

private:
    IDXGIFactory* m_pFactory = nullptr;
    ID3D11RenderTargetView* m_pRTV = nullptr;
    ID3D11VertexShader* m_pVertexShader = nullptr;  // 顶点着色器
    ID3D11PixelShader* m_pPixelShader = nullptr;    // 片段着色器
    ID3D11InputLayout* m_pInputLayout = nullptr;
    ID3D11SamplerState* m_pPSSamplerState = nullptr;    // 判断着色采样
};

#endif //D3DRENDER_H