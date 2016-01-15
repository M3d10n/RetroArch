#include "content_vm.h"
#include "system_library.h"
#include <ppltasks.h>

using namespace concurrency;
using namespace RetroArch_Win10;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;

RetroArch_Win10::ContentViewModel::ContentViewModel()
{
}

void RetroArch_Win10::ContentViewModel::PickGamesToAdd()
{
   auto selectedSystem = SystemLibrary::Get()->GetSelectedSystem();
   if (selectedSystem == nullptr)
   {
      return;
   }

   auto picker = ref new Pickers::FileOpenPicker();
   picker->FileTypeFilter->Append(".zip");
   picker->FileTypeFilter->Append(".md");
   picker->FileTypeFilter->Append(".smd");
   picker->FileTypeFilter->Append(".bin");
   picker->FileTypeFilter->Append(".gen");
   picker->FileTypeFilter->Append(".7z");

   auto async = picker->PickMultipleFilesAsync();
   auto asyncTask = create_task(async);

   asyncTask.then( [=](IVectorView<StorageFile^>^ results)
   {
      for (auto file : results)
      {
         create_task(file->CopyAsync(Windows::Storage::ApplicationData::Current->LocalFolder, file->Name, NameCollisionOption::ReplaceExisting)).
            then([=](StorageFile^ copiedFile)
         {
            auto game = ref new Game();
            game->Path = copiedFile->Path;
            game->Title = copiedFile->DisplayName;
            game->System = selectedSystem->Id;

            GameLibrary::Get()->AddGame(game);
         });         
      }
   });
}

IGame^ ContentViewModel::SelectedItem::get()
{
   return m_selectedItem;
}

void ContentViewModel::SelectedItem::set(IGame^ Value)
{
   m_selectedItem = Value;
}

IObservableVector<IGame^>^ ContentViewModel::Games::get()
{
   auto selectedSystem = SystemLibrary::Get()->GetSelectedSystem();

   if (selectedSystem == nullptr)
   {
      return GameLibrary::Get()->GetGames();
   }

   return GameLibrary::Get()->GetGamesBySystem(selectedSystem->Id);
}