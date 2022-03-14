#include "yuv_file.h"
#include <fstream>
#include <windows.h>
#include <d3d11.h>
using namespace std;
using namespace std::filesystem;

constexpr int32_t PLANAR_MAX = 1024 * 1024;

CYuvFile::CYuvFile(const string & y_file_path, const string & uv_file_path, 
                int32_t width, int32_t height) {
    path y_path = path("./").append(y_file_path);
    path uv_path = path("./").append(uv_file_path);
    std::ifstream y_file_in(y_path, std::ios::binary);
    if (y_file_in.is_open()) {
        std::stringstream buffer;
        buffer << y_file_in.rdbuf();
        this->m_y_planar = buffer.str();
    }

    std::ifstream uv_file_in(uv_path, std::ios::binary);
    if (uv_file_in.is_open()) {
        std::stringstream buffer;
        buffer << uv_file_in.rdbuf();
        this->m_uv_planar = buffer.str();
    }
}

CYuvFile::~CYuvFile() {

}

const uint8_t *CYuvFile::y_planar() const {
    return (const uint8_t*)(this->m_y_planar.c_str());
}

const uint8_t *CYuvFile::uv_planar() const {
    return (const uint8_t*)(this->m_y_planar.c_str());
}

int32_t CYuvFile::y_planar_size() const {
    return this->m_y_planar.size();
}

int32_t CYuvFile::uv_planar_size() const {
    return this->m_uv_planar.size();
}


char buf[1024];
NV12Frame* ReadNV12FromFile()
{
	FILE *file = nullptr;
	sprintf_s(buf, "16.nv12");
	fopen_s(&file, buf, "rb");

	int size = sizeof(NV12Frame);
	NV12Frame *nv12Frame = (NV12Frame*)malloc(size);
	int readBytes = fread(nv12Frame, size, 1, file);

	size = nv12Frame->pitch * nv12Frame->height;
	nv12Frame->Y = (BYTE *)malloc(size);
	readBytes = fread(nv12Frame->Y, size, 1, file);

	size = nv12Frame->pitch * nv12Frame->height / 2;
	nv12Frame->UV = (BYTE *)malloc(size);
	readBytes = fread(nv12Frame->UV, size, 1, file);

	fclose(file);

	return nv12Frame;
}

void WriteNV12ToTexture(NV12Frame *nv12Frame, ID3D11DeviceContext * pDevice, ID3D11Texture2D *pTexture)
{
	// Copy from CPU access texture to bitmap buffer
	D3D11_MAPPED_SUBRESOURCE resource;
	UINT subresource = D3D11CalcSubresource(0, 0, 0);
	pDevice->Map(pTexture, subresource, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	BYTE* dptr = reinterpret_cast<BYTE*>(resource.pData);

	for (int i = 0; i < nv12Frame->height; i++)
	{
		memcpy(dptr + resource.RowPitch * i, nv12Frame->Y + nv12Frame->pitch * i, nv12Frame->pitch);
	}

	for (int i = 0; i < nv12Frame->height / 2; i++)
	{
		memcpy(dptr + resource.RowPitch *(nv12Frame->height + i), nv12Frame->UV + nv12Frame->pitch * i, nv12Frame->pitch);
	}

	pDevice->Unmap(pTexture, subresource);
}