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
// Note: This is a copy of qtbase/src/tools/rcc/rcc.h.

#ifndef RCC_H
#define RCC_H

#include <QtCore/qstringlist.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class RCCFileInfo;
class QIODevice;
class QTextStream;


class RCCResourceLibrary
{
    RCCResourceLibrary(const RCCResourceLibrary &);
    RCCResourceLibrary &operator=(const RCCResourceLibrary &);

public:
    RCCResourceLibrary();
    ~RCCResourceLibrary();

    bool output(QIODevice &out, QIODevice &errorDevice);

    bool readFiles(bool ignoreErrors, QIODevice &errorDevice);

    enum Format { Binary, C_Code };
    void setFormat(Format f) { m_format = f; }
    Format format() const { return m_format; }

    void setInputFiles(const QStringList &files) { m_fileNames = files; }
    QStringList inputFiles() const { return m_fileNames; }

    QStringList dataFiles() const;

    // Return a map of resource identifier (':/newPrefix/images/p1.png') to file.
    typedef QHash<QString, QString> ResourceDataFileMap;
    ResourceDataFileMap resourceDataFileMap() const;

    void setVerbose(bool b) { m_verbose = b; }
    bool verbose() const { return m_verbose; }

    void setInitName(const QString &name) { m_initName = name; }
    QString initName() const { return m_initName; }

    void setCompressLevel(int c) { m_compressLevel = c; }
    int compressLevel() const { return m_compressLevel; }

    void setCompressThreshold(int t) { m_compressThreshold = t; }
    int compressThreshold() const { return m_compressThreshold; }

    void setResourceRoot(const QString &root) { m_resourceRoot = root; }
    QString resourceRoot() const { return m_resourceRoot; }

    void setUseNameSpace(bool v) { m_useNameSpace = v; }
    bool useNameSpace() const { return m_useNameSpace; }

    QStringList failedResources() const { return m_failedResources; }

private:
    struct Strings {
        Strings();
        const QString TAG_RCC;
        const QString TAG_RESOURCE;
        const QString TAG_FILE;
        const QString ATTRIBUTE_LANG;
        const QString ATTRIBUTE_PREFIX;
        const QString ATTRIBUTE_ALIAS;
        const QString ATTRIBUTE_THRESHOLD;
        const QString ATTRIBUTE_COMPRESS;
    };
    friend class RCCFileInfo;
    void reset();
    bool addFile(const QString &alias, const RCCFileInfo &file);
    bool interpretResourceFile(QIODevice *inputDevice, const QString &file,
        QString currentPath = QString(), bool ignoreErrors = false);
    bool writeHeader();
    bool writeDataBlobs();
    bool writeDataNames();
    bool writeDataStructure();
    bool writeInitializer();
    void writeMangleNamespaceFunction(const QByteArray &name);
    void writeAddNamespaceFunction(const QByteArray &name);
    void writeHex(quint8 number);
    void writeNumber2(quint16 number);
    void writeNumber4(quint32 number);
    void writeChar(char c) { m_out.append(c); }
    void writeByteArray(const QByteArray &);
    void write(const char *, int len);

    const Strings m_strings;
    RCCFileInfo *m_root;
    QStringList m_fileNames;
    QString m_resourceRoot;
    QString m_initName;
    Format m_format;
    bool m_verbose;
    int m_compressLevel;
    int m_compressThreshold;
    int m_treeOffset;
    int m_namesOffset;
    int m_dataOffset;
    bool m_useNameSpace;
    QStringList m_failedResources;
    QIODevice *m_errorDevice;
    QByteArray m_out;
};

QT_END_NAMESPACE

#endif // RCC_H
