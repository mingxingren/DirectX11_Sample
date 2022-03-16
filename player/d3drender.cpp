#include "d3drender.h"
#include <array>
#include <iostream>
#include <directxmath.h>
#include "common_type.h"
#include "shader/fragment_shader.inc"
#include "shader/vertex_shader.inc"

#define CHECK_AND_RETURN(x) if (x < 0) { \
    printf("##########file: %s,  line: %d, error: %x \n", __FILE__, __LINE__, x); \
    return false; \
}

#define TEXTURE_WIDTH 2560
#define TEXTURE_HEIGHT 1440

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT2;

CD3DRender::CD3DRender() {

}

CD3DRender::~CD3DRender() {
    if (this->m_pRTV) {
        this->m_pRTV->Release();
        this->m_pRTV = nullptr;
    }

    if (this->m_pDeviceContext) {
        this->m_pDeviceContext->Release();
        this->m_pDeviceContext = nullptr;
    }

    if (this->m_pDevice) {
        this->m_pDevice->Release();
        this->m_pDevice = nullptr;
    }

    if (this->m_pSwapChain) {
        this->m_pSwapChain->Release();
        this->m_pSwapChain = nullptr;
    }

    if (this->m_pLuminanceView) {
        this->m_pLuminanceView->Release();
        this->m_pLuminanceView = nullptr;
    }

    if (this->m_pChrominanceView) {
        this->m_pChrominanceView->Release();
        this->m_pChrominanceView = nullptr;
    }

    if (this->m_pSharedSurf) {
        this->m_pSharedSurf->Release();
        this->m_pSharedSurf = nullptr;
    }

    if (this->m_pFactory) {
        this->m_pFactory->Release();
        this->m_pFactory = nullptr;
    }
}

