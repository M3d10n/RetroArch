#include "cores.h"

using namespace RetroArch_Win10;

RetroArch_Win10::Core::Core()
{
   Name = "Genesis";
   Icon = "ms-appx:///Assets/xmb/flatui/png/Sega - Mega Drive - Genesis.png";
}

RetroArch_Win10::CoreGroup::CoreGroup()
{
   _items = ref new Platform::Collections::Vector<Object^>();
}
