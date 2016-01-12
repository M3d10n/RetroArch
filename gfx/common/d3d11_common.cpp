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

#include "d3d11_common.h"
#include <windows.ui.xaml.media.dxinterop.h>


using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Platform;

Windows::UI::Core::CoreDispatcher^ d3d11::ui_dispatcher;

static Windows::UI::Xaml::Controls::SwapChainPanel ^ g_swap_chain_panel;

// Constructor for DeviceResources.
d3d11::DeviceResources::DeviceResources(const video_info_t* info) :
	m_screenViewport(),
	m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
	m_d3dRenderTargetSize(),
	m_outputSize(),
	m_logicalSize(),
	m_nativeOrientation(DisplayOrientations::None),
	m_currentOrientation(DisplayOrientations::None),
	m_dpi(-1.0f),
   m_compositionScaleX(1.0f),
   m_compositionScaleY(1.0f),
	m_deviceNotify(nullptr),
   m_bitmapConversionBufferSize(0),
   m_videoInfo(*info),
   m_bitmapOverlayCount(0)
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();

	// Grab the window from the UI thread
	auto async = d3d11::ui_dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([=]()
	{
      if (g_swap_chain_panel)
      {
         this->SetSwapChainPanel(g_swap_chain_panel);
      } 
      else
      {
		   this->SetWindow(CoreWindow::GetForCurrentThread(), DisplayInformation::GetForCurrentView());
      }
	}, Platform::CallbackContext::Any));
	while (async->Status != AsyncStatus::Completed);
}

void d3d11::DeviceResources::SetGlobalSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel ^ panel)
{
   g_swap_chain_panel = panel;
}

// Configures resources that don't depend on the Direct3D device.
void d3d11::DeviceResources::CreateDeviceIndependentResources()
{
	// Initialize Direct2D resources.
	D2D1_FACTORY_OPTIONS options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	// If the project is in a debug build, enable Direct2D debugging via SDK Layers.
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	// Initialize the Direct2D Factory.
	d3d11::ThrowIfFailed(
		D2D1CreateFactory(
         D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory2),
			&options,
			&m_d2dFactory
			)
		);

	// Initialize the DirectWrite Factory.
	d3d11::ThrowIfFailed(
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory2),
			&m_dwriteFactory
			)
		);

	// Initialize the Windows Imaging Component (WIC) Factory.
	d3d11::ThrowIfFailed(
		CoCreateInstance(
			CLSID_WICImagingFactory2,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wicFactory)
			)
		);
}

// Configures the Direct3D device, and stores handles to it and the device context.
void d3d11::DeviceResources::CreateDeviceResources()
{
	// This flag adds support for surfaces with a different color channel ordering
	// than the API default. It is required for compatibility with Direct2D.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	if (d3d11::SdkLayersAvailable())
	{
		// If the project is in a debug build, enable debugging via SDK Layers with this flag.
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	// This array defines the set of DirectX hardware feature levels this app will support.
	// Note the ordering should be preserved.
	// Don't forget to declare your application's minimum required feature level in its
	// description.  All applications are assumed to support 9.1 unless otherwise stated.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_9_1
	};

	// Create the Direct3D 11 API device object and a corresponding context.
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;

	HRESULT hr = D3D11CreateDevice(
		nullptr,					// Specify nullptr to use the default adapter.
		D3D_DRIVER_TYPE_HARDWARE,	// Create a device using the hardware graphics driver.
		0,							// Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
		creationFlags,				// Set debug and Direct2D compatibility flags.
		featureLevels,				// List of feature levels this app can support.
		ARRAYSIZE(featureLevels),	// Size of the list above.
		D3D11_SDK_VERSION,			// Always set this to D3D11_SDK_VERSION for Windows Store apps.
		&device,					// Returns the Direct3D device created.
		&m_d3dFeatureLevel,			// Returns feature level of device created.
		&context					// Returns the device immediate context.
		);

	if (FAILED(hr))
	{
		// If the initialization fails, fall back to the WARP device.
		// For more information on WARP, see: 
		// http://go.microsoft.com/fwlink/?LinkId=286690
		d3d11::ThrowIfFailed(
			D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
				0,
				creationFlags,
				featureLevels,
				ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION,
				&device,
				&m_d3dFeatureLevel,
				&context
				)
			);
	}

	// Store pointers to the Direct3D 11.1 API device and immediate context.
	d3d11::ThrowIfFailed(
		device.As(&m_d3dDevice)
		);

	d3d11::ThrowIfFailed(
		context.As(&m_d3dContext)
		);

	// Create the Direct2D device object and a corresponding context.
	ComPtr<IDXGIDevice3> dxgiDevice;
	d3d11::ThrowIfFailed(
		m_d3dDevice.As(&dxgiDevice)
		);

	d3d11::ThrowIfFailed(
		m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
		);

	d3d11::ThrowIfFailed(
		m_d2dDevice->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			&m_d2dContext
			)
		);
}

