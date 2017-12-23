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

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

project(communi)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/IrcCore
  ${CMAKE_CURRENT_SOURCE_DIR}/include/IrcModel
  ${CMAKE_CURRENT_SOURCE_DIR}/include/IrcUtil
)

set(SRCS
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
  src/model/ircbuffer.cpp
  src/model/ircbuffermodel.cpp
  src/model/ircchannel.cpp
  src/model/ircmodel.cpp
  src/model/ircuser.cpp
  src/model/ircusermodel.cpp
  src/util/irccommandparser.cpp
  src/util/irccommandqueue.cpp
  src/util/irccompleter.cpp
  src/util/irclagtimer.cpp
  src/util/ircpalette.cpp
  src/util/irctextformat.cpp
  src/util/irctoken.cpp
  src/util/ircutil.cpp
)
set(HDRS
  include/IrcCore/irccommand_p.h
  include/IrcCore/ircconnection_p.h
  include/IrcCore/ircdebug_p.h
  include/IrcCore/ircmessage_p.h
  include/IrcCore/ircmessagecomposer_p.h
  include/IrcCore/ircmessagedecoder_p.h
  include/IrcCore/ircnetwork_p.h
  include/IrcModel/ircuser_p.h
  include/IrcModel/ircbuffer_p.h
  include/IrcModel/ircchannel_p.h
  include/IrcModel/ircusermodel_p.h
  include/IrcModel/ircbuffermodel_p.h
  include/IrcUtil/irctoken_p.h
  include/IrcUtil/irclagtimer_p.h
  include/IrcUtil/irccommandqueue_p.h
  include/IrcUtil/irccommandparser_p.h
)
set(MOC_HDRS
  include/IrcCore/irc.h
  include/IrcCore/irccommand.h
  include/IrcCore/ircconnection.h
  include/IrcCore/ircmessage.h
  include/IrcCore/ircnetwork.h
  include/IrcCore/ircprotocol.h
  include/IrcModel/ircbuffer.h
  include/IrcModel/ircbuffermodel.h
  include/IrcModel/ircchannel.h
  include/IrcModel/ircmodel.h
  include/IrcModel/ircuser.h
  include/IrcModel/ircusermodel.h
  include/IrcUtil/irccommandparser.h
  include/IrcUtil/irccommandqueue.h
  include/IrcUtil/irccompleter.h
  include/IrcUtil/irclagtimer.h
  include/IrcUtil/ircpalette.h
  include/IrcUtil/irctextformat.h
  include/IrcUtil/ircutil.h
)
set(PUB_HDRS
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
  include/IrcModel/IrcBuffer
  include/IrcModel/IrcBufferModel
  include/IrcModel/IrcChannel
  include/IrcModel/IrcModel
  include/IrcModel/IrcUser
  include/IrcModel/IrcUserModel
  include/IrcUtil/IrcCommandParser
  include/IrcUtil/IrcCommandQueue
  include/IrcUtil/IrcCompleter
  include/IrcUtil/IrcLagTimer
  include/IrcUtil/IrcPalette
  include/IrcUtil/IrcTextFormat
  include/IrcUtil/IrcUtil
)

QT5_GENERATE_MOC(include/IrcCore/irc.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_irc.cpp)
QT5_GENERATE_MOC(include/IrcCore/irccommand.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_irccommand.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircconnection.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircconnection.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircmessage.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircmessage.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircmessagecomposer_p.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircmessagecomposer_p.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircnetwork.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircnetwork.cpp)
QT5_GENERATE_MOC(include/IrcCore/ircprotocol.h ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircprotocol.cpp)
QT5_GENERATE_MOC(include/IrcModel/ircusermodel.h ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircusermodel.cpp)
QT5_GENERATE_MOC(include/IrcModel/ircchannel.h ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircchannel.cpp)
QT5_GENERATE_MOC(include/IrcModel/ircbuffer.h ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircbuffer.cpp)
QT5_GENERATE_MOC(include/IrcModel/ircbuffermodel_p.h ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircbuffermodel_p.cpp)
QT5_GENERATE_MOC(include/IrcModel/ircbuffermodel.h ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircbuffermodel.cpp)
QT5_GENERATE_MOC(include/IrcModel/ircuser.h ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircuser.cpp)
QT5_GENERATE_MOC(include/IrcModel/ircusermodel.h ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircusermodel.cpp)
QT5_GENERATE_MOC(include/IrcUtil/irccompleter.h ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irccompleter.cpp)
QT5_GENERATE_MOC(include/IrcUtil/irccommandparser.h ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irccommandparser.cpp)
QT5_GENERATE_MOC(include/IrcUtil/irccommandqueue.h ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irccommandqueue.cpp)
QT5_GENERATE_MOC(include/IrcUtil/irccommandqueue_p.h ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irccommandqueue_p.cpp)
QT5_GENERATE_MOC(include/IrcUtil/irclagtimer.h ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irclagtimer.cpp)
QT5_GENERATE_MOC(include/IrcUtil/irclagtimer_p.h ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irclagtimer_p.cpp)
QT5_GENERATE_MOC(include/IrcUtil/irctextformat.h ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irctextformat.cpp)
QT5_GENERATE_MOC(include/IrcUtil/ircpalette.h ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_ircpalette.cpp)

set_property(SOURCE src/core/irc.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_irc.cpp)
set_property(SOURCE src/core/irccommand.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_irccommand.cpp)
set_property(SOURCE src/core/ircconnection.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircconnection.cpp)
set_property(SOURCE src/core/ircmessage.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircmessage.cpp)
set_property(SOURCE src/core/ircmessagecomposer.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircmessagecomposer_p.cpp)
set_property(SOURCE src/core/ircnetwork.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircnetwork.cpp)
set_property(SOURCE src/core/ircprotocol.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcCore/moc_ircprotocol.cpp)
set_property(SOURCE src/model/ircusermodel.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircusermodel.cpp)
set_property(SOURCE src/model/ircchannel.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircchannel.cpp)
set_property(SOURCE src/model/ircbuffer.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircbuffer.cpp)
set_property(SOURCE src/model/ircbuffermodel.cpp 
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircbuffermodel.cpp)
set_property(SOURCE src/model/ircbuffermodel.cpp 
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircbuffermodel_p.cpp)
set_property(SOURCE src/model/ircuser.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircuser.cpp)
set_property(SOURCE src/model/ircusermodel.cpp 
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcModel/moc_ircusermodel.cpp)
set_property(SOURCE src/util/irccompleter.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irccompleter.cpp)
set_property(SOURCE src/util/irccommandparser.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irccommandparser.cpp)
set_property(SOURCE src/util/irccommandqueue.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irccommandqueue.cpp)
set_property(SOURCE src/util/irccommandqueue.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irccommandqueue_p.cpp)
set_property(SOURCE src/util/irclagtimer.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irclagtimer.cpp)
set_property(SOURCE src/util/irclagtimer.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irclagtimer_p.cpp)
set_property(SOURCE src/util/irctextformat.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_irctextformat.cpp)
set_property(SOURCE src/util/ircpalette.cpp
  APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil/moc_ircpalette.cpp)

add_library(communi STATIC ${SRCS} ${HDRS} ${PUB_HDRS})

target_link_libraries(communi ${Qt5Core_LIBRARIES} ${Qt5Network_LIBRARIES})

target_include_directories(communi PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/IrcCore
  ${CMAKE_CURRENT_BINARY_DIR}/IrcModel
  ${CMAKE_CURRENT_BINARY_DIR}/IrcUtil
)
target_include_directories(communi PUBLIC
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Network_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/IrcCore
  ${CMAKE_CURRENT_SOURCE_DIR}/include/IrcModel
  ${CMAKE_CURRENT_SOURCE_DIR}/include/IrcUtil
)
