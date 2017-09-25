#include <thread>
#include <mutex>

#include "PlatformBase.h"
#include "OpenCVTest.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2\features2d.hpp"
#include "opencv2\xfeatures2d.hpp"
#include "opencv2\xfeatures2d\nonfree.hpp"
#include "opencv2/core.hpp"
#include "opencv2/core/directx.hpp"
#include "opencv2/core/ocl.hpp"

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

typedef UMat MyMat;

class Camera
{
public:
	Camera(ID3D11Device *d3d11device, int camDevice)
		: camera_(camDevice)
		, width_(static_cast<int>(camera_.get(cv::CAP_PROP_FRAME_WIDTH)))
		, height_(static_cast<int>(camera_.get(cv::CAP_PROP_FRAME_HEIGHT)))
	{
		StartCapture(d3d11device);
	}

	~Camera()
	{
		StopCapture();
		camera_.release();
	}

private:
	void StartCapture(ID3D11Device *device)
	{
		thread_ = std::thread([this, device] {
			isRunning_ = true;
			initOpenCL(device);
			Ptr<FeatureDetector> detector = ORB::create(100, 2, 8); //SURF::create(1500.0);
			while (isRunning_ && camera_.isOpened()) {
				MyMat rgb;
				camera_ >> rgb;
				{
					std::lock_guard<std::mutex> lock(mutex_);
					cv::cvtColor(rgb, image_, cv::COLOR_BGR2RGBA);								
				}
				
				///*
				MyMat grayFrame;
				//cv::UMat ff;
				cvtColor(rgb, grayFrame, CV_BGR2GRAY);
				vector<KeyPoint> keypoints;
				MyMat descriptors;
				m_timer.start();
				detector->detectAndCompute(grayFrame, MyMat(), keypoints, descriptors);
				m_timer.stop();
				cv::String strTime = cv::format("time: %4.1f msec", m_timer.time(Timer::UNITS::MSEC));
				sprintf((char*)textBuffer, strTime.c_str());

				//*/
				/*
				{
					std::lock_guard<std::mutex> lock(mutex_);
					drawKeypoints(rgb, keypoints,
						image_, Scalar(255, 255, 255),
						DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
				}
				*/
			}
		});
	}

	void StopCapture()
	{
		isRunning_ = false;
		if (thread_.joinable()) {
			thread_.join();
		}
	}

	void initOpenCL(ID3D11Device *device) {

		// initialize OpenCL context of OpenCV lib from DirectX
		if (cv::ocl::haveOpenCL())
		{
			//cv::ocl::setUseOpenCL(true);

			//m_oclCtx = cv::directx::ocl::initializeContextFromD3D11Device(device);

			//cv::ocl::attachContext(m_oclCtx.)
		}
		else {
			throw 2;
			//sprintf((char*)textBuffer, "no opencl :(");
		}

		
		m_oclDevName = cv::ocl::useOpenCL() ?
			m_oclCtx.device(0).name() ://cv::ocl::Context::getDefault().device(0).name() :
			"No OpenCL device";

	}


public:
	void Update(ID3D11DeviceContext *context, ID3D11Texture2D *texture)
	{
		if (image_.empty()) return;

		std::lock_guard<std::mutex> lock(mutex_);
		
		//auto device = unity_->Get<IUnityGraphicsD3D11>()->GetDevice();
		//ID3D11DeviceContext* context;
		//device->GetImmediateContext(&context);
		
		context->UpdateSubresource(texture, 0, nullptr, image_.getMat(0).data, image_.step, 0);
		//context->UpdateSubresource(texture, 0, nullptr, image_.data, image_.step, 0);

		//context->UpdateSubresource(texture, 0, nullptr, image_.getMat(0).data, image_.step, 0);
		//cv::directx::convertToD3D11Texture2D(image_, texture);
	}

	int GetWidth() const
	{
		return width_;
	}

	int GetHeight() const
	{
		return height_;
	}
	/*
	void SetTexturePtr(void* ptr)
	{
		texture_ = static_cast<ID3D11Texture2D*>(ptr);
	}
	*/
private:
	cv::VideoCapture camera_;
	int width_, height_;
	MyMat image_;

	Timer          m_timer;
	cv::ocl::Context        m_oclCtx;
	cv::String              m_oclPlatformName;
	cv::String              m_oclDevName;

	//ID3D11Texture2D* texture_ = nullptr;

	std::thread thread_;
	std::mutex mutex_;
	bool isRunning_ = false;
};

struct OpenCVImpl {

	//VideoCapture   m_cap;
	//int         m_width;
	//int         m_height;


	Camera *m_pcamera = nullptr;

	UMat            m_frame_bgr;
	UMat            m_frame_rgba;

	//Timer          m_timer;
	//cv::ocl::Context        m_oclCtx;
	//cv::String              m_oclPlatformName;
	//cv::String              m_oclDevName;

	Ptr<FeatureDetector> m_detector;
	Ptr<DescriptorExtractor> m_descriptor;

	ID3D11Device* m_pD3D11Dev = nullptr;
	ID3D11DeviceContext *m_pD3D11Ctx = nullptr;
	ID3D11Texture2D* m_pSurfaceRGBA = nullptr;
	ID3D11ShaderResourceView* mTextureView = nullptr;
	//void initOpenCL();
	void initialize(ID3D11Device * D3Ddevice, ID3D11DeviceContext *D3DCtx);
	void render();
	void shutDown();
};

OpenCVTest::OpenCVTest(ID3D11Device* D3Ddevice, ID3D11DeviceContext *D3DCtx)
{
	pImpl = new OpenCVImpl;
	pImpl->initialize(D3Ddevice, D3DCtx);
}

OpenCVTest::~OpenCVTest()
{
	pImpl->shutDown();
	delete pImpl;
	pImpl = nullptr;
}

void OpenCVTest::RenderOnTexture()
{
	pImpl->render();
	
}


/*
void OpenCVImpl::initOpenCL() {

	// initialize OpenCL context of OpenCV lib from DirectX
	if (cv::ocl::haveOpenCL())
	{
		cv::ocl::setUseOpenCL(true);

		m_oclCtx = cv::directx::ocl::initializeContextFromD3D11Device(m_pD3D11Dev);

		//cv::ocl::attachContext(m_oclCtx.)
	}
	else {
		sprintf((char*)textBuffer, "no opencl :(");
	}

	m_oclDevName = cv::ocl::useOpenCL() ?
		m_oclCtx.device(0).name() ://cv::ocl::Context::getDefault().device(0).name() :
		"No OpenCL device";

}
*/

void OpenCVImpl::initialize(ID3D11Device * D3Ddevice, ID3D11DeviceContext *D3DCtx)
{
	m_pD3D11Dev = D3Ddevice;
	m_pD3D11Ctx = D3DCtx;
	
	//initOpenCL();

	int deviceIdx = 0;
	m_pcamera = new Camera(D3Ddevice, deviceIdx);


	D3D11_TEXTURE2D_DESC desc_rgba;

	desc_rgba.Width = m_pcamera->GetWidth();
	desc_rgba.Height = m_pcamera->GetHeight();
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
	}


	//m_detector = SURF::create(1500.0);//ORB::create(100, 1.2, 8); //BRISK::create(50, 5);//SURF::create(1500.0);//SIFT::create(50, 5); 
	//m_descriptor = m_detector; //FREAK::create(); //detector; // FREAK::create();
	//sprintf((char*)textBuffer, m_oclDevName.c_str());

}


void OpenCVImpl::render()
{

	m_pcamera->Update(m_pD3D11Ctx, m_pSurfaceRGBA);
}



void OpenCVImpl::shutDown()
{
	SAFE_RELEASE(m_pSurfaceRGBA);
	SAFE_RELEASE(mTextureView);
	m_pD3D11Ctx->Release();
	delete m_pcamera;
}

ID3D11ShaderResourceView* OpenCVTest::GetTexture()
{
	return pImpl->mTextureView;
}
