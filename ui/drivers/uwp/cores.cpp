#include "cores.h"

using namespace RetroArch_Win10;



RetroArch_Win10::CoresViewModel::CoresViewModel()
{
      
}

Windows::Foundation::Collections::IObservableVector<ISystem^>^ CoresViewModel::Systems::get()
{
   return SystemLibrary::Get()->GetSystems();
}

ISystem^ CoresViewModel::SelectedItem::get()
{
   return m_selectedItem;
}

void CoresViewModel::SelectedItem::set(ISystem^ Value)
{
   m_selectedItem = Value;
   SystemLibrary::Get()->SetSelectedSystem(Value);
   ItemSelected(Value);
}