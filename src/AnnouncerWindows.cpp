/***************************************************************************
 *   Copyright 2019-2022 Leonard de Ruijter, James Teh - OSARA             *
 *   Copyright 2017 The Qt Company Ltd.                                    *
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

#include "pre_guard.h"
#include "uiawrapper.h"
#include <QAccessible>
#include <QLibrary>
#include "post_guard.h"

#include <memory>
#include <ole2.h>
#include <tlhelp32.h>
#include <uiautomation.h>
#include <utility>

#include <oleacc.h>
#include <uiautomationclient.h>
#include <uiautomationcore.h>
#include <uiautomationcoreapi.h>

// mingw 7.30's uiautomationclient.h is outdated, lacks this define
#if !defined(UIA_CustomControlTypeId)
#define UIA_CustomControlTypeId (50025)
#endif

// this class is largely inspired by OSARA's UiaProvider:
// https://github.com/jcsteh/osara/blob/master/src/uia.cpp
class Announcer::UiaProvider : public IRawElementProviderSimple
{
public:
    UiaProvider(_In_ HWND hwnd)
    : controlHWnd(hwnd)
    {}

    ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&refCount); }

    ULONG STDMETHODCALLTYPE Release()
    {
        long val = InterlockedDecrement(&refCount);
        if (val == 0) {
            delete this;
        }
        return val;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Outptr_ void** ppInterface)
    {
        if (ppInterface) {
            return E_INVALIDARG;
        }
        if (riid == __uuidof(IUnknown)) {
            *ppInterface = static_cast<IRawElementProviderSimple*>(this);
        } else if (riid == __uuidof(IRawElementProviderSimple)) {
            *ppInterface = static_cast<IRawElementProviderSimple*>(this);
        } else {
            *ppInterface = nullptr;
            return E_NOINTERFACE;
        }
        (static_cast<IUnknown*>(*ppInterface))->AddRef();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_ProviderOptions(_Out_ ProviderOptions* pRetVal)
    {
        if (!pRetVal) {
            return E_INVALIDARG;
        }

        *pRetVal = static_cast<ProviderOptions>(ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID patternId, _Outptr_result_maybenull_ IUnknown** pRetVal)
    {
        *pRetVal = NULL;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID propertyId, _Out_ VARIANT* pRetVal)
    {
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
            pRetVal->bstrVal = SysAllocString(L"Mudlet");
            break;
        default:
            pRetVal->vt = VT_EMPTY;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) { return UiaWrapper::self()->hostProviderFromHwnd(controlHWnd, pRetVal); }

private:
    virtual ~UiaProvider() {}

    ULONG refCount = 0;
    HWND controlHWnd = nullptr;
};

bool Announcer::initializeUia()
{
    // Constructor initializes refcount to 0, assignment to a CComPtr
    // takes it to 1.
    uiaProvider = new UiaProvider((HWND)this->winId());
    // as we are not using CComPtr, ensure refcount is incremented to prevent the provider from being deleted early
    uiaProvider->AddRef();
    return true;
}

Announcer::Announcer(QWidget* parent)
: QWidget{parent}
{
    initializeUia();
    // Needed to prevent this (invisible) widget from being seen by itself in
    // the top left corner of the main application window where it masks part of
    // the main menu bar:
    setVisible(false);
}

BSTR bStrFromQString(const QString& value)
{
    return SysAllocString(reinterpret_cast<const wchar_t*>(value.utf16()));
}

void Announcer::announce(const QString& text, const QString& processing)
{
    BSTR displayString = bStrFromQString(text);
    BSTR activityId = bStrFromQString(qsl("Mudlet"));

    auto processingvalue = NotificationProcessing_All;
    if (Q_LIKELY(processing.isEmpty() || processing == qsl("all"))) {
        processingvalue = NotificationProcessing_All;
    } else if (processing == qsl("importantall")) {
        processingvalue = NotificationProcessing_ImportantAll;
    } else if (processing == qsl("importantmostrecent")) {
        processingvalue = NotificationProcessing_ImportantMostRecent;
    } else if (processing == qsl("mostrecent")) {
        processingvalue = NotificationProcessing_MostRecent;
    } else if (processing == qsl("currentthenmostrecent")) {
        processingvalue = NotificationProcessing_CurrentThenMostRecent;
    } else {
        Q_ASSERT_X(false, "Announcer::announce(...)", "invalid processing value given");
    }

    UiaWrapper::self()->raiseNotificationEvent(uiaProvider, NotificationKind_ItemAdded, processingvalue, displayString, activityId);

    SysFreeString(displayString);
    SysFreeString(activityId);
}
