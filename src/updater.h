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

class TUpdateDialog : public dblsqd::UpdateDialog
{
public:
    explicit TUpdateDialog(dblsqd::Feed* feed, Type type = dblsqd::UpdateDialog::Type::OnUpdateAvailable,
                           QWidget* parent = nullptr, QSettings* settings = new QSettings())
    : UpdateDialog(feed, type, parent, settings)
    {
    }

private:
    void startUpdate() override;
};

class Updater : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(Updater)
    explicit Updater(QObject* parent = nullptr);
    void checkUpdatesOnStart();
    void manuallyCheckUpdates();

private:
    dblsqd::Feed* feed;
    TUpdateDialog* updateDialog;

    void setupOnLinux();
    void untarOnLinux(const QString& fileName) const;

#ifdef Q_OS_MACOS
    AutoUpdater* updater;
    void setupOnMacOS();
#endif


signals:

public slots:

    void updateBinaryOnLinux() const;
};

#endif // UPDATER_H
