#include "Utility.h"
#include "TextureClass.h"
#include "RenderAPI_D3D11.h"

#include "OpenCVTest.h"
#include "TrackedCameraOpenVRTest.h"

// Direct3D 11 implementation of RenderAPI.


RenderAPI* CreateRenderAPI_D3D11()
{
	return new RenderAPI_D3D11();
}


RenderAPI_D3D11::RenderAPI_D3D11()
	: m_Device(NULL)
	, m_VB(NULL)
	, m_IB(NULL)
	, m_CB(NULL)
	, m_VertexShader(NULL)
	, m_PixelShader(NULL)
	, m_InputLayout(NULL)
	, m_textureVertexShader(NULL)
	, m_texturePixelShader(NULL)
	, m_textureInputLayout(NULL)
	, m_RasterState(NULL)
	, m_BlendState(NULL)
	, m_DepthState(NULL)
	, mSamplerState(NULL)
	, mTextureView(NULL)
	, mOpenCVTest(NULL)
	, mCameraOpenVR(NULL)
{
}


void RenderAPI_D3D11::ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
{
	switch (type)
	{
	case kUnityGfxDeviceEventInitialize:
	{
		IUnityGraphicsD3D11* d3d = interfaces->Get<IUnityGraphicsD3D11>();
		m_Device = d3d->GetDevice();
		CreateResources();
		//initOpenCV();
		//initOpenVR();
		break;
	}
	case kUnityGfxDeviceEventShutdown:
		ReleaseResources();
		break;
	}
}


void RenderAPI_D3D11::DrawSimpleTriangles(const float worldMatrix[16], const float view[16], const float proj[16], int triangleCount, const void* verticesFloat3Byte4)
{
	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);

	// Set basic render state
	ctx->OMSetDepthStencilState(m_DepthState, 0);
	ctx->RSSetState(m_RasterState);
	ctx->OMSetBlendState(m_BlendState, NULL, 0xFFFFFFFF);

	// Update constant buffer - just the world matrix in our case
	ctx->UpdateSubresource(m_CB, 0, NULL, worldMatrix, 64, 0);

	// Set shaders
	ctx->VSSetConstantBuffers(0, 1, &m_CB);
	ctx->VSSetShader(m_VertexShader, NULL, 0);
	ctx->PSSetShader(m_PixelShader, NULL, 0);

	// Update vertex buffer
	const int kVertexSize = 3 * 4 + 4 * 4;//12 + 4;
	ctx->UpdateSubresource(m_VB, 0, NULL, verticesFloat3Byte4, triangleCount * 3 * kVertexSize, 0);

	// set input assembler data and draw
	ctx->IASetInputLayout(m_InputLayout);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT stride = kVertexSize;
	UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);
	ctx->Draw(triangleCount * 3, 0);

	ctx->Release();
}

static const XMFLOAT4X4 Identity = XMFLOAT4X4(1.0f, 0.0f, 0.0f, 0.0f,
												0.0f, 1.0f, 0.0f, 0.0f,
												0.0f, 0.0f, 1.0f, 0.0f,
												0.0f, 0.0f, 0.0f, 1.0f);

void RenderAPI_D3D11::DrawSimpleTrianglesD3D11(const XMFLOAT4X4 *world, const XMFLOAT4X4 *view, const XMFLOAT4X4 *proj, int triangleCount, const BasicMaterialVertex* vertices)
{
	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);

	// Set basic render state
	ctx->OMSetDepthStencilState(m_DepthState, 0);
	ctx->RSSetState(m_RasterState);
	ctx->OMSetBlendState(m_BlendState, NULL, 0xFFFFFFFF);

	XMMATRIX w = XMMatrixTranspose(XMLoadFloat4x4(world)); // &Identity
	XMMATRIX v = (XMLoadFloat4x4(view)); // 
	XMMATRIX p = (XMLoadFloat4x4(proj)); // 
	//XMMATRIX wvp = w;
	
	XMMATRIX wvp = XMMatrixMultiply(w, XMMatrixMultiply(v, p)); //XMMatrixMultiply(p, XMMatrixMultiply(v, w)));//XMMatrixTranspose(XMMatrixMultiply(w, XMMatrixMultiply(v, p))); // XMMatrixTranspose
	MatrixBufferType cbmat;
	//cbmat.wvp
	XMStoreFloat4x4(&cbmat.wvp, wvp);


	////cbmat.w = *world;
	//XMStoreFloat4x4(&cbmat.w, w);
	//cbmat.v = *view;
	//cbmat.p = *proj;

	// Update constant buffer
	ctx->UpdateSubresource(m_CB, 0, NULL, &cbmat.wvp, sizeof(MatrixBufferType), 0);

	// Set shaders
	ctx->VSSetConstantBuffers(0, 1, &m_CB);
	ctx->VSSetShader(m_VertexShader, NULL, 0);
	ctx->PSSetShader(m_PixelShader, NULL, 0);

	// Update vertex buffer
	const int kVertexSize = BasicMaterialVertex::VertexSize();//12 + 4;
	ctx->UpdateSubresource(m_VB, 0, NULL, vertices, triangleCount * 3 * kVertexSize, 0);

	// set input assembler data and draw
	ctx->IASetInputLayout(m_InputLayout);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT stride = kVertexSize;
	UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);
	ctx->Draw(triangleCount * 3, 0);

	ctx->Release();
}

