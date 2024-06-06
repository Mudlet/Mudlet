/***************************************************************************
 **
 ** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
 ** Contact: https://www.qt.io/licensing/
 **
 ** This file is part of the QtNfc module of the Qt Toolkit.
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

#ifndef QNEARFIELDSHAREMANAGER_H
#define QNEARFIELDSHAREMANAGER_H

#include <QtCore/QObject>
#include <QtNfc/qtnfcglobal.h>

QT_BEGIN_NAMESPACE

class QNearFieldShareManagerPrivate;
class QNearFieldShareTarget;

class Q_NFC_EXPORT QNearFieldShareManager : public QObject
{
    Q_OBJECT

public:
    explicit QNearFieldShareManager(QObject *parent = nullptr);
    ~QNearFieldShareManager();

    enum ShareError {
        NoError,
        UnknownError,
        InvalidShareContentError,
        ShareCanceledError,
        ShareInterruptedError,
        ShareRejectedError,
        UnsupportedShareModeError,
        ShareAlreadyInProgressError,
        SharePermissionDeniedError
    };
    Q_ENUM(ShareError)

    enum ShareMode {
        NoShare = 0x00,
        NdefShare = 0x01,
        FileShare = 0x02
    };
    Q_ENUM(ShareMode)
    Q_DECLARE_FLAGS(ShareModes, ShareMode)

public:
    static QNearFieldShareManager::ShareModes supportedShareModes();
    void setShareModes(ShareModes modes);
    QNearFieldShareManager::ShareModes shareModes() const;
    QNearFieldShareManager::ShareError shareError() const;

Q_SIGNALS:
    void targetDetected(QNearFieldShareTarget* shareTarget);
    void shareModesChanged(QNearFieldShareManager::ShareModes modes);
    void error(QNearFieldShareManager::ShareError error);

private:
    QScopedPointer<QNearFieldShareManagerPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(QNearFieldShareManager)
    Q_DISABLE_COPY(QNearFieldShareManager)

    friend class QNearFieldShareManagerPrivateImpl;
    friend class QNearFieldShareTargetPrivateImpl;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QNearFieldShareManager::ShareModes)

QT_END_NAMESPACE

#endif /* QNEARFIELDSHAREMANAGER_H */
