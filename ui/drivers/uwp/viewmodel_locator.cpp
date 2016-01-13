#include "viewmodel_locator.h"

using namespace RetroArch_Win10;

RetroArch_Win10::ViewModelLocator::ViewModelLocator()
{
   m_cores_vm = ref new CoresViewModel();
   m_content_vm = ref new ContentViewModel();
}

CoresViewModel^ ViewModelLocator::CoresVM::get()
{
   return m_cores_vm;
}


ContentViewModel^ ViewModelLocator::ContentVM::get()
{
   return m_content_vm;
}