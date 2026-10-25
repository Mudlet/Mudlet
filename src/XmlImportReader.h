#ifndef MUDLET_XMLIMPORTREADER_H
#define MUDLET_XMLIMPORTREADER_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QAnyStringView>
#include <QByteArray>
#include <QLatin1StringView>
#include <QList>
#include <QString>
#include <QStringView>
#include <QXmlStreamReader>

#include <memory>
#include <utility>
#include <vector>

class QIODevice;

namespace pugi {
class xml_document;
struct xml_attribute_struct;
struct xml_node_struct;
} // namespace pugi

// The part of QXmlStreamReader that XMLimport uses, giving the same tokens, text and errors.
// A document is parsed whole with pugixml when its bytes keep to a plain subset of XML that
// both parsers read identically; any other document - including every one QXmlStreamReader
// rejects - is read by QXmlStreamReader itself from the same device.
class XmlImportReader
{
public:
    class Attributes
    {
    public:
        bool hasAttribute(QAnyStringView name) const;
        // Valid until the next readNext(), as QXmlStreamReader's are
        QStringView value(QAnyStringView name) const;
        bool isEmpty() const;
        QList<std::pair<QString, QString>> all() const;

    private:
        friend class XmlImportReader;
        explicit Attributes(const XmlImportReader* reader)
        : mpReader(reader)
        {
        }
        const XmlImportReader* mpReader;
    };

    XmlImportReader();
    ~XmlImportReader();
    XmlImportReader(const XmlImportReader&) = delete;
    XmlImportReader& operator=(const XmlImportReader&) = delete;

    // Read from its current position on the first readNext(), as QXmlStreamReader does
    void setDevice(QIODevice*);

    bool atEnd() const;
    QXmlStreamReader::TokenType readNext();
    QXmlStreamReader::TokenType tokenType() const;
    bool isStartElement() const { return tokenType() == QXmlStreamReader::StartElement; }
    bool isEndElement() const { return tokenType() == QXmlStreamReader::EndElement; }
    bool isCharacters() const { return tokenType() == QXmlStreamReader::Characters; }
    bool isCDATA() const;
    QAnyStringView name() const;
    QStringView text() const;
    Attributes attributes() const;
    QString readElementText(QXmlStreamReader::ReadElementTextBehaviour behaviour = QXmlStreamReader::ErrorOnUnexpectedElement);
    void skipCurrentElement();

    bool hasError() const;
    QXmlStreamReader::Error error() const;
    QString errorString() const;
    // Read from a tree, exact at a start element and at the error an element inside text raises;
    // anywhere else, where the last start element ended
    qint64 lineNumber() const;
    qint64 columnNumber() const;
    qint64 characterOffset() const;

    bool readingParsedTree() const { return mpDocument != nullptr; }
    static void setAlwaysUseStreamReaderForTesting(bool);

private:
    struct Position
    {
        qint64 byte = 0;
        qint64 characters = 0;
        qint64 line = 1;
        qint64 lineStart = 0;
    };

    void load();
    bool parseTree();
    void moveTo(pugi::xml_node_struct*, QXmlStreamReader::TokenType);
    void advanceFrom(pugi::xml_node_struct*);
    void raiseTreeError(QXmlStreamReader::Error, const QString& message, pugi::xml_node_struct* at);
    QStringView keep(const char* utf8) const;
    pugi::xml_attribute_struct* findAttribute(QAnyStringView name) const;
    const Position& positionAfterStartTag(pugi::xml_node_struct*) const;

    QIODevice* mpDevice = nullptr;
    bool mLoaded = false;
    std::unique_ptr<QXmlStreamReader> mpStream;

    qint64 mStartPos = 0;
    qsizetype mBomLength = 0;
    QByteArray mBytes;
    std::unique_ptr<pugi::xml_document> mpDocument;
    pugi::xml_node_struct* mpNode = nullptr;
    QXmlStreamReader::TokenType mToken = QXmlStreamReader::NoToken;
    QLatin1StringView mName;
    QXmlStreamReader::Error mError = QXmlStreamReader::NoError;
    QString mErrorString;
    pugi::xml_node_struct* mpPositionNode = nullptr;
    mutable std::vector<QString> mKept;
    mutable QByteArray mOriginal;
    mutable Position mPosition;
    mutable Position mPositionCache;
    mutable pugi::xml_node_struct* mpPositionCacheNode = nullptr;
};

#endif // MUDLET_XMLIMPORTREADER_H
