#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <d3d11.h>
#include "opencv2/core.hpp"
#include "openvr.h"

class TrackedCameraOpenVRTest {
public:
	TrackedCameraOpenVRTest(ID3D11Device * D3Ddevice, ID3D11DeviceContext *D3DCtx);
	~TrackedCameraOpenVRTest();

	void renderOnTexture();
	ID3D11ShaderResourceView* GetTexture();

private:
	bool	InitOpenVR();
	bool    initializeD3D(ID3D11Device * D3Ddevice, ID3D11DeviceContext *D3DCtx);
	bool	StartVideoPreview();
	void	StopVideoPreview();
	void    DisplayUpdate();
	void	StartCapture();
	void	StopCapture();

	vr::IVRSystem					*m_pVRSystem;
	vr::IVRTrackedCamera			*m_pVRTrackedCamera;
	vr::TrackedCameraHandle_t	m_hTrackedCamera;

	std::string m_HMDSerialNumberString;

	uint32_t				m_nCameraFrameWidth;
	uint32_t				m_nCameraFrameHeight;
	uint32_t				m_nCameraFrameBufferSize;
	uint8_t					*m_pCameraFrameBuffer;

	uint32_t				m_nLastFrameSequence;

	std::thread thread_;
	std::mutex mutex_;
	bool isRunning_ = false;
	
	ID3D11Device* m_pD3D11Dev = nullptr;
	ID3D11DeviceContext *m_pD3D11Ctx = nullptr;
	ID3D11Texture2D* m_pSurfaceRGBA = nullptr;
	ID3D11ShaderResourceView* mTextureView = nullptr;

	//cv::Mat m_image;
	cv::Mat *m_imageRGBA;
};