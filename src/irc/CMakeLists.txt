############################################################################
# Communi                                                                  #
#                                                                          #
#    Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            #
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

IF(MSVC)
  CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)
ELSE()
  CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)
ENDIF()

IF(POLICY CMP0020)
  CMAKE_POLICY(SET CMP0020 NEW)
ENDIF()

PROJECT(communi)

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)

SET(SRCS src/irc.cpp src/irccodecplugin.cpp src/irccommand.cpp src/ircdecoder.cpp src/ircmessage.cpp src/ircparser.cpp src/ircsender.cpp src/ircsession.cpp src/ircutil.cpp)
SET(HDRS include/ircdecoder_p.h include/ircparser_p.h include/ircsession_p.h)
SET(MOC_HDRS include/irc.h include/irccodecplugin.h include/irccommand.h include/ircmessage.h include/ircsession.h)
SET(PUB_HDRS ${MOC_HDRS} include/ircglobal.h include/ircsender.h include/ircutil.h include/Irc include/IrcCodecPlugin include/IrcCommand include/IrcGlobal include/IrcMessage include/IrcSender include/IrcSession include/IrcUtil)

QT5_GENERATE_MOC(include/irc.h ${CMAKE_CURRENT_BINARY_DIR}/moc_irc.cpp)
QT5_GENERATE_MOC(include/irccodecplugin.h ${CMAKE_CURRENT_BINARY_DIR}/moc_irccodecplugin.cpp)
QT5_GENERATE_MOC(include/irccommand.h ${CMAKE_CURRENT_BINARY_DIR}/moc_irccommand.cpp)
QT5_GENERATE_MOC(include/ircmessage.h ${CMAKE_CURRENT_BINARY_DIR}/moc_ircmessage.cpp)
QT5_GENERATE_MOC(include/ircsession.h ${CMAKE_CURRENT_BINARY_DIR}/moc_ircsession.cpp)

SET_PROPERTY(SOURCE src/ircsession.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_ircsession.cpp)

ADD_LIBRARY(irccqt STATIC ${SRCS} ${HDRS} ${PUB_HDRS}
  ${CMAKE_CURRENT_BINARY_DIR}/moc_irc.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_irccodecplugin.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_irccommand.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_ircmessage.cpp
)

TARGET_LINK_LIBRARIES(irccqt ${Qt5Core_LIBRARIES} ${Qt5Network_LIBRARIES})

INCLUDE_DIRECTORIES(${Qt5Core_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
