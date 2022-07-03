#include "utils.h"
#include "uiawrapper.h"
#include <QLibrary>
#include <QDebug>

UiaWrapper::UiaWrapper() {
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

UiaWrapper::~UiaWrapper(){};

UiaWrapper *UiaWrapper::self() {
  static UiaWrapper wrapper;
  return &wrapper;
}

BOOL UiaWrapper::ready() {
  return m_pUiaRaiseNotificationEvent && m_pUiaDisconnectAllProviders &&
         m_pUiaDisconnectProvider && m_pUiaHostProviderFromHwnd &&
         m_pUiaClientsAreListening;
}

HRESULT UiaWrapper::raiseNotificationEvent(
    IRawElementProviderSimple *provider, NotificationKind notificationKind,
    NotificationProcessing notificationProcessing, BSTR displayString,
    BSTR activityId) {
  if (!m_pUiaRaiseNotificationEvent) {
    return UIA_E_NOTSUPPORTED;
  }

  qDebug() << "calling m_pUiaRaiseNotificationEvent" << provider;
  return m_pUiaRaiseNotificationEvent(provider, notificationKind,
                                      notificationProcessing, displayString,
                                      activityId);
}

HRESULT UiaWrapper::disconnectAllProviders() {
  if (!m_pUiaDisconnectAllProviders) {
    return UIA_E_NOTSUPPORTED;
  }
  return m_pUiaDisconnectAllProviders();
}

HRESULT UiaWrapper::disconnectProvider(IRawElementProviderSimple *pProvider) {
  if (!m_pUiaDisconnectProvider) {
    return UIA_E_NOTSUPPORTED;
  }
  return m_pUiaDisconnectProvider(pProvider);
}

BOOL UiaWrapper::clientsAreListening() {
  if (!m_pUiaClientsAreListening) {
    return FALSE;
  }
  return m_pUiaClientsAreListening();
}

HRESULT
UiaWrapper::hostProviderFromHwnd(HWND hwnd,
                                 IRawElementProviderSimple **ppProvider) {
  if (!m_pUiaHostProviderFromHwnd) {
    return UIA_E_NOTSUPPORTED;
  }
  return m_pUiaHostProviderFromHwnd(hwnd, ppProvider);
}