void RenderAPI_D3D11::DrawSimpleQuadD3D11(const XMFLOAT4X4 * world, const XMFLOAT4X4 * view, const XMFLOAT4X4 * proj, const BasicMaterialVertex * vertices, const UINT *indices)
{
	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);

	// Set basic render state
	ctx->OMSetDepthStencilState(m_DepthState, 0);
	ctx->RSSetState(m_RasterState);
	ctx->OMSetBlendState(m_BlendState, NULL, 0xFFFFFFFF);

	XMMATRIX w = XMMatrixTranspose(XMLoadFloat4x4(world)); // &Identity
	XMMATRIX v = (XMLoadFloat4x4(view)); // 
	XMMATRIX p = (XMLoadFloat4x4(proj)); // 
										 //XMMATRIX wvp = w;

	XMMATRIX wvp = XMMatrixMultiply(w, XMMatrixMultiply(v, p)); //XMMatrixMultiply(p, XMMatrixMultiply(v, w)));//XMMatrixTranspose(XMMatrixMultiply(w, XMMatrixMultiply(v, p))); // XMMatrixTranspose
	MatrixBufferType cbmat;

	XMStoreFloat4x4(&cbmat.wvp, wvp);

	// Update constant buffer
	ctx->UpdateSubresource(m_CB, 0, NULL, &cbmat.wvp, sizeof(MatrixBufferType), 0);

	// Set shaders
	ctx->VSSetConstantBuffers(0, 1, &m_CB);
	ctx->VSSetShader(m_VertexShader, NULL, 0);
	ctx->PSSetShader(m_PixelShader, NULL, 0);

	// Update vertex buffer
	const int kVertexSize = BasicMaterialVertex::VertexSize();
	ctx->UpdateSubresource(m_VB, 0, NULL, vertices, 4 * kVertexSize, 0);

	// Update index buffer
	const int kIndexSize = sizeof(UINT);
	ctx->UpdateSubresource(m_IB, 0, NULL, indices, 6 * kIndexSize, 0);

	// set input assembler data and draw
	ctx->IASetInputLayout(m_InputLayout);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT stride = kVertexSize;
	UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);
	ctx->IASetIndexBuffer(m_IB, DXGI_FORMAT_R32_UINT, 0);
	
	ctx->DrawIndexed(6, 0, 0);

	ctx->Release();
}

