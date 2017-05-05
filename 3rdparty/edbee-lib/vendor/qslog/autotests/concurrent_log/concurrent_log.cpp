#include "QsLog.h"
#include "QsLogDest.h"
#include <QtCore/QCoreApplication>
#include <QDir>
#include <QtConcurrentMap>
#include <iostream>

static void logString(const QString& string)
{
   QLOG_INFO() << "got a:" << string;
}

int main(int argc, char *argv[])
{
   QCoreApplication a(argc, argv);

   // init the logging mechanism
   QsLogging::Logger& logger = QsLogging::Logger::instance();
   logger.setLoggingLevel(QsLogging::TraceLevel);
   const QString sLogPath(QDir(a.applicationDirPath()).filePath("log.txt"));
   QsLogging::DestinationPtr fileDestination(
      QsLogging::DestinationFactory::MakeFileDestination(sLogPath) );
   QsLogging::DestinationPtr debugDestination(
      QsLogging::DestinationFactory::MakeDebugOutputDestination() );
   logger.addDestination(debugDestination.get());
   logger.addDestination(fileDestination.get());

   QLOG_INFO() << "Program started";
   QLOG_INFO() << "Built with Qt" << QT_VERSION_STR << "running on" << qVersion();

   QStringList strings;
   for(int i = 0;i < 500;++i)
   {
      strings << QString("string %1").arg(i);
   }
   QtConcurrent::blockingMap(strings.constBegin(), strings.constEnd(),
      &logString);

   return 0;
}
