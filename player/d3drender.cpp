#include "d3drender.h"
#include <array>
#include <iostream>
#include <vector>
#include "common_type.h"
#include "time.h"
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

}

bool CD3DRender::Init(HWND window) {
    this->m_iWindow = window;
    // 获取窗口尺寸
    RECT dialog_rect;
    GetClientRect(this->m_iWindow, &dialog_rect);
    int dialog_width = dialog_rect.right - dialog_rect.left;
    int dialog_height = dialog_rect.bottom - dialog_rect.top;

    // 创建 Deivce、DeviceContext、Swapchain
    if (!this->InitDevice(this->m_iWindow, dialog_width, dialog_height)) {
        return false;
    }

    // 初始化 Shader 
    if (!this->InitShader()) {
        return false;
    }
   
    // // 初始化 Texture
    // if (!this->ReinitTexture(DXGI_FORMAT_NV12, TEXTURE_WIDTH, TEXTURE_HEIGHT)) {
    //     return false;
    // }

    // 创建渲染目标
    ComPtr<ID3D11Texture2D> back_buffer = nullptr;
    // 获取交换链的缓冲区纹理
    HRESULT res = this->m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), 
                                                reinterpret_cast<void**>(back_buffer.GetAddressOf()));
    CHECK_AND_RETURN(res)
    // 创建渲染目标的视图
    res = this->m_pDevice->CreateRenderTargetView(back_buffer.Get(), nullptr, m_pRTV.GetAddressOf());
    CHECK_AND_RETURN(res)
    
    // 设置窗口输出大小
    this->SetViewport(dialog_width, dialog_height);

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
    if (this->texture_width != frame->width || this->texture_height != frame->height) {
        this->texture_width = frame->width;
        this->texture_height = frame->height;
        this->ReinitTexture(DXGI_FORMAT_NV12, this->texture_width, this->texture_height);
    }

    // 将FFMPEG的数据拷入到共享纹理中
    ID3D11Texture2D* new_texture= (ID3D11Texture2D*)frame->data[0];
    int64_t new_texture_index = (int64_t)frame->data[1];

	ComPtr<ID3D11Device> device;
	new_texture->GetDevice(device.GetAddressOf());

	ComPtr<ID3D11DeviceContext> deviceCtx;
	device->GetImmediateContext(&deviceCtx);

	ComPtr<ID3D11Texture2D> videoTexture;
	HRESULT res_handle = device->OpenSharedResource(this->m_pTextureShareHandle, __uuidof(ID3D11Texture2D), (void**)&videoTexture);
    if (res_handle != S_OK) {
        printf("##########file: %s,  line: %d, error: %x \n", __FILE__, __LINE__, res_handle);
    }

	deviceCtx->CopySubresourceRegion(videoTexture.Get(), 0, 0, 0, 0, new_texture, new_texture_index, 0);
	deviceCtx->Flush();

    // this->m_pDeviceContext->CopySubresourceRegion(this->m_pTexture.Get(), 0, 0, 0, 0, new_texture, new_texture_index, nullptr);

    // Timer clock;
    // Set resources
    FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    // 设置混合
    this->m_pDeviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
    // 将目标渲染视图 和 深度视图合并到管线
    this->m_pDeviceContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);

    // 将顶点描述为三角形
    this->m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    m_pDeviceContext->Draw(NUMVERTICES, 0);

    // // // 反转后备缓冲
    // // this->m_pDeviceContext->Flush();
    HRESULT res = this->m_pSwapChain->Present(0, 0);
    if (res != 0) {
        printf("##########m_pSwapChain->Present  file: %s,  line: %d, error: %x \n", __FILE__, __LINE__, res);
    }
}

bool CD3DRender::InitDevice(HWND window, int dialog_width, int dialog_height) {
    D3D_FEATURE_LEVEL feature_level;
    this->pCurAdapter = this->ListGPUDevice();
    HRESULT res = D3D11CreateDevice(pCurAdapter.Get(), pCurAdapter.Get() == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN, 
                                    NULL, 0, NULL, 0, D3D11_SDK_VERSION, 
                                    this->m_pDevice.GetAddressOf(), &feature_level,  this->m_pDeviceContext.GetAddressOf());

    // HRESULT res = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, 
    // this->m_pDevice.GetAddressOf(), &feature_level,  this->m_pDeviceContext.GetAddressOf());
    CHECK_AND_RETURN(res)
    
    // 创建交换链
    DXGI_SWAP_CHAIN_DESC swap_chain_desc;
    ZeroMemory(&swap_chain_desc, sizeof(swap_chain_desc));
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.BufferDesc.Width = dialog_width;
    swap_chain_desc.BufferDesc.Height = dialog_height;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = this->m_iWindow;
    swap_chain_desc.SampleDesc.Count = 1;       // 置为无效
    swap_chain_desc.SampleDesc.Quality = 0;     // 置为无效
    swap_chain_desc.Windowed = true;
    // swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGIDevice> dxgi_deivce = nullptr;
    res = this->m_pDevice->QueryInterface(__uuidof(IDXGIDevice), 
                                        reinterpret_cast<void**>(dxgi_deivce.ReleaseAndGetAddressOf()));
    CHECK_AND_RETURN(res)

    ComPtr<IDXGIAdapter> dxgi_adapter = nullptr;
    res = dxgi_deivce->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(dxgi_adapter.ReleaseAndGetAddressOf()));
    CHECK_AND_RETURN(res)

    res = dxgi_adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(this->m_pFactory.ReleaseAndGetAddressOf()));
    CHECK_AND_RETURN(res)

    res = this->m_pFactory->CreateSwapChain(this->m_pDevice.Get(), &swap_chain_desc, this->m_pSwapChain.GetAddressOf());
    CHECK_AND_RETURN(res)

    // this->m_pFactory->MakeWindowAssociation(this->m_iWindow, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
    return true;
}

bool CD3DRender::InitShader() {
    // 描述缓冲的布局
    constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> Layout = {{
        {
            "POSITION",     // 语义名
            0,              // 语义索引
            DXGI_FORMAT_R32G32B32_FLOAT,    // 数据格式 DXGI_FORMAT_R32G32B32_FLOAT格式为三个向量, 都为float类型
            0,              // 输入槽索引(0 ~ 15)
            0,              // 初始位置 (字节偏移量)
            D3D11_INPUT_PER_VERTEX_DATA, // 输入类型 D3D11_INPUT_PER_VERTEX_DATA:按每个顶点数输入  D3D11_INPUT_PER_INSTANCE_DATA:按每个数据实例输入
            0               // 忽略
        },
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    }};

    UINT Size = ARRAYSIZE(g_vertex_shader);
    // 创建输入布局                    布局描述         布局数组长度    包含输入签名顶点shader  shader长度  out
    HRESULT res = this->m_pDevice->CreateInputLayout(Layout.data(), Layout.size(), g_vertex_shader, Size, this->m_pInputLayout.GetAddressOf());
    CHECK_AND_RETURN(res)

    // 绑定输入布局
    this->m_pDeviceContext->IASetInputLayout(this->m_pInputLayout.Get());

    // 初始化顶点着色器
    res = this->m_pDevice->CreateVertexShader(g_vertex_shader, Size, nullptr, this->m_pVertexShader.ReleaseAndGetAddressOf());
    CHECK_AND_RETURN(res)

    // 初始化片段着色器
    Size = ARRAYSIZE(g_fragment_shader);
    res = this->m_pDevice->CreatePixelShader(g_fragment_shader, Size, nullptr, this->m_pPixelShader.ReleaseAndGetAddressOf());
    CHECK_AND_RETURN(res)

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
    res = this->m_pDevice->CreateSamplerState(&sample_desc, m_pPSSamplerState.ReleaseAndGetAddressOf());
    CHECK_AND_RETURN(res)

    // 填入顶点缓冲
    VERTEX Vertices[NUMVERTICES] = {
        { XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f) },
    };

    UINT Stride = sizeof(VERTEX);
    UINT Offset = 0;

    D3D11_BUFFER_DESC BufferDec;
    ZeroMemory(&BufferDec, sizeof(BufferDec));
    BufferDec.Usage = D3D11_USAGE_DEFAULT;
    BufferDec.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
    BufferDec.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDec.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = Vertices;

    res = this->m_pDevice->CreateBuffer(&BufferDec, &InitData, this->VertexBuffer.GetAddressOf());
    if (res != 0) {
        printf("##########CreateBuffer  file: %s,  line: %d, error: %x \n", __FILE__, __LINE__, res);
    }
    this->m_pDeviceContext->IASetVertexBuffers(0, 1, this->VertexBuffer.GetAddressOf(), &Stride, &Offset);

    // 设备绑定shader
    this->m_pDeviceContext->VSSetShader(this->m_pVertexShader.Get(), nullptr, 0);
    this->m_pDeviceContext->PSSetShader(this->m_pPixelShader.Get(), nullptr, 0);
    this->m_pDeviceContext->PSSetSamplers(0, 1, this->m_pPSSamplerState.GetAddressOf());

    return true;
}

bool CD3DRender::ReinitTexture(DXGI_FORMAT img_format, int texture_width, int texture_height) {
    // 删除共享纹理
    this->m_pTexture.Reset();
    this->m_pChrominanceView.Reset();
    this->m_pLuminanceView.Reset();

    // 创建共享纹理
    CD3D11_TEXTURE2D_DESC texture_desc = CD3D11_TEXTURE2D_DESC( 
        img_format,
        texture_width,
        texture_height,
        1,
        1,
        D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT,
        0
    );
    texture_desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    HRESULT res = this->m_pDevice->CreateTexture2D(&texture_desc, nullptr, this->m_pTexture.GetAddressOf());
    CHECK_AND_RETURN(res)

    ComPtr<IDXGIResource> dxgiShareTexture;
    res = this->m_pTexture->QueryInterface(__uuidof(IDXGIResource), (void**)dxgiShareTexture.GetAddressOf());
    CHECK_AND_RETURN(res)
    
    res = dxgiShareTexture->GetSharedHandle(&this->m_pTextureShareHandle);
    CHECK_AND_RETURN(res)

    // 亮度通道 y channel
    D3D11_SHADER_RESOURCE_VIEW_DESC luminancePlaneDesc;
    this->SetShaderResViewDesc(&luminancePlaneDesc, this->m_pTexture.Get(), 
                            D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8_UNORM);
    res = this->m_pDevice->CreateShaderResourceView(this->m_pTexture.Get(), &luminancePlaneDesc, this->m_pLuminanceView.GetAddressOf());
    CHECK_AND_RETURN(res)

    // 色度通道 uv channel
    D3D11_SHADER_RESOURCE_VIEW_DESC chrominancePlaneDesc;
    this->SetShaderResViewDesc(&chrominancePlaneDesc, this->m_pTexture.Get(), 
                            D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8_UNORM);
    res = this->m_pDevice->CreateShaderResourceView(this->m_pTexture.Get(), &chrominancePlaneDesc, this->m_pChrominanceView.GetAddressOf());
    CHECK_AND_RETURN(res)

    std::array<ID3D11ShaderResourceView*, 2> const textureViews = {
        this->m_pLuminanceView.Get(), this->m_pChrominanceView.Get()
    };

    // 绑定 NV12 channels to shader
    // 在shader中texture id 分别为 0, 1
    this->m_pDeviceContext->PSSetShaderResources(0, textureViews.size(), textureViews.data());

    return true;
}

bool CD3DRender::ResetTargetView(ID3D11Texture2D* texture_begin, int index) {
    // 删除共享纹理
    this->m_pChrominanceView.Reset();
    this->m_pLuminanceView.Reset();
    
    // 亮度通道 y channel
    D3D11_SHADER_RESOURCE_VIEW_DESC luminancePlaneDesc;
    this->SetShaderResViewDesc(&luminancePlaneDesc, texture_begin, 
                            D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8_UNORM);
    HRESULT res = this->m_pDevice->CreateShaderResourceView(this->m_pTexture.Get(), &luminancePlaneDesc, this->m_pLuminanceView.GetAddressOf());
    CHECK_AND_RETURN(res)

    // 色度通道 uv channel
    D3D11_SHADER_RESOURCE_VIEW_DESC chrominancePlaneDesc;
    this->SetShaderResViewDesc(&chrominancePlaneDesc, texture_begin, 
                            D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8_UNORM);
    res = this->m_pDevice->CreateShaderResourceView(texture_begin, &chrominancePlaneDesc, this->m_pChrominanceView.GetAddressOf());
    CHECK_AND_RETURN(res)

    std::array<ID3D11ShaderResourceView*, 2> const textureViews = {
        this->m_pLuminanceView.Get(), this->m_pChrominanceView.Get()
    };

    // 绑定 NV12 channels to shader
    // 在shader中texture id 分别为 0, 1
    this->m_pDeviceContext->PSSetShaderResources(0, textureViews.size(), textureViews.data());

    return true;
}

void CD3DRender::SetShaderResViewDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* desc, 
                                    _In_ ID3D11Texture2D* pTex2D,
                                    D3D11_SRV_DIMENSION viewDimension,
                                    DXGI_FORMAT format,
                                    UINT mostDetailedMip,
                                    UINT mipLevels,
                                    UINT firstArraySlice, // First2DArrayFace for TEXTURECUBEARRAY
                                    UINT arraySize) {
                                        // D3D11_SRV_DIMENSION_TEXTURE2D

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

std::string WStringToString(const std::wstring &wstr)
{
    std::string str(wstr.length(), ' ');
    std::copy(wstr.begin(), wstr.end(), str.begin());
    return str;
}

ComPtr<IDXGIAdapter> CD3DRender::ListGPUDevice() {
    ComPtr<IDXGIFactory1> pFactory;
        // 创建一个DXGI工厂  
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(pFactory.ReleaseAndGetAddressOf()));

    if (FAILED(hr)) {
        return nullptr;
    }

    ComPtr<IDXGIAdapter> pAdapter;
    ComPtr<IDXGIAdapter> pCurrentAdapter = nullptr;
    std::vector <ComPtr<IDXGIAdapter>> vAdapters;            // 显卡           
    int iAdapterNum = 0; // 显卡的数量 

    // 枚举适配器  
    while (pFactory->EnumAdapters(iAdapterNum, pAdapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND)
    {
        vAdapters.push_back(pAdapter);
        ++iAdapterNum;
    }

    SIZE_T iMaxDedicatedVideoMemory = 0;
    int index = 0;

    // 信息输出   
    std::cout << TEXT("get cpu nums:") << iAdapterNum <<  std::endl;
    for (size_t i = 0; i < vAdapters.size(); i++)
    {
        ComPtr<IDXGIAdapter1> pAdapter1 = nullptr;
        vAdapters[i]->QueryInterface(__uuidof(IDXGIAdapter1), reinterpret_cast<void**>(pAdapter1.ReleaseAndGetAddressOf()));

        // 获取信息  
        DXGI_ADAPTER_DESC1 adapterDesc;
        pAdapter1->GetDesc1(&adapterDesc);
        std::wstring aa(adapterDesc.Description);

        std::string deivce_name = WStringToString(aa);
        
        if (deivce_name.find("NVIDIA") != std::string::npos) {
            std::cout << "find NVIDIA adapter: " << deivce_name << std::endl;
            ComPtr<ID3D11Device> d3d11_device;
            hr = D3D11CreateDevice(pAdapter1.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION, d3d11_device.GetAddressOf(), nullptr, nullptr);
            if (FAILED(hr)) {
                return nullptr;
            }

            ComPtr<ID3D11VideoContext> d3d11_video_device;
            hr = d3d11_device->QueryInterface(__uuidof(ID3D11VideoDevice), reinterpret_cast<void**>(d3d11_video_device.ReleaseAndGetAddressOf()));
            if (FAILED(hr)) {
                return nullptr;
            }

            pCurrentAdapter = vAdapters[i];
            m_iCurAdapterIndex = i;
        }

        // 输出显卡信息
        std::cout << TEXT("device describe: ") <<  WStringToString(aa) << "  " << vAdapters[i] << std::endl;
        std::cout << TEXT("VendorId: ") << adapterDesc.VendorId << std::endl;
        std::cout << TEXT("DeviceId: ") << adapterDesc.DeviceId << std::endl;
        std::cout << TEXT("SubSysId: ") << adapterDesc.SubSysId << std::endl;
        std::cout << TEXT("Revision: ") << adapterDesc.Revision << std::endl;
        std::cout << TEXT("system video memory: ") << adapterDesc.DedicatedSystemMemory / 1024 / 1024 << "M" << std::endl;
        std::cout << TEXT("zhuanyong video memory: ") << adapterDesc.DedicatedVideoMemory / 1024 / 1024 << "M" << std::endl;
        std::cout << TEXT("shared system memory: ") << adapterDesc.SharedSystemMemory / 1024 / 1024 << "M" << std::endl;
        std::cout << TEXT("flag: ") << adapterDesc.Flags << std::endl;

        std::cout << std::endl;
    }
    vAdapters.clear();
    return pCurrentAdapter;
}