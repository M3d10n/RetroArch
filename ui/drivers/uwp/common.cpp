
#include "common.h"

using namespace RetroArch_Win10::Common;

using namespace Platform;
using namespace Windows::UI::Xaml::Data;


void BindableBase::OnPropertyChanged(String^ propertyName)
{
   PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
}

Windows::UI::Xaml::Data::ICustomProperty^ BindableBase::GetCustomProperty(Platform::String^ name)
{
   return nullptr;
}

Windows::UI::Xaml::Data::ICustomProperty^ BindableBase::GetIndexedProperty(Platform::String^ name, Windows::UI::Xaml::Interop::TypeName type)
{
   return nullptr;
}

Platform::String^ BindableBase::GetStringRepresentation()
{
   return this->ToString();
}

Dispatcher ^ RetroArch_Win10::Common::Dispatcher::Get()
{
   static Dispatcher ^ StaticDispatcher = ref new Dispatcher();
   return StaticDispatcher;
}
