#include "cores.h"

using namespace RetroArch_Win10;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;


RetroArch_Win10::CoresViewModel::CoresViewModel()
{
      
}

void RetroArch_Win10::CoresViewModel::SetSelectedSystem(System ^ Value)
{
   SystemLibrary::Get()->SetSelectedSystem(Value);
   ItemSelected(Value);
   PropertyChanged(this, ref new PropertyChangedEventArgs("SelectedSystem"));
}

System^ RetroArch_Win10::CoresViewModel::SelectedSystem::get()
{
   return SystemLibrary::Get()->GetSelectedSystem();
}

Windows::Foundation::Collections::IObservableVector<System^>^ CoresViewModel::Systems::get()
{
   return SystemLibrary::Get()->GetSystems();
}
