/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef DEVICEPROFILE_H
#define DEVICEPROFILE_H

#include "shared_global_p.h"

#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QWidget;
class QStyle;

namespace qdesigner_internal {

class DeviceProfileData;

/* DeviceProfile for embedded design. They influence
 * default properties (for example, fonts), dpi and
 * style of the form. This class represents a device
 * profile. */

class QDESIGNER_SHARED_EXPORT DeviceProfile {
public:
    DeviceProfile();

    DeviceProfile(const DeviceProfile&);
    DeviceProfile& operator=(const DeviceProfile&);
    ~DeviceProfile();

    void clear();

    // Device name
    QString name() const;
    void setName(const QString &);

    // System settings active
    bool isEmpty() const;

    // Default font family of the embedded system
    QString fontFamily() const;
    void setFontFamily(const QString &);

    // Default font size of the embedded system
    int fontPointSize() const;
    void setFontPointSize(int p);

    // Display resolution of the embedded system
    int dpiX() const;
    void setDpiX(int d);
    int dpiY() const;
    void setDpiY(int d);

    // Style
    QString style() const;
    void setStyle(const QString &);

    // Initialize from desktop system
    void fromSystem();

    static void systemResolution(int *dpiX, int *dpiY);
    static void widgetResolution(const QWidget *w, int *dpiX, int *dpiY);

    bool equals(const DeviceProfile& rhs) const;

    // Apply to form/preview (using font inheritance)
    enum ApplyMode {
        /* Pre-Apply to parent widget of form being edited: Apply font
         * and make use of property inheritance to be able to modify the
         * font property freely. */
        ApplyFormParent,
        /* Post-Apply to preview widget: Change only inherited font
         * sub properties. */
        ApplyPreview
    };
    void apply(const QDesignerFormEditorInterface *core, QWidget *widget, ApplyMode am) const;

    static void applyDPI(int dpiX, int dpiY, QWidget *widget);

    QString toString() const;

    QString toXml() const;
    bool fromXml(const QString &xml, QString *errorMessage);

private:
    QSharedDataPointer<DeviceProfileData> m_d;
};

inline bool operator==(const DeviceProfile &s1, const DeviceProfile &s2)
    { return s1.equals(s2); }
inline bool operator!=(const DeviceProfile &s1, const DeviceProfile &s2)
    { return !s1.equals(s2); }

}


QT_END_NAMESPACE

#endif // DEVICEPROFILE_H
