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

#include "EventNames.h"

#include "utils.h"

#include <QRegularExpression>

namespace eventNames {

bool fromGame(const QString& name)
{
    // Protocol prefixes nothing else in a profile uses. ATCP is here for its table's name alone:
    // cTelnet::setATCPVariables() takes the dots out, so no atcp.* event is raised.
    static const QStringList protocols{qsl("gmcp"), qsl("msdp"), qsl("mssp"), qsl("mxp"), qsl("atcp")};
    for (const QString& protocol : protocols) {
        if (name == protocol) {
            return true;
        }
        if (name.size() > protocol.size() && name.at(protocol.size()) == QLatin1Char('.') && name.startsWith(protocol)) {
            return true;
        }
    }
    return false;
}

QStringList systemEvents()
{
    // Kept by hand, so a newly raised event must be added here. To gather the candidates again:
    //   grep -rEn '"sys[A-Za-z0-9]+"' src/*.cpp src/mudlet-lua/lua | grep -E 'mArgumentList.append|raiseEvent\(|raiseProtocolEvent\('
    // The map, speech and channel 102 events do not start with sys. Sorted without regard to case, which is the order they are offered in.
    static const QStringList names{qsl("channel102Message"),
                                   qsl("mapModeChangeEvent"),
                                   qsl("mapOpenEvent"),
                                   qsl("sysApplicationFocusChangeEvent"),
                                   qsl("sysAppStyleSheetChange"),
                                   qsl("sysBufferShrinkEvent"),
                                   qsl("sysCommandClicked"),
                                   qsl("sysCommandLineDeleted"),
                                   qsl("sysConnectionEvent"),
                                   qsl("sysConsoleSizeChanged"),
                                   qsl("sysCustomHttpDone"),
                                   qsl("sysCustomHttpError"),
                                   qsl("sysDataSendRequest"),
                                   qsl("sysDeleteHttpDone"),
                                   qsl("sysDeleteHttpError"),
                                   qsl("sysDisconnectionEvent"),
                                   qsl("sysDownloadDone"),
                                   qsl("sysDownloadError"),
                                   qsl("sysDownloadFileProgress"),
                                   qsl("sysDropEvent"),
                                   qsl("sysDropUrlEvent"),
                                   qsl("sysExitEvent"),
                                   qsl("sysFontChangeEvent"),
                                   qsl("sysGetHttpDone"),
                                   qsl("sysGetHttpError"),
                                   qsl("sysInstall"),
                                   qsl("sysInstallModule"),
                                   qsl("sysInstallPackage"),
                                   qsl("sysIrcMessage"),
                                   qsl("sysLabelDeleted"),
                                   qsl("sysLoadEvent"),
                                   qsl("sysLuaInstallModule"),
                                   qsl("sysLuaUninstallModule"),
                                   qsl("sysManualLocationSetEvent"),
                                   qsl("sysMapAreaChanged"),
                                   qsl("sysMapDownloadEvent"),
                                   qsl("sysMapperButtonAction"),
                                   qsl("sysMapWindowMousePressEvent"),
                                   qsl("sysMediaFinished"),
                                   qsl("sysMediaPaused"),
                                   qsl("sysMediaStarted"),
                                   qsl("sysMiniConsoleDeleted"),
                                   qsl("sysPathChanged"),
                                   qsl("sysPostHttpDone"),
                                   qsl("sysPostHttpError"),
                                   qsl("sysProfileFocusChangeEvent"),
                                   qsl("sysProtocolDisabled"),
                                   qsl("sysProtocolEnabled"),
                                   qsl("sysPutHttpDone"),
                                   qsl("sysPutHttpError"),
                                   qsl("sysScrollBoxDeleted"),
                                   qsl("sysServerGuiInstalled"),
                                   qsl("sysSettingChanged"),
                                   qsl("sysSpeedwalkFinished"),
                                   qsl("sysSpeedwalkPaused"),
                                   qsl("sysSpeedwalkResumed"),
                                   qsl("sysSpeedwalkStarted"),
                                   qsl("sysSpeedwalkStopped"),
                                   qsl("sysSTTCapabilitiesChanged"),
                                   qsl("sysSTTError"),
                                   qsl("sysSTTHandover"),
                                   qsl("sysSTTPartialResult"),
                                   qsl("sysSTTResult"),
                                   qsl("sysSTTStateChanged"),
                                   qsl("sysSTTWords"),
                                   qsl("sysSyncInstallModule"),
                                   qsl("sysSyncUninstallModule"),
                                   qsl("sysTelnetEvent"),
                                   qsl("sysTextEditDeleted"),
                                   qsl("sysUiTourFinished"),
                                   qsl("sysUninstall"),
                                   qsl("sysUninstallModule"),
                                   qsl("sysUninstallPackage"),
                                   qsl("sysUnzipDone"),
                                   qsl("sysUnzipError"),
                                   qsl("sysUserWindowResizeEvent"),
                                   qsl("sysWindowMousePressEvent"),
                                   qsl("sysWindowMouseReleaseEvent"),
                                   qsl("sysWindowOverflowEvent"),
                                   qsl("sysWindowResizeEvent"),
                                   qsl("ttsPitchChanged"),
                                   qsl("ttsRateChanged"),
                                   qsl("ttsSpeechError"),
                                   qsl("ttsSpeechPaused"),
                                   qsl("ttsSpeechQueued"),
                                   qsl("ttsSpeechReady"),
                                   qsl("ttsSpeechStarted"),
                                   qsl("ttsSpeechSynthesizing"),
                                   qsl("ttsVoiceChanged"),
                                   qsl("ttsVolumeChanged")};
    return names;
}

QStringList namesInCode(const QString& code)
{
    // One pass keeps the names in written order. The closing quote back-references the opening
    // one so an apostrophe inside "..." does not end a name; barring \n stops an unclosed quote
    // swallowing the file. registerNamedEventHandler's skipped arguments may be literals or variables.
    static const QRegularExpression regex{qsl(R"((?:\b(?:raiseEvent|raiseGlobalEvent|registerAnonymousEventHandler)\s*\(\s*(["'])([^\n]*?)\1)"
                                              R"()|(?:\bregisterNamedEventHandler\s*\(\s*[^,()\n]+,\s*[^,()\n]+,\s*(["'])([^\n]*?)\3))")};

    QStringList names;
    QRegularExpressionMatchIterator matches = regex.globalMatch(code);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        const QString name = match.captured(2).isEmpty() ? match.captured(4) : match.captured(2);
        if (name.isEmpty() || names.contains(name)) {
            continue;
        }
        names << name;
    }
    return names;
}

} // namespace eventNames
