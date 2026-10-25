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

#include "XmlImportReader.h"

#include <QCoreApplication>
#include <QIODevice>

#include <pugixml.hpp>

#include <array>
#include <cstring>
#include <string_view>

namespace {

bool smAlwaysUseStreamReader = false;

constexpr bool isXmlSpace(uchar c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

constexpr bool isNameStart(uchar c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr bool isNameChar(uchar c)
{
    return isNameStart(c) || (c >= '0' && c <= '9') || c == '-' || c == '.';
}

using StopTable = std::array<bool, 256>;

// Every byte a run of ordinary characters ends at: control characters, the start of any
// multi-byte UTF-8 sequence, and whatever has a meaning of its own in that context
constexpr StopTable stopsAt(std::string_view specials)
{
    StopTable table{};
    for (int c = 0; c < 256; ++c) {
        table[c] = (c < 0x20 && !isXmlSpace(static_cast<uchar>(c))) || c >= 0x80;
    }
    for (const char c : specials) {
        table[static_cast<uchar>(c)] = true;
    }
    return table;
}

constexpr StopTable scmTextStops = stopsAt("<&]");
constexpr StopTable scmAttributeStops = stopsAt("<&\"'");
constexpr StopTable scmCommentStops = stopsAt("-");
constexpr StopTable scmCdataStops = stopsAt("]");

// Characters outside ASCII that both parsers pass through untouched. The C1 controls, the
// noncharacters and anything else QXmlStreamReader might object to are left to it.
constexpr bool plainCharacter(char32_t c)
{
    return c >= 0xA0 && !(c >= 0xD800 && c <= 0xDFFF) && !(c >= 0xFDD0 && c <= 0xFDEF) && (c & 0xFFFE) != 0xFFFE && c <= 0x10FFFF;
}

// QString::fromUtf8() takes a U+FEFF at the start for a byte order mark and drops it, where
// QXmlStreamReader keeps it as the character it is
QString fromUtf8(const char* utf8)
{
    if (std::strncmp(utf8, "\xEF\xBB\xBF", 3) != 0) {
        return QString::fromUtf8(utf8);
    }
    return QChar(0xFEFF) + fromUtf8(utf8 + 3);
}

// Accepts only a plain subset of XML 1.0: UTF-8, an optional declaration of version 1.0 in
// UTF-8, an optional DOCTYPE without an internal subset, ASCII names without namespaces,
// the five predefined entities and character references, comments and CDATA sections, and
// nothing after the root element but whitespace and comments. QXmlStreamReader and pugixml
// read such a document identically, so pugixml is only trusted with one this accepts.
class SubsetScanner
{
public:
    SubsetScanner(const char* data, qsizetype size)
    : mpAt(reinterpret_cast<const uchar*>(data))
    , mBegin(mpAt)
    , mEnd(mpAt + size)
    {
    }

    bool scan(qsizetype& bomLength)
    {
        if (!prolog(bomLength)) {
            return false;
        }
        int depth = 0;
        for (;;) {
            if (startsWith("</")) {
                if (!endTag()) {
                    return false;
                }
                if (--depth == 0) {
                    break;
                }
            } else if (startsWith("<!--")) {
                if (!comment()) {
                    return false;
                }
            } else if (startsWith("<![CDATA[")) {
                if (!cdata()) {
                    return false;
                }
            } else if (mEnd - mpAt >= 2 && *mpAt == '<' && isNameStart(mpAt[1])) {
                bool selfClosing = false;
                if (!startTag(selfClosing)) {
                    return false;
                }
                if (!selfClosing) {
                    ++depth;
                } else if (depth == 0) {
                    break;
                }
            } else {
                return false;
            }
            if (!text() || mpAt >= mEnd) {
                return false;
            }
        }
        for (;;) {
            skipSpace();
            if (mpAt >= mEnd) {
                return true;
            }
            if (!startsWith("<!--") || !comment()) {
                return false;
            }
        }
    }

private:
    bool startsWith(std::string_view literal) const { return static_cast<size_t>(mEnd - mpAt) >= literal.size() && std::memcmp(mpAt, literal.data(), literal.size()) == 0; }

    bool skipSpace()
    {
        const uchar* from = mpAt;
        while (mpAt < mEnd && isXmlSpace(*mpAt)) {
            ++mpAt;
        }
        return mpAt != from;
    }

    void skip(const StopTable& stops)
    {
        while (mEnd - mpAt >= 4 && !(stops[mpAt[0]] | stops[mpAt[1]] | stops[mpAt[2]] | stops[mpAt[3]])) {
            mpAt += 4;
        }
        while (mpAt < mEnd && !stops[*mpAt]) {
            ++mpAt;
        }
    }

    bool name(const uchar*& start, qsizetype& length)
    {
        if (mpAt >= mEnd || !isNameStart(*mpAt)) {
            return false;
        }
        start = mpAt++;
        while (mpAt < mEnd && isNameChar(*mpAt)) {
            ++mpAt;
        }
        length = mpAt - start;
        return true;
    }

    bool character()
    {
        const uchar lead = *mpAt;
        qsizetype length = 0;
        char32_t c = 0;
        if (lead >= 0xC2 && lead <= 0xDF) {
            length = 2;
            c = lead & 0x1F;
        } else if (lead >= 0xE0 && lead <= 0xEF) {
            length = 3;
            c = lead & 0x0F;
        } else if (lead >= 0xF0 && lead <= 0xF4) {
            length = 4;
            c = lead & 0x07;
        } else {
            return false;
        }
        if (mEnd - mpAt < length) {
            return false;
        }
        for (qsizetype i = 1; i < length; ++i) {
            if ((mpAt[i] & 0xC0) != 0x80) {
                return false;
            }
            c = (c << 6) | (mpAt[i] & 0x3F);
        }
        if ((length == 3 && c < 0x800) || (length == 4 && c < 0x10000) || !plainCharacter(c)) {
            return false;
        }
        mpAt += length;
        return true;
    }

    bool reference()
    {
        ++mpAt;
        for (const std::string_view entity : {std::string_view("amp;"), std::string_view("lt;"), std::string_view("gt;"), std::string_view("quot;"), std::string_view("apos;")}) {
            if (startsWith(entity)) {
                mpAt += entity.size();
                return true;
            }
        }
        if (mpAt >= mEnd || *mpAt != '#') {
            return false;
        }
        ++mpAt;
        const bool hex = mpAt < mEnd && *mpAt == 'x';
        if (hex) {
            ++mpAt;
        }
        char32_t value = 0;
        int digits = 0;
        for (; mpAt < mEnd && digits < 8; ++mpAt, ++digits) {
            const uchar c = *mpAt;
            if (c >= '0' && c <= '9') {
                value = value * (hex ? 16 : 10) + (c - '0');
            } else if (hex && c >= 'a' && c <= 'f') {
                value = value * 16 + (c - 'a' + 10);
            } else if (hex && c >= 'A' && c <= 'F') {
                value = value * 16 + (c - 'A' + 10);
            } else {
                break;
            }
        }
        if (digits == 0 || mpAt >= mEnd || *mpAt != ';') {
            return false;
        }
        ++mpAt;
        return value == 0x9 || value == 0xA || value == 0xD || (value >= 0x20 && value < 0x7F) || plainCharacter(value);
    }

    bool attributeValue()
    {
        if (mpAt >= mEnd || (*mpAt != '"' && *mpAt != '\'')) {
            return false;
        }
        const uchar quote = *mpAt++;
        for (;;) {
            skip(scmAttributeStops);
            if (mpAt >= mEnd) {
                return false;
            }
            const uchar c = *mpAt;
            if (c == quote) {
                ++mpAt;
                return true;
            }
            if (c == '"' || c == '\'') {
                ++mpAt;
            } else if (c == '&') {
                if (!reference()) {
                    return false;
                }
            } else if (c < 0x80 || !character()) {
                return false;
            }
        }
    }

    bool startTag(bool& selfClosing)
    {
        ++mpAt;
        const uchar* start = nullptr;
        qsizetype length = 0;
        if (!name(start, length)) {
            return false;
        }
        mAttributeNames.clear();
        for (;;) {
            const bool spaced = skipSpace();
            if (mpAt >= mEnd) {
                return false;
            }
            if (*mpAt == '>') {
                ++mpAt;
                selfClosing = false;
                return true;
            }
            if (*mpAt == '/') {
                if (mEnd - mpAt < 2 || mpAt[1] != '>') {
                    return false;
                }
                mpAt += 2;
                selfClosing = true;
                return true;
            }
            if (!spaced || !name(start, length)) {
                return false;
            }
            const std::string_view attributeName(reinterpret_cast<const char*>(start), length);
            if (attributeName == "xmlns") {
                return false;
            }
            for (const std::string_view seen : mAttributeNames) {
                if (seen == attributeName) {
                    return false;
                }
            }
            mAttributeNames.push_back(attributeName);
            skipSpace();
            if (mpAt >= mEnd || *mpAt != '=') {
                return false;
            }
            ++mpAt;
            skipSpace();
            if (!attributeValue()) {
                return false;
            }
        }
    }

    bool endTag()
    {
        mpAt += 2;
        const uchar* start = nullptr;
        qsizetype length = 0;
        if (!name(start, length)) {
            return false;
        }
        skipSpace();
        if (mpAt >= mEnd || *mpAt != '>') {
            return false;
        }
        ++mpAt;
        return true;
    }

    bool comment()
    {
        mpAt += 4;
        for (;;) {
            skip(scmCommentStops);
            if (mpAt >= mEnd) {
                return false;
            }
            if (*mpAt == '-') {
                if (mEnd - mpAt >= 2 && mpAt[1] == '-') {
                    if (mEnd - mpAt >= 3 && mpAt[2] == '>') {
                        mpAt += 3;
                        return true;
                    }
                    return false;
                }
                ++mpAt;
            } else if (*mpAt < 0x80 || !character()) {
                return false;
            }
        }
    }

    bool cdata()
    {
        mpAt += 9;
        for (;;) {
            skip(scmCdataStops);
            if (mpAt >= mEnd) {
                return false;
            }
            if (*mpAt == ']') {
                if (startsWith("]]>")) {
                    mpAt += 3;
                    return true;
                }
                ++mpAt;
            } else if (*mpAt < 0x80 || !character()) {
                return false;
            }
        }
    }

    // Character data up to the next tag, which the caller reads
    bool text()
    {
        for (;;) {
            skip(scmTextStops);
            if (mpAt >= mEnd || *mpAt == '<') {
                return true;
            }
            if (*mpAt == '&') {
                if (!reference()) {
                    return false;
                }
            } else if (*mpAt == ']') {
                if (startsWith("]]>")) {
                    return false;
                }
                ++mpAt;
            } else if (*mpAt < 0x80 || !character()) {
                return false;
            }
        }
    }

    bool equals()
    {
        skipSpace();
        if (mpAt >= mEnd || *mpAt != '=') {
            return false;
        }
        ++mpAt;
        skipSpace();
        return true;
    }

    bool xmlDeclaration()
    {
        mpAt += 5;
        skipSpace();
        if (!startsWith("version")) {
            return false;
        }
        mpAt += 7;
        if (!equals() || !(startsWith("\"1.0\"") || startsWith("'1.0'"))) {
            return false;
        }
        mpAt += 5;
        if (skipSpace() && startsWith("encoding")) {
            mpAt += 8;
            if (!equals() || !(startsWith("\"") || startsWith("'"))) {
                return false;
            }
            const uchar quote = *mpAt++;
            constexpr std::string_view utf8 = "utf-8";
            if (mEnd - mpAt < static_cast<qsizetype>(utf8.size()) + 1 || mpAt[utf8.size()] != quote) {
                return false;
            }
            for (const char expected : utf8) {
                if ((*mpAt | 0x20) != static_cast<uchar>(expected)) {
                    return false;
                }
                ++mpAt;
            }
            ++mpAt;
            skipSpace();
        }
        if (!startsWith("?>")) {
            return false;
        }
        mpAt += 2;
        return true;
    }

    bool doctype()
    {
        mpAt += 9;
        const uchar* start = nullptr;
        qsizetype length = 0;
        if (!skipSpace() || !name(start, length)) {
            return false;
        }
        skipSpace();
        if (mpAt >= mEnd || *mpAt != '>') {
            return false;
        }
        ++mpAt;
        return true;
    }

    // Ends on the root element's start tag
    bool prolog(qsizetype& bomLength)
    {
        if (startsWith("\xEF\xBB\xBF")) {
            mpAt += 3;
        }
        bomLength = mpAt - mBegin;
        if (startsWith("<?xml") && mEnd - mpAt > 5 && isXmlSpace(mpAt[5]) && !xmlDeclaration()) {
            return false;
        }
        bool sawDoctype = false;
        for (;;) {
            skipSpace();
            if (startsWith("<!--")) {
                if (!comment()) {
                    return false;
                }
            } else if (!sawDoctype && startsWith("<!DOCTYPE")) {
                if (!doctype()) {
                    return false;
                }
                sawDoctype = true;
            } else {
                return mEnd - mpAt >= 2 && *mpAt == '<' && isNameStart(mpAt[1]);
            }
        }
    }

    const uchar* mpAt;
    const uchar* const mBegin;
    const uchar* const mEnd;
    std::vector<std::string_view> mAttributeNames;
};

QString streamError(const char* message)
{
    return QCoreApplication::translate("QXmlStream", message);
}

} // namespace

bool XmlImportReader::Attributes::hasAttribute(QAnyStringView name) const
{
    if (mpReader->mpStream) {
        return mpReader->mpStream->attributes().hasAttribute(name);
    }
    return mpReader->findAttribute(name) != nullptr;
}

QStringView XmlImportReader::Attributes::value(QAnyStringView name) const
{
    if (mpReader->mpStream) {
        return mpReader->mpStream->attributes().value(name);
    }
    const pugi::xml_attribute attribute(mpReader->findAttribute(name));
    return attribute ? mpReader->keep(attribute.value()) : QStringView();
}

bool XmlImportReader::Attributes::isEmpty() const
{
    if (mpReader->mpStream) {
        return mpReader->mpStream->attributes().isEmpty();
    }
    return mpReader->mToken != QXmlStreamReader::StartElement || !pugi::xml_node(mpReader->mpNode).first_attribute();
}

QList<std::pair<QString, QString>> XmlImportReader::Attributes::all() const
{
    QList<std::pair<QString, QString>> result;
    if (mpReader->mpStream) {
        for (const auto& attribute : mpReader->mpStream->attributes()) {
            result.append({attribute.qualifiedName().toString(), attribute.value().toString()});
        }
    } else if (mpReader->mToken == QXmlStreamReader::StartElement) {
        for (const pugi::xml_attribute attribute : pugi::xml_node(mpReader->mpNode).attributes()) {
            result.append({QString::fromUtf8(attribute.name()), fromUtf8(attribute.value())});
        }
    }
    return result;
}

XmlImportReader::XmlImportReader() = default;
XmlImportReader::~XmlImportReader() = default;

void XmlImportReader::setAlwaysUseStreamReaderForTesting(bool always)
{
    smAlwaysUseStreamReader = always;
}

void XmlImportReader::setDevice(QIODevice* device)
{
    mpDevice = device;
    mLoaded = false;
    mpStream.reset();
    mpDocument.reset();
    mBytes.clear();
    mOriginal.clear();
    mKept.clear();
    mpNode = nullptr;
    mToken = QXmlStreamReader::NoToken;
    mName = {};
    mError = QXmlStreamReader::NoError;
    mErrorString.clear();
    mpPositionNode = nullptr;
    mpPositionCacheNode = nullptr;
    mPosition = {};
    mPositionCache = {};
}

void XmlImportReader::load()
{
    mLoaded = true;
    if (!smAlwaysUseStreamReader && mpDevice && mpDevice->isOpen() && mpDevice->isReadable() && !mpDevice->isSequential()) {
        mStartPos = mpDevice->pos();
        mBytes = mpDevice->readAll();
        if (parseTree()) {
            return;
        }
        mBytes.clear();
        mpDevice->seek(mStartPos);
    }
    mpStream = std::make_unique<QXmlStreamReader>(mpDevice);
}

bool XmlImportReader::parseTree()
{
    qsizetype bomLength = 0;
    if (!SubsetScanner(mBytes.constData(), mBytes.size()).scan(bomLength)) {
        return false;
    }
    auto document = std::make_unique<pugi::xml_document>();
    // QXmlStreamReader reports whitespace-only text and comments as tokens of their own
    constexpr unsigned int flags = pugi::parse_default | pugi::parse_ws_pcdata | pugi::parse_comments | pugi::parse_doctype;
    if (!document->load_buffer_inplace(mBytes.data() + bomLength, mBytes.size() - bomLength, flags, pugi::encoding_utf8)) {
        return false;
    }
    int elements = 0;
    for (const pugi::xml_node child : document->children()) {
        elements += child.type() == pugi::node_element;
    }
    if (elements != 1) {
        return false;
    }
    mBomLength = bomLength;
    mpDocument = std::move(document);
    mpNode = mpDocument->internal_object();
    return true;
}

void XmlImportReader::moveTo(pugi::xml_node_struct* node, QXmlStreamReader::TokenType token)
{
    mpNode = node;
    mToken = token;
    if (token == QXmlStreamReader::StartElement || token == QXmlStreamReader::EndElement) {
        mName = QLatin1StringView(pugi::xml_node(node).name());
        if (token == QXmlStreamReader::StartElement) {
            mpPositionNode = node;
        }
    } else {
        mName = {};
    }
}

// The token for the first node from here on that gives one, else the end of their parent
void XmlImportReader::advanceFrom(pugi::xml_node_struct* first)
{
    pugi::xml_node node(first);
    const pugi::xml_node parent = node ? node.parent() : pugi::xml_node(mpNode);
    for (; node; node = node.next_sibling()) {
        switch (node.type()) {
        case pugi::node_element:
            moveTo(node.internal_object(), QXmlStreamReader::StartElement);
            return;
        case pugi::node_pcdata:
            // Whitespace outside the root element is no token for QXmlStreamReader
            if (parent.type() != pugi::node_document) {
                moveTo(node.internal_object(), QXmlStreamReader::Characters);
                return;
            }
            break;
        case pugi::node_cdata:
            moveTo(node.internal_object(), QXmlStreamReader::Characters);
            return;
        case pugi::node_comment:
            moveTo(node.internal_object(), QXmlStreamReader::Comment);
            return;
        case pugi::node_doctype:
            moveTo(node.internal_object(), QXmlStreamReader::DTD);
            return;
        default:
            break;
        }
    }
    moveTo(parent.internal_object(), parent.type() == pugi::node_document ? QXmlStreamReader::EndDocument : QXmlStreamReader::EndElement);
}

QXmlStreamReader::TokenType XmlImportReader::readNext()
{
    if (!mLoaded) {
        load();
    }
    if (mpStream) {
        return mpStream->readNext();
    }
    mKept.clear();
    const pugi::xml_node node(mpNode);
    switch (mToken) {
    case QXmlStreamReader::NoToken:
        moveTo(mpDocument->internal_object(), QXmlStreamReader::StartDocument);
        break;
    case QXmlStreamReader::Invalid:
        break;
    case QXmlStreamReader::EndDocument:
        raiseTreeError(QXmlStreamReader::PrematureEndOfDocumentError, streamError("Premature end of document."), nullptr);
        break;
    case QXmlStreamReader::StartDocument:
    case QXmlStreamReader::StartElement:
        if (const pugi::xml_node child = node.first_child()) {
            advanceFrom(child.internal_object());
        } else {
            moveTo(mpNode, mToken == QXmlStreamReader::StartDocument ? QXmlStreamReader::EndDocument : QXmlStreamReader::EndElement);
        }
        break;
    default:
        if (const pugi::xml_node sibling = node.next_sibling()) {
            advanceFrom(sibling.internal_object());
        } else {
            const pugi::xml_node parent = node.parent();
            moveTo(parent.internal_object(), parent.type() == pugi::node_document ? QXmlStreamReader::EndDocument : QXmlStreamReader::EndElement);
        }
    }
    return mToken;
}

void XmlImportReader::raiseTreeError(QXmlStreamReader::Error error, const QString& message, pugi::xml_node_struct* at)
{
    mError = error;
    mErrorString = message;
    mToken = QXmlStreamReader::Invalid;
    mName = {};
    if (at) {
        mpPositionNode = at;
    }
}

bool XmlImportReader::atEnd() const
{
    if (mpStream) {
        return mpStream->atEnd();
    }
    return mToken == QXmlStreamReader::EndDocument || mToken == QXmlStreamReader::Invalid;
}

QXmlStreamReader::TokenType XmlImportReader::tokenType() const
{
    return mpStream ? mpStream->tokenType() : mToken;
}

bool XmlImportReader::isCDATA() const
{
    if (mpStream) {
        return mpStream->isCDATA();
    }
    return mToken == QXmlStreamReader::Characters && pugi::xml_node(mpNode).type() == pugi::node_cdata;
}

QAnyStringView XmlImportReader::name() const
{
    if (mpStream) {
        return mpStream->name();
    }
    return mName;
}

QStringView XmlImportReader::keep(const char* utf8) const
{
    if (!*utf8) {
        return QStringView(u"", 0);
    }
    mKept.push_back(fromUtf8(utf8));
    return mKept.back();
}

QStringView XmlImportReader::text() const
{
    if (mpStream) {
        return mpStream->text();
    }
    if (mToken == QXmlStreamReader::Characters || mToken == QXmlStreamReader::Comment) {
        return keep(pugi::xml_node(mpNode).value());
    }
    return {};
}

pugi::xml_attribute_struct* XmlImportReader::findAttribute(QAnyStringView name) const
{
    if (mToken != QXmlStreamReader::StartElement) {
        return nullptr;
    }
    for (const pugi::xml_attribute attribute : pugi::xml_node(mpNode).attributes()) {
        if (QAnyStringView::equal(QLatin1StringView(attribute.name()), name)) {
            return attribute.internal_object();
        }
    }
    return nullptr;
}

XmlImportReader::Attributes XmlImportReader::attributes() const
{
    return Attributes(this);
}

QString XmlImportReader::readElementText(QXmlStreamReader::ReadElementTextBehaviour behaviour)
{
    if (!mLoaded) {
        load();
    }
    if (mpStream) {
        return mpStream->readElementText(behaviour);
    }
    if (mToken != QXmlStreamReader::StartElement) {
        return QString();
    }
    const pugi::xml_node first = pugi::xml_node(mpNode).first_child();
    if (!first || (!first.next_sibling() && (first.type() == pugi::node_pcdata || first.type() == pugi::node_cdata))) {
        mKept.clear();
        mToken = QXmlStreamReader::EndElement;
        const char* value = first ? first.value() : "";
        return *value ? fromUtf8(value) : QString();
    }
    // Step by step, as QXmlStreamReader does, for what it does with a child element
    QString result;
    for (;;) {
        switch (readNext()) {
        case QXmlStreamReader::Characters:
            if (const char* value = pugi::xml_node(mpNode).value(); *value) {
                result.append(fromUtf8(value));
            }
            break;
        case QXmlStreamReader::EndElement:
            return result;
        case QXmlStreamReader::ProcessingInstruction:
        case QXmlStreamReader::Comment:
            break;
        case QXmlStreamReader::StartElement:
            if (behaviour == QXmlStreamReader::SkipChildElements) {
                skipCurrentElement();
                break;
            }
            if (behaviour == QXmlStreamReader::IncludeChildElements) {
                result += readElementText(behaviour);
                break;
            }
            [[fallthrough]];
        default:
            if (mError != QXmlStreamReader::NoError || behaviour == QXmlStreamReader::ErrorOnUnexpectedElement) {
                if (mError == QXmlStreamReader::NoError) {
                    raiseTreeError(QXmlStreamReader::UnexpectedElementError, streamError("Expected character data."), mpNode);
                }
                return result;
            }
        }
    }
}

void XmlImportReader::skipCurrentElement()
{
    if (!mLoaded) {
        load();
    }
    if (mpStream) {
        mpStream->skipCurrentElement();
        return;
    }
    if (mToken == QXmlStreamReader::StartElement) {
        mKept.clear();
        mToken = QXmlStreamReader::EndElement;
    }
}

bool XmlImportReader::hasError() const
{
    return mpStream ? mpStream->hasError() : mError != QXmlStreamReader::NoError;
}

QXmlStreamReader::Error XmlImportReader::error() const
{
    return mpStream ? mpStream->error() : mError;
}

QString XmlImportReader::errorString() const
{
    return mpStream ? mpStream->errorString() : mErrorString;
}

// Counted the way QXmlStreamReader counts: in UTF-16 code units from after any byte order
// mark, up to the end of the start tag. Only diagnostics ask, so the bytes pugixml parsed in
// place are read again from the device the first time one does.
const XmlImportReader::Position& XmlImportReader::positionAfterStartTag(pugi::xml_node_struct* element) const
{
    static const Position none;
    if (!element || !mpDevice) {
        return none;
    }
    if (element == mpPositionCacheNode) {
        return mPositionCache;
    }
    if (mOriginal.isEmpty()) {
        const qint64 position = mpDevice->pos();
        if (!mpDevice->seek(mStartPos)) {
            return none;
        }
        mOriginal = mpDevice->read(mBytes.size());
        mpDevice->seek(position);
        if (mOriginal.size() != mBytes.size()) {
            mOriginal.clear();
            return none;
        }
    }
    const ptrdiff_t nameOffset = pugi::xml_node(element).offset_debug();
    if (nameOffset < 0) {
        return none;
    }
    const auto* bytes = reinterpret_cast<const uchar*>(mOriginal.constData());
    const qint64 size = mOriginal.size();
    qint64 end = mBomLength + nameOffset;
    for (uchar quote = 0; end < size; ++end) {
        const uchar c = bytes[end];
        if (quote) {
            quote = c == quote ? 0 : quote;
        } else if (c == '"' || c == '\'') {
            quote = c;
        } else if (c == '>') {
            ++end;
            break;
        }
    }
    if (end < mPosition.byte || mPosition.byte < mBomLength) {
        mPosition = Position{mBomLength, 0, 1, 0};
    }
    for (qint64 i = mPosition.byte; i < end; ++i) {
        const uchar c = bytes[i];
        if ((c & 0xC0) != 0x80) {
            mPosition.characters += c >= 0xF0 ? 2 : 1;
        }
        if (c == '\n' || (c == '\r' && (i + 1 >= size || bytes[i + 1] != '\n'))) {
            ++mPosition.line;
            mPosition.lineStart = mPosition.characters;
        }
    }
    mPosition.byte = end;
    mPositionCache = mPosition;
    mpPositionCacheNode = element;
    return mPositionCache;
}

qint64 XmlImportReader::lineNumber() const
{
    return mpStream ? mpStream->lineNumber() : positionAfterStartTag(mpPositionNode).line;
}

qint64 XmlImportReader::columnNumber() const
{
    if (mpStream) {
        return mpStream->columnNumber();
    }
    const Position& position = positionAfterStartTag(mpPositionNode);
    return position.characters - position.lineStart;
}

qint64 XmlImportReader::characterOffset() const
{
    return mpStream ? mpStream->characterOffset() : positionAfterStartTag(mpPositionNode).characters;
}
