//
// cores.xaml.cpp
// Implementation of the cores class
//

#include "pch.h"
#include "cores.xaml.h"
#include "content.xaml.h"

using namespace RetroArch_Win10;

using namespace Platform;
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

cores::cores()
{
   InitializeComponent();

   auto cores_vm = static_cast<CoresViewModel^>(DataContext);
   cores_vm->ItemSelected += ref new RetroArch_Win10::SystemSelectedDelegate(this, &RetroArch_Win10::cores::OnItemSelected);

   
}

void RetroArch_Win10::cores::OnItemSelected(RetroArch_Win10::ISystem ^)
{
   auto frame = static_cast<Windows::UI::Xaml::Controls::Frame^>(Parent);
   bool ok = frame->Navigate(content::typeid);

   if (ok && frame->CanGoBack)
   {
      Windows::UI::Core::SystemNavigationManager::GetForCurrentView()->AppViewBackButtonVisibility =
         Windows::UI::Core::AppViewBackButtonVisibility::Visible;
   }
}
