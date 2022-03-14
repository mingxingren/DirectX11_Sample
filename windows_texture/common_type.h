#ifndef COMMON_TYPE_H
#define COMMON_TYPE_H

#include <DirectXMath.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#define NUMVERTICES 6

typedef struct _VERTEX{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT2 TexCoord;
}VERTEX;

#endif  // COMMON_TYPE_H