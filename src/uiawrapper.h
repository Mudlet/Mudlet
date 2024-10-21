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

#ifndef UIAWRAPPER_H
#define UIAWRAPPER_H

#include <memory>
#include <ole2.h>
#include <tlhelp32.h>
#include <uiautomation.h>
#include <utility>

#include <oleacc.h>
#include <uiautomationclient.h>
#include <uiautomationcore.h>
#include <uiautomationcoreapi.h>


class UiaWrapper {
  UiaWrapper();
  virtual ~UiaWrapper();

public:
  static UiaWrapper *self();
  BOOL ready();
  LRESULT returnRawElementProvider(HWND hwnd, WPARAM wParam, LPARAM lParam,
                                   IRawElementProviderSimple *el);
  HRESULT raiseNotificationEvent(IRawElementProviderSimple *provider,
                                 NotificationKind notificationKind,
                                 NotificationProcessing notificationProcessing,
                                 BSTR displayString, BSTR activityId);
  HRESULT disconnectAllProviders();
  HRESULT disconnectProvider(IRawElementProviderSimple *pProvider);
  HRESULT hostProviderFromHwnd(HWND hwnd,
                               IRawElementProviderSimple **ppProvider);
  BOOL clientsAreListening();

private:
  typedef HRESULT(WINAPI *PtrUiaRaiseNotificationEvent)(
      IRawElementProviderSimple *, NotificationKind, NotificationProcessing,
      BSTR, BSTR);
  typedef HRESULT(WINAPI *PtrUiaDisconnectAllProviders)();
  typedef HRESULT(WINAPI *PtrUiaDisconnectProvider)(
      IRawElementProviderSimple *);
  typedef HRESULT(WINAPI *PtrUiaHostProviderFromHwnd)(
      HWND, IRawElementProviderSimple **);
  typedef BOOL(WINAPI *PtrUiaClientsAreListening)();
  PtrUiaRaiseNotificationEvent m_pUiaRaiseNotificationEvent = nullptr;
  PtrUiaDisconnectAllProviders m_pUiaDisconnectAllProviders = nullptr;
  PtrUiaDisconnectProvider m_pUiaDisconnectProvider = nullptr;
  PtrUiaHostProviderFromHwnd m_pUiaHostProviderFromHwnd = nullptr;
  PtrUiaClientsAreListening m_pUiaClientsAreListening = nullptr;
};

#endif // UIAWRAPPER_H
