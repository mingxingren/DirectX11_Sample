#include <iostream>
#include <memory>
#include <windows.h>
#include "d3drender.h"
#include "decode/decode_thd.h"
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_syswm.h"

#define DIALOG_WIDTH 1280
#define DIALOG_HEIGHT 800

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error:" << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    int window_flag = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
    SDL_Window * window = SDL_CreateWindow("player width sdl", 
                                        SDL_WINDOWPOS_CENTERED, 
                                        SDL_WINDOWPOS_CENTERED, 
                                        DIALOG_WIDTH, 
                                        DIALOG_HEIGHT, 
                                        window_flag);    

    if (window == nullptr) {
        std::cerr << "There was an error creating the window: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(window, &info);
    HWND window_handle = info.info.win.window;

    // 创建渲染设备
    CD3DRender render;
    if (!render.Init(window_handle)) {
        return 0;
    }

    // 创建解码线程
    CDecodeThd decode_thread("C:\\Users\\MMK\\Desktop\\DDATest_3.h264", &render);
    decode_thread.StartThd();

    // 消息循环
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    quit = true;
                    break;
                case SDL_MOUSEMOTION:
                    break;
                case SDL_WINDOWEVENT: {

                }
                    break;
                default:
                    break;
            }
        }
    }

    return 0;
}