bool CD3DRender::Init(HWND window) {
    this->m_iWindow = window;

    // 获取窗口尺寸
    RECT dialog_rect;
    GetClientRect(this->m_iWindow, &dialog_rect);

    D3D_FEATURE_LEVEL feature_level;
    HRESULT res = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, 
                    &this->m_pDevice, &feature_level, &this->m_pDeviceContext);
    CHECK_AND_RETURN(res)

    // 创建交换链
    DXGI_SWAP_CHAIN_DESC swap_chain_desc;
    ZeroMemory(&swap_chain_desc, sizeof(swap_chain_desc));
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.BufferDesc.Width = dialog_rect.right - dialog_rect.left;
    swap_chain_desc.BufferDesc.Height = dialog_rect.bottom - dialog_rect.top;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = this->m_iWindow;
    swap_chain_desc.SampleDesc.Count = 1;       // 置为无效
    swap_chain_desc.SampleDesc.Quality = 0;     // 置为无效
    swap_chain_desc.Windowed = true;

    IDXGIDevice* dxgi_deivce = nullptr;
    res = this->m_pDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_deivce));
    CHECK_AND_RETURN(res)

    IDXGIAdapter* dxgi_adapter = nullptr;
    res = dxgi_deivce->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgi_adapter));
    CHECK_AND_RETURN(res)
    dxgi_deivce->Release();
    dxgi_deivce = nullptr;

    res = dxgi_adapter->GetParent(__uuidof(m_pFactory), reinterpret_cast<void**>(&this->m_pFactory));
    CHECK_AND_RETURN(res)
    dxgi_adapter->Release();
    dxgi_adapter = nullptr;

    res = this->m_pFactory->CreateSwapChain(this->m_pDevice, &swap_chain_desc, &this->m_pSwapChain);
    CHECK_AND_RETURN(res)

    // this->m_pFactory->MakeWindowAssociation(this->m_iWindow, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
    
    // 创建共享纹理
    CD3D11_TEXTURE2D_DESC texture_desc = CD3D11_TEXTURE2D_DESC( 
        DXGI_FORMAT_NV12,
        TEXTURE_WIDTH,
        TEXTURE_HEIGHT,
        1,
        1,
        D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DYNAMIC,
        D3D11_CPU_ACCESS_WRITE
    );

    res = this->m_pDevice->CreateTexture2D(&texture_desc, nullptr, &m_pTexture);
    CHECK_AND_RETURN(res)
    
    // 亮度通道 y channel
    D3D11_SHADER_RESOURCE_VIEW_DESC luminancePlaneDesc;
    this->SetShaderResViewDesc(&luminancePlaneDesc, this->m_pTexture, 
                            D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8_UNORM);
    res = this->m_pDevice->CreateShaderResourceView(this->m_pTexture, &luminancePlaneDesc, &this->m_pLuminanceView);
    CHECK_AND_RETURN(res)

    // 色度通道 uv channel
    D3D11_SHADER_RESOURCE_VIEW_DESC chrominancePlaneDesc;
    this->SetShaderResViewDesc(&chrominancePlaneDesc, this->m_pTexture, 
                            D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8_UNORM);
    res = this->m_pDevice->CreateShaderResourceView(this->m_pTexture, &chrominancePlaneDesc, &this->m_pChrominanceView);
    CHECK_AND_RETURN(res)

    // 创建渲染目标
    ID3D11Texture2D * back_buffer = nullptr;
    // 获取交换链的缓冲区纹理
    res = this->m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
    CHECK_AND_RETURN(res)
    // 创建渲染目标的视图
    res = this->m_pDevice->CreateRenderTargetView(back_buffer, nullptr, &m_pRTV);
    CHECK_AND_RETURN(res)
    // 将目标渲染视图结合到管线
    this->m_pDeviceContext->OMSetRenderTargets(1, &m_pRTV, nullptr);
    
    this->SetViewport(dialog_rect.right - dialog_rect.left, dialog_rect.bottom - dialog_rect.top);
    
    // 创建片段采样器
    D3D11_SAMPLER_DESC sample_desc;
    ZeroMemory(&sample_desc, sizeof(sample_desc));
    sample_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sample_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sample_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sample_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
   	sample_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sample_desc.MinLOD = 0;
    sample_desc.MaxLOD = D3D11_FLOAT32_MAX;
    res = this->m_pDevice->CreateSamplerState(&sample_desc, &m_pPSSamplerState);
    CHECK_AND_RETURN(res)

    // 描述缓冲的布局
    constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> Layout = {{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    }};

    // 初始化顶点着色器
    UINT Size = ARRAYSIZE(g_vertex_shader);
    res = this->m_pDevice->CreateVertexShader(g_vertex_shader, Size, nullptr, &this->m_pVertexShader);
    CHECK_AND_RETURN(res)

    // 创建输入布局                    布局描述         布局数组长度    包含输入签名顶点shader  shader长度  out
    res = m_pDevice->CreateInputLayout(Layout.data(), Layout.size(), g_vertex_shader, Size, &this->m_pInputLayout);
    CHECK_AND_RETURN(res)

    // 绑定输入布局
    this->m_pDeviceContext->IASetInputLayout(this->m_pInputLayout);

    // 初始化片段着色器
    Size = ARRAYSIZE(g_fragment_shader);
    res = this->m_pDevice->CreatePixelShader(g_fragment_shader, Size, nullptr, &this->m_pPixelShader);
    CHECK_AND_RETURN(res)

    // 将yuv_file 读取到texture中
    // CYuvFile yuv_texture("output.y-1", "output.uv-1", TEXTURE_WIDTH, TEXTURE_HEIGHT);
    // D3D11_MAPPED_SUBRESOURCE resource;
    // UINT subresource = D3D11CalcSubresource(0, 0, 0);
    // this->m_pDeviceContext->Map(this->m_pTexture, subresource, D3D11_MAP_WRITE_DISCARD, 0, &resource);

    // std::cout << "###################yuv file  y size: " << yuv_texture.y_planar_size() 
    // << " uv size:  " << yuv_texture.uv_planar_size() << std::endl; 

    // BYTE* dptr = reinterpret_cast<BYTE*>(resource.pData);
    // memcpy(dptr, yuv_texture.y_planar(), yuv_texture.y_planar_size());

    // memcpy(dptr + yuv_texture.y_planar_size(), yuv_texture.uv_planar(), yuv_texture.uv_planar_size());
    // this->m_pDeviceContext->Unmap(this->m_pTexture, subresource);


    // NV12Frame* flie_frame = ReadNV12FromFile();
    // WriteNV12ToTexture(flie_frame, this->m_pDeviceContext, this->m_pTexture);

    return true;
}

void CD3DRender::SetViewport(UINT32 width, UINT32 height) {
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<FLOAT>(width);
    vp.Height = static_cast<FLOAT>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    this->m_pDeviceContext->RSSetViewports(1, &vp);
}

