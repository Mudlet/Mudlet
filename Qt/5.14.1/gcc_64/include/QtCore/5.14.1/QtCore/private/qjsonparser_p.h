/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QJSONPARSER_P_H
#define QJSONPARSER_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <qjsondocument.h>
#include <qvarlengtharray.h>

QT_BEGIN_NAMESPACE

namespace QJsonPrivate {

class Parser
{
public:
    Parser(const char *json, int length);

    QJsonDocument parse(QJsonParseError *error);

    class ParsedObject
    {
    public:
        ParsedObject(Parser *p, int pos) : parser(p), objectPosition(pos) {
            offsets.reserve(64);
        }
        void insert(uint offset);

        Parser *parser;
        int objectPosition;
        QVector<uint> offsets;

        inline QJsonPrivate::Entry *entryAt(int i) const {
            return reinterpret_cast<QJsonPrivate::Entry *>(parser->data + objectPosition + offsets[i]);
        }
    };


private:
    inline void eatBOM();
    inline bool eatSpace();
    inline char nextToken();

    bool parseObject();
    bool parseArray();
    bool parseMember(int baseOffset);
    bool parseString(bool *latin1);
    bool parseValue(QJsonPrivate::Value *val, int baseOffset);
    bool parseNumber(QJsonPrivate::Value *val, int baseOffset);
    const char *head;
    const char *json;
    const char *end;

    char *data;
    int dataLength;
    int current;
    int nestingLevel;
    QJsonParseError::ParseError lastError;

    inline int reserveSpace(int space) {
        if (current + space >= dataLength) {
            dataLength = 2*dataLength + space;
            char *newData = (char *)realloc(data, dataLength);
            if (!newData) {
                lastError = QJsonParseError::DocumentTooLarge;
                return -1;
            }
            data = newData;
        }
        int pos = current;
        current += space;
        return pos;
    }
};

}

QT_END_NAMESPACE

#endif
