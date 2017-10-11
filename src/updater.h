#ifndef UPDATER_H
#define UPDATER_H

#include "../3rdparty/dblsqd/feed.h"
#include "../3rdparty/dblsqd/update_dialog.h"

#include "pre_guard.h"
#include <QObject>
#include "post_guard.h"

class Updater : public QObject
{
    Q_OBJECT

    Q_DISABLE_COPY(Updater)

public:
    explicit Updater(QObject* parent = nullptr, bool mautomaticUpdates = false);
    void doUpdates();

private:
    bool mautomaticUpdates;
    dblsqd::Feed* feed;
    dblsqd::UpdateDialog* updateDialog;

    void silentlyUpdate() const;
    void setupManualUpdate() const;
    void untarOnLinux(const QString &fileName) const;

signals:

public slots:

    void updateBinaryOnLinux() const;
};

#endif // UPDATER_H
