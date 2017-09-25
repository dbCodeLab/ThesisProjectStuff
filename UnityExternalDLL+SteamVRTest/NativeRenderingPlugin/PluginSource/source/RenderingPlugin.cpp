// Example low level rendering Unity plugin

#include "PlatformBase.h"
#include "RenderAPI.h"
#include "RenderAPI_D3D11.h"

#include <assert.h>
#include <math.h>
#include <vector>


using debug_log_func_type = void(*)(const char*);
debug_log_func_type debug_log_func = nullptr;


void debug_log(const char* msg)
{
	if (debug_log_func != nullptr) debug_log_func(msg);
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API Set_debug_log_func(debug_log_func_type func)
{
	debug_log_func = func;
}




static float nearPlane, farPlane, aspectRatio, vFov;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetProjectionParameters(
	float FovAngleY,
	float AspectHByW,
	float NearZ,
	float FarZ) {
	vFov = FovAngleY;
	aspectRatio = AspectHByW;
	nearPlane = NearZ;
	farPlane = FarZ;
}


static void *g_projMatrix;
static void *g_viewMatrix;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetProjectionMatrixFromUnity(void *projMatrix) {
	g_projMatrix = projMatrix;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetViewMatrixFromUnity(void *viewMatrix) {
	g_viewMatrix = viewMatrix;
}


// --------------------------------------------------------------------------
// SetCharArrayFromUnity, an example function we export which is called by one of the scripts.

#define BUFFER_LENGTH 1000
char buffer[BUFFER_LENGTH] = { 0 };
char *textBuffer = &buffer[0];

/*
void *g_charArray;
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetCharArrayFromUnity(void *charArray) { 
	g_charArray = charArray; 
}
*/


// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the scripts.

static float g_Time;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTimeFromUnity(float t) { g_Time = t; }



// --------------------------------------------------------------------------
// SetTextureFromUnity, an example function we export which is called by one of the scripts.

static void* g_TextureHandle = NULL;
static int   g_TextureWidth = 0;
static int   g_TextureHeight = 0;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTextureFromUnity(void* textureHandle, int w, int h)
{
	// A script calls this at initialization time; just remember the texture pointer here.
	// Will update texture pixels each frame from the plugin rendering event (texture update
	// needs to happen on the rendering thread).
	g_TextureHandle = textureHandle;
	g_TextureWidth = w;
	g_TextureHeight = h;
}


// --------------------------------------------------------------------------
// SetMeshBuffersFromUnity, an example function we export which is called by one of the scripts.

static void* g_VertexBufferHandle = NULL;
static int g_VertexBufferVertexCount;

struct MeshVertex
{
	float pos[3];
	float normal[3];
	float uv[2];
};
static std::vector<MeshVertex> g_VertexSource;


extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetMeshBuffersFromUnity(void* vertexBufferHandle, int vertexCount, float* sourceVertices, float* sourceNormals, float* sourceUV)
{
	// A script calls this at initialization time; just remember the pointer here.
	// Will update buffer data each frame from the plugin rendering event (buffer update
	// needs to happen on the rendering thread).
	g_VertexBufferHandle = vertexBufferHandle;
	g_VertexBufferVertexCount = vertexCount;

	// The script also passes original source mesh data. The reason is that the vertex buffer we'll be modifying
	// will be marked as "dynamic", and on many platforms this means we can only write into it, but not read its previous
	// contents. In this example we're not creating meshes from scratch, but are just altering original mesh data --
	// so remember it. The script just passes pointers to regular C# array contents.
	g_VertexSource.resize(vertexCount);
	for (int i = 0; i < vertexCount; ++i)
	{
		MeshVertex& v = g_VertexSource[i];
		v.pos[0] = sourceVertices[0];
		v.pos[1] = sourceVertices[1];
		v.pos[2] = sourceVertices[2];
		v.normal[0] = sourceNormals[0];
		v.normal[1] = sourceNormals[1];
		v.normal[2] = sourceNormals[2];
		v.uv[0] = sourceUV[0];
		v.uv[1] = sourceUV[1];
		sourceVertices += 3;
		sourceNormals += 3;
		sourceUV += 2;
	}
}



// --------------------------------------------------------------------------
// UnitySetInterfaces

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	s_UnityInterfaces = unityInterfaces;
	s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
	s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

	// Run OnGraphicsDeviceEvent(initialize) manually on plugin load
	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

#if UNITY_WEBGL
typedef void	(UNITY_INTERFACE_API * PluginLoadFunc)(IUnityInterfaces* unityInterfaces);
typedef void	(UNITY_INTERFACE_API * PluginUnloadFunc)();

extern "C" void	UnityRegisterRenderingPlugin(PluginLoadFunc loadPlugin, PluginUnloadFunc unloadPlugin);

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API RegisterPlugin()
{
	UnityRegisterRenderingPlugin(UnityPluginLoad, UnityPluginUnload);
}
#endif

// --------------------------------------------------------------------------
// GraphicsDeviceEvent


static RenderAPI* s_CurrentAPI = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;

static ID3D11ShaderResourceView* mTextureView = nullptr;

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	// Create graphics API implementation upon initialization
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		assert(s_CurrentAPI == NULL);
		s_DeviceType = s_Graphics->GetRenderer();
		s_CurrentAPI = CreateRenderAPI(s_DeviceType);
	}

	// Let the implementation process the device related events
	if (s_CurrentAPI)
	{
		s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);

	}

	// Cleanup graphics API implementation upon shutdown
	if (eventType == kUnityGfxDeviceEventShutdown)
	{
		delete s_CurrentAPI;
		s_CurrentAPI = NULL;
		s_DeviceType = kUnityGfxRendererNull;
	}
}



// --------------------------------------------------------------------------
// OnRenderEvent
// This will be called for GL.IssuePluginEvent script calls; eventID will
// be the integer passed to IssuePluginEvent. In this example, we just ignore
// that value.

/*
void printMatrix(const float *matrix) {
	//extern void* g_charArray;
	char *str = (char*)g_charArray;
	XMFLOAT4X4 *matrixF4x4 = (XMFLOAT4X4*)matrix;
	//matrix->m[i][j]
	int offset = 0;
	for (int i = 0; i != 4; ++i) {
		for (int j = 0; j != 4; ++j) {
			sprintf(str + offset, "%3.3f\t", matrixF4x4->m[i][j]); //matrix[i * 4 + j]);
			offset = strlen(str);
		}
	}
}
*/



static void DrawTexturedQuad()
{
	// Transformation matrix: rotate around Z axis based on time.
	float phi = g_Time; // time set externally from Unity script
	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);

	RenderAPI_D3D11 *d3dRenderApi = dynamic_cast<RenderAPI_D3D11*>(s_CurrentAPI);
	if (d3dRenderApi) {

		float scale = 8.0f;

		float worldMatrix[16] = {
			cosPhi,0,sinPhi,-20,
			0,1,0,0,
			-sinPhi,0,cosPhi,0,
			0,0,0,1,
		};

		XMMATRIX world = XMMatrixMultiply(XMLoadFloat4x4((XMFLOAT4X4 *)worldMatrix),
			XMMatrixScaling(scale, scale, scale));

		XMStoreFloat4x4((XMFLOAT4X4 *)worldMatrix, world);

		BasicMaterialTextureVertex vertsD3D11[4] = {
			BasicMaterialTextureVertex(XMFLOAT3(-1.0f, -1.0f,  0.0f), XMFLOAT2(0.0f, 1.0f)),
			BasicMaterialTextureVertex(XMFLOAT3(-1.0f,  1.0f ,  0.0f), XMFLOAT2(0.0f, 0.0f)),
			BasicMaterialTextureVertex(XMFLOAT3(1.0f, -1.0f,  0.0f), XMFLOAT2(1.0f, 1.0f)),
			BasicMaterialTextureVertex(XMFLOAT3(1.0f, 1.0f,  0.0f), XMFLOAT2(1.0f, 0.0f))
		};

		UINT indices[6] = {
			0, 1, 2,
			1, 3, 2
		};

		d3dRenderApi->DrawSimpleQuadTexturedD3D11((XMFLOAT4X4 *)worldMatrix, (XMFLOAT4X4 *)(g_viewMatrix), (XMFLOAT4X4 *)(g_projMatrix), vertsD3D11, indices);
	}
}