void RenderAPI_D3D11::DrawSimpleQuadTexturedD3D11(const XMFLOAT4X4 * world, const XMFLOAT4X4 * view, const XMFLOAT4X4 * proj, const BasicMaterialTextureVertex * vertices, const UINT * indices)
{
	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);

	// Set basic render state
	ctx->OMSetDepthStencilState(m_DepthState, 0);
	ctx->RSSetState(m_RasterState);
	ctx->OMSetBlendState(m_BlendState, NULL, 0xFFFFFFFF);

	XMMATRIX w = XMMatrixTranspose(XMLoadFloat4x4(world)); // &Identity
	XMMATRIX v = (XMLoadFloat4x4(view)); // 
	XMMATRIX p = (XMLoadFloat4x4(proj)); // 
										 //XMMATRIX wvp = w;

	XMMATRIX wvp = XMMatrixMultiply(w, XMMatrixMultiply(v, p)); //XMMatrixMultiply(p, XMMatrixMultiply(v, w)));//XMMatrixTranspose(XMMatrixMultiply(w, XMMatrixMultiply(v, p))); // XMMatrixTranspose
	MatrixBufferType cbmat;

	XMStoreFloat4x4(&cbmat.wvp, wvp);

	// Update constant buffer
	ctx->UpdateSubresource(m_CB, 0, NULL, &cbmat.wvp, sizeof(MatrixBufferType), 0);

	// Set shaders
	ctx->VSSetConstantBuffers(0, 1, &m_CB);
	ctx->VSSetShader(m_textureVertexShader, NULL, 0);
	ctx->PSSetShader(m_texturePixelShader, NULL, 0);
	ctx->PSSetSamplers(0, 1, &mSamplerState);
	ctx->PSSetShaderResources(0, 1, &mTextureView);

	// Update vertex buffer
	const int kVertexSize = BasicMaterialTextureVertex::VertexSize();
	ctx->UpdateSubresource(m_VB, 0, NULL, vertices, 4 * kVertexSize, 0);

	// Update index buffer
	const int kIndexSize = sizeof(UINT);
	ctx->UpdateSubresource(m_IB, 0, NULL, indices, 6 * kIndexSize, 0);

	// set input assembler data and draw
	ctx->IASetInputLayout(m_textureInputLayout);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT stride = kVertexSize;
	UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, &m_VB, &stride, &offset);
	ctx->IASetIndexBuffer(m_IB, DXGI_FORMAT_R32_UINT, 0);

	ctx->DrawIndexed(6, 0, 0);

	ctx->Release();
}

void RenderAPI_D3D11::RenderQuadTextureOpenCV(const XMFLOAT4X4 * world, const XMFLOAT4X4 * view, const XMFLOAT4X4 * proj, const BasicMaterialTextureVertex * vertices, const UINT * indices)
{
	if (mOpenCVTest) {
		mOpenCVTest->RenderOnTexture();
		mTextureView = mOpenCVTest->GetTexture();
		DrawSimpleQuadTexturedD3D11(world, view, proj, vertices, indices);
	}	
}

void RenderAPI_D3D11::RenderQuadTextureCameraOpenVR(const XMFLOAT4X4 * world, const XMFLOAT4X4 * view, const XMFLOAT4X4 * proj, const BasicMaterialTextureVertex * vertices, const UINT * indices)
{
	if (mCameraOpenVR) {
		mCameraOpenVR->renderOnTexture();
		mTextureView = mCameraOpenVR->GetTexture();
		DrawSimpleQuadTexturedD3D11(world, view, proj, vertices, indices);
	}
}


void RenderAPI_D3D11::loadTexture()
{
	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);

	/*
	TextureClass texture;
	std::string textureName = "C:\\Users\\Public\\Documents\\Unity Projects\\UnityExternalDLL\\NativeRenderingPlugin\\PluginSource\\source\\EarthCompositeTarga32.tga";
	texture.Initialize(m_Device, ctx, textureName.c_str());
	mTextureView = texture.GetTexture();
	*/

	D3D11_SAMPLER_DESC samplerDesc;

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; //D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; //D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = m_Device->CreateSamplerState(&samplerDesc, &mSamplerState);
	if (FAILED(hr))
	{
		throw 2;
	}


	ctx->Release();

}

void RenderAPI_D3D11::initOpenCV()
{
	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);
	mOpenCVTest = new OpenCVTest(m_Device, ctx);
}

void RenderAPI_D3D11::initOpenVR()
{
	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);
	mCameraOpenVR = new TrackedCameraOpenVRTest(m_Device, ctx);
}



