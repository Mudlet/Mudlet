######## Project settings
cmake_minimum_required(VERSION 2.8)
set (PACKAGE_NAME opencc)
project (${PACKAGE_NAME} CXX)
enable_testing()

######## Package information
set (PACKAGE_URL http://code.google.com/p/uchardet/)
set (PACKAGE_BUGREPORT http://code.google.com/p/uchardet/issues/entry)
set (UCHARDET_VERSION_MAJOR 0)
set (UCHARDET_VERSION_MINOR 0)
set (UCHARDET_VERSION_REVISION 1)

if (CMAKE_BUILD_TYPE MATCHES Debug)
	set (version_suffix .Debug)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

set (
	UCHARDET_VERSION
	${UCHARDET_VERSION_MAJOR}.${UCHARDET_VERSION_MINOR}.${UCHARDET_VERSION_REVISION}${version_suffix}
)

######## Windows

if (WIN32)
	set(CMAKE_SHARED_LIBRARY_PREFIX ${CMAKE_INSTALL_PREFIX})
	set(CMAKE_STATIC_LIBRARY_PREFIX ${CMAKE_INSTALL_PREFIX})
endif (WIN32)

######## Directory

set (DIR_PREFIX ${CMAKE_INSTALL_PREFIX})
set (DIR_LIBRARY ${DIR_PREFIX}/${CMAKE_SHARED_LIBRARY_PREFIX})
set (DIR_LIBRARY_STATIC ${DIR_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX})
set (DIR_INCLUDE ${DIR_PREFIX}/include)
set (DIR_SHARE ${DIR_PREFIX}/share)
set (DIR_BIN ${DIR_PREFIX}/bin)
set (DIR_ETC ${DIR_PREFIX}/etc)

if (DEFINED CMAKE_INSTALL_LIBDIR)
	set (DIR_LIBRARY ${CMAKE_INSTALL_LIBDIR})
	set (DIR_LIBRARY_STATIC ${CMAKE_INSTALL_LIBDIR})
endif (DEFINED CMAKE_INSTALL_LIBDIR)

if (DEFINED SHARE_INSTALL_PREFIX)
	set (DIR_SHARE ${SHARE_INSTALL_PREFIX})
endif (DEFINED SHARE_INSTALL_PREFIX)

if (DEFINED INCLUDE_INSTALL_DIR)
	set (DIR_INCLUDE ${INCLUDE_INSTALL_DIR})
endif (DEFINED INCLUDE_INSTALL_DIR)

if (DEFINED SYSCONF_INSTALL_DIR)
	set (DIR_ETC ${SYSCONF_INSTALL_DIR})
endif (DEFINED SYSCONF_INSTALL_DIR)

set (DIR_SHARE_UCHARDET ${DIR_SHARE}/opencc)
set (DIR_SHARE_LOCALE ${DIR_SHARE}/locale)

######## Configuration

configure_file(
	uchardet.pc.in
	uchardet.pc
	@ONLY
)

install(
	FILES
		${CMAKE_BINARY_DIR}/uchardet.pc
	DESTINATION
		${DIR_LIBRARY}/pkgconfig
)

######## Subdirectories

add_subdirectory(src)
add_subdirectory(doc)
