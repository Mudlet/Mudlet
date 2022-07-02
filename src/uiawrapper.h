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

enum NotificationProcessing {
  NotificationProcessing_ImportantAll = 0,
  NotificationProcessing_ImportantMostRecent = 1,
  NotificationProcessing_All = 2,
  NotificationProcessing_MostRecent = 3,
  NotificationProcessing_CurrentThenMostRecent = 4
};

enum NotificationKind {
  NotificationKind_ItemAdded = 0,
  NotificationKind_ItemRemoved = 1,
  NotificationKind_ActionCompleted = 2,
  NotificationKind_ActionAborted = 3,
  NotificationKind_Other = 4
};

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