void RenderAPI_D3D11::initializeShaders(const std::wstring &fileNameVS, const std::wstring &fileNamePS)
{
	//SetCurrentDirectory(Utility::ExecutableDirectory().c_str());

	std::vector<char> compiledVS;
	Utility::LoadBinaryFile(fileNameVS, compiledVS);

	HRESULT hr;
	hr = m_Device->CreateVertexShader(&compiledVS[0], compiledVS.size(), nullptr, &m_VertexShader);
	if (FAILED(hr)) {
		OutputDebugStringA("Failed to create vertex shader.\n");
		throw 1;
	}

	std::vector<char> compiledPS;
	Utility::LoadBinaryFile(fileNamePS, compiledPS);

	hr = m_Device->CreatePixelShader(&compiledPS[0], compiledPS.size(), nullptr, &m_PixelShader);
	if (FAILED(hr)) {
		OutputDebugStringA("Failed to create pixel shader.\n");
		throw 1;
	}

	D3D11_INPUT_ELEMENT_DESC s_DX11InputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, //D3D11_APPEND_ALIGNED_ELEMENT
	};
	
	hr = m_Device->CreateInputLayout(s_DX11InputElementDesc, 2, &compiledVS[0], compiledVS.size(), &m_InputLayout);
	if (FAILED(hr))
	{
		throw 1; //D3DAppException("ID3D11Device::CreateInputLayout() failed.", hr);
	}

}


void RenderAPI_D3D11::initializeTextureShaders(const std::wstring &fileNameVS, const std::wstring &fileNamePS)
{
	//SetCurrentDirectory(Utility::ExecutableDirectory().c_str());

	std::vector<char> compiledVS;
	Utility::LoadBinaryFile(fileNameVS, compiledVS);

	HRESULT hr;
	hr = m_Device->CreateVertexShader(&compiledVS[0], compiledVS.size(), nullptr, &m_textureVertexShader);
	if (FAILED(hr)) {
		OutputDebugStringA("Failed to create vertex shader.\n");
		throw 1;
	}

	std::vector<char> compiledPS;
	Utility::LoadBinaryFile(fileNamePS, compiledPS);

	hr = m_Device->CreatePixelShader(&compiledPS[0], compiledPS.size(), nullptr, &m_texturePixelShader);
	if (FAILED(hr)) {
		OutputDebugStringA("Failed to create pixel shader.\n");
		throw 1;
	}

	D3D11_INPUT_ELEMENT_DESC s_DX11InputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, //D3D11_APPEND_ALIGNED_ELEMENT
	};

	hr = m_Device->CreateInputLayout(s_DX11InputElementDesc, 2, &compiledVS[0], compiledVS.size(), &m_textureInputLayout);
	if (FAILED(hr))
	{
		throw 1; //D3DAppException("ID3D11Device::CreateInputLayout() failed.", hr);
	}

}



void RenderAPI_D3D11::CreateResources()
{
	D3D11_BUFFER_DESC desc;
	memset(&desc, 0, sizeof(desc));

	// vertex buffer
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = 1024;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	m_Device->CreateBuffer(&desc, NULL, &m_VB);

	// index buffer
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = 1024;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	m_Device->CreateBuffer(&desc, NULL, &m_IB);

	// constant buffer
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = sizeof(MatrixBufferType); // hold 1 matrix
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = 0;
	m_Device->CreateBuffer(&desc, NULL, &m_CB);


#ifdef _DEBUG
	initializeShaders(L"C:\\Users\\Public\\Documents\\Unity Projects\\UnityExternalDLL\\NativeRenderingPlugin\\PluginSource\\build\\x64\\Debug\\VertexShader.cso",
					L"C:\\Users\\Public\\Documents\\Unity Projects\\UnityExternalDLL\\NativeRenderingPlugin\\PluginSource\\build\\x64\\Debug\\PixelShader.cso");
	initializeTextureShaders(L"C:\\Users\\Public\\Documents\\Unity Projects\\UnityExternalDLL\\NativeRenderingPlugin\\PluginSource\\build\\x64\\Debug\\textureVS.cso",
		L"C:\\Users\\Public\\Documents\\Unity Projects\\UnityExternalDLL\\NativeRenderingPlugin\\PluginSource\\build\\x64\\Debug\\texturePS.cso");
#else
	initializeShaders(L"C:\\Users\\Public\\Documents\\Unity Projects\\UnityExternalDLL\\NativeRenderingPlugin\\PluginSource\\build\\x64\\Release\\VertexShader.cso",
		L"C:\\Users\\Public\\Documents\\Unity Projects\\UnityExternalDLL\\NativeRenderingPlugin\\PluginSource\\build\\x64\\Release\\PixelShader.cso");
	initializeTextureShaders(L"C:\\Users\\Public\\Documents\\Unity Projects\\UnityExternalDLL\\NativeRenderingPlugin\\PluginSource\\build\\x64\\Release\\textureVS.cso",
		L"C:\\Users\\Public\\Documents\\Unity Projects\\UnityExternalDLL\\NativeRenderingPlugin\\PluginSource\\build\\x64\\Release\\texturePS.cso");
#endif
	loadTexture();

	// render states
	D3D11_RASTERIZER_DESC rsdesc;
	memset(&rsdesc, 0, sizeof(rsdesc));
	rsdesc.FillMode = D3D11_FILL_SOLID;
	rsdesc.CullMode = D3D11_CULL_NONE;
	rsdesc.DepthClipEnable = TRUE;
	m_Device->CreateRasterizerState(&rsdesc, &m_RasterState);

	D3D11_DEPTH_STENCIL_DESC dsdesc;
	memset(&dsdesc, 0, sizeof(dsdesc));
	dsdesc.DepthEnable = TRUE;
	dsdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;//D3D11_DEPTH_WRITE_MASK_ALL;
	dsdesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL; //GetUsesReverseZ() ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
	m_Device->CreateDepthStencilState(&dsdesc, &m_DepthState);

	D3D11_BLEND_DESC bdesc;
	memset(&bdesc, 0, sizeof(bdesc));
	bdesc.RenderTarget[0].BlendEnable = FALSE;
	bdesc.RenderTarget[0].RenderTargetWriteMask = 0xF;
	m_Device->CreateBlendState(&bdesc, &m_BlendState);
}


void RenderAPI_D3D11::ReleaseResources()
{
	SAFE_RELEASE(m_VB);
	SAFE_RELEASE(m_IB);
	SAFE_RELEASE(m_CB);
	SAFE_RELEASE(m_VertexShader);
	SAFE_RELEASE(m_PixelShader);
	SAFE_RELEASE(m_InputLayout);
	SAFE_RELEASE(m_textureVertexShader);
	SAFE_RELEASE(m_texturePixelShader);
	SAFE_RELEASE(m_textureInputLayout);
	SAFE_RELEASE(mSamplerState);
	SAFE_RELEASE(mTextureView);
	SAFE_RELEASE(m_RasterState);
	SAFE_RELEASE(m_BlendState);
	SAFE_RELEASE(m_DepthState);

	delete mOpenCVTest;
	mOpenCVTest = nullptr;

	delete mCameraOpenVR;
	mCameraOpenVR = nullptr;
}




void* RenderAPI_D3D11::BeginModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int* outRowPitch)
{
	const int rowPitch = textureWidth * 4;
	// Just allocate a system memory buffer here for simplicity
	unsigned char* data = new unsigned char[rowPitch * textureHeight];
	*outRowPitch = rowPitch;
	return data;
}


void RenderAPI_D3D11::EndModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int rowPitch, void* dataPtr)
{
	ID3D11Texture2D* d3dtex = (ID3D11Texture2D*)textureHandle;
	assert(d3dtex);

	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);
	// Update texture data, and free the memory buffer
	ctx->UpdateSubresource(d3dtex, 0, NULL, dataPtr, rowPitch, 0);
	delete[](unsigned char*)dataPtr;
	ctx->Release();
}


void* RenderAPI_D3D11::BeginModifyVertexBuffer(void* bufferHandle, size_t* outBufferSize)
{
	ID3D11Buffer* d3dbuf = (ID3D11Buffer*)bufferHandle;
	assert(d3dbuf);
	D3D11_BUFFER_DESC desc;
	d3dbuf->GetDesc(&desc);
	*outBufferSize = desc.ByteWidth;

	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);
	D3D11_MAPPED_SUBRESOURCE mapped;
	ctx->Map(d3dbuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	ctx->Release();

	return mapped.pData;
}


void RenderAPI_D3D11::EndModifyVertexBuffer(void* bufferHandle)
{
	ID3D11Buffer* d3dbuf = (ID3D11Buffer*)bufferHandle;
	assert(d3dbuf);

	ID3D11DeviceContext* ctx = NULL;
	m_Device->GetImmediateContext(&ctx);
	ctx->Unmap(d3dbuf, 0);
	ctx->Release();
}

