#include "viewmodel_locator.h"

using namespace RetroArch_Win10;

RetroArch_Win10::ViewModelLocator::ViewModelLocator()
{
}

CoresViewModel^ ViewModelLocator::CoresVM::get()
{
   return ref new CoresViewModel();
}


ContentViewModel^ ViewModelLocator::ContentVM::get()
{
   return ref new ContentViewModel();
}