void CD3DRender::RenderFrame(AVFrame * frame) {
    // 将FFMPEG的数据拷入到共享纹理中
    ID3D11Texture2D* new_texture= (ID3D11Texture2D*)frame->data[0];
    int64_t new_texture_index = (int64_t)frame->data[1];
    this->m_pDeviceContext->CopySubresourceRegion(this->m_pTexture, 0, 0, 0, 0,  
                                                new_texture, new_texture_index, nullptr);


    VERTEX Vertices[NUMVERTICES] = {
        { XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f) },
    };

    std::array<ID3D11ShaderResourceView*, 2> const textureViews = {
        this->m_pLuminanceView, this->m_pChrominanceView
    };

    // 绑定 NV12 channels to shader
    // texture id 分别为 0, 1
    this->m_pDeviceContext->PSSetShaderResources(0, textureViews.size(), textureViews.data());
    
    // Set resources
    UINT Stride = sizeof(VERTEX);
    UINT Offset = 0;
    FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    // 设置混合
    this->m_pDeviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
    // 设置渲染目标
    this->m_pDeviceContext->OMSetRenderTargets(1, &m_pRTV, nullptr);
    // 设置shader到设备上
    this->m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    this->m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
    this->m_pDeviceContext->PSSetSamplers(0, 1, &m_pPSSamplerState);
    // 将顶点描述为三角形
    this->m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_BUFFER_DESC BufferDec;
    ZeroMemory(&BufferDec, sizeof(BufferDec));
    BufferDec.Usage = D3D11_USAGE_DEFAULT;
    BufferDec.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
    BufferDec.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDec.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = Vertices;

    ID3D11Buffer * VertexBuffer = nullptr;
    HRESULT res = this->m_pDevice->CreateBuffer(&BufferDec, &InitData, &VertexBuffer);
    if (res != 0) {
        printf("##########CreateBuffer  file: %s,  line: %d, error: %x \n", __FILE__, __LINE__, res);
    }
    m_pDeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
    m_pDeviceContext->Draw(NUMVERTICES, 0);
    
    VertexBuffer->Release();
    VertexBuffer = nullptr;

    res = this->m_pSwapChain->Present(0, 0);
    if (res != 0) {
        printf("##########m_pSwapChain->Present  file: %s,  line: %d, error: %x \n", __FILE__, __LINE__, res);
    }
}

void CD3DRender::SetShaderResViewDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* desc, 
                                    _In_ ID3D11Texture2D* pTex2D,
                                    D3D11_SRV_DIMENSION viewDimension,
                                    DXGI_FORMAT format,
                                    UINT mostDetailedMip,
                                    UINT mipLevels,
                                    UINT firstArraySlice, // First2DArrayFace for TEXTURECUBEARRAY
                                    UINT arraySize) {
    desc->ViewDimension = viewDimension;
    if (DXGI_FORMAT_UNKNOWN == format || 
        (-1 == mipLevels &&
            D3D11_SRV_DIMENSION_TEXTURE2DMS != viewDimension &&
            D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY != viewDimension) ||
        (-1 == arraySize &&
            (D3D11_SRV_DIMENSION_TEXTURE2DARRAY == viewDimension ||
            D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY == viewDimension ||
            D3D11_SRV_DIMENSION_TEXTURECUBEARRAY == viewDimension)))
    {
        D3D11_TEXTURE2D_DESC TexDesc;
        pTex2D->GetDesc( &TexDesc );
        if (DXGI_FORMAT_UNKNOWN == format) format = TexDesc.Format;
        if (-1 == mipLevels) mipLevels = TexDesc.MipLevels - mostDetailedMip;
        if (-1 == arraySize)
        {
            arraySize = TexDesc.ArraySize - firstArraySlice;
            if (D3D11_SRV_DIMENSION_TEXTURECUBEARRAY == viewDimension) arraySize /= 6;
        }
    }
    desc->Format = format;
    switch (viewDimension)
    {
    case D3D11_SRV_DIMENSION_TEXTURE2D:
        desc->Texture2D.MostDetailedMip = mostDetailedMip;
        desc->Texture2D.MipLevels = mipLevels;
        break;
    case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
        desc->Texture2DArray.MostDetailedMip = mostDetailedMip;
        desc->Texture2DArray.MipLevels = mipLevels;
        desc->Texture2DArray.FirstArraySlice = firstArraySlice;
        desc->Texture2DArray.ArraySize = arraySize;
        break;
    case D3D11_SRV_DIMENSION_TEXTURE2DMS:
        break;
    case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
        desc->Texture2DMSArray.FirstArraySlice = firstArraySlice;
        desc->Texture2DMSArray.ArraySize = arraySize;
        break;
    case D3D11_SRV_DIMENSION_TEXTURECUBE:
        desc->TextureCube.MostDetailedMip = mostDetailedMip;
        desc->TextureCube.MipLevels = mipLevels;
        break;
    case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
        desc->TextureCubeArray.MostDetailedMip = mostDetailedMip;
        desc->TextureCubeArray.MipLevels = mipLevels;
        desc->TextureCubeArray.First2DArrayFace = firstArraySlice;
        desc->TextureCubeArray.NumCubes = arraySize;
        break;
    default: break;
    }
}

