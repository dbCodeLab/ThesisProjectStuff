#pragma once
#include "RenderAPI.h"
#include "PlatformBase.h"

#include <string>
#include <assert.h>
#include <d3d11.h>
#include "Unity/IUnityGraphicsD3D11.h"

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace DirectX::PackedVector;

struct MatrixBufferType
{
	XMFLOAT4X4 wvp;
	//XMFLOAT4X4 w;
	//XMFLOAT4X4 v;
	//XMFLOAT4X4 p;
};

struct BasicMaterialVertex
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;

	BasicMaterialVertex() { }

	BasicMaterialVertex(XMFLOAT3 position, XMFLOAT4 color)
		: Position(position), Color(color) { }

	static size_t VertexSize()
	{
		return sizeof(BasicMaterialVertex);
	}
};

struct BasicMaterialTextureVertex
{
	XMFLOAT3 Position;
	XMFLOAT2 TextureCoordinates;

	BasicMaterialTextureVertex() { }

	BasicMaterialTextureVertex(XMFLOAT3 position, XMFLOAT2 textureCoordinates)
		: Position(position), TextureCoordinates(textureCoordinates) { }
	
	static size_t VertexSize()
	{
		return sizeof(BasicMaterialTextureVertex);
	}
};

class OpenCVTest;
class TrackedCameraOpenVRTest;

class RenderAPI_D3D11 : public RenderAPI
{
public:
	RenderAPI_D3D11();
	virtual ~RenderAPI_D3D11() { }

	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces);

	virtual bool GetUsesReverseZ() { return (int)m_Device->GetFeatureLevel() >= (int)D3D_FEATURE_LEVEL_10_0; }

	virtual void DrawSimpleTriangles(const float worldMatrix[16], const float viewMatrix[16], const float projMatrix[16], int triangleCount, const void* verticesFloat3Byte4);
	void DrawSimpleTrianglesD3D11(const XMFLOAT4X4 *world, const XMFLOAT4X4 *view, const XMFLOAT4X4 *proj, int triangleCount, const BasicMaterialVertex* vertices);

	void DrawSimpleQuadD3D11(const XMFLOAT4X4 *world, const XMFLOAT4X4 *view, const XMFLOAT4X4 *proj, const BasicMaterialVertex* vertices, const UINT *indices);
	void DrawSimpleQuadTexturedD3D11(const XMFLOAT4X4 *world, const XMFLOAT4X4 *view, const XMFLOAT4X4 *proj, const BasicMaterialTextureVertex* vertices, const UINT *indices);
	void RenderQuadTextureOpenCV(const XMFLOAT4X4 * world, const XMFLOAT4X4 * view, const XMFLOAT4X4 * proj, const BasicMaterialTextureVertex * vertices, const UINT * indices);
	void RenderQuadTextureCameraOpenVR(const XMFLOAT4X4 * world, const XMFLOAT4X4 * view, const XMFLOAT4X4 * proj, const BasicMaterialTextureVertex * vertices, const UINT * indices);


	virtual void* BeginModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int* outRowPitch);
	virtual void EndModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int rowPitch, void* dataPtr);

	virtual void* BeginModifyVertexBuffer(void* bufferHandle, size_t* outBufferSize);
	virtual void EndModifyVertexBuffer(void* bufferHandle);

private:
	void CreateResources();
	void ReleaseResources();
	void initializeShaders(const std::wstring &fileNameVS, const std::wstring &fileNamePS);
	void initializeTextureShaders(const std::wstring &fileNameVS, const std::wstring &fileNamePS);
	void loadTexture();
	void initOpenCV();
	void initOpenVR();

private:
	ID3D11Device* m_Device;
	ID3D11Buffer* m_VB; // vertex buffer
	ID3D11Buffer* m_IB; // index buffer
	ID3D11Buffer* m_CB; // constant buffer
	ID3D11VertexShader* m_VertexShader;
	ID3D11VertexShader* m_textureVertexShader;
	ID3D11PixelShader* m_PixelShader;
	ID3D11PixelShader* m_texturePixelShader;
	ID3D11InputLayout* m_InputLayout;
	ID3D11InputLayout* m_textureInputLayout;
	ID3D11RasterizerState* m_RasterState;
	ID3D11BlendState* m_BlendState;
	ID3D11DepthStencilState* m_DepthState;
	ID3D11SamplerState* mSamplerState;
	//ID3D11Texture2D* mTexture;
	ID3D11ShaderResourceView* mTextureView;

	OpenCVTest *mOpenCVTest;
	TrackedCameraOpenVRTest *mCameraOpenVR;

};