/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QGENERICUNIXTHEMES_H
#define QGENERICUNIXTHEMES_H

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

#include <qpa/qplatformtheme.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QFont>

QT_BEGIN_NAMESPACE

class ResourceHelper
{
public:
    ResourceHelper();
    ~ResourceHelper() { clear(); }

    void clear();

    QPalette *palettes[QPlatformTheme::NPalettes];
    QFont *fonts[QPlatformTheme::NFonts];
};

class QGenericUnixThemePrivate;

class QGenericUnixTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QGenericUnixTheme)
public:
    QGenericUnixTheme();

    static QPlatformTheme *createUnixTheme(const QString &name);
    static QStringList themeNames();

    const QFont *font(Font type) const override;
    QVariant themeHint(ThemeHint hint) const override;

    static QStringList xdgIconThemePaths();
    static QStringList iconFallbackPaths();
#ifndef QT_NO_DBUS
    QPlatformMenuBar *createPlatformMenuBar() const override;
#endif
#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif

    static const char *name;
};

#if QT_CONFIG(settings)
class QKdeThemePrivate;

class QKdeTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QKdeTheme)
public:
    QKdeTheme(const QStringList& kdeDirs, int kdeVersion);

    static QPlatformTheme *createKdeTheme();
    QVariant themeHint(ThemeHint hint) const override;

    QIcon fileIcon(const QFileInfo &fileInfo,
                   QPlatformTheme::IconOptions iconOptions = nullptr) const override;

    const QPalette *palette(Palette type = SystemPalette) const override;

    const QFont *font(Font type) const override;
#ifndef QT_NO_DBUS
    QPlatformMenuBar *createPlatformMenuBar() const override;
#endif
#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif

    static const char *name;
};
#endif // settings

class QGnomeThemePrivate;

class QGnomeTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QGnomeTheme)
public:
    QGnomeTheme();
    QVariant themeHint(ThemeHint hint) const override;
    QIcon fileIcon(const QFileInfo &fileInfo,
                   QPlatformTheme::IconOptions = nullptr) const override;
    const QFont *font(Font type) const override;
    QString standardButtonText(int button) const override;

    virtual QString gtkFontName() const;
#ifndef QT_NO_DBUS
    QPlatformMenuBar *createPlatformMenuBar() const override;
#endif
#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif

    static const char *name;
};

QPlatformTheme *qt_createUnixTheme();

QT_END_NAMESPACE

#endif // QGENERICUNIXTHEMES_H
