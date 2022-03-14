#ifndef YUV_FILE_H
#define YUV_FILE_H

#include <cstdint>
#include <string>
#include <filesystem>
#include <windows.h>
#include <d3d11.h>

class CYuvFile {
public:
    CYuvFile(const std::string & y_file_path, const std::string & uv_file_path, int32_t width, int32_t height);
    ~CYuvFile();
    CYuvFile(const CYuvFile&) = delete;
    CYuvFile& operator=(const CYuvFile&) = delete;
    
    const uint8_t *y_planar() const;
    const uint8_t *uv_planar() const;

    int32_t y_planar_size() const;
    int32_t uv_planar_size() const;

private:
    std::string m_y_planar = "";
    std::string m_uv_planar = "";
};

struct NV12Frame
{
	UINT width;
	UINT height;
	UINT pitch;
	BYTE *Y;
	BYTE *UV;
};

NV12Frame* ReadNV12FromFile();
void WriteNV12ToTexture(NV12Frame *nv12Frame, ID3D11DeviceContext * pDevice, ID3D11Texture2D *pTexture);


#endif // YUV_FILE_H