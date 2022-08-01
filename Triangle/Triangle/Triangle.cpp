// Triangle.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <assert.h>
#include <iostream>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <Windows.h>
#include <WinUser.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define DIALOG_WIDTH 800
#define DIALOG_HEIGHT 600

LRESULT CALLBACK WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    // 类名
    std::wstring class_name = L"My Dialog";
    // 设计窗口类
    WNDCLASS wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wc.lpszMenuName = NULL;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpfnWndProc = WindowProc;    // 处理窗口消息函数
    wc.lpszClassName = class_name.data();
    wc.hInstance = hInstance;
    // 注册窗口类
    RegisterClass(&wc);

    const char* title_name = "Win32 Test application";
    // 创建窗口
    HWND hwnd = CreateWindowA("My Dialog",
        title_name,
        WS_OVERLAPPEDWINDOW,    // 窗口外观样式
        200,     // 窗口相对于父级的X坐标
        200,     // 窗口相对于父级的Y坐标
        DIALOG_WIDTH,    // 窗口宽度
        DIALOG_HEIGHT,   // 窗口高度
        NULL,   // 父窗口句柄
        NULL,   // 菜单
        hInstance,
        NULL);
    if (hwnd == NULL) {
        DWORD err = GetLastError();
        std::cout << "Create Window return err: " << err << std::endl;
        return 0;
    }

    // 显示窗口
    ShowWindow(hwnd, SW_SHOW);

    // 更新窗口
    UpdateWindow(hwnd);

    // 创建D3D11
    ID3D11Device* device_ptr = nullptr;
    ID3D11DeviceContext* device_context_ptr = nullptr;
    IDXGISwapChain* swap_chain_ptr = nullptr;
    ID3D11RenderTargetView* render_target_view_ptr = nullptr;

    DXGI_SWAP_CHAIN_DESC swap_chain_descr = { 0 };
    swap_chain_descr.BufferDesc.RefreshRate.Numerator = 0;
    swap_chain_descr.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_descr.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_descr.SampleDesc.Count = 1;
    swap_chain_descr.SampleDesc.Quality = 0;
    swap_chain_descr.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_descr.BufferCount = 1;
    swap_chain_descr.OutputWindow = hwnd;
    swap_chain_descr.Windowed = true;

    D3D_FEATURE_LEVEL feature_level;
    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined(DEBUG) || defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, 
                                                                            D3D_DRIVER_TYPE_HARDWARE, 
                                                                            NULL,
                                                                            flags,
                                                                            NULL,
                                                                            0,
                                                                            D3D11_SDK_VERSION,
                                                                            &swap_chain_descr,
                                                                            &swap_chain_ptr,
                                                                            &device_ptr,
                                                                            &feature_level,
                                                                            &device_context_ptr);
    assert(S_OK == hr && swap_chain_ptr && device_ptr && device_context_ptr);

    ID3D11Texture2D* framebuffer;
    hr = swap_chain_ptr->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&framebuffer));
    assert(SUCCEEDED(hr));

    // 用后备缓冲 创建渲染目标
    hr = device_ptr->CreateRenderTargetView(framebuffer, 0, &render_target_view_ptr);
    assert(SUCCEEDED(hr));
    framebuffer->Release();

    // 编译 shader 文件
    flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef defined( DEBUG ) || defined(_DEBUG)
    flags != D3DCOMPILE_DEBUG;
#endif
    ID3DBlob* vs_blob_ptr = nullptr;
    ID3DBlob* ps_blob_ptr = nullptr;
    ID3DBlob* error_blob = nullptr;

    hr = D3DCompileFromFile(L"shaders.hlsli", nullptr, 
                                            D3D_COMPILE_STANDARD_FILE_INCLUDE, 
                                            "vs_main", "vs_5_0",
                                            flags, 0, 
                                            &vs_blob_ptr, &error_blob);
    if (FAILED(hr)) {
        if (error_blob) {
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        if (vs_blob_ptr) { vs_blob_ptr->Release(); }
        assert(false);
    }

    hr = D3DCompileFromFile(L"shaders.hlsli", nullptr, 
                                        D3D_COMPILE_STANDARD_FILE_INCLUDE, 
                                        "ps_main", "ps_5_0", 
                                        flags, 0, 
                                        &ps_blob_ptr, &error_blob);
    if (FAILED(hr)) {
        if (error_blob) {
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        if (ps_blob_ptr) { ps_blob_ptr->Release(); }
        assert(false);
    }

    ID3D11VertexShader* vertex_shader_ptr = nullptr;
    ID3D11PixelShader* pixel_shader_ptr = nullptr;

    hr = device_ptr->CreateVertexShader(vs_blob_ptr->GetBufferPointer(),
                                                            vs_blob_ptr->GetBufferSize(),
                                                            NULL,
                                                            &vertex_shader_ptr);
    assert(SUCCEEDED(hr));
    hr = device_ptr->CreatePixelShader(ps_blob_ptr->GetBufferPointer(),
        ps_blob_ptr->GetBufferSize(),
        NULL,
        &pixel_shader_ptr);
    assert(SUCCEEDED(hr));

    ID3D11InputLayout* input_layout_ptr = NULL;
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device_ptr->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), 
        vs_blob_ptr->GetBufferPointer(), vs_blob_ptr->GetBufferSize(), &input_layout_ptr);
    assert(SUCCEEDED(hr));

    float vertex_data_array[] = {
       0.0f,  0.5f,  0.0f,
       0.5f, -0.5f,  0.0f,
      -0.5f, -0.5f,  0.0f,
    };
    UINT vertex_stride = 3 * sizeof(float);
    UINT vertex_offset = 0;
    UINT vertex_count = 3;

    ID3D11Buffer* vertex_buffer_ptr = NULL;
    {
        D3D11_BUFFER_DESC vertex_buff_descr = {};
        vertex_buff_descr.ByteWidth = sizeof(vertex_data_array);
        vertex_buff_descr.Usage = D3D11_USAGE_DEFAULT;
        vertex_buff_descr.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sr_data = { 0 };
        sr_data.pSysMem = vertex_data_array;
        hr =  device_ptr->CreateBuffer(&vertex_buff_descr, &sr_data, &vertex_buffer_ptr);
        assert(SUCCEEDED(hr));
    }

    // 设置视口大小
    RECT winRect;
    GetClientRect(hwnd, &winRect);
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)(winRect.right - winRect.left), (FLOAT)(winRect.bottom - winRect.top), 0.0f, 1.0f };
    device_context_ptr->RSSetViewports(1, &viewport);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT) {
            break;
        }

        {
            float background_colour[4] = { 0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
            device_context_ptr->ClearRenderTargetView(render_target_view_ptr, background_colour);
            device_context_ptr->OMSetRenderTargets(1, &render_target_view_ptr, nullptr);
            device_context_ptr->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            device_context_ptr->IASetInputLayout(input_layout_ptr);
            device_context_ptr->IASetVertexBuffers(0, 1, &vertex_buffer_ptr, &vertex_stride, &vertex_offset);
            device_context_ptr->VSSetShader(vertex_shader_ptr, NULL, 0);
            device_context_ptr->PSSetShader(pixel_shader_ptr, NULL, 0);

            device_context_ptr->Draw(vertex_count, 0);
            swap_chain_ptr->Present(1, 0);
        }
    }
    return 0;
}

LRESULT CALLBACK WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, 
    _In_ WPARAM wParam, _In_ LPARAM lParam) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


//int main()
//{
//	ID3D11Device* device_ptr = nullptr;
//	ID3D11DeviceContext* device_context_ptr = nullptr;
//	IDXGISwapChain* swap_chain_ptr = nullptr;
//	ID3D11RenderTargetView* render_target_view_ptr = nullptr;
//
//	DXGI_SWAP_CHAIN_DESC swap_chain_descr = { 0 };
//	swap_chain_descr.BufferDesc.RefreshRate.Numerator = 0;
//	swap_chain_descr.BufferDesc.RefreshRate.Denominator = 1;
//	swap_chain_descr.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
//	swap_chain_descr.SampleDesc.Count = 1;
//	swap_chain_descr.SampleDesc.Quality = 0;
//	swap_chain_descr.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//	swap_chain_descr.BufferCount = 1;
//	swap_chain_descr.OutputWindow = hwnd;
//	swap_chain_descr.Windowed = true;
//}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