// These resources need to be recreated every time the window size is changed.
void d3d11::DeviceResources::CreateWindowSizeDependentResources()
{
	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView = nullptr;
	m_d2dContext->SetTarget(nullptr);
	m_d2dTargetBitmap = nullptr;
	m_d3dDepthStencilView = nullptr;
	m_d3dContext->Flush();

	// Calculate the necessary render target size in pixels.
	m_outputSize.Width = d3d11::ConvertDipsToPixels(m_logicalSize.Width, m_dpi);
	m_outputSize.Height = d3d11::ConvertDipsToPixels(m_logicalSize.Height, m_dpi);

	// Prevent zero size DirectX content from being created.
	m_outputSize.Width = max(m_outputSize.Width, 1);
	m_outputSize.Height = max(m_outputSize.Height, 1);

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	m_d3dRenderTargetSize.Width = m_outputSize.Width;
	m_d3dRenderTargetSize.Height = m_outputSize.Height;

	if (m_swapChain != nullptr)
	{
		// If the swap chain already exists, resize it.
		HRESULT hr = m_swapChain->ResizeBuffers(
			2, // Double-buffered swap chain.
			lround(m_d3dRenderTargetSize.Width),
			lround(m_d3dRenderTargetSize.Height),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
			);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			HandleDeviceLost();

			// Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
			// and correctly set up the new device.
			return;
		}
		else
		{
			d3d11::ThrowIfFailed(hr);
		}
	}
	else
	{
		// Otherwise, create a new one using the same adapter as the existing Direct3D device.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

		swapChainDesc.Width = lround(m_d3dRenderTargetSize.Width); // Match the size of the window.
		swapChainDesc.Height = lround(m_d3dRenderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
		swapChainDesc.Flags = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		// This sequence obtains the DXGI factory that was used to create the Direct3D device above.
		ComPtr<IDXGIDevice3> dxgiDevice;
		d3d11::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		ComPtr<IDXGIAdapter> dxgiAdapter;
		d3d11::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter)
			);

		ComPtr<IDXGIFactory4> dxgiFactory;
		d3d11::ThrowIfFailed(
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
			);

      if (m_window.Get())
      {
         ComPtr<IDXGISwapChain1> swapChain;

         d3d11::ThrowIfFailed(
            dxgiFactory->CreateSwapChainForCoreWindow(
               m_d3dDevice.Get(),
               reinterpret_cast<IUnknown*>(m_window.Get()),
               &swapChainDesc,
               nullptr,
               &swapChain
               )
            );

         d3d11::ThrowIfFailed(
            swapChain.As(&m_swapChain)
            );
      } 
      else if (m_swapChainPanel) {

         // Composition requires this
         swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

         ComPtr<IDXGISwapChain1> swapChain;
         d3d11::ThrowIfFailed(
            dxgiFactory->CreateSwapChainForComposition(
               m_d3dDevice.Get(),
               &swapChainDesc,
               nullptr,
               &swapChain
               )
            );

         d3d11::ThrowIfFailed(
            swapChain.As(&m_swapChain)
            );

         // Associate swap chain with SwapChainPanel
         // UI changes will need to be dispatched back to the UI thread
         m_swapChainPanel->Dispatcher->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
         {
            // Get backing native interface for SwapChainPanel
            ComPtr<ISwapChainPanelNative> panelNative;
            d3d11::ThrowIfFailed(
               reinterpret_cast<IUnknown*>(m_swapChainPanel)->QueryInterface(IID_PPV_ARGS(&panelNative))
               );

            d3d11::ThrowIfFailed(
               panelNative->SetSwapChain(m_swapChain.Get())
               );
         }, CallbackContext::Any));
      }

		// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
		// ensures that the application will only render after each VSync, minimizing power consumption.
		d3d11::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1)
			);
	}

   // Setup inverse scale on the swap chain
   DXGI_MATRIX_3X2_F inverseScale = { 0 };
   inverseScale._11 = 1.0f / m_compositionScaleX;
   inverseScale._22 = 1.0f / m_compositionScaleY;
   d3d11::ThrowIfFailed(
      m_swapChain->SetMatrixTransform(&inverseScale)
      );

	// Create a render target view of the swap chain back buffer.
	ComPtr<ID3D11Texture2D> backBuffer;
	d3d11::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
		);

	d3d11::ThrowIfFailed(
		m_d3dDevice->CreateRenderTargetView(
			backBuffer.Get(),
			nullptr,
			&m_d3dRenderTargetView
			)
		);

	// Create a depth stencil view for use with 3D rendering if needed.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		lround(m_d3dRenderTargetSize.Width),
		lround(m_d3dRenderTargetSize.Height),
		1, // This depth stencil view has only one texture.
		1, // Use a single mipmap level.
		D3D11_BIND_DEPTH_STENCIL
		);

	ComPtr<ID3D11Texture2D> depthStencil;
	d3d11::ThrowIfFailed(
		m_d3dDevice->CreateTexture2D(
			&depthStencilDesc,
			nullptr,
			&depthStencil
			)
		);

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	d3d11::ThrowIfFailed(
		m_d3dDevice->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			&m_d3dDepthStencilView
			)
		);

	// Set the 3D rendering viewport to target the entire window.
	m_screenViewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		m_logicalSize.Width, //m_d3dRenderTargetSize.Width,
      m_logicalSize.Height //m_d3dRenderTargetSize.Height
		);

	m_d3dContext->RSSetViewports(1, &m_screenViewport);

	// Create a Direct2D target bitmap associated with the
	// swap chain back buffer and set it as the current target.
	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			96,
			96
			);

	ComPtr<IDXGISurface2> dxgiBackBuffer;
	d3d11::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
		);

	d3d11::ThrowIfFailed(
		m_d2dContext->CreateBitmapFromDxgiSurface(
			dxgiBackBuffer.Get(),
			&bitmapProperties,
			&m_d2dTargetBitmap
			)
		);

	m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());

	// Grayscale text anti-aliasing is recommended for all Windows Store apps.
	m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}

