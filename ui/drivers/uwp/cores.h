#pragma once

#include <collection.h> 

namespace RetroArch_Win10
{
   [Windows::UI::Xaml::Data::Bindable]
   public ref class Core sealed
   {
   public:
      Core();

      property Platform::String^ Name;
      property Platform::String^ Icon;

   };

   [Windows::UI::Xaml::Data::Bindable]
   public ref class CoreGroup sealed
   {
   public:
      CoreGroup();

      void Insert(Core^ item)
      {
         _items->Append(item);
      }

      property Windows::Foundation::Collections::IObservableVector<Platform::Object^>^ Items
      {
         Windows::Foundation::Collections::IObservableVector<Platform::Object^>^ get()
         {
            return _items;
         }
      }
   private:
      Platform::Collections::Vector<Object^>^ _items;
   };

}