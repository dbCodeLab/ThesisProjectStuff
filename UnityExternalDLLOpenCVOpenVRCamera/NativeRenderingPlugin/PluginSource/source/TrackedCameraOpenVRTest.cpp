
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2\features2d.hpp"
#include "opencv2\xfeatures2d.hpp"
#include "opencv2\xfeatures2d\nonfree.hpp"
#include "opencv2/core.hpp"
#include "opencv2/core/directx.hpp"
#include "opencv2/core/ocl.hpp"

#include "TrackedCameraOpenVRTest.h"

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;
using namespace std;


extern char *textBuffer;

class Timer
{
public:
	enum UNITS
	{
		USEC = 0,
		MSEC,
		SEC
	};

	Timer() : m_t0(0), m_diff(0)
	{
		m_tick_frequency = (float)cv::getTickFrequency();

		m_unit_mul[USEC] = 1000000;
		m_unit_mul[MSEC] = 1000;
		m_unit_mul[SEC] = 1;
	}

	void start()
	{
		m_t0 = cv::getTickCount();
	}

	void stop()
	{
		m_diff = cv::getTickCount() - m_t0;
	}

	float time(UNITS u = UNITS::MSEC)
	{
		float sec = m_diff / m_tick_frequency;

		return sec * m_unit_mul[u];
	}

public:
	float m_tick_frequency;
	int64 m_t0;
	int64 m_diff;
	int   m_unit_mul[3];
};



TrackedCameraOpenVRTest::TrackedCameraOpenVRTest(ID3D11Device * D3Ddevice, ID3D11DeviceContext *D3DCtx)
{
	m_pVRSystem = nullptr;
	m_pVRTrackedCamera = nullptr;

	m_hTrackedCamera = INVALID_TRACKED_CAMERA_HANDLE;

	m_nCameraFrameWidth = 0;
	m_nCameraFrameHeight = 0;
	m_nCameraFrameBufferSize = 0;
	m_pCameraFrameBuffer = nullptr;

	bool bValidOpenVR = InitOpenVR();
	if (!bValidOpenVR) {
		throw 2;
	}

	StartVideoPreview();

	initializeD3D(D3Ddevice, D3DCtx);

	StartCapture();
}

TrackedCameraOpenVRTest::~TrackedCameraOpenVRTest()
{

	StopCapture();

	StopVideoPreview();

	vr::VR_Shutdown();

	m_pVRSystem = nullptr;
	m_pVRTrackedCamera = nullptr;

	delete m_imageRGBA;
	m_imageRGBA = nullptr;

}

void TrackedCameraOpenVRTest::renderOnTexture()
{
	std::lock_guard<std::mutex> lock(mutex_);

	m_pD3D11Ctx->UpdateSubresource(m_pSurfaceRGBA, 0, nullptr, m_imageRGBA->data, m_imageRGBA->step, 0);
}

ID3D11ShaderResourceView * TrackedCameraOpenVRTest::GetTexture()
{
	return mTextureView;
}

bool TrackedCameraOpenVRTest::InitOpenVR()
{
	// Loading the SteamVR Runtime
	//LogMessage(LogInfo, "\nStarting OpenVR...\n");
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pVRSystem = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if (eError != vr::VRInitError_None)
	{
		m_pVRSystem = nullptr;
		//char buf[1024];
		//sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsSymbol(eError));
		//LogMessage(LogError, "%s\n", buf);
		return false;
	}
	else
	{
		char systemName[1024];
		char serialNumber[1024];
		m_pVRSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String, systemName, sizeof(systemName));
		m_pVRSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String, serialNumber, sizeof(serialNumber));

		m_HMDSerialNumberString = serialNumber;

		//LogMessage(LogInfo, "VR HMD: %s %s\n", systemName, serialNumber);
	}

	m_pVRTrackedCamera = vr::VRTrackedCamera();
	if (!m_pVRTrackedCamera)
	{
		//LogMessage(LogError, "Unable to get Tracked Camera interface.\n");
		return false;
	}
	
	bool bHasCamera = false;
	vr::EVRTrackedCameraError nCameraError = m_pVRTrackedCamera->HasCamera(vr::k_unTrackedDeviceIndex_Hmd, &bHasCamera);
	if (nCameraError != vr::VRTrackedCameraError_None || !bHasCamera)
	{
		//LogMessage(LogError, "No Tracked Camera Available! (%s)\n", m_pVRTrackedCamera->GetCameraErrorNameFromEnum(nCameraError));
		return false;
	}

	// Accessing the FW description is just a further check to ensure camera communication is valid as expected.
	vr::ETrackedPropertyError propertyError;
	char buffer[128];
	m_pVRSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_CameraFirmwareDescription_String, buffer, sizeof(buffer), &propertyError);
	if (propertyError != vr::TrackedProp_Success)
	{
		//LogMessage(LogError, "Failed to get tracked camera firmware description!\n");
		return false;
	}

	//LogMessage(LogInfo, "Camera Firmware: %s\n\n", buffer);

	return true;
}

bool TrackedCameraOpenVRTest::StartVideoPreview()
{
	//LogMessage(LogInfo, "StartVideoPreview()\n");

	// Allocate for camera frame buffer requirements
	uint32_t nCameraFrameBufferSize = 0;
	if (m_pVRTrackedCamera->GetCameraFrameSize(vr::k_unTrackedDeviceIndex_Hmd, vr::VRTrackedCameraFrameType_Undistorted, &m_nCameraFrameWidth, &m_nCameraFrameHeight, &nCameraFrameBufferSize) != vr::VRTrackedCameraError_None)
	{
		//LogMessage(LogError, "GetCameraFrameBounds() Failed!\n");
		return false;
	}

	if (nCameraFrameBufferSize && nCameraFrameBufferSize != m_nCameraFrameBufferSize)
	{
		delete[] m_pCameraFrameBuffer;
		m_nCameraFrameBufferSize = nCameraFrameBufferSize;
		m_pCameraFrameBuffer = new uint8_t[m_nCameraFrameBufferSize];
		memset(m_pCameraFrameBuffer, 0, m_nCameraFrameBufferSize);
	}

	m_nLastFrameSequence = 0;
	//m_VideoSignalTime.start();

	m_pVRTrackedCamera->AcquireVideoStreamingService(vr::k_unTrackedDeviceIndex_Hmd, &m_hTrackedCamera);
	if (m_hTrackedCamera == INVALID_TRACKED_CAMERA_HANDLE)
	{
		//LogMessage(LogError, "AcquireVideoStreamingService() Failed!\n");
		return false;
	}

	m_imageRGBA = new cv::Mat(m_nCameraFrameHeight, m_nCameraFrameWidth, CV_8UC4);

	return true;
}

void TrackedCameraOpenVRTest::StopVideoPreview()
{
	//LogMessage(LogInfo, "StopVideoPreview()\n");

	m_pVRTrackedCamera->ReleaseVideoStreamingService(m_hTrackedCamera);
	m_hTrackedCamera = INVALID_TRACKED_CAMERA_HANDLE;
}

void TrackedCameraOpenVRTest::DisplayUpdate()
{
	if (!m_pVRTrackedCamera || !m_hTrackedCamera)
		return;

	/*
	if (m_VideoSignalTime.elapsed() >= 2000)
	{
		// No frames after 2 seconds...
		LogMessage(LogError, "No Video Frames Arriving!\n");
		m_VideoSignalTime.restart();
	}
	*/

	// get the frame header only
	vr::CameraVideoStreamFrameHeader_t frameHeader;
	vr::EVRTrackedCameraError nCameraError = m_pVRTrackedCamera->GetVideoStreamFrameBuffer(m_hTrackedCamera, vr::VRTrackedCameraFrameType_Undistorted, nullptr, 0, &frameHeader, sizeof(frameHeader));
	if (nCameraError != vr::VRTrackedCameraError_None) {
		throw 2;
		return;
	}

	if (frameHeader.nFrameSequence == m_nLastFrameSequence)
	{
		// frame hasn't changed yet, nothing to do
		return;
	}

	//m_VideoSignalTime.restart();

	// Frame has changed, do the more expensive frame buffer copy
	//std::lock_guard<std::mutex> lock(mutex_);
	nCameraError = m_pVRTrackedCamera->GetVideoStreamFrameBuffer(m_hTrackedCamera, vr::VRTrackedCameraFrameType_Undistorted, m_pCameraFrameBuffer, m_nCameraFrameBufferSize, &frameHeader, sizeof(frameHeader));
	if (nCameraError != vr::VRTrackedCameraError_None) {
		throw 2;
		return;
	}
	
	m_nLastFrameSequence = frameHeader.nFrameSequence;

	{
		if (!m_pCameraFrameBuffer) return;

		const uint8_t *pFrameImage = m_pCameraFrameBuffer;

		std::lock_guard<std::mutex> lock(mutex_);
		
		struct RGB {
			uchar red;
			uchar green;
			uchar blue;
			uchar alpha;
		};

		for (uint32_t y = 0; y < m_nCameraFrameHeight; y++)
		{
			for (uint32_t x = 0; x < m_nCameraFrameWidth; x++)
			{
				RGB& rgb = m_imageRGBA->ptr<RGB>(y)[x];
				rgb.red = pFrameImage[0];
				rgb.green = pFrameImage[1];
				rgb.blue = pFrameImage[2];
				rgb.alpha = 255;
				pFrameImage += 4;
			}
		}
		
	}


	//m_pCameraPreviewImage->SetFrameImage(m_pCameraFrameBuffer, m_nCameraFrameWidth, m_nCameraFrameHeight, &frameHeader);

}

void TrackedCameraOpenVRTest::StartCapture()
{
	thread_ = std::thread([this] {
		isRunning_ = true;
		
		while (isRunning_) {
			/*
			static Timer timer;
			cv::String strTime = cv::format("w: %d, h: %d, buff size: %d", m_nCameraFrameWidth, m_nCameraFrameHeight, m_nCameraFrameBufferSize);
			sprintf((char*)textBuffer, strTime.c_str());
			*/

			DisplayUpdate();
		}
		
	});
}

void TrackedCameraOpenVRTest::StopCapture()
{
	isRunning_ = false;
	if (thread_.joinable()) {
		thread_.join();
	}
}



bool TrackedCameraOpenVRTest::initializeD3D(ID3D11Device * D3Ddevice, ID3D11DeviceContext *D3DCtx)
{
	m_pD3D11Dev = D3Ddevice;
	m_pD3D11Ctx = D3DCtx;

	D3D11_TEXTURE2D_DESC desc_rgba;

	desc_rgba.Width = m_nCameraFrameWidth;
	desc_rgba.Height = m_nCameraFrameHeight;
	desc_rgba.MipLevels = 1;
	desc_rgba.ArraySize = 1;
	desc_rgba.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc_rgba.SampleDesc.Count = 1;
	desc_rgba.SampleDesc.Quality = 0;
	desc_rgba.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc_rgba.Usage = D3D11_USAGE_DEFAULT; //D3D11_USAGE_DYNAMIC;
	desc_rgba.CPUAccessFlags = 0; //D3D11_CPU_ACCESS_WRITE;
	desc_rgba.MiscFlags = 0;

	m_pSurfaceRGBA = 0;
	HRESULT r = D3Ddevice->CreateTexture2D(&desc_rgba, 0, &m_pSurfaceRGBA);
	if (FAILED(r))
	{
		throw std::runtime_error("Can't create DX texture");
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	// Setup the shader resource view description.
	srvDesc.Format = desc_rgba.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	// Create the shader resource view for the texture.
	r = D3Ddevice->CreateShaderResourceView(m_pSurfaceRGBA, &srvDesc, &mTextureView);
	if (FAILED(r))
	{
		throw std::runtime_error("Can't create srv");
		return false;
	}

	//m_detector = SURF::create(1500.0);//ORB::create(100, 1.2, 8); //BRISK::create(50, 5);//SURF::create(1500.0);//SIFT::create(50, 5); 
	//m_descriptor = m_detector; //FREAK::create(); //detector; // FREAK::create();
	//sprintf((char*)textBuffer, m_oclDevName.c_str());

}