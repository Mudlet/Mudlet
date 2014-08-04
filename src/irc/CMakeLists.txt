######################################################################
# LibIrcClient-Qt
######################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

PROJECT(ircclient-qt)

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)

SET (SRCS src/irc.cpp src/ircbuffer.cpp src/ircsession.cpp src/ircutil.cpp)
SET (HDRS include/ircbuffer_p.h include/ircsession_p.h)
SET (MOC_HDRS include/irc.h include/ircbuffer.h include/ircsession.h)
SET (PUB_HDRS ${MOC_HDRS} include/ircglobal.h include/ircutil.h include/Irc include/IrcBuffer include/IrcGlobal include/IrcSession include/IrcUtil)

QT5_GENERATE_MOC(include/ircbuffer.h ${CMAKE_CURRENT_BINARY_DIR}/moc_ircbuffer.cpp)
QT5_GENERATE_MOC(include/ircsession.h ${CMAKE_CURRENT_BINARY_DIR}/moc_ircsession.cpp)

SET_PROPERTY(SOURCE src/ircbuffer.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_ircbuffer.cpp)
SET_PROPERTY(SOURCE src/ircsession.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_ircsession.cpp)

ADD_LIBRARY(irccqt STATIC ${SRCS} ${HDRS} ${PUB_HDRS})
TARGET_LINK_LIBRARIES(irccqt ${Qt5Core_LIBRARIES} ${Qt5Network_LIBRARIES})

INCLUDE_DIRECTORIES(${Qt5Core_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
