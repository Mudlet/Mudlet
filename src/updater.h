#ifndef UPDATER_H
#define UPDATER_H

#include "dblsqd/feed.h"
#include "dblsqd/update_dialog.h"

#ifdef Q_OS_MACOS
#include "../3rdparty/sparkle-glue/AutoUpdater.h"
#endif

#include "pre_guard.h"
#include <QObject>
#include "post_guard.h"

class Updater : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(Updater)
    explicit Updater(QObject* parent = nullptr);
    void checkUpdatesOnStart();
    void manuallyCheckUpdates();
    void showChangelog() const;

private:
    dblsqd::Feed* feed;
    dblsqd::UpdateDialog* updateDialog;
    QPushButton* installOrRestartButton;
    bool mUpdateInstalled;

    void setupOnLinux();
    void untarOnLinux(const QString& fileName) const;
    void writeUpdateNote() const;

#if defined(Q_OS_MACOS)
    AutoUpdater* updater;
    void setupOnMacOS();
#endif


signals:
    void updateInstalled();

public slots:
#if defined(Q_OS_LINUX)
    // might want to make these private
    void updateBinaryOnLinux();
    void installOrRestartClicked(QAbstractButton *button, QString filePath);
#endif
};

#endif // UPDATER_H
