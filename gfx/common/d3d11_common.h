/*  RetroArch - A frontend for libretro.
*  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
*  Copyright (C) 2011-2015 - Daniel De Matteis
*
*  RetroArch is free software: you can redistribute it and/or modify it under the terms
*  of the GNU General Public License as published by the Free Software Found-
*  ation, either version 3 of the License, or (at your option) any later version.
*
*  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
*  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*  PURPOSE.  See the GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along with RetroArch.
*  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _D3D11_COMMON_H
#define _D3D11_COMMON_H

#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>
#include <collection.h>

#include <retro_assert.h>

#include "../font_driver.h"
#include "../font_renderer_driver.h"
#include "../video_context_driver.h"

namespace d3d11
{
	// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	// The static dispatcher for the UI thread
	extern Windows::UI::Core::CoreDispatcher^ ui_dispatcher;

   struct OverlayImage
   {
      D2D1_RECT_F Geometry;
      D2D1_RECT_F TexCoord;

      Microsoft::WRL::ComPtr<ID2D1Bitmap1> Bitmap;

      OverlayImage() :
         Geometry({ 0,0,0,0 }),
         TexCoord({ 0,0,1,1 }) 
      {}
   };

   class DeviceResources* Get();

   // Constant buffer used to send the display matrix to the vertex shader.
   struct DisplayMatrixConstantBuffer
   {
      DirectX::XMFLOAT4X4 display;
	  DirectX::XMFLOAT4 size;
	  DirectX::XMFLOAT4 params;
   };


	// Controls all the DirectX device resources.
	class DeviceResources
	{
	public:
		DeviceResources(const video_info_t* info);
		void SetWindow(Windows::UI::Core::CoreWindow^ window, Windows::Graphics::Display::DisplayInformation^ currentDisplayInformation);
      void SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
		void SetLogicalSize(Windows::Foundation::Size logicalSize);
		void SetCompositionScale(float compositionScaleX, float compositionScaleY);

		void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi);
		void ValidateDevice();
		void HandleDeviceLost();
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		void Trim();
      void Render(const D2D1_MATRIX_3X2_F &displayMatrix, bool displayOverlays);
		void Present();
            
      void SetFrameTexture(const void *frame, bool rgb32, unsigned width, unsigned height, unsigned pitch);

      void UpdateBitmap(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, const void *frame, bool rgb32, unsigned width, unsigned height, unsigned pitch, float alpha, bool has_alpha);

      void InitOverlays(const struct texture_image* image_data, unsigned num_images);
      OverlayImage* GetOverlay(unsigned image);
      void RenderOverlays();

		// Device Accessors.
		Windows::Foundation::Size GetOutputSize() const { return m_outputSize; }
		Windows::Foundation::Size GetLogicalSize() const { return m_logicalSize; }

		void CreateWindowSizeDependentResources();
		
		// D3D Accessors.
		ID3D11Device3*			GetD3DDevice() const { return m_d3dDevice.Get(); }
		ID3D11DeviceContext3*	GetD3DDeviceContext() const { return m_d3dContext.Get(); }
		IDXGISwapChain2*		GetSwapChain() const { return m_swapChain.Get(); }
		D3D_FEATURE_LEVEL		GetDeviceFeatureLevel() const { return m_d3dFeatureLevel; }
		ID3D11RenderTargetView*	GetBackBufferRenderTargetView() const { return m_d3dRenderTargetView.Get(); }
		ID3D11DepthStencilView* GetDepthStencilView() const { return m_d3dDepthStencilView.Get(); }
		D3D11_VIEWPORT			GetScreenViewport() const { return m_screenViewport; }
		DirectX::XMFLOAT4X4		GetOrientationTransform3D() const { return m_orientationTransform3D; }

		// D2D Accessors.
		ID2D1Factory2*			GetD2DFactory() const { return m_d2dFactory.Get(); }
		ID2D1Device1*			GetD2DDevice() const { return m_d2dDevice.Get(); }
		ID2D1DeviceContext1*	GetD2DDeviceContext() const { return m_d2dContext.Get(); }
		ID2D1Bitmap1*			GetD2DTargetBitmap() const { return m_d2dTargetBitmap.Get(); }
		IDWriteFactory2*		GetDWriteFactory() const { return m_dwriteFactory.Get(); }
		IWICImagingFactory2*	GetWicImagingFactory() const { return m_wicFactory.Get(); }
		D2D1::Matrix3x2F		GetOrientationTransform2D() const { return m_orientationTransform2D; }
            
      ID2D1Bitmap1*			GetD2DFrameBitmap() const { return m_d2dFrameBitmap.Get(); }

      const video_info_t*  GetVideoInfo() const { return &m_videoInfo; }

      static void SetGlobalSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
		
	private:
		void CreateDeviceIndependentResources();
		void CreateDeviceResources();
      void CreateRenderingResources();

		// Direct3D objects.
		Microsoft::WRL::ComPtr<ID3D11Device3>			m_d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext3>	m_d3dContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain2>			m_swapChain;

		// Direct3D rendering objects. Required for 3D.
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	m_d3dRenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	m_d3dDepthStencilView;
		D3D11_VIEWPORT									m_screenViewport;

		// Direct2D drawing components.
		Microsoft::WRL::ComPtr<ID2D1Factory2>		m_d2dFactory;
		Microsoft::WRL::ComPtr<ID2D1Device1>		m_d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext1>	m_d2dContext;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1>		m_d2dTargetBitmap;
      Microsoft::WRL::ComPtr<ID2D1Bitmap1>		m_d2dFrameBitmap;

		// DirectWrite drawing components.
		Microsoft::WRL::ComPtr<IDWriteFactory2>		m_dwriteFactory;
		Microsoft::WRL::ComPtr<IWICImagingFactory2>	m_wicFactory;

		// Cached reference to the Window.
		Platform::Agile<Windows::UI::Core::CoreWindow> m_window;

      // CAched reference to the swap chain panel
      Windows::UI::Xaml::Controls::SwapChainPanel^	m_swapChainPanel;


		// Cached device properties.
		D3D_FEATURE_LEVEL								m_d3dFeatureLevel;
		Windows::Foundation::Size						m_d3dRenderTargetSize;
		Windows::Foundation::Size						m_outputSize;
		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;
      float											m_compositionScaleX;
      float											m_compositionScaleY;

		// Transforms used for display orientation.
		D2D1::Matrix3x2F	m_orientationTransform2D;
		DirectX::XMFLOAT4X4	m_orientationTransform3D;


      // Direct3D resources for full screen quad geometry.
      bool m_loadingComplete;
      Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
      Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
      Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
      Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
      Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
      Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;
      Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterState;
      Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;

      // Buffer
      Microsoft::WRL::ComPtr<ID3D11Texture2D> m_textureDisplay;
      Microsoft::WRL::ComPtr<ID3D11Texture2D> m_textureStaging;
      Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureDisplayRSV;

      DisplayMatrixConstantBuffer	m_constantBufferData;
      
		std::shared_ptr<uint8>	m_bitmapConversionBuffer;
      size_t                  m_bitmapConversionBufferSize;

      std::vector<OverlayImage>     m_bitmapOverlays;
      unsigned int                  m_bitmapOverlayCount;

      video_info_t m_videoInfo;

		// The IDeviceNotify can be held directly as it owns the DeviceResources.
		IDeviceNotify* m_deviceNotify;
	};

	inline void ThrowIfFailed(HRESULT hr)
	{
		retro_assert(!FAILED(hr));
	}

	// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
	}

#if defined(_DEBUG)
	// Check for SDK Layer support.
	inline bool SdkLayersAvailable()
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
			0,
			D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
			nullptr,                    // Any feature level will do.
			0,
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
			nullptr,                    // No need to keep the D3D device reference.
			nullptr,                    // No need to know the feature level.
			nullptr                     // No need to keep the D3D device context reference.
			);

		return SUCCEEDED(hr);
	}
#endif
}

#endif