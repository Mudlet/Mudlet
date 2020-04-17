/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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


#include <QRegularExpression>
#include "TMxpTagProcessor.h"

// a map of supported MXP elements and attributes
const QMap<QString, QVector<QString>> TMxpTagProcessor::mSupportedMxpElements = {
        {QStringLiteral("send"), {"href", "hint", "prompt"}},
        {QStringLiteral("br"), {}},
        {QStringLiteral("a"), {"href", "hint"}}
};

TMxpProcessingResult TMxpTagProcessor::process(cTelnet& mTelnet,
                                               TLinkStore& mLinkStore,
                                               const std::string& currentToken,
                                               char& ch)
{
    std::string::size_type _pfs = currentToken.find_first_of(' ');
    QString _tn;
    if (_pfs == std::string::npos) {
        _tn = currentToken.c_str();
    } else {
        _tn = currentToken.substr(0, _pfs).c_str();
    }
    _tn = _tn.toUpper();
    if (_tn == "VERSION") {
        QString payload = QStringLiteral("\n\x1b[1z<VERSION MXP=1.0 CLIENT=Mudlet VERSION=%1%2>\n").arg(APP_VERSION, APP_BUILD);
        mTelnet.sendData(payload);
    } else if (_tn == QLatin1String("SUPPORT")) {
        auto response = processSupportsRequest(currentToken.c_str());
        QString payload = QStringLiteral("\n\x1b[1z<SUPPORTS %1>\n").arg(response);
        mTelnet.sendData(payload);
    }
    if (_tn == "BR") {
        // a <BR> is a newline, but doesn't reset the MXP mode
        ch = '\n';
        return HANDLER_COMMIT_LINE; // jump ahead of the part that resets MXP mode on newline
    }
    if (_tn.startsWith("!EL")) {
        return processElementDefinition(currentToken, _tn);
    }

    if (mMXP_LINK_MODE) { // Is inside a custom TAG
        if (_tn.indexOf('/') != -1) { // is a closing or single tag
            mMXP_LINK_MODE = false;
        }
    }

    if (mMXP_SEND_NO_REF_MODE) {
        if (_tn.indexOf('/') != -1) {
            mMXP_SEND_NO_REF_MODE = false;
            const QStringList& currentLinks = mLinkStore.getCurrentLinks();
            if (currentLinks.front() == "send([[]])") {
                QString _t_ref = "send([[";
                _t_ref.append(mAssembleRef.c_str());
                _t_ref.append("]])");
                QStringList _t_ref_list;
                _t_ref_list << _t_ref;
                mLinkStore.setCurrentLinks(_t_ref_list);
            } else {
                QStringList copy = QStringList(currentLinks);
                copy.replaceInStrings("&text;", mAssembleRef.c_str());
                mLinkStore.setCurrentLinks(copy);
            }
            mAssembleRef.clear();
        }
    } else if (mMXP_Elements.contains(_tn)) {
        return processCustomElement(mLinkStore, currentToken, _tn);
    }

    return HANDLER_NEXT_CHAR;
}

QString TMxpTagProcessor::processSupportsRequest(const QString& elements)
{
    // strip initial SUPPORT and tokenize all of the requested elements
    auto elementsList = elements.trimmed().remove(0, 7).split(QStringLiteral(" "), QString::SkipEmptyParts);
    QStringList result;

    auto reportEntireElement = [](auto element, auto& result) {
        result.append("+" + element);

        for (const auto& attribute : mSupportedMxpElements.value(element)) {
            result.append("+" + element + QStringLiteral(".") + attribute);
        }

        return result;
    };

    auto reportAllElements = [reportEntireElement](auto& result) {
        auto elementsIterator = mSupportedMxpElements.constBegin();
        while (elementsIterator != mSupportedMxpElements.constEnd()) {
            result = reportEntireElement(elementsIterator.key(), result);
            ++elementsIterator;
        }

        return result;
    };

    // empty <SUPPORT> - report all known elements
    if (elementsList.isEmpty()) {
        result = reportAllElements(result);
    } else {
        // otherwise it's <SUPPORT element1 element2 element3>
        for (auto& element : elementsList) {
            // prune any enclosing quotes
            if (element.startsWith(QChar('"'))) {
                element = element.remove(0, 1);
            }
            if (element.endsWith(QChar('"'))) {
                element.chop(1);
            }

            if (!element.contains(QChar('.'))) {
                if (mSupportedMxpElements.contains(element)) {
                    result = reportEntireElement(element, result);
                } else {
                    result.append("-" + element);
                }
            } else {
                auto elementName = element.section(QChar('.'), 0, 0);
                auto attributeName = element.section(QChar('.'), 1, 1);

                if (!mSupportedMxpElements.contains(elementName)) {
                    result.append("-" + element);
                } else if (attributeName == QLatin1String("*")) {
                    result = reportEntireElement(elementName, result);
                } else {
                    if (mSupportedMxpElements.value(elementName).contains(attributeName)) {
                        result.append("+" + element + "." + attributeName);
                    } else {
                        result.append("-" + element + "." + attributeName);
                    }
                }
            }
        }
    }

    return result.join(QLatin1String(" "));
}

TMxpProcessingResult TMxpTagProcessor::processElementDefinition(const std::string& currentToken, QString _tn)
{
    QString _tp = currentToken.substr(currentToken.find_first_of(' ')).c_str();
    _tn = _tp.section(' ', 1, 1).toUpper();
    _tp = _tp.section(' ', 2).toUpper();
    if ((_tp.indexOf("<SEND") != -1)) {
        QString _t2 = _tp;
        int pRef = _t2.indexOf("HREF=");
        bool _got_ref = false;
        // wenn kein href angegeben ist, dann gilt das 1. parameter als href
        if (pRef == -1) {
            pRef = _t2.indexOf("<SEND ") + 1;
        } else {
            _got_ref = true;
        }

        if (pRef == -1) {
            return HANDLER_SKIP_INPUT;
        }
        pRef += 5;

        QChar _quote_type = _t2[pRef];
        int pRef2;
        if (_quote_type != '&') {
            pRef2 = _t2.indexOf(_quote_type, pRef + 1); //' ', pRef );
        } else {
            pRef2 = _t2.indexOf(' ', pRef + 1);
        }
        QString _ref = _t2.mid(pRef, pRef2 - pRef);

        // gegencheck, ob es keine andere variable ist

        if (_ref.startsWith('\'')) {
            int pRef3 = _t2.indexOf('\'', _t2.indexOf('\'', pRef) + 1);
            int pRef4 = _t2.indexOf('=');
            if (((pRef4 == -1) || (pRef4 != 0 && pRef4 > pRef3)) || (_got_ref)) {
                _ref = _t2.mid(pRef, pRef2 - pRef);
            }
        } else if (_ref.startsWith('\"')) {
            int pRef3 = _t2.indexOf('\"', _t2.indexOf('\"', pRef) + 1);
            int pRef4 = _t2.indexOf('=');
            if (((pRef4 == -1) || (pRef4 != 0 && pRef4 > pRef3)) || (_got_ref)) {
                _ref = _t2.mid(pRef, pRef2 - pRef);
            }
        } else if (_ref.startsWith('&')) {
            _ref = _t2.mid(pRef, _t2.indexOf(' ', pRef + 1) - pRef);
        } else {
            _ref = "";
        }
        _ref = _ref.replace(';', "");
        _ref = _ref.replace("&quot", "");
        _ref = _ref.replace("&amp", "&");
        _ref = _ref.replace('\'', ""); //NEU
        _ref = _ref.replace('\"', ""); //NEU
        _ref = _ref.replace("&#34", R"(")");

        pRef = _t2.indexOf("HINT=");
        QString _hint;
        if (pRef != -1) {
            pRef += 5;
            int pRef2 = _t2.indexOf(' ', pRef);
            _hint = _t2.mid(pRef, pRef2 - pRef);
            if (_hint.startsWith('\'') || pRef2 < 0) {
                pRef2 = _t2.indexOf('\'', _t2.indexOf('\'', pRef) + 1);
                _hint = _t2.mid(pRef, pRef2 - pRef);
            } else if (_hint.startsWith('\"') || pRef2 < 0) {
                pRef2 = _t2.indexOf('\"', _t2.indexOf('\"', pRef) + 1);
                _hint = _t2.mid(pRef, pRef2 - pRef);
            }
            _hint = _hint.replace(';', "");
            _hint = _hint.replace("&quot", "");
            _hint = _hint.replace("&amp", "&");
            _hint = _hint.replace('\'', ""); //NEU
            _hint = _hint.replace('\"', ""); //NEU
            _hint = _hint.replace("&#34", R"(")");
        }
        TMxpElement _element;
        _element.name = _tn;
        _element.href = _ref;
        _element.hint = _hint;
        mMXP_Elements[_tn] = _element;
    }

    return HANDLER_NEXT_CHAR;
}
bool TMxpTagProcessor::isInLinkMode() const
{
    return mMXP_LINK_MODE;
}

TMxpProcessingResult TMxpTagProcessor::processCustomElement(TLinkStore& mLinkStore, const std::string& currentToken, QString _tn)
{
    QString _tp;
    std::string::size_type _fs = currentToken.find_first_of(' ');
    if (_fs != std::string::npos) {
        _tp = currentToken.substr(_fs).c_str();
    }
    QString _t1 = _tp.toUpper();
    const TMxpElement& _element = mMXP_Elements[_tn];
    QString _t2 = _element.href;
    QString _t3 = _element.hint;
    bool _userTag = true;
    if (_t2.size() < 1) {
        _userTag = false;
    }
    QRegularExpression _rex;
    QStringList _rl1, _rl2;
    int _ki1 = _tp.indexOf('\'');
    int _ki2 = _tp.indexOf('\"');
    int _ki3 = _tp.indexOf('=');
    // is the first parameter to send given in the form
    // send "what" hint="bla" or send href="what" hint="bla"

    // handle the first case without a variable assignment
    if ((_ki3 == -1)                                           // no = whatsoever
            || ((_ki3 != -1) && ((_ki2 < _ki3) || (_ki1 < _ki3)))) // first parameter is given without =
    {
        if ((_ki1 < _ki2 && _ki1 != -1) || (_ki2 == -1 && _ki1 != -1)) {
            if (_ki1 < _ki3 || _ki3 == -1) {
                _rl1 << "HREF";
                int _cki1 = _tp.indexOf('\'', _ki1 + 1);
                if (_cki1 > -1) {
                    _rl2 << _tp.mid(_ki1 + 1, _cki1 - (_ki1 + 1));
                }
            }
        } else if ((_ki2 < _ki1 && _ki2 != -1) || (_ki1 == -1 && _ki2 != -1)) {
            if (_ki2 < _ki3 || _ki3 == -1) {
                _rl1 << "HREF";
                int _cki2 = _tp.indexOf('\"', _ki2 + 1);
                if (_cki2 > -1) {
                    _rl2 << _tp.mid(_ki2 + 1, _cki2 - (_ki2 + 1));
                }
            }
        }
    }
    // parse parameters in the form var="val" or var='val' where val can be given in the form "foo'b'ar" or 'foo"b"ar'
    if (_tp.contains(QStringLiteral(R"(=')"))) {
        _rex = QRegularExpression(QStringLiteral(R"(\b(\w+)=\'([^\']*) ?)"));
    } else {
        _rex = QRegularExpression(QStringLiteral(R"(\b(\w+)=\"([^\"]*) ?)"));
    }

    int _rpos = 0;
    QRegularExpressionMatch match = _rex.match(_tp, _rpos);
    while ((_rpos = match.capturedStart()) != -1) {
        _rl1 << match.captured(1).toUpper();
        _rl2 << match.captured(2);
        _rpos += match.capturedLength();

        match = _rex.match(_tp, _rpos);
    }

    if ((_rl1.size() == _rl2.size()) && (!_rl1.empty())) {
        for (int i = 0; i < _rl1.size(); i++) {
            QString _var = _rl1[i];
            _var.prepend('&');
            if (_userTag || _t2.indexOf(_var) != -1) {
                _t2 = _t2.replace(_var, _rl2[i]);
                _t3 = _t3.replace(_var, _rl2[i]);
            } else {
                if (_rl1[i] == QStringLiteral("HREF")) {
                    _t2 = _rl2[i];
                }
                if (_rl1[i] == QStringLiteral("HINT")) {
                    _t3 = _rl2[i];
                }
            }
        }
    }

    // handle print to prompt feature PROMPT
    bool _send_to_command_line = false;
    if (_t1.endsWith("PROMPT")) {
        _send_to_command_line = true;
    }

    mMXP_LINK_MODE = true;
    if (_t2.size() < 1 || _t2.contains("&text;")) {
        mMXP_SEND_NO_REF_MODE = true;
    }

    QStringList _tl = _t2.split('|');
    for (int i = 0, total = _tl.size(); i < total; ++i) {
        _tl[i].replace("|", "");
        if (_element.name == "A") {
            _tl[i] = "openUrl([[" + _tl[i] + "]])";
        } else if (!_send_to_command_line) {
            _tl[i] = "send([[" + _tl[i] + "]])";
        } else {
            _tl[i] = "printCmdLine([[" + _tl[i] + "]])";
        }
    }

    _t3 = _t3.replace("&quot;", R"(")");
    _t3 = _t3.replace("&amp;", "&");
    _t3 = _t3.replace("&apos;", "'");
    _t3 = _t3.replace("&#34;", R"(")");

    QStringList _tl2 = _t3.split('|');
    _tl2.replaceInStrings("|", "");
    if (_tl2.size() >= _tl.size() + 1) {
        _tl2.pop_front();
    }

    mLinkStore.addLinks(_tl, _tl2);

    return HANDLER_NEXT_CHAR;
}
void TMxpTagProcessor::processTextContent(char& ch)
{
    if (mMXP_SEND_NO_REF_MODE) {
        mAssembleRef += ch;
    }
}
