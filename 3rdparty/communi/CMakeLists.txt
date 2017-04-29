############################################################################
# Communi                                                                  #
#                                                                          #
#    Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

IF(POLICY CMP0020)
  CMAKE_POLICY(SET CMP0020 NEW)
ENDIF()

PROJECT(communi)

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)

SET(SRCS
  src/core/irc.cpp
  src/core/irccommand.cpp
  src/core/ircconnection.cpp
  src/core/irccore.cpp
  src/core/ircfilter.cpp
  src/core/ircmessage.cpp
  src/core/ircmessage_p.cpp
  src/core/ircmessagecomposer.cpp
  src/core/ircmessagedecoder.cpp
  src/core/ircmessagedecoder_none.cpp
  src/core/ircnetwork.cpp
  src/core/ircprotocol.cpp
  src/3rdparty/mozilla/rdf_utils.c
)
SET(HDRS
  include/IrcCore/irccommand_p.h
  include/IrcCore/ircconnection_p.h
  include/IrcCore/ircdebug_p.h
  include/IrcCore/ircmessage_p.h
  include/IrcCore/ircmessagecomposer_p.h
  include/IrcCore/ircmessagedecoder_p.h
  include/IrcCore/ircnetwork_p.h
)
SET(MOC_HDRS
  include/IrcCore/irc.h
  include/IrcCore/irccommand.h
  include/IrcCore/ircconnection.h
  include/IrcCore/ircmessage.h
  include/IrcCore/ircnetwork.h
  include/IrcCore/ircprotocol.h
)
SET(PUB_HDRS
  ${MOC_HDRS}
  include/IrcCore/irccore.h
  include/IrcCore/ircfilter.h
  include/IrcCore/ircglobal.h
  include/IrcCore/Irc
  include/IrcCore/IrcCommand
  include/IrcCore/IrcCommandFilter
  include/IrcCore/IrcConnection
  include/IrcCore/IrcCore
  include/IrcCore/IrcGlobal
  include/IrcCore/IrcMessage
  include/IrcCore/IrcMessageFilter
  include/IrcCore/IrcNetwork
  include/IrcCore/IrcProtocol
)

QT5_GENERATE_MOC(include/IrcCore/irc.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_irc.cpp)
QT5_GENERATE_MOC(include/IrcCore/irccommand.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_irccommand.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircconnection.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircconnection.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircmessage.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircmessage.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircmessagecomposer_p.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircmessagecomposer_p.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircnetwork.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircnetwork.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircprotocol.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircprotocol.cpp)

SET_PROPERTY(SOURCE src/core/irc.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_irc.cpp)
SET_PROPERTY(SOURCE src/core/irccommand.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_irccommand.cpp)
SET_PROPERTY(SOURCE src/core/ircconnection.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircconnection.cpp)
SET_PROPERTY(SOURCE src/core/ircmessage.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircmessage.cpp)
SET_PROPERTY(SOURCE src/core/ircmessagecomposer.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircmessagecomposer_p.cpp)
SET_PROPERTY(SOURCE src/core/ircnetwork.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircnetwork.cpp)
SET_PROPERTY(SOURCE src/core/ircprotocol.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircprotocol.cpp)

ADD_LIBRARY(communi STATIC ${SRCS} ${HDRS} ${PUB_HDRS})

TARGET_LINK_LIBRARIES(communi ${Qt5Core_LIBRARIES} ${Qt5Network_LIBRARIES})

TARGET_INCLUDE_DIRECTORIES(communi PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/IrcCore
)
TARGET_INCLUDE_DIRECTORIES(communi PUBLIC
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Network_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/IrcCore
)
