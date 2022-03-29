#include <iostream>
#include <memory>
#include <windows.h>
#include "d3drender.h"
#include "decode/decode_thd.h"

#define DIALOG_WIDTH 1600
#define DIALOG_HEIGHT 1200

LRESULT CALLBACK WindowProc(_In_ HWND hwnd, 
                            _In_ UINT uMsg, 
                            _In_ WPARAM wParam,
                            _In_ LPARAM lParam);

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, 
_In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
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
    wc.lpszClassName = (const char*)class_name.data();
    wc.hInstance = hInstance;
    // 注册窗口类
    RegisterClass(&wc);

    LPCTSTR title_name = TEXT("Win32 Test application");
    // 创建窗口
    HWND hwnd = CreateWindowA((const char*)class_name.data(), 
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
        return 0;
    }

    // 显示窗口
    ShowWindow(hwnd, SW_SHOW);

    // 更新窗口
    UpdateWindow(hwnd);

    // 创建渲染设备
    CD3DRender render;
    if (!render.Init(hwnd)) {
        return 0;
    }

    // 创建解码线程
    CDecodeThd decode_thread("DDATest_3.h264", &render);
    decode_thread.StartThd();

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(_In_ HWND hwnd, 
                            _In_ UINT uMsg, 
                            _In_ WPARAM wParam, 
                            _In_ LPARAM lParam) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
