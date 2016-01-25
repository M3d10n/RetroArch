//
// main_page.xaml.cpp
// Implementation of the main_page class
//

#include "pch.h"
#include "main_page.xaml.h"
#include "cores.xaml.h"
#include "uwp.h"


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
   swapChainPanel->CompositionScaleChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Xaml::Controls::SwapChainPanel ^, Platform::Object ^>(this, &RetroArch_Win10::main_page::OnCompositionScaleChanged);

   // Set the UI dispatcher
   d3d11::ui_dispatcher = Window::Current->CoreWindow->Dispatcher;

   // Store the swap chain panel so the D3D11 driver can grab it during start up
   d3d11::DeviceResources::SetGlobalSwapChainPanel(swapChainPanel);

   frame->Navigate(cores::typeid);

   Windows::UI::Core::SystemNavigationManager::GetForCurrentView()->GetForCurrentView()->BackRequested += 
      ref new Windows::Foundation::EventHandler<Windows::UI::Core::BackRequestedEventArgs ^>(this, &RetroArch_Win10::main_page::OnBackRequested);

   GameLibrary::Get()->GetDispatcher()->GameStarted += ref new RetroArch_Win10::GameDelegate(this, &RetroArch_Win10::main_page::OnGameStarted);
}


bool RetroArch_Win10::main_page::UseOVerlay()
{
   auto keyboardCaps = ref new Windows::Devices::Input::KeyboardCapabilities();
   auto touchCaps = ref new Windows::Devices::Input::TouchCapabilities();
   return keyboardCaps->KeyboardPresent == 0 && touchCaps->TouchPresent > 0;
}

void RetroArch_Win10::main_page::OnSizeChanged(Platform::Object ^sender, Windows::UI::Xaml::SizeChangedEventArgs ^e)
{
   if (!RetroarchMain::Instance.get() || !RetroarchMain::Instance->IsRunning())
   {
      return;
   }
   
   critical_section::scoped_lock lock(RetroarchMain::Instance->GetCriticalSection());

   d3d11::Get()->SetLogicalSize(e->NewSize);

   auto system = SystemLibrary::Get()->GetSelectedSystem();
   if (system)
   {
      if (!UseOVerlay())
      {
         RetroarchMain::Instance->ChangeOverlay("");
      }
      else if (swapChainPanel->ActualHeight > swapChainPanel->ActualWidth)
      {
         RetroarchMain::Instance->ChangeOverlay(system->PortraitOverlay);
      }
      else
      {
         RetroarchMain::Instance->ChangeOverlay(system->LandscapeOverlay);
      }
   }
}


void RetroArch_Win10::main_page::OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel ^sender, Platform::Object ^args)
{
   if (!RetroarchMain::Instance.get() || !RetroarchMain::Instance->IsRunning())
   {
      return;
   }

   critical_section::scoped_lock lock(RetroarchMain::Instance->GetCriticalSection());
   d3d11::Get()->SetCompositionScale(sender->CompositionScaleX, sender->CompositionScaleY);
}


void RetroArch_Win10::main_page::OnBackRequested(Platform::Object ^sender, Windows::UI::Core::BackRequestedEventArgs ^args)
{
   Common::Dispatcher::Get()->DispatchBackRequested(sender, args);

   if (frame->CanGoBack && args->Handled == false)
   {
      frame->GoBack();
      args->Handled = true;
   }
   else if (!frame->CanGoBack)
   {
      Windows::UI::Core::SystemNavigationManager::GetForCurrentView()->AppViewBackButtonVisibility =
         Windows::UI::Core::AppViewBackButtonVisibility::Collapsed;
   }
}


void RetroArch_Win10::main_page::OnGameStarted(Game ^game)
{
   auto system = SystemLibrary::Get()->GetSelectedSystem();
   if (system)
   {
      if (!UseOVerlay())
      {
         RetroarchMain::Instance->ChangeOverlay("");
      }
      else if (swapChainPanel->ActualHeight > swapChainPanel->ActualWidth)
      {
         RetroarchMain::Instance->ChangeOverlay(system->PortraitOverlay);
      }
      else
      {
         RetroarchMain::Instance->ChangeOverlay(system->LandscapeOverlay);
      }
   }
}
