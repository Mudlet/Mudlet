# Precompiled headers for mudlet_core and the test binaries that link it.
#
# Only third-party headers belong in the list: they change with the toolchain,
# not with a pull request. A Mudlet header in it would turn every edit to that
# header into a rebuild of every file, and would hide a missing #include of it
# everywhere. The list is the Qt headers that at least a third of mudlet_core
# pulls in, directly or through Mudlet's own headers.
#
# -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON turns all of this off. CI's push-only
# ubuntu / clang job does so, to catch a missing #include the precompiled
# header would otherwise supply.

if(ENABLE_STATIC_ANALYSIS)
  # clang-tidy cannot read a GCC precompiled header, and should see each file's
  # own includes anyway
  set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
endif()

set(MUDLET_PRECOMPILED_HEADERS
  <algorithm> <chrono> <functional> <list> <map> <memory> <optional> <string>
  <utility> <vector>
  <QByteArray> <QCache> <QChar> <QCoreApplication> <QDateTime> <QDebug>
  <QDebugStateSaver> <QElapsedTimer> <QEvent> <QFile> <QFileSystemWatcher>
  <QFlags> <QFuture> <QHash> <QIODevice> <QJsonObject> <QLibrary> <QList> <QMap>
  <QMargins> <QMetaType> <QMultiHash> <QMultiMap> <QObject> <QPair> <QPoint>
  <QPointer> <QProcess> <QQueue> <QRect> <QScopeGuard> <QSet>
  <QSharedDataPointer> <QSharedPointer> <QSize> <QSizeF> <QStack> <QString>
  <QStringDecoder> <QStringList> <QStringMatcher> <QStringView> <QTextStream>
  <QThread> <QTime> <QTimer> <QTimerEvent> <QUrl> <QVariant> <QVarLengthArray>
  <QVector>
  <QAction> <QColor> <QEnterEvent> <QFont> <QIcon> <QKeySequence> <QMovie>
  <QPixmap> <QResizeEvent> <QShortcut> <QTextOption> <QTransform>
  <QApplication> <QBoxLayout> <QCheckBox> <QComboBox> <QDialog> <QFrame>
  <QGridLayout> <QGroupBox> <QHBoxLayout> <QLabel> <QLineEdit> <QMainWindow>
  <QMdiArea> <QMenu> <QMenuBar> <QPlainTextEdit> <QPushButton> <QSpacerItem>
  <QSpinBox> <QSystemTrayIcon> <QTabWidget> <QToolButton> <QTreeWidget>
  <QVBoxLayout> <QWidget>
  <QHostAddress> <QHostInfo> <QNetworkAccessManager> <QNetworkCookie>
  <QNetworkCookieJar> <QNetworkReply> <QNetworkRequest> <QSslConfiguration>
  <QSslPreSharedKeyAuthenticator> <QSslSocket>
)
if(WIN32)
  # Ahead of anything that brings in windows.h, as INCLUDE_WINSOCK2 arranges
  list(PREPEND MUDLET_PRECOMPILED_HEADERS <winsock2.h>)
endif()

# ccache only takes direct-mode hits on a file that uses a precompiled header
# when told to be sloppy about the #defines and time macros it can no longer
# see. The rest of the build keeps whatever the developer has configured.
if(CCACHE_FOUND AND NOT CMAKE_DISABLE_PRECOMPILE_HEADERS)
  execute_process(
    COMMAND ${CCACHE_FOUND} --get-config sloppiness
    OUTPUT_VARIABLE ccacheSloppiness
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  string(REPLACE " " "" ccacheSloppiness "${ccacheSloppiness}")
  string(REPLACE "," ";" ccacheSloppiness "${ccacheSloppiness}")
  list(APPEND ccacheSloppiness pch_defines time_macros)
  list(REMOVE_ITEM ccacheSloppiness "")
  list(REMOVE_DUPLICATES ccacheSloppiness)
  list(JOIN ccacheSloppiness "," ccacheSloppiness)
  set(MUDLET_PCH_COMPILER_LAUNCHER
      ${CMAKE_COMMAND} -E env "CCACHE_SLOPPINESS=${ccacheSloppiness}" ${CCACHE_FOUND})
endif()

function(mudlet_use_pch_compiler_launcher target)
  if(MUDLET_PCH_COMPILER_LAUNCHER)
    set_target_properties(${target} PROPERTIES CXX_COMPILER_LAUNCHER "${MUDLET_PCH_COMPILER_LAUNCHER}")
  endif()
endfunction()

function(mudlet_precompile_headers target)
  target_precompile_headers(${target} PRIVATE ${MUDLET_PRECOMPILED_HEADERS} ${ARGN})
  # Clang otherwise records each header's mtime in the precompiled header and
  # rejects it from ccache once a fresh checkout has given the headers new ones
  target_compile_options(${target} PRIVATE
    "$<$<CXX_COMPILER_ID:Clang,AppleClang>:SHELL:-Xclang -fno-pch-timestamp>")
  mudlet_use_pch_compiler_launcher(${target})
endfunction()

# Test executables are mostly one or two files each, so a precompiled header of
# their own would cost more than it saves. They share one per directory
# instead - one directory, because Qt6::Test defines QT_TESTCASE_BUILDDIR and
# QT_TESTCASE_SOURCEDIR from it, and a precompiled header is only reused under
# the definitions it was built with.
function(mudlet_add_test_pch_provider provider)
  if(CMAKE_DISABLE_PRECOMPILE_HEADERS)
    return()
  endif()
  file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${provider}.cpp" CONTENT "")
  add_library(${provider} OBJECT "${CMAKE_CURRENT_BINARY_DIR}/${provider}.cpp")
  set_target_properties(${provider} PROPERTIES AUTOMOC OFF)
  target_link_libraries(${provider} PRIVATE Qt6::Test ${LIB_MUDLET_TARGET})
  mudlet_precompile_headers(${provider} <QSignalSpy> <QTest>)
endfunction()

function(mudlet_reuse_test_pch target provider)
  if(CMAKE_DISABLE_PRECOMPILE_HEADERS)
    return()
  endif()
  target_precompile_headers(${target} REUSE_FROM ${provider})
  mudlet_use_pch_compiler_launcher(${target})
endfunction()