// This method is called when the CoreWindow is created (or re-created).
void d3d11::DeviceResources::SetWindow(CoreWindow^ window, Windows::Graphics::Display::DisplayInformation^ currentDisplayInformation)
{
	m_window = window;
	m_logicalSize = Windows::Foundation::Size(window->Bounds.Width, window->Bounds.Height);
	m_nativeOrientation = currentDisplayInformation->NativeOrientation;
	m_currentOrientation = currentDisplayInformation->CurrentOrientation;
	m_dpi = currentDisplayInformation->LogicalDpi;
	m_d2dContext->SetDpi(m_dpi, m_dpi);

	CreateWindowSizeDependentResources();
}

void d3d11::DeviceResources::SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel ^ panel)
{
   DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

   m_swapChainPanel = panel;
   m_logicalSize = Windows::Foundation::Size(static_cast<float>(panel->ActualWidth), static_cast<float>(panel->ActualHeight));
   m_nativeOrientation = currentDisplayInformation->NativeOrientation;
   m_currentOrientation = currentDisplayInformation->CurrentOrientation;
   m_compositionScaleX = panel->CompositionScaleX;
   m_compositionScaleY = panel->CompositionScaleY;
   m_dpi = currentDisplayInformation->LogicalDpi;
   m_d2dContext->SetDpi(m_dpi, m_dpi);

   CreateWindowSizeDependentResources();
}

