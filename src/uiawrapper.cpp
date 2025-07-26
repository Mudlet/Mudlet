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

#include "uiawrapper.h"
#include "utils.h"
#include <QDebug>
#include <QLibrary>

// this class is largely inspired from Qt's QWindowsUiaWrapper:
// https://github.com/qt/qtbase/blob/dev/src/gui/accessible/windows/apisupport/qwindowsuiawrapper.cpp
UiaWrapper::UiaWrapper()
{
    QLibrary uiaLibrary(qsl("UIAutomationCore"));
    if (uiaLibrary.load()) {
        m_pUiaRaiseNotificationEvent =
                reinterpret_cast<PtrUiaRaiseNotificationEvent>(
                        uiaLibrary.resolve("UiaRaiseNotificationEvent"));
        m_pUiaDisconnectAllProviders =
                reinterpret_cast<PtrUiaDisconnectAllProviders>(
                        uiaLibrary.resolve("UiaDisconnectAllProviders"));
        m_pUiaDisconnectProvider = reinterpret_cast<PtrUiaDisconnectProvider>(
                uiaLibrary.resolve("UiaDisconnectProvider"));
        m_pUiaHostProviderFromHwnd = reinterpret_cast<PtrUiaHostProviderFromHwnd>(
                uiaLibrary.resolve("UiaHostProviderFromHwnd"));
        m_pUiaClientsAreListening = reinterpret_cast<PtrUiaClientsAreListening>(
                uiaLibrary.resolve("UiaClientsAreListening"));
    }
}

UiaWrapper::~UiaWrapper()
{}

UiaWrapper *UiaWrapper::self()
{
    static UiaWrapper wrapper;
    return &wrapper;
}

BOOL UiaWrapper::ready()
{
    return m_pUiaRaiseNotificationEvent && m_pUiaDisconnectAllProviders &&
           m_pUiaDisconnectProvider && m_pUiaHostProviderFromHwnd &&
           m_pUiaClientsAreListening;
}

HRESULT UiaWrapper::raiseNotificationEvent(
        IRawElementProviderSimple *provider, NotificationKind notificationKind,
        NotificationProcessing notificationProcessing, BSTR displayString,
        BSTR activityId)
{
    if (!m_pUiaRaiseNotificationEvent) {
        return UIA_E_NOTSUPPORTED;
    }

    return m_pUiaRaiseNotificationEvent(provider, notificationKind,
                                        notificationProcessing, displayString,
                                        activityId);
}

HRESULT UiaWrapper::disconnectAllProviders()
{
    if (!m_pUiaDisconnectAllProviders) {
        return UIA_E_NOTSUPPORTED;
    }
    return m_pUiaDisconnectAllProviders();
}

HRESULT UiaWrapper::disconnectProvider(IRawElementProviderSimple *pProvider)
{
    if (!m_pUiaDisconnectProvider) {
        return UIA_E_NOTSUPPORTED;
    }
    return m_pUiaDisconnectProvider(pProvider);
}

BOOL UiaWrapper::clientsAreListening()
{
    if (!m_pUiaClientsAreListening) {
        return FALSE;
    }
    return m_pUiaClientsAreListening();
}

HRESULT UiaWrapper::hostProviderFromHwnd(HWND hwnd,
                                         IRawElementProviderSimple **ppProvider)
{
    if (!m_pUiaHostProviderFromHwnd) {
        return UIA_E_NOTSUPPORTED;
    }
    return m_pUiaHostProviderFromHwnd(hwnd, ppProvider);
}