static void DrawOpenCVQuad()
{
	// Transformation matrix: rotate around Z axis based on time.


	RenderAPI_D3D11 *d3dRenderApi = dynamic_cast<RenderAPI_D3D11*>(s_CurrentAPI);
	if (d3dRenderApi) {

		/*
		float phi = g_Time; // time set externally from Unity script
		float cosPhi = cosf(phi);
		float sinPhi = sinf(phi);
		float worldMatrix[16] = {
			cosPhi,0,sinPhi,-17,
			0,1,0,0,
			-sinPhi,0,cosPhi,0,
			0,0,0,1,
		};
		*/

		float worldMatrix[16] = {
			0,0,-1,-17,
			0,1,0,0,
			1,0,0,0,
			0,0,0,1,
		};

		float scale = 9.0f;

		XMMATRIX world = XMMatrixMultiply(XMLoadFloat4x4((XMFLOAT4X4 *)worldMatrix),
			XMMatrixScaling(scale, scale, scale));

		XMStoreFloat4x4((XMFLOAT4X4 *)worldMatrix, world);

		BasicMaterialTextureVertex vertsD3D11[4] = {
			BasicMaterialTextureVertex(XMFLOAT3(-1.0f, -1.0f,  0.0f), XMFLOAT2(0.0f, 1.0f)),
			BasicMaterialTextureVertex(XMFLOAT3(-1.0f,  1.0f ,  0.0f), XMFLOAT2(0.0f, 0.0f)),
			BasicMaterialTextureVertex(XMFLOAT3(1.0f, -1.0f,  0.0f), XMFLOAT2(1.0f, 1.0f)),
			BasicMaterialTextureVertex(XMFLOAT3(1.0f, 1.0f,  0.0f), XMFLOAT2(1.0f, 0.0f))
		};

		UINT indices[6] = {
			0, 1, 2,
			1, 3, 2
		};

		d3dRenderApi->RenderQuadTextureOpenCV((XMFLOAT4X4 *)worldMatrix, (XMFLOAT4X4 *)(g_viewMatrix), (XMFLOAT4X4 *)(g_projMatrix), vertsD3D11, indices);
	}
}

static void DrawOpenVRCameraQuad()
{
	// Transformation matrix: rotate around Z axis based on time.


	RenderAPI_D3D11 *d3dRenderApi = dynamic_cast<RenderAPI_D3D11*>(s_CurrentAPI);
	if (d3dRenderApi) {

		/*
		float phi = g_Time; // time set externally from Unity script
		float cosPhi = cosf(phi);
		float sinPhi = sinf(phi);
		float worldMatrix[16] = {
		cosPhi,0,sinPhi,-17,
		0,1,0,0,
		-sinPhi,0,cosPhi,0,
		0,0,0,1,
		};
		*/

		float worldMatrix[16] = {
			0,0,-1,-17,
			0,1,0,0,
			1,0,0,0,
			0,0,0,1,
		};

		float scale = 9.0f;

		XMMATRIX world = XMMatrixMultiply(XMLoadFloat4x4((XMFLOAT4X4 *)worldMatrix),
			XMMatrixScaling(scale, scale, scale));

		XMStoreFloat4x4((XMFLOAT4X4 *)worldMatrix, world);

		BasicMaterialTextureVertex vertsD3D11[4] = {
			BasicMaterialTextureVertex(XMFLOAT3(-1.0f, -1.0f,  0.0f), XMFLOAT2(0.0f, 1.0f)),
			BasicMaterialTextureVertex(XMFLOAT3(-1.0f,  1.0f ,  0.0f), XMFLOAT2(0.0f, 0.0f)),
			BasicMaterialTextureVertex(XMFLOAT3(1.0f, -1.0f,  0.0f), XMFLOAT2(1.0f, 1.0f)),
			BasicMaterialTextureVertex(XMFLOAT3(1.0f, 1.0f,  0.0f), XMFLOAT2(1.0f, 0.0f))
		};

		UINT indices[6] = {
			0, 1, 2,
			1, 3, 2
		};

		d3dRenderApi->RenderQuadTextureCameraOpenVR((XMFLOAT4X4 *)worldMatrix, (XMFLOAT4X4 *)(g_viewMatrix), (XMFLOAT4X4 *)(g_projMatrix), vertsD3D11, indices);
	}
}

