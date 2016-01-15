﻿#pragma once

#include "gfx/common/d3d11_common.h"

namespace RetroArch_Win10
{
   d3d11::DeviceResources* GetResources();

   //public interface ISystem;

   class RetroarchMain : public d3d11::IDeviceNotify
   {
   public:
      RetroarchMain(Platform::String^ core, Platform::String^ content);
      ~RetroarchMain();

      void StartUpdateThread();
      void StopUpdateThread();

      Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }
      bool IsInitialized();

      static std::unique_ptr<RetroarchMain> Instance;


      Platform::String^ GetEntryPoint() { return m_entryPoint; }

      //void SetSystem(ISystem^ system) { m_system = system; }

      // IDeviceNotify
      virtual void OnDeviceLost();
      virtual void OnDeviceRestored();

   private:
      bool m_initialized;
      bool m_running;
      bool m_shutdown;

      Windows::Foundation::IAsyncAction^ m_updateWorker;
      Platform::String^ m_entryPoint;

      Platform::String^ m_core;
      Platform::String^ m_content;

      //ISystem^ m_system;


      Concurrency::critical_section m_criticalSection;
      Concurrency::critical_section m_initCriticalSection;
   };

}