// This method is called in the event handler for the SizeChanged event.
void d3d11::DeviceResources::SetLogicalSize(Windows::Foundation::Size logicalSize)
{
	if (m_logicalSize != logicalSize)
	{
		m_logicalSize = logicalSize;
		CreateWindowSizeDependentResources();
	}
}

// This method is called in the event handler for the CompositionScaleChanged event.
void d3d11::DeviceResources::SetCompositionScale(float compositionScaleX, float compositionScaleY)
{
   if (m_compositionScaleX != compositionScaleX ||
      m_compositionScaleY != compositionScaleY)
   {
      m_compositionScaleX = compositionScaleX;
      m_compositionScaleY = compositionScaleY;
      CreateWindowSizeDependentResources();
   }
}

// This method is called in the event handler for the DpiChanged event.
void d3d11::DeviceResources::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		m_dpi = dpi;
		m_d2dContext->SetDpi(m_dpi, m_dpi);
	}
}

// This method is called in the event handler for the OrientationChanged event.
void d3d11::DeviceResources::SetCurrentOrientation(DisplayOrientations currentOrientation)
{
	if (m_currentOrientation != currentOrientation)
	{
		m_currentOrientation = currentOrientation;
      CreateWindowSizeDependentResources();
	}
}

// This method is called in the event handler for the DisplayContentsInvalidated event.
void d3d11::DeviceResources::ValidateDevice()
{
	// The D3D Device is no longer valid if the default adapter changed since the device
	// was created or if the device has been removed.

	// First, get the information for the default adapter from when the device was created.

	ComPtr<IDXGIDevice3> dxgiDevice;
	d3d11::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

	ComPtr<IDXGIAdapter> deviceAdapter;
	d3d11::ThrowIfFailed(dxgiDevice->GetAdapter(&deviceAdapter));

	ComPtr<IDXGIFactory2> deviceFactory;
	d3d11::ThrowIfFailed(deviceAdapter->GetParent(IID_PPV_ARGS(&deviceFactory)));

	ComPtr<IDXGIAdapter1> previousDefaultAdapter;
	d3d11::ThrowIfFailed(deviceFactory->EnumAdapters1(0, &previousDefaultAdapter));

	DXGI_ADAPTER_DESC previousDesc;
	d3d11::ThrowIfFailed(previousDefaultAdapter->GetDesc(&previousDesc));

	// Next, get the information for the current default adapter.

	ComPtr<IDXGIFactory2> currentFactory;
	d3d11::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&currentFactory)));

	ComPtr<IDXGIAdapter1> currentDefaultAdapter;
	d3d11::ThrowIfFailed(currentFactory->EnumAdapters1(0, &currentDefaultAdapter));

	DXGI_ADAPTER_DESC currentDesc;
	d3d11::ThrowIfFailed(currentDefaultAdapter->GetDesc(&currentDesc));

	// If the adapter LUIDs don't match, or if the device reports that it has been removed,
	// a new D3D device must be created.

	if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
		previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
		FAILED(m_d3dDevice->GetDeviceRemovedReason()))
	{
		// Release references to resources related to the old device.
		dxgiDevice = nullptr;
		deviceAdapter = nullptr;
		deviceFactory = nullptr;
		previousDefaultAdapter = nullptr;

		// Create a new device and swap chain.
		HandleDeviceLost();
	}
}

// Recreate all device resources and set them back to the current state.
void d3d11::DeviceResources::HandleDeviceLost()
{
	m_swapChain = nullptr;

	if (m_deviceNotify != nullptr)
	{
		m_deviceNotify->OnDeviceLost();
	}

	CreateDeviceResources();
	m_d2dContext->SetDpi(m_dpi, m_dpi);
	CreateWindowSizeDependentResources();

	if (m_deviceNotify != nullptr)
	{
		m_deviceNotify->OnDeviceRestored();
	}
}

// Register our DeviceNotify to be informed on device lost and creation.
void d3d11::DeviceResources::RegisterDeviceNotify(d3d11::IDeviceNotify* deviceNotify)
{
	m_deviceNotify = deviceNotify;
}

// Call this method when the app suspends. It provides a hint to the driver that the app 
// is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
void d3d11::DeviceResources::Trim()
{
	ComPtr<IDXGIDevice3> dxgiDevice;
	m_d3dDevice.As(&dxgiDevice);

	dxgiDevice->Trim();
}

// Present the contents of the swap chain to the screen.
void d3d11::DeviceResources::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = m_swapChain->Present(1, 0);

	// Discard the contents of the render target.
	// This is a valid operation only when the existing contents will be entirely
	// overwritten. If dirty or scroll rects are used, this call should be removed.
	m_d3dContext->DiscardView(m_d3dRenderTargetView.Get());

	// Discard the contents of the depth stencil.
	m_d3dContext->DiscardView(m_d3dDepthStencilView.Get());

	// If the device was removed either by a disconnection or a driver upgrade, we 
	// must recreate all device resources.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		HandleDeviceLost();
	}
	else
	{
		d3d11::ThrowIfFailed(hr);
	}
}

// Creates or updates the menu texture contents
void d3d11::DeviceResources::SetMenuTextureFrame(const void * frame, bool rgb32, unsigned width, unsigned height, float alpha)
{
   UpdateBitmap(m_d2dMenuBitmap, frame, rgb32, width, height, width, alpha, true);
}

void d3d11::DeviceResources::SetFrameTexture(const void * frame, bool rgb32, unsigned width, unsigned height, unsigned pitch)
{
   UpdateBitmap(m_d2dFrameBitmap, frame, rgb32, width, height, pitch, 1.0f, false);
}

