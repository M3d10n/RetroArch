//
// cores.xaml.cpp
// Implementation of the cores class
//

#include "pch.h"
#include "cores.xaml.h"

using namespace RetroArch_Win10;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

cores::cores()
{
	InitializeComponent();


   _items = ref new Platform::Collections::Vector<Object^>();

   _items->Append(ref new Core());
   _items->Append(ref new Core());
   _items->Append(ref new Core());

   gridView->ItemsSource = _items;
}
