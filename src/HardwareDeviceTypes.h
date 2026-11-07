#ifndef MUDLET_HARDWAREDEVICETYPES_H
#define MUDLET_HARDWAREDEVICETYPES_H

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

// Only the executables' main() includes this: mudlet_core must not load libraries, as an app embedding it may not

#include <QtGlobal>
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include "utils.h"

#include <QByteArray>
#include <QFile>
#include <QLibrary>
#endif

// On Linux, names FFmpeg's hardware device types unless already set; otherwise Qt Multimedia opens a device of each
// type as the first sound plays, blocking the main thread
inline void nameHardwareDeviceTypes()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    // Named types skip Qt's driver prechecks, so this repeats its CUDA one: FFmpeg can crash without libnvcuvid
    // (QTBUG-122199)
    QLibrary nvcuvid(qsl("libnvcuvid.so"));
    const bool cuda = QFile::exists(qsl("/proc/driver/nvidia/version")) && nvcuvid.load();
    nvcuvid.unload();
    // Qt's Linux order, less Vulkan, which can be a software driver that decodes nothing. A type that doesn't
    // work here is skipped when a video opens
    const QByteArray deviceTypes = cuda ? "cuda,vaapi,vdpau,qsv,drm" : "vaapi,vdpau,qsv,drm";
    for (const char* variable : {"QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "QT_FFMPEG_ENCODING_HW_DEVICE_TYPES"}) {
        if (!qEnvironmentVariableIsSet(variable)) {
            qputenv(variable, deviceTypes);
        }
    }
#endif
}

#endif // MUDLET_HARDWAREDEVICETYPES_H
