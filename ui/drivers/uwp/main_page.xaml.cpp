﻿//
// main_page.xaml.cpp
// Implementation of the main_page class
//

#include "pch.h"
#include "main_page.xaml.h"


using namespace RetroArch_Win10;

using namespace Platform;
using namespace concurrency;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

main_page::main_page()
{
	InitializeComponent();

   swapChainPanel->SizeChanged += ref new Windows::UI::Xaml::SizeChangedEventHandler(this, &RetroArch_Win10::main_page::OnSizeChanged);

   // Set the UI dispatcher
   d3d11::ui_dispatcher = Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher;

   // Store the swap chain panel so the D3D11 driver can grab it during start up
   d3d11::DeviceResources::SetGlobalSwapChainPanel(swapChainPanel);

   // Create our "main"
   m_main = std::unique_ptr<RetroarchMain>(new RetroarchMain(""));
   m_main->StartUpdateThread();
}


void RetroArch_Win10::main_page::OnSizeChanged(Platform::Object ^sender, Windows::UI::Xaml::SizeChangedEventArgs ^e)
{
   if (!m_main->IsInitialized())
   {
      return;
   }
   
   critical_section::scoped_lock lock(m_main->GetCriticalSection());
   d3d11::Get()->SetLogicalSize(e->NewSize);
}