static void DrawColoredQuad()
{

	// Transformation matrix: rotate around Z axis based on time.
	float phi = g_Time; // time set externally from Unity script
	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);
	
	RenderAPI_D3D11 *d3dRenderApi = dynamic_cast<RenderAPI_D3D11*>(s_CurrentAPI);
	if (d3dRenderApi) {

		float worldMatrix[16] = {
			cosPhi,0,sinPhi,-20,
			0,1,0,0,
			-sinPhi,0,cosPhi,0,
			0,0,0,1,
		};

		BasicMaterialVertex vertsD3D11[4] = {
			BasicMaterialVertex(XMFLOAT3(-4.0f, -4.0f,  0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
			BasicMaterialVertex(XMFLOAT3(-4.0f,  4.0f ,  0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)),
			BasicMaterialVertex(XMFLOAT3(4.0f, -4.0f,  0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)),
			BasicMaterialVertex(XMFLOAT3(4.0f, 4.0f,  0.0f), XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f))
		};

		UINT indices[6] = {
			0, 1, 2,
			1, 3, 2
		};

		d3dRenderApi->DrawSimpleQuadD3D11((XMFLOAT4X4 *)worldMatrix, (XMFLOAT4X4 *)(g_viewMatrix), (XMFLOAT4X4 *)(g_projMatrix), vertsD3D11, indices);
	}

}

static void DrawColoredTriangle()
{
	// Draw a colored triangle. Note that colors will come out differently
	// in D3D and OpenGL, for example, since they expect color bytes
	// in different ordering.

	// Transformation matrix: rotate around Z axis based on time.
	float phi = g_Time; // time set externally from Unity script
	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);
	//float depth = 0.7f;
	//float finalDepth = s_CurrentAPI->GetUsesReverseZ() ? 1.0f - depth : depth;
	

	RenderAPI_D3D11 *d3dRenderApi = dynamic_cast<RenderAPI_D3D11*>(s_CurrentAPI);
	if (d3dRenderApi) {
		/*
		finalDepth = 0.0f;
		float worldMatrix[16] = {
			cosPhi,-sinPhi,0,0,
			sinPhi,cosPhi,0,0,
			0,0,1,0,
			0,0,0,1,
		};*/
		/*
		float worldMatrix[16] = {
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			0,0,0,1,
		};
		*/
		///*
		float worldMatrix[16] = {
			cosPhi,0,sinPhi,0,
			0,1,0,0,
			-sinPhi,0,cosPhi,10,
			0,0,0,1,
		};
		//*/

		BasicMaterialVertex vertsD3D11[3] = {
			//BasicMaterialVertex(XMFLOAT3(-0.5f, -0.5f,  0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
			//BasicMaterialVertex(XMFLOAT3(0,     0.5f ,  0), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)),
			//BasicMaterialVertex(XMFLOAT3(0.5f, -0.5f,  0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)),
			BasicMaterialVertex(XMFLOAT3(-4.0f, -4.0f,  0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
			BasicMaterialVertex(XMFLOAT3(0,     4.0f ,  0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)),
			BasicMaterialVertex(XMFLOAT3(4.0f, -4.0f,  0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)),
		};


/*
		XMFLOAT4X4 pproj = XMFLOAT4X4(1.0f, 0.0f, 0.0f, 0.0f,
										0.0f, 1.0f, 0.0f, 0.0f,
										0.0f, 0.0f, 1.0f, 0.0f,
										0.0f, 0.0f, 0.0f, 1.0f);
*/
		//printMatrix((float *)(g_projMatrix));
		/*
		XMFLOAT4X4 *projMat = (XMFLOAT4X4*)&pproj; //g_projMatrix;
		projMat->m[0][0] = 1 / (aspectRatio * tanf(vFov / 2));
		projMat->m[1][1] = 1 / tanf(vFov / 2);

		projMat->m[2][2] = (farPlane) / (farPlane - nearPlane);
		projMat->m[2][3] = 1.0f;
		projMat->m[3][2] = (-farPlane * nearPlane) / (farPlane - nearPlane);
		
		//projMat->m[2][2] = (farPlane + nearPlane) / (farPlane - nearPlane);
		//projMat->m[2][3] = 1.0f;
		//projMat->m[3][2] = (2 * farPlane * nearPlane) / (nearPlane - farPlane);
		//printMatrix((float *)(g_projMatrix));
		
		*/

		/*
		projMat->m[2][2] = (farPlane) / (farPlane - nearPlane);
		projMat->m[2][3] = 1.0f;
		projMat->m[3][2] = (- farPlane * nearPlane) / (farPlane - nearPlane);
		*/

		/*
		//XMMATRIX projXM = XMMatrixPerspectiveFovLH(1.047198, 2.641791, 1, 30);
		//sprintf((char*)g_charArray, "%f %f %f %f", vFov, aspectRatio, nearPlane, farPlane);
		// Projection matrix computation
		XMMATRIX projXM = XMMatrixPerspectiveFovLH(vFov, aspectRatio, nearPlane, farPlane);
		
		XMStoreFloat4x4(&pproj, projXM);
		//printMatrix((float *)(&pproj));
		*/

		//printMatrix((float *)(g_viewMatrix));
		/*
		XMFLOAT4X4 *mView = (XMFLOAT4X4 *)(g_viewMatrix);
		XMMATRIX view = XMMatrixInverse(nullptr, XMLoadFloat4x4(mView));
		XMStoreFloat4x4(mView, view);
		printMatrix((float *)(g_viewMatrix));
		*/
		d3dRenderApi->DrawSimpleTrianglesD3D11((XMFLOAT4X4 *)(worldMatrix), (XMFLOAT4X4 *)(g_viewMatrix), (XMFLOAT4X4 *)(g_projMatrix), 1, vertsD3D11); // &pproj


		BasicMaterialVertex verts2D3D11[3] = {
			//BasicMaterialVertex(XMFLOAT3(-0.5f, -0.5f,  0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
			//BasicMaterialVertex(XMFLOAT3(0,     0.5f ,  0), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)),
			//BasicMaterialVertex(XMFLOAT3(0.5f, -0.5f,  0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)),
			BasicMaterialVertex(XMFLOAT3(-4.0f, -4.0f,  0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)),
			BasicMaterialVertex(XMFLOAT3(0,     4.0f ,  0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)),
			BasicMaterialVertex(XMFLOAT3(4.0f, -4.0f,  0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
		};

		float worldMatrix2[16] = {
			cosPhi,-sinPhi,0,10,
			sinPhi,cosPhi,0,0,
			0,0,1,10,
			0,0,0,1,
		};

		d3dRenderApi->DrawSimpleTrianglesD3D11((XMFLOAT4X4 *)(worldMatrix2), (XMFLOAT4X4 *)(g_viewMatrix), (XMFLOAT4X4 *)(g_projMatrix), 1, verts2D3D11); // &pproj

	}
	
}

/*
static void ModifyCharArray() {
	char *arr = (char*)g_charArray;
	//sprintf(arr, "test");
}
*/

static void ModifyTexturePixels()
{
	void* textureHandle = g_TextureHandle;
	int width = g_TextureWidth;
	int height = g_TextureHeight;
	if (!textureHandle)
		return;

	int textureRowPitch;
	void* textureDataPtr = s_CurrentAPI->BeginModifyTexture(textureHandle, width, height, &textureRowPitch);
	if (!textureDataPtr)
		return;

	const float t = g_Time * 4.0f;

	unsigned char* dst = (unsigned char*)textureDataPtr;
	for (int y = 0; y < height; ++y)
	{
		unsigned char* ptr = dst;
		for (int x = 0; x < width; ++x)
		{
			// Simple "plasma effect": several combined sine waves
			int vv = int(
				(127.0f + (127.0f * sinf(x / 7.0f + t))) +
				(127.0f + (127.0f * sinf(y / 5.0f - t))) +
				(127.0f + (127.0f * sinf((x + y) / 6.0f - t))) +
				(127.0f + (127.0f * sinf(sqrtf(float(x*x + y*y)) / 4.0f - t)))
				) / 4;

			// Write the texture pixel
			ptr[0] = vv;
			ptr[1] = vv;
			ptr[2] = vv;
			ptr[3] = vv;

			// To next pixel (our pixels are 4 bpp)
			ptr += 4;
		}

		// To next image row
		dst += textureRowPitch;
	}

	s_CurrentAPI->EndModifyTexture(textureHandle, width, height, textureRowPitch, textureDataPtr);
}


static void ModifyVertexBuffer()
{
	void* bufferHandle = g_VertexBufferHandle;
	int vertexCount = g_VertexBufferVertexCount;
	if (!bufferHandle)
		return;

	size_t bufferSize;
	void* bufferDataPtr = s_CurrentAPI->BeginModifyVertexBuffer(bufferHandle, &bufferSize);
	if (!bufferDataPtr)
		return;
	int vertexStride = int(bufferSize / vertexCount);

	const float t = g_Time * 3.0f;

	char* bufferPtr = (char*)bufferDataPtr;
	// modify vertex Y position with several scrolling sine waves,
	// copy the rest of the source data unmodified
	for (int i = 0; i < vertexCount; ++i)
	{
		const MeshVertex& src = g_VertexSource[i];
		MeshVertex& dst = *(MeshVertex*)bufferPtr;
		dst.pos[0] = src.pos[0];
		dst.pos[1] = src.pos[1] + sinf(src.pos[0] * 1.1f + t) * 0.4f + sinf(src.pos[2] * 0.9f - t) * 0.3f;
		dst.pos[2] = src.pos[2];
		dst.normal[0] = src.normal[0];
		dst.normal[1] = src.normal[1];
		dst.normal[2] = src.normal[2];
		dst.uv[0] = src.uv[0];
		dst.uv[1] = src.uv[1];
		bufferPtr += vertexStride;
	}

	s_CurrentAPI->EndModifyVertexBuffer(bufferHandle);
}


static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
	// Unknown / unsupported graphics device type? Do nothing
	if (s_CurrentAPI == NULL)
		return;

	 //ModifyCharArray();
	DrawColoredTriangle();
	 //DrawColoredQuad();
	 //DrawTexturedQuad();
	//DrawOpenCVQuad();

	//DrawOpenVRCameraQuad();

	ModifyTexturePixels();
	ModifyVertexBuffer();


#ifdef _DEBUG
	//sprintf((char *)g_charArray, textBuffer);
#endif
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API Debug_log_test()
{
	//sprintf((char*)textBuffer, "%3.2f", g_Time);
	//sprintf((char *)g_charArray, textBuffer);
	debug_log(textBuffer);
}

// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return OnRenderEvent;
}

