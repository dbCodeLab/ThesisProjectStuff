#pragma once
#include <d3d11.h>

struct OpenCVImpl;

class OpenCVTest {
public:
	OpenCVTest(ID3D11Device* D3Ddevice, ID3D11DeviceContext *D3DCtx);
	~OpenCVTest();
	void RenderOnTexture();
	ID3D11ShaderResourceView* GetTexture();
private:
	OpenCVImpl *pImpl;
};