#pragma once

namespace RetroArch_Win10
{
   public enum class FileImportStatus
   {
      Pending,
      Working,
      Completed,
      Canceled,
      Error,
   };


   namespace Common
   {
      public ref class Dispatcher sealed
      {
      public:
         static Dispatcher^ Get();

         event Windows::Foundation::EventHandler<Windows::UI::Core::BackRequestedEventArgs^>^ BackRequested;
         void DispatchBackRequested(Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ args) { BackRequested(sender, args); }
      };

      [Windows::Foundation::Metadata::WebHostHidden]
      public ref class BindableBase : Windows::UI::Xaml::DependencyObject, Windows::UI::Xaml::Data::INotifyPropertyChanged, Windows::UI::Xaml::Data::ICustomPropertyProvider
      {
      public:
         virtual event Windows::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

      public:
         // ICustomPropertyProvider
         virtual Windows::UI::Xaml::Data::ICustomProperty^ GetCustomProperty(Platform::String^ name);
         virtual Windows::UI::Xaml::Data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, Windows::UI::Xaml::Interop::TypeName type);
         virtual Platform::String^ GetStringRepresentation();

         property Windows::UI::Xaml::Interop::TypeName Type
         {
            virtual Windows::UI::Xaml::Interop::TypeName get() { return this->GetType(); }
         }


      protected:
         virtual void OnPropertyChanged(Platform::String^ propertyName);
      };

      public ref class BooleanVisibilityConverter  sealed : Windows::UI::Xaml::Data::IValueConverter
      {
         // This converts the DateTime object to the Platform::String^ to display.
      public:
         property bool Negate;

         virtual Platform::Object^ Convert(Platform::Object^ value, Windows::UI::Xaml::Interop::TypeName targetType,
            Platform::Object^ parameter, Platform::String^ language)
         {
            bool b = safe_cast<bool>(value);
            if (Negate)
            {
               b = !b;
            }
            return b ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
         }

         // No need to implement converting back on a one-way binding 
         virtual Platform::Object^ ConvertBack(Platform::Object^ value, Windows::UI::Xaml::Interop::TypeName targetType,
            Platform::Object^ parameter, Platform::String^ language)
         {
            throw ref new Platform::NotImplementedException();
         }
      };

   }
}