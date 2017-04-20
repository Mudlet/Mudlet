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

#include "ircmessagedecoder_p.h"
#include <unicode/ucsdet.h>

IRC_BEGIN_NAMESPACE

#ifndef IRC_DOXYGEN
#define UCSD(x) reinterpret_cast<UCharsetDetector*>(x)

void IrcMessageDecoder::initialize()
{
    UErrorCode status = U_ZERO_ERROR;
    d.detector = ucsdet_open(&status);
    if (U_FAILURE(status))
        qWarning("IrcMessageDecoder: ICU initialization failed: %s", u_errorName(status));
}

void IrcMessageDecoder::uninitialize()
{
    ucsdet_close(UCSD(d.detector));
}

QByteArray IrcMessageDecoder::codecForData(const QByteArray &data) const
{
    QByteArray encoding;
    UErrorCode status = U_ZERO_ERROR;
    if (d.detector) {
        ucsdet_setText(UCSD(d.detector), data.constData(), data.length(), &status);
        if (!U_FAILURE(status)) {
            const UCharsetMatch* match = ucsdet_detect(UCSD(d.detector), &status);
            if (match && !U_FAILURE(status))
                encoding = ucsdet_getName(match, &status);
        }
    }
    if (U_FAILURE(status))
        qWarning("IrcMessageDecoder::codecForData() failed: %s", u_errorName(status));
    return encoding;
}
#endif // IRC_DOXYGEN

IRC_END_NAMESPACE
