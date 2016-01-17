#include "system_library.h"

using namespace RetroArch_Win10;

RetroArch_Win10::SystemLibrary::SystemLibrary()
{
   m_library = ref new SystemsVector();
   m_library_map = ref new SystemsMap();
   m_dispatcher = ref new SystemLibraryDispatcher();

   // Test
   auto system = ref new System();
   system->Id = ESystemId::MegaDrive;
   system->Name = "Mega Drive - Genesis";
   system->Maker = "Sega";
   system->Icon = "ms-appx:///media/assets/xmb/flatui/png/Sega - Mega Drive - Genesis.png";
   system->Background = "ms-appx:///media/assets/wallpapers/bichromatic pads/1440x900/Sega - Mega Drive - Genesis.png";
   system->Core = "genesis_plus_gx";
   system->LandscapeOverlay = "gamepads/flat/genesis-landscape.cfg";
   system->PortraitOverlay = "gamepads/flat/genesis-portrait.cfg";
   system->FileTypes->Append(".mdx");
   system->FileTypes->Append(".md");
   system->FileTypes->Append(".smd");
   system->FileTypes->Append(".gen");
   system->FileTypes->Append(".bin");
   system->FileTypes->Append(".cue");
   system->FileTypes->Append(".iso");

   RegisterSystem(system);

   system = ref new System();
   system->Id = ESystemId::MasterSystem;
   system->Name = "Master System - Mark III";
   system->Maker = "Sega";
   system->Icon = "ms-appx:///media/assets/xmb/flatui/png/Sega - Master System - Mark III.png";
   system->Background = "ms-appx:///media/assets/wallpapers/bichromatic pads/1440x900/Sega - Master System - Mark III.png";
   system->Core = "genesis_plus_gx";
   system->LandscapeOverlay = "gamepads/flat/sms-landscape.cfg";
   system->PortraitOverlay = "gamepads/flat/sms-portrait.cfg";
   system->FileTypes->Append(".bin");
   system->FileTypes->Append(".sms");
   system->FileTypes->Append(".gg");
   system->FileTypes->Append(".sg");

   RegisterSystem(system);
}

SystemLibrary * RetroArch_Win10::SystemLibrary::Get()
{
   static SystemLibrary * singleton = nullptr;

   if (singleton == nullptr)
   {
      singleton = new SystemLibrary();
   }

   return singleton;
}

void RetroArch_Win10::SystemLibrary::RegisterSystem(System ^ newSystem)
{
   if (m_library_map->HasKey(newSystem->Id))
   {
      return;
   }

   m_library->Append(newSystem);
   m_library_map->Insert(newSystem->Id, newSystem);
}

System ^ RetroArch_Win10::SystemLibrary::GetSystem(ESystemId Id)
{
   if (m_library_map->HasKey(Id))
   {
      return m_library_map->Lookup(Id);
   }
   return nullptr;
}

void RetroArch_Win10::SystemLibrary::SetSelectedSystem(System ^ system)
{
   m_selectedSystem = system;
   m_dispatcher->DispatchSelectedSystemChanged(system);
}

void RetroArch_Win10::SystemLibraryDispatcher::DispatchSelectedSystemChanged(System ^ newSystem)
{
   SelectedSystemChanged(newSystem);
}

RetroArch_Win10::System::System()
{
   FileTypes = ref new Platform::Collections::Vector<Platform::String^>();
}
