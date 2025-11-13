#include "mudlet.h"
#include "Host.h"
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#ifdef Q_OS_WIN
#include <QSettings>
#include <windows.h>
#endif
#ifdef Q_OS_MACOS
#include <CoreServices/CoreServices.h>
#endif

QString mudlet::addProfile(const QString& host, const int port, const QString& login, const QString& password)
{
    QString profileName = host.trimmed();
    if (profileName.isEmpty()) {
        profileName = QStringLiteral("Profile");
    }
    // Ensure unique profile name
    generateUniqueProfileName(profileName);

    // Persist basic profile data
    writeProfileData(profileName, QStringLiteral("url"), host);
    writeProfileData(profileName, QStringLiteral("port"), QString::number(port));

    if (!login.isEmpty()) {
        writeProfileData(profileName, QStringLiteral("login"), login);
    }

    if (!password.isEmpty()) {
        // If secure storage is enabled, profile password may still be stored
        // via other workflows – here we store in profile as fallback
        writeProfileData(profileName, QStringLiteral("password"), password);
    }

    // Defaults
    writeProfileData(profileName, QStringLiteral("ssl_tsl"), QString::number(Qt::Unchecked));
    writeProfileData(profileName, QStringLiteral("autologin"), QString::number(Qt::Unchecked));
    writeProfileData(profileName, QStringLiteral("autoreconnect"), QString::number(Qt::Unchecked));

    return profileName;
}

bool mudlet::mudletIsDefault()
{
#ifdef Q_OS_LINUX
    QProcess proc;
    proc.start("xdg-mime", {"query", "default", "x-scheme-handler/telnet"});
    if (!proc.waitForFinished(2000)) return false;
    const QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    return output.contains("mudlet.desktop", Qt::CaseInsensitive);
#elif defined(Q_OS_WIN)
    QSettings handler("HKEY_CURRENT_USER/Software/Classes/TELNET/shell/open/command", QSettings::NativeFormat);
    const QString command = handler.value(".").toString();
    if (command.isEmpty()) return false;
    const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    return command.contains(exePath, Qt::CaseInsensitive);
#elif defined(Q_OS_MACOS)
    CFStringRef scheme = CFSTR("telnet");
    CFStringRef handler = LSCopyDefaultHandlerForURLScheme(scheme);
    if (!handler) return false;
    CFBundleRef bundle = CFBundleGetMainBundle();
    CFStringRef myId = bundle ? CFBundleGetIdentifier(bundle) : nullptr;
    bool isDefault = (myId && CFStringCompare(handler, myId, kCFCompareCaseInsensitive) == kCFCompareEqualTo);
    if (handler) CFRelease(handler);
    return isDefault;
#else
    return false;
#endif
}

void mudlet::openDefaultCheck()
{
    if (!mudletIsDefault()) {
        setMudletAsDefault();
    }
}

void mudlet::setMudletAsDefault()
{
#ifdef Q_OS_LINUX
    QProcess::execute("xdg-mime", {"default", "mudlet.desktop", "x-scheme-handler/telnet"});
#elif defined(Q_OS_WIN)
    const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString cmd = '"' + exePath + '"' + " \"%1\"";

    QSettings root("HKEY_CURRENT_USER/Software/Classes/TELNET", QSettings::NativeFormat);
    root.setValue("(Default)", "URL:Telnet Protocol");
    root.setValue("URL Protocol", "");

    QSettings iconKey("HKEY_CURRENT_USER/Software/Classes/TELNET/DefaultIcon", QSettings::NativeFormat);
    iconKey.setValue("(Default)", exePath);

    QSettings commandKey("HKEY_CURRENT_USER/Software/Classes/TELNET/shell/open/command", QSettings::NativeFormat);
    commandKey.setValue("(Default)", cmd);
#elif defined(Q_OS_MACOS)
    CFStringRef scheme = CFSTR("telnet");
    CFBundleRef bundle = CFBundleGetMainBundle();
    CFStringRef myId = bundle ? CFBundleGetIdentifier(bundle) : nullptr;
    if (myId) {
        LSSetDefaultHandlerForURLScheme(scheme, myId);
    }
#endif
}

void mudlet::installDefaultPackages(Host* pHost)
{
    if (!pHost) {
        return;
    }

    // Prepare package list based on the host URL
    setupPackagesToInstall(pHost);

    const auto url = pHost->getUrl();

    if (url.contains(QStringLiteral("aetolia.com"), Qt::CaseInsensitive) ||
        url.contains(QStringLiteral("achaea.com"), Qt::CaseInsensitive) ||
        url.contains(QStringLiteral("lusternia.com"), Qt::CaseInsensitive) ||
        url.contains(QStringLiteral("imperian.com"), Qt::CaseInsensitive)) {
        mPackagesToInstallList.append(QStringLiteral(":/mudlet-mapper.xml"));
    } else if (url.contains(QStringLiteral("3scapes.org"), Qt::CaseInsensitive) ||
               url.contains(QStringLiteral("3k.org"), Qt::CaseInsensitive)) {
        mPackagesToInstallList.append(QStringLiteral(":/3k-mapper.xml"));
    } else if (!url.contains(QStringLiteral("mudlet.org"), Qt::CaseInsensitive)) {
        mPackagesToInstallList.append(QStringLiteral(":/mudlet-lua/lua/generic-mapper/generic_mapper.xml"));
    }

    if (url.contains(QStringLiteral("mudlet.org"), Qt::CaseInsensitive)) {
        mPackagesToInstallList.append(QStringLiteral(":/run-tests.xml"));
    } else {
        mPackagesToInstallList.append(QStringLiteral(":/send-text-to-all-games.xml"));
        mPackagesToInstallList.append(QStringLiteral(":/deleteOldProfiles.xml"));
        mPackagesToInstallList.append(QStringLiteral(":/echo.xml"));
    }

    mPackagesToInstallList.append(QStringLiteral(":/run-lua-code-v4.xml"));
}
