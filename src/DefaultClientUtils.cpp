
#include "DefaultClientUtils.h"
#include "utils.h"
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>

const QString desktopFileName = QDir::homePath() + qsl("/.local/share/applications/mudlet.desktop");

QString getCurrentTelnetOpenCommand()
{
#if defined(Q_OS_WIN)
    QSettings settings("HKEY_CLASSES_ROOT/telnet/shell/open/command", QSettings::NativeFormat);
    QString value = settings.value(".", QString()).toString();
    return value;
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    QProcess process;
    process.start(qsl("xdg-mime"), QStringList() << qsl("query") << qsl("default") << qsl("x-scheme-handler/telnet"));
    process.waitForFinished();
    const QString output = process.readAllStandardOutput().trimmed();
    if (output != qsl("mudlet.desktop")) {
        return QString();
    }

    // Read the desktop file to see if this specific version of Mudlet is used
    QFile desktopFile(desktopFileName);
    if (!desktopFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    const QString content = desktopFile.readAll();
    desktopFile.close();
    const QString execFieldName = qsl("Exec");
    const QRegularExpression fieldPattern(qsl("%1=([^\n]*)").arg(execFieldName), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = fieldPattern.match(content);

    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    return QString();
#elif defined(Q_OS_MACOS)
    QString swiftScript = qsl(R"(
import Cocoa
let url = URL(string: "telnet://")!
if let appURL = NSWorkspace.shared.urlForApplication(toOpen: url) {
    print(appURL.path)
} else {
    print("")
})";

    QProcess process;
    process.start(qsl("swift"),
                  QStringList() << qsl("c")
                                << swiftScript);
    process.waitForFinished();

    return process.readAllStandardOutput().trimmed();
#else
    Q_STATIC_ASSERT(false);
#endif
}

QString commandForCurrentExecutable()
{
    QString executablePath = qsl("\"") + QCoreApplication::applicationFilePath() + qsl("\"");
#if defined(Q_OS_WIN)
    return executablePath + qsl(" \"%1\"");
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    return executablePath + qsl(" \"%f\"");

#elif defined(Q_OS_MACOS)
    return executablePath;
#else
    Q_STATIC_ASSERT(false);
#endif
}
void setCurrentExecutableAsTelnetOpenCommand()
{
#if defined(Q_OS_WIN)
    QSettings settings(qsl("HKEY_CLASSES_ROOT"), QSettings::NativeFormat);

    settings.setValue(qsl("telnet/."), qsl("URL:Telnet Protocol"));
    settings.setValue(qsl("telnet/URL Protocol"), qsl(""));
    settings.setValue(qsl("telnet/shell/open/command/."), commandForCurrentExecutable());

    settings.setValue(qsl("mudlet/."), qsl("URL:Mudlet Protocol"));
    settings.setValue(qsl("mudlet/URL Protocol"), qsl(""));
    settings.setValue(qsl("mudlet/shell/open/command/."), commandForCurrentExecutable());
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    QProcess process;

    // Read mudlet.desktop
    QFile desktopFile(desktopFileName);
    if (!desktopFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << qsl("Failed to open") << desktopFileName;
        return;
    }
    QString content = desktopFile.readAll();
    desktopFile.close();

    // Find exising Exec field and update it
    QRegularExpression execPattern("^Exec=(.*)$", QRegularExpression::MultilineOption);
    QString updatedContent = content.replace(execPattern, qsl("Exec=") + commandForCurrentExecutable());

    // Write changes to mudlet.desktop
    if (!desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << qsl("Failed to open") << desktopFileName << qsl("for writing");
        return;
    }
    QTextStream out(&desktopFile);
    out << updatedContent;
    desktopFile.close();

    // Tell xdg-mime to use the updated mudlet.desktop
    process.start(qsl("xdg-mime"), QStringList() << qsl("default") << qsl("mudlet.desktop") << qsl("x-scheme-handler/telnet"));
    process.waitForFinished(-1);
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << qsl("Failed to set ") << commandForCurrentExecutable() << qsl(" as the default handler for telnet links.");
    }
#elif defined(Q_OS_MACOS)
    QProcess process;
    process.start(qsl("/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister"), QStringList() << qsl("-f") << commandForCurrentExecutable());
    process.waitForFinished(-1);
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << qsl("Failed to set " << commandForCurrentExecutable() << " as the default handler for telnet links.");
    }
#else
    Q_STATIC_ASSERT(false);
#endif
}

bool isCurrentExecutableDefault()
{
    QString current = getCurrentTelnetOpenCommand();
    if (current.isEmpty()) {
        return false;
    }
    return current == commandForCurrentExecutable();
}
