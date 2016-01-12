#include "content_vm.h"

using namespace RetroArch_Win10;

RetroArch_Win10::ContentViewModel::ContentViewModel()
{
}

IContentDisplay^ ContentViewModel::SelectedItem::get()
{
   return m_selectedItem;
}

void ContentViewModel::SelectedItem::set(IContentDisplay^ Value)
{
   m_selectedItem = Value;
}