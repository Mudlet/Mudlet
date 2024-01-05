#include "FileOpenHandler.h"
#include "MudletInstanceCoordinator.h"

FileOpenHandler::FileOpenHandler(QObject* parent) : QObject(parent)
{
    QCoreApplication::instance()->installEventFilter(this);
}

bool FileOpenHandler::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent* openEvent = static_cast<QFileOpenEvent*>(event);
        Q_ASSERT(mudlet::self());
        MudletInstanceCoordinator* instanceCoordinator = mudlet::self()->getInstanceCoordinator();
        const QString absPath = QDir(openEvent->file()).absolutePath();
        instanceCoordinator->queuePackage(absPath);
        instanceCoordinator->installPackagesLocally();
        return true;
    }
    return QObject::eventFilter(obj, event);
}

