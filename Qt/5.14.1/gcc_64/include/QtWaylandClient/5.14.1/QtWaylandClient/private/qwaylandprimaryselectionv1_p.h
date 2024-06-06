/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWAYLANDPRIMARYSELECTIONV1_P_H
#define QWAYLANDPRIMARYSELECTIONV1_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWaylandClient/private/qwayland-wp-primary-selection-unstable-v1.h>

#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>
#include <QtWaylandClient/private/qwaylanddataoffer_p.h>

#include <QtCore/QObject>

QT_REQUIRE_CONFIG(wayland_client_primary_selection);

QT_BEGIN_NAMESPACE

class QMimeData;

namespace QtWaylandClient {

class QWaylandInputDevice;
class QWaylandPrimarySelectionDeviceV1;

class QWaylandPrimarySelectionDeviceManagerV1 : public QtWayland::zwp_primary_selection_device_manager_v1
{
public:
    explicit QWaylandPrimarySelectionDeviceManagerV1(QWaylandDisplay *display, uint id, uint version);
    QWaylandPrimarySelectionDeviceV1 *createDevice(QWaylandInputDevice *seat);
    QWaylandDisplay *display() const { return m_display; }

private:
    QWaylandDisplay *m_display = nullptr;
};

class QWaylandPrimarySelectionOfferV1 : public QtWayland::zwp_primary_selection_offer_v1, public QWaylandAbstractDataOffer
{
public:
    explicit QWaylandPrimarySelectionOfferV1(QWaylandDisplay *display, ::zwp_primary_selection_offer_v1 *offer);
    ~QWaylandPrimarySelectionOfferV1() override { destroy(); }
    void startReceiving(const QString &mimeType, int fd) override;
    QMimeData *mimeData() override { return m_mimeData.data(); }

protected:
    void zwp_primary_selection_offer_v1_offer(const QString &mime_type) override;

private:
    QWaylandDisplay *m_display = nullptr;
    QScopedPointer<QWaylandMimeData> m_mimeData;
};

class Q_WAYLAND_CLIENT_EXPORT QWaylandPrimarySelectionSourceV1 : public QObject, public QtWayland::zwp_primary_selection_source_v1
{
    Q_OBJECT
public:
    explicit QWaylandPrimarySelectionSourceV1(QWaylandPrimarySelectionDeviceManagerV1 *manager, QMimeData *mimeData);
    ~QWaylandPrimarySelectionSourceV1() override;

    QMimeData *mimeData() const { return m_mimeData; }

signals:
    void cancelled();

protected:
    void zwp_primary_selection_source_v1_send(const QString &mime_type, int32_t fd) override;
    void zwp_primary_selection_source_v1_cancelled() override { emit cancelled(); }

private:
    QWaylandDisplay *m_display = nullptr;
    QMimeData *m_mimeData = nullptr;
};

class QWaylandPrimarySelectionDeviceV1 : public QObject, public QtWayland::zwp_primary_selection_device_v1
{
    Q_OBJECT
    QWaylandPrimarySelectionDeviceV1(QWaylandPrimarySelectionDeviceManagerV1 *manager, QWaylandInputDevice *seat);

public:
    ~QWaylandPrimarySelectionDeviceV1() override;
    QWaylandPrimarySelectionOfferV1 *selectionOffer() const { return m_selectionOffer.data(); }
    void invalidateSelectionOffer();
    QWaylandPrimarySelectionSourceV1 *selectionSource() const { return m_selectionSource.data(); }
    void setSelectionSource(QWaylandPrimarySelectionSourceV1 *source);

protected:
    void zwp_primary_selection_device_v1_data_offer(struct ::zwp_primary_selection_offer_v1 *offer) override;
    void zwp_primary_selection_device_v1_selection(struct ::zwp_primary_selection_offer_v1 *id) override;

private:
    QWaylandDisplay *m_display = nullptr;
    QWaylandInputDevice *m_seat = nullptr;
    QScopedPointer<QWaylandPrimarySelectionOfferV1> m_selectionOffer;
    QScopedPointer<QWaylandPrimarySelectionSourceV1> m_selectionSource;
    friend class QWaylandPrimarySelectionDeviceManagerV1;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDPRIMARYSELECTIONV1_P_H
