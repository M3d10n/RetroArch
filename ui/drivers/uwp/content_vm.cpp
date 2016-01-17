#include "content_vm.h"
#include "system_library.h"
#include <ppltasks.h>

using namespace concurrency;
using namespace RetroArch_Win10;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

RetroArch_Win10::ContentViewModel::ContentViewModel()
{
   SystemLibrary::Get()->GetDispatcher()->SelectedSystemChanged += ref new SelectedSystemChangedDelegate([this](System^system)
   {
      this->PropertyChanged(this, ref new PropertyChangedEventArgs("Games"));
   });
}

void RetroArch_Win10::ContentViewModel::PickGamesToAdd()
{
   auto selectedSystem = SystemLibrary::Get()->GetSelectedSystem();
   if (selectedSystem == nullptr)
   {
      return;
   }

   auto picker = ref new Pickers::FileOpenPicker();

   for (auto fileType : selectedSystem->FileTypes)
   {
      picker->FileTypeFilter->Append(fileType);
   }

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
            game->BoxArt = selectedSystem->MediaIcon;

            GameLibrary::Get()->AddGame(game);
         });         
      }
   });
}

Game^ ContentViewModel::SelectedItem::get()
{
   return m_selectedItem;
}

void ContentViewModel::SelectedItem::set(Game^ Value)
{
   m_selectedItem = Value;
}

IObservableVector<Game^>^ ContentViewModel::Games::get()
{
   auto selectedSystem = SystemLibrary::Get()->GetSelectedSystem();

   if (selectedSystem == nullptr)
   {
      return GameLibrary::Get()->GetGames();
   }

   return GameLibrary::Get()->GetGamesBySystem(selectedSystem->Id);
}