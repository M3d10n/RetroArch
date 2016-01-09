#include "cores.h"

using namespace RetroArch_Win10;

RetroArch_Win10::Core::Core()
{
   Name = "Genesis";
   Icon = "ms-appx:///media/assets/xmb/flatui/png/Sega - Mega Drive - Genesis.png";
   Background = "ms-appx:///media/assets/wallpapers/bichromatic pads/1440x900/Sega - Mega Drive - Genesis.png";
}

RetroArch_Win10::CoresViewModel::CoresViewModel()
{
   m_cores = ref new Platform::Collections::Vector<ICoreInfoDisplay^>();
   m_cores->Append(ref new Core());
   m_cores->Append(ref new Core());
   m_cores->Append(ref new Core());
   m_cores->Append(ref new Core());
   m_cores->Append(ref new Core());

   
}

Windows::Foundation::Collections::IObservableVector<ISystem^>^ CoresViewModel::Systems::get()
{
   return SystemLibrary::Get()->GetSystems();
}

ICoreInfoDisplay^ CoresViewModel::SelectedItem::get()
{
   return m_selectedItem;
}

void CoresViewModel::SelectedItem::set(ICoreInfoDisplay^ Value)
{
   m_selectedItem = Value;
}