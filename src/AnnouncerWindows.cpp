/***************************************************************************
 *   Copyright 2019-2022 Leonard de Ruijter, James Teh - OSARA             *
 *   Copyright (C) 2022 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "Announcer.h"
#include "mudlet.h"
#include "uiawrapper.h"

#include <QAccessible>
#include <QDebug>
#include <QLibrary>

#include <memory>
#include <ole2.h>
#include <tlhelp32.h>
#include <uiautomation.h>
#include <utility>

#include <oleacc.h>
#include <uiautomationclient.h>
#include <uiautomationcore.h>
#include <uiautomationcoreapi.h>

///////////////////
using namespace std;

HWND uiaWnd = nullptr;

// mingw 7.30's uiautomationclient.h is outdated, lacks this define
#define UIA_CustomControlTypeId (50025)

// Some UIA functions aren't available in earlier versions of Windows, so we
// must fetch those at runtime. Otherwise, it will fail to load. This class
// handles loading/freeing the dll and getting the required functions.
class UiaCore {
private:
  HMODULE dll = LoadLibraryA("UIAutomationCore.dll");

  template <typename FuncType> FuncType *getFunc(const char *funcName) {
    return (FuncType *)GetProcAddress(this->dll, funcName);
  }

public:
  ~UiaCore() { FreeLibrary(this->dll); }
};

unique_ptr<UiaCore> uiaCore;

// Provider code based on Microsoft's uiautomationSimpleProvider example.
class Announcer::UiaProvider : public IRawElementProviderSimple {
public:
  UiaProvider(_In_ HWND hwnd) : refCount(0), controlHWnd(hwnd) {
      qDebug() << "initialized provider for Mudlet with" << hwnd;
  }

  // IUnknown methods
  ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&refCount); }

  ULONG STDMETHODCALLTYPE Release() {
    long val = InterlockedDecrement(&refCount);
    if (val == 0) {
      delete this;
    }
    return val;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid,
                                           _Outptr_ void **ppInterface) {
    if (ppInterface) {
      return E_INVALIDARG;
    }
    if (riid == __uuidof(IUnknown)) {
      *ppInterface = static_cast<IRawElementProviderSimple *>(this);
    } else if (riid == __uuidof(IRawElementProviderSimple)) {
      *ppInterface = static_cast<IRawElementProviderSimple *>(this);
    } else {
      *ppInterface = nullptr;
      return E_NOINTERFACE;
    }
    (static_cast<IUnknown *>(*ppInterface))->AddRef();
    return S_OK;
  }

  // IRawElementProviderSimple methods
  HRESULT STDMETHODCALLTYPE
  get_ProviderOptions(_Out_ ProviderOptions *pRetVal) {
    if (!pRetVal)
      return E_INVALIDARG;
    // We are STA, (OleInitialize()).
    *pRetVal = static_cast<ProviderOptions>(ProviderOptions_ServerSideProvider |
                                            ProviderOptions_UseComThreading);
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetPatternProvider(
      PATTERNID patternId, _Outptr_result_maybenull_ IUnknown **pRetVal) {
    qDebug() << "UIA requested patternId " << patternId;
    *pRetVal = NULL;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID propertyId,
                                             _Out_ VARIANT *pRetVal) {
    switch (propertyId) {
    case UIA_ControlTypePropertyId:
      // Stop Narrator from ever speaking this as a window
      pRetVal->vt = VT_I4;
      pRetVal->lVal = UIA_CustomControlTypeId;
      break;
    case UIA_IsControlElementPropertyId:
    case UIA_IsContentElementPropertyId:
    case UIA_IsKeyboardFocusablePropertyId:
      pRetVal->vt = VT_BOOL;
      pRetVal->boolVal = VARIANT_FALSE;
      break;
    case UIA_ProviderDescriptionPropertyId:
      pRetVal->vt = VT_BSTR;
      pRetVal->bstrVal = SysAllocString(L"REAPER OSARA");
      break;
    default:
      pRetVal->vt = VT_EMPTY;
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE
  get_HostRawElementProvider(IRawElementProviderSimple **pRetVal) {
    return UiaWrapper::self()->hostProviderFromHwnd(controlHWnd, pRetVal);
  }

private:
  virtual ~UiaProvider() {}

  ULONG refCount;   // Ref Count for this COM object
  HWND controlHWnd; // The HWND for the control.
};

bool Announcer::initializeUia() {
  uiaCore = make_unique<UiaCore>();

  // Constructor  initializes refcount to 0, assignment to a CComPtr
  // takes it to 1.
  qDebug() << "HWND for Mudlet is" << (HWND)mudlet::self()->effectiveWinId();
  uiaProvider = new UiaProvider((HWND)mudlet::self()->effectiveWinId());
  return true;
}

bool Announcer::terminateUia() {
  if (uiaProvider) {
    // Null out uiaProvider so it can't be returned by WM_GETOBJECT during
    // disconnection.
    UiaProvider *tmpProv = std::move(uiaProvider);
    uiaProvider = nullptr;
    UiaWrapper::self()->disconnectProvider(tmpProv);
  }

  UiaWrapper::self();
  uiaCore = nullptr;
  return true;
}

// bool Announcer::sendUiaNotification(const string& message, bool interrupt) {
//    if (!UiaClientsAreListening() || message.empty()) {
//        return true;
//    }
//    return (raiseNotificationEvent(
//        uiaProvider,
//        NotificationKind::NotificationKind_Other,
//        interrupt ? NotificationProcessing::NotificationProcessing_MostRecent
//        : NotificationProcessing::NotificationProcessing_All,
//        SysAllocString(widen(message).c_str()),
//        SysAllocString(L"REAPER_OSARA")
//    ) == S_OK);
//}

///////////////////

Announcer::Announcer(QWidget *parent) : QWidget{parent} { initializeUia(); }

BSTR bStrFromQString(const QString &value) {
  return SysAllocString(reinterpret_cast<const wchar_t *>(value.utf16()));
}

void Announcer::announce(const QString text) {
  // check UiaClientsAreListening here or much earlier on
  qDebug() << "announce" << text;

  BSTR displayString = bStrFromQString(text);
  BSTR activityId = bStrFromQString(qsl("Mudlet"));

  UiaWrapper::self()->raiseNotificationEvent(
      uiaProvider, NotificationKind_ItemAdded, NotificationProcessing_All,
      displayString, activityId);

  ::SysFreeString(displayString);
  ::SysFreeString(activityId);
}
