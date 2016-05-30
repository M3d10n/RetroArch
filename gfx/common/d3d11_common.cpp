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
#include "performance.h"
#include <windows.ui.xaml.media.dxinterop.h>
#include <ppltasks.h>

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
   m_bitmapOverlayCount(0),
   m_loadingComplete(false)
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

   CreateRenderingResources();
}

// Function that reads from a binary file asynchronously.
inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
{
   using namespace Windows::Storage;
   using namespace Concurrency;

   auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

   return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([](StorageFile^ file)
   {
      return FileIO::ReadBufferAsync(file);
   }).then([](Streams::IBuffer^ fileBuffer) -> std::vector<byte>
   {
      std::vector<byte> returnBuffer;
      returnBuffer.resize(fileBuffer->Length);
      Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
      return returnBuffer;
   });
}

// Creates the resources used to displaying the core buffer to the screen
void d3d11::DeviceResources::CreateRenderingResources()
{
   // Load shaders asynchronously.
   auto loadVSTask = ReadDataAsync(L"d3d11_vs.cso");
   auto loadPSTask = ReadDataAsync(L"d3d11_ps.cso");

   // After the vertex shader file is loaded, create the shader and input layout.
   auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
      ThrowIfFailed(
         GetD3DDevice()->CreateVertexShader(
            &fileData[0],
            fileData.size(),
            nullptr,
            &m_vertexShader
         )
      );

      static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
      {
         { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      };

      ThrowIfFailed(
         GetD3DDevice()->CreateInputLayout(
            vertexDesc,
            ARRAYSIZE(vertexDesc),
            &fileData[0],
            fileData.size(),
            &m_inputLayout
         )
      );
   });

   // After the pixel shader file is loaded, create the shader and constant buffer.
   auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
      ThrowIfFailed(
         GetD3DDevice()->CreatePixelShader(
            &fileData[0],
            fileData.size(),
            nullptr,
            &m_pixelShader
         )
      );

      CD3D11_BUFFER_DESC constantBufferDesc(sizeof(DisplayMatrixConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
      ThrowIfFailed(
         GetD3DDevice()->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &m_constantBuffer
         )
      );
   });

   // Once both shaders are loaded, create the mesh.
   auto createQuadTask = (createPSTask && createVSTask).then([this]() {

      // Load mesh vertices. Each vertex has a position and a color.
      static const DirectX::XMFLOAT3 quadVertices[] =
      {
         XMFLOAT3(0.0f, 0.0f, 0.0f),
         XMFLOAT3(0.0f,  1.0f, 0.0f),
         XMFLOAT3(1.0f, 0.0f, 0.0f),
         XMFLOAT3(1.0f,  1.0f, 0.0f),
      };

      D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
      vertexBufferData.pSysMem = quadVertices;
      vertexBufferData.SysMemPitch = 0;
      vertexBufferData.SysMemSlicePitch = 0;
      CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(quadVertices), D3D11_BIND_VERTEX_BUFFER);
      ThrowIfFailed(
         GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &m_vertexBuffer
         )
      );

      // Load mesh indices. Each trio of indices represents
      // a triangle to be rendered on the screen.
      // For example: 0,2,1 means that the vertices with indexes
      // 0, 2 and 1 from the vertex buffer compose the 
      // first triangle of this mesh.
      static const unsigned short quadIndices[] =
      {
         0,1,2,
         2,1,3
      };

      D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
      indexBufferData.pSysMem = quadIndices;
      indexBufferData.SysMemPitch = 0;
      indexBufferData.SysMemSlicePitch = 0;
      CD3D11_BUFFER_DESC indexBufferDesc(sizeof(quadIndices), D3D11_BIND_INDEX_BUFFER);
      ThrowIfFailed(
         GetD3DDevice()->CreateBuffer(
            &indexBufferDesc,
            &indexBufferData,
            &m_indexBuffer
         )
      );
   });

   // Once the quad is loaded, the object is ready to be rendered.
   createQuadTask.then([this]() {
      m_loadingComplete = true;
   });

   // Setup the raster description which will determine how and what polygons will be drawn.
   D3D11_RASTERIZER_DESC rasterDesc;

   rasterDesc.AntialiasedLineEnable = false;
   rasterDesc.CullMode = D3D11_CULL_NONE;
   rasterDesc.DepthBias = 0;
   rasterDesc.DepthBiasClamp = 0.0f;
   rasterDesc.DepthClipEnable = true;
   rasterDesc.FillMode = D3D11_FILL_SOLID;
   rasterDesc.FrontCounterClockwise = false;
   rasterDesc.MultisampleEnable = false;
   rasterDesc.ScissorEnable = false;
   rasterDesc.SlopeScaledDepthBias = 0.0f;

   // Create the rasterizer state from the description we just filled out.
   ThrowIfFailed(
      GetD3DDevice()->CreateRasterizerState(&rasterDesc, &m_rasterState)
   );
   // Now set the rasterizer state.
   m_d3dContext->RSSetState(m_rasterState.Get());

   // Create a texture sampler state description.
   D3D11_SAMPLER_DESC samplerDesc;
   samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
   samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
   samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
   samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
   samplerDesc.MipLODBias = 0.0f;
   samplerDesc.MaxAnisotropy = 1;
   samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
   samplerDesc.BorderColor[0] = 0;
   samplerDesc.BorderColor[1] = 0;
   samplerDesc.BorderColor[2] = 0;
   samplerDesc.BorderColor[3] = 0;
   samplerDesc.MinLOD = 0;
   samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

   // Create the texture sampler state.
   ThrowIfFailed(
      m_d3dDevice->CreateSamplerState(&samplerDesc, &m_samplerState)
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
   m_loadingComplete = false;

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

void d3d11::DeviceResources::Render(const D2D1_MATRIX_3X2_F &displayMatrix, bool displayOverlays)
{
   // Reset the viewport to target the whole screen.
   m_d3dContext->RSSetViewports(1, &m_screenViewport);

   // Reset render targets to the screen.
   ID3D11RenderTargetView *const targets[1] = { GetBackBufferRenderTargetView() };
   m_d3dContext->OMSetRenderTargets(1, targets, GetDepthStencilView());

   // Clear the back buffer and depth stencil view.
   m_d3dContext->ClearRenderTargetView(GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
   m_d3dContext->ClearDepthStencilView(GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

   // Nothing to display
   if (!m_loadingComplete)
   {
      return;
   }

   // Update the constant buffer matrix
   XMMATRIX matrix = XMMatrixOrthographicOffCenterLH(0, 1, 1, 0, -100, 100);
   matrix = XMMatrixMultiply(XMMatrixTranslation(displayMatrix._31 / m_screenViewport.Width, displayMatrix._32 / m_screenViewport.Height, 0), matrix);
   matrix = XMMatrixMultiply(XMMatrixScaling(displayMatrix._11, displayMatrix._22, 1), matrix);
   
   XMStoreFloat4x4(&m_constantBufferData.display, XMMatrixTranspose(matrix));

   // Prepare the constant buffer to send it to the graphics device.
   m_d3dContext->UpdateSubresource1(
      m_constantBuffer.Get(),
      0,
      NULL,
      &m_constantBufferData,
      0,
      0,
      0
   );

   // Each vertex is one instance of the VertexPositionColor struct.
   UINT stride = sizeof(XMFLOAT3);
   UINT offset = 0;
   m_d3dContext->IASetVertexBuffers(
      0,
      1,
      m_vertexBuffer.GetAddressOf(),
      &stride,
      &offset
   );

   m_d3dContext->IASetIndexBuffer(
      m_indexBuffer.Get(),
      DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
      0
   );

   m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   m_d3dContext->IASetInputLayout(m_inputLayout.Get());

   // Attach our vertex shader.
   m_d3dContext->VSSetShader(
      m_vertexShader.Get(),
      nullptr,
      0
   );

   // Send the constant buffer to the graphics device.
   m_d3dContext->VSSetConstantBuffers1(
      0,
      1,
      m_constantBuffer.GetAddressOf(),
      nullptr,
      nullptr
   );

   // Attach our pixel shader.
   m_d3dContext->PSSetShader(
      m_pixelShader.Get(),
      nullptr,
      0
   );

   // Set samplers
   m_d3dContext->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

   // Set texture
   m_d3dContext->PSSetShaderResources(0, 1, m_textureDisplayRSV.GetAddressOf());

   // Draw the objects.
   m_d3dContext->DrawIndexed(
      6,
      0,
      0
   );

   // Draw overlays
   m_d2dContext->BeginDraw();
   m_d2dContext->SetTransform(displayMatrix);
#ifdef HAVE_OVERLAY
   if (displayOverlays)
   {
      RenderOverlays();
   }
#endif
   d3d11::ThrowIfFailed(m_d2dContext->EndDraw());
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

void d3d11::DeviceResources::SetFrameTexture(const void * frame, bool rgb32, unsigned width, unsigned height, unsigned pitch)
{
   HRESULT hr;

   D3D11_TEXTURE2D_DESC desc;
   if (m_textureDisplay.Get())
   {
      m_textureDisplay->GetDesc(&desc);
   }

   if (!m_textureDisplay.Get() || desc.Width != width || desc.Height != height)
   {
      // Release the textures
      m_textureDisplay.Reset();
      m_textureStaging.Reset();
      m_textureDisplayRSV.Reset();

      unsigned targetPitch = width * (rgb32 ? 4 : 2);

      // Setup texture desc
      desc.Width = width;
      desc.Height = height;
      desc.MipLevels = 1; 
      desc.ArraySize = 1;
      desc.Format = rgb32 ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_B5G6R5_UNORM;
      desc.CPUAccessFlags = 0; 
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.MiscFlags = 0;
      desc.SampleDesc.Count = 1; 
      desc.SampleDesc.Quality = 0;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

      // Setup initial resource
      D3D11_SUBRESOURCE_DATA sr;
      sr.pSysMem = frame;
      sr.SysMemPitch = targetPitch;
      sr.SysMemSlicePitch = targetPitch * height;
      
      // Create display texture
      hr = m_d3dDevice->CreateTexture2D(&desc, &sr, &m_textureDisplay);
      d3d11::ThrowIfFailed(hr);

      // Create the display texture resource view
      hr = m_d3dDevice->CreateShaderResourceView(m_textureDisplay.Get(), nullptr, &m_textureDisplayRSV);
      d3d11::ThrowIfFailed(hr);

      // Change desc for staging texture
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;


      // Create staging texture
      hr = m_d3dDevice->CreateTexture2D(&desc, &sr, &m_textureStaging);
      d3d11::ThrowIfFailed(hr);
   }

   // Update the staging texture   
   D3D11_MAPPED_SUBRESOURCE mapped;
   m_d3dContext->Map(m_textureStaging.Get(), 0, D3D11_MAP_READ_WRITE, 0, &mapped);
   unsigned char* dst = (unsigned char*)mapped.pData;
   unsigned char* src = (unsigned char*)frame;
   unsigned int copyPitch = min(mapped.RowPitch, pitch);
   for (unsigned i = 0; i < height; i++)
   {
      memcpy(&dst[mapped.RowPitch * i], &src[pitch * i], copyPitch);
   }
   m_d3dContext->Unmap(m_textureStaging.Get(), 0);
   
   // Update the display texture
   m_d3dContext->CopyResource(m_textureDisplay.Get(), m_textureStaging.Get());
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

   static struct retro_perf_counter perf_convert = { 0 };
   rarch_perf_init(&perf_convert, "update_bitmap");
   retro_perf_start(&perf_convert);

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

   retro_perf_stop(&perf_convert);

   static struct retro_perf_counter perf_copy = { 0 };
   rarch_perf_init(&perf_copy, "copy_bitmap");
   retro_perf_start(&perf_copy);

   // Copy to the GPU bitmap
   hr = bitmap->CopyFromMemory(NULL, bits, dst_pitch);
   d3d11::ThrowIfFailed(hr);

   retro_perf_stop(&perf_copy);

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