void d3d11::DeviceResources::UpdateBitmap(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, const void * frame, bool rgb32, unsigned width, unsigned height, unsigned pitch, float alpha, bool has_alpha)
{
   HRESULT hr;

   // Create or re-create the bitmap
   if (!bitmap.Get() || bitmap->GetPixelSize().width != width || bitmap->GetPixelSize().height != height)
   {
      D2D1_SIZE_U newSize = { width, height };
      D2D1_BITMAP_PROPERTIES1 properties;
      properties.dpiX = m_dpi;
      properties.dpiY = m_dpi;
      properties.pixelFormat = { DXGI_FORMAT_B8G8R8A8_UNORM , has_alpha ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE };
      properties.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;
      properties.colorContext = NULL;

      hr = m_d2dContext->CreateBitmap(newSize, NULL, 0, properties, &bitmap);
      d3d11::ThrowIfFailed(hr);

   }

   // Resize the buffer used to convert the data, if necessary
   size_t frame_size = width * height;
   if (frame_size > m_bitmapConversionBufferSize)
   {
      m_bitmapConversionBuffer.reset((uint8*)calloc(frame_size, sizeof(uint32_t)), free);
      m_bitmapConversionBufferSize = frame_size;
   }

   // Write into the CPU-accessible bitmap
   unsigned dst_pitch = width * sizeof(uint32_t);
   void* bits = m_bitmapConversionBuffer.get();
   unsigned h, w;
   if (rgb32)
   {
      if (!has_alpha)
      {
         uint8_t        *dst = (uint8_t*)bits;
         const uint32_t *src = (const uint32_t*)frame;

         for (h = 0; h < height; h++, dst += dst_pitch, src += width)
         {
            memcpy(dst, src, width * sizeof(uint32_t));
            memset(dst + width * sizeof(uint32_t), 0,
               dst_pitch - width * sizeof(uint32_t));
         }
      }
      else
      {
         uint32_t       *dst = (uint32_t*)bits;
         const uint32_t *src = (const uint32_t*)frame;

         for (h = 0; h < height; h++, dst += width, src += width)
         {
            for (w = 0; w < width; w++)
            {
               uint32_t c = src[w];
               uint32_t r = (c >> 16) & 0xff;
               uint32_t g = (c >> 8) & 0xff;
               uint32_t b = (c >> 0) & 0xff;
               uint32_t a = (c >> 24) & 0xff;
               r = (r * a >> 8) << 16;
               g = (g * a >> 8) << 8;
               b = (b * a >> 8) << 0;
               a = a << 24;
               dst[w] = r | g | b | a;
            }
         }
      }
   }
   else if (has_alpha)
   {
      uint32_t       *dst = (uint32_t*)bits;
      const uint16_t *src = (const uint16_t*)frame;

      for (h = 0; h < height; h++, dst += dst_pitch >> 2, src += width)
      {
         for (w = 0; w < width; w++)
         {
            uint16_t c = src[w];
            uint32_t r = (c >> 12) & 0xf;
            uint32_t g = (c >> 8) & 0xf;
            uint32_t b = (c >> 4) & 0xf;
            uint32_t a = (c >> 0) & 0xf;
            r = ((r << 4) | r) << 16;
            g = ((g << 4) | g) << 8;
            b = ((b << 4) | b) << 0;
            a = ((a << 4) | a) << 24;
            dst[w] = r | g | b | a;
         }
      }
   }
   else
   {
      uint32_t       *dst = (uint32_t*)bits;
      const uint16_t *src = (const uint16_t*)frame;

      for (h = 0; h < height; h++, dst += dst_pitch >> 2, src += (pitch >> 1))
      {
         for (w = 0; w < width; w++)
         {
            uint16_t c = src[w];

            uint32_t r = (c & 0xf800) << 8;
            uint32_t g = (c & 0x07e0) << 5;
            uint32_t b = (c & 0x001f) << 3;

            dst[w] = r | g | b;
         }
      }
   }

   // Copy to the GPU bitmap
   hr = bitmap->CopyFromMemory(NULL, bits, dst_pitch);
   d3d11::ThrowIfFailed(hr);
}

void d3d11::DeviceResources::InitOverlays(const texture_image * image_data, unsigned num_images)
{
   m_bitmapOverlayCount = num_images;

   m_bitmapOverlays.clear();
   m_bitmapOverlays.resize(num_images);

   for (unsigned i = 0; i < num_images; i++)
   {
      const texture_image& image = image_data[i];

      UpdateBitmap(m_bitmapOverlays[i].Bitmap, image.pixels, true, image.width, image.height, image.width, 1.0f, true);
      m_bitmapOverlays[i].Geometry = D2D1_RECT_F{ 0, 0, 1, 1 };
   }
}

d3d11::OverlayImage* d3d11::DeviceResources::GetOverlay(unsigned image)
{
   if (image >= m_bitmapOverlayCount)
   {
      return NULL;
   }
   return &m_bitmapOverlays[image];
}

void d3d11::DeviceResources::RenderOverlays()
{
   auto viewport = GetScreenViewport();

   m_d2dContext->SetTransform(Matrix3x2F::Identity());



   for (unsigned i = 0; i < m_bitmapOverlayCount; i++)
   {
      const auto& overlay = m_bitmapOverlays[i];

      D2D1_RECT_F rect = { 
         overlay.Geometry.left * viewport.Width, 
         overlay.Geometry.top * viewport.Height,
         (overlay.Geometry.left + overlay.Geometry.right) * viewport.Width,
         (overlay.Geometry.top + overlay.Geometry.bottom) * viewport.Height
      };

      m_d2dContext->DrawBitmap(overlay.Bitmap.Get(), &rect);
   }
}

d3d11::DeviceResources * d3d11::Get()
{
   return (d3d11::DeviceResources*)video_driver_get_ptr(false);
}