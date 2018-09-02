/*
  Copyright (C) 2008-2016 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "irctoken_p.h"
#include <QStringList>

IRC_BEGIN_NAMESPACE

#ifndef IRC_DOXYGEN
static QList<IrcToken> tokenize(const QString& str)
{
    int idx = -1;
    int pos = 0;
    QList<IrcToken> tokens;
    foreach (const QString& txt, str.split(QLatin1String(" "))) {
        if (!txt.isEmpty())
            tokens += IrcToken(++idx, pos, txt);
        pos += txt.length() + 1;
    }
    return tokens;
}

IrcTokenizer::IrcTokenizer(const QString& str) : len(str.length()), t(tokenize(str))
{
}

int IrcTokenizer::count() const
{
    return t.count();
}

bool IrcTokenizer::isEmpty() const
{
    return t.isEmpty();
}

QList<IrcToken> IrcTokenizer::tokens() const
{
    return t;
}

IrcToken IrcTokenizer::at(int index) const
{
    return t.value(index);
}

IrcTokenizer IrcTokenizer::mid(int index) const
{
    IrcTokenizer tt;
    tt.t = t.mid(index);
    if (!tt.isEmpty()) {
        int d = tt.t.first().position();
        tt.len = len - d;
        for (int i = 0; i < tt.t.length(); ++i) {
            tt.t[i].idx = i;
            tt.t[i].pos -= d;
        }
    }
    return tt;
}

void IrcTokenizer::clear()
{
    t.clear();
}

void IrcTokenizer::replace(int index, const QString& text)
{
    IrcToken token = t.value(index);
    if (token.isValid()) {
        int d = text.length() - token.length();
        token = IrcToken(index, token.position(), text);
        t.replace(index, token);
        len += d;
        for (int i = index + 1; i < t.length(); ++i)
            t[i].pos += d;
    }
}

IrcToken IrcTokenizer::find(int pos) const
{
    IrcToken token;
    foreach (const IrcToken& tk, t) {
        if (tk.position() > pos)
            break;
        token = tk;
    }
    return token;
}

QString IrcTokenizer::toString() const
{
    QString str(len, QLatin1Char(' '));
    foreach (const IrcToken& token, t)
        str.replace(token.position(), token.length(), token.text());
    return str;
}
#endif // IRC_DOXYGEN

IRC_END_NAMESPACE
