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

#include "BrowserTransport.h"

#include "THeadlessSession.h"

#include <QPointer>

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

#include <string>

// The JavaScript below is stringified by the preprocessor, which joins its lines, so every statement ends in a
// semicolon. A throw from the page's code would unwind through Mudlet's frames, so each call into it is caught.
namespace {

QPointer<THeadlessSession> gpSession;

// clang-format off
EM_JS(int, pageHasTransport, (), {
    return Module.mudletTransport ? 1 : 0;
});

// Each dial hands the page a connection of its own, whose calls do nothing once the profile has closed it or dialled
// again, and whose reports carry the id the profile gave the dial. The profile asks for no close() of a connection it
// is told has ended, so when the page's code throws or hands over what is not bytes, the page's close() is called here.
EM_JS(void, pageDial, (double id, const char* host, int port, int tlsRequested), {
    if (Module.mudletConnection) {
        Module.mudletConnection.live = false;
    }
    var connection = {
        live: true,
        connected: function() {
            if (connection.live) {
                Module.mudletTransportConnected(id);
            }
        },
        received: function(bytes) {
            if (!connection.live) {
                return;
            }
            if (bytes instanceof ArrayBuffer) {
                bytes = new Uint8Array(bytes);
            } else if (ArrayBuffer.isView(bytes)) {
                bytes = new Uint8Array(bytes.buffer, bytes.byteOffset, bytes.byteLength);
            } else {
                var given = Object.prototype.toString.call(bytes).slice(8, -1);
                connection.fail("connection.received() takes an ArrayBuffer or a view of one such as a Uint8Array, not " + given
                                + (given === "Blob" ? " (is the WebSocket binaryType \"arraybuffer\"?)" : ""));
                return;
            }
            Module.mudletTransportReceived(id, bytes);
        },
        closed: function(reason) {
            if (connection.live) {
                connection.live = false;
                Module.mudletTransportClosed(id, reason == null ? "" : String(reason));
            }
        },
        fail: function(reason) {
            if (!connection.live) {
                return;
            }
            console.error("mudletTransport: " + reason);
            connection.live = false;
            try {
                Module.mudletTransport.close();
            } catch (e) {
                console.error("mudletTransport.close() failed: " + e);
            }
            setTimeout(function() {
                Module.mudletTransportClosed(id, reason);
            }, 0);
        }
    };
    Module.mudletConnection = connection;
    try {
        Module.mudletTransport.dial(UTF8ToString(host), port, tlsRequested !== 0, {
            connected: connection.connected,
            received: connection.received,
            closed: connection.closed
        });
    } catch (e) {
        connection.fail("mudletTransport.dial() failed: " + e);
    }
});

EM_JS(void, pageWrite, (const char* data, int length), {
    var connection = Module.mudletConnection;
    if (!connection || !connection.live) {
        return;
    }
    try {
        Module.mudletTransport.write(HEAPU8.slice(data, data + length));
    } catch (e) {
        connection.fail("mudletTransport.write() failed: " + e);
    }
});

EM_JS(void, pageClose, (), {
    var connection = Module.mudletConnection;
    Module.mudletConnection = null;
    if (!connection || !connection.live) {
        return;
    }
    connection.live = false;
    try {
        Module.mudletTransport.close();
    } catch (e) {
        console.error("mudletTransport.close() failed: " + e);
    }
});
// clang-format on

// The id is a JavaScript number, which holds any the profile hands out exactly
void connectedFromPage(const double connection)
{
    if (gpSession) {
        gpSession->transportConnected(static_cast<quint64>(connection));
    }
}

// Embind passes a Uint8Array for a std::string byte for byte
void receivedFromPage(const double connection, const std::string& bytes)
{
    if (gpSession) {
        gpSession->transportReceived(static_cast<quint64>(connection), QByteArray::fromStdString(bytes));
    }
}

void closedFromPage(const double connection, const std::string& reason)
{
    if (gpSession) {
        gpSession->transportClosed(static_cast<quint64>(connection), QString::fromStdString(reason));
    }
}

} // namespace

EMSCRIPTEN_BINDINGS(mudlet_transport)
{
    emscripten::function("mudletTransportConnected", &connectedFromPage);
    emscripten::function("mudletTransportReceived", &receivedFromPage);
    emscripten::function("mudletTransportClosed", &closedFromPage);
}

bool pageSuppliesTransport()
{
    return pageHasTransport() != 0;
}

void bridgeTransportToPage(THeadlessSession* pSession)
{
    gpSession = pSession;
    QObject::connect(pSession, &THeadlessSession::transportDialRequested, pSession, [](const QString&, const quint64 connection, const QString& host, const int port, const bool tlsRequested) {
        pageDial(static_cast<double>(connection), host.toUtf8().constData(), port, tlsRequested);
    });
    QObject::connect(pSession, &THeadlessSession::transportWriteRequested, pSession, [](const QString&, const QByteArray& data) {
        pageWrite(data.constData(), static_cast<int>(data.size()));
    });
    QObject::connect(pSession, &THeadlessSession::transportCloseRequested, pSession, [](const QString&) {
        pageClose();
    });
}
