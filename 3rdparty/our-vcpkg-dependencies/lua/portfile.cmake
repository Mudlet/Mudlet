include(vcpkg_common_functions)

vcpkg_download_distfile(ARCHIVE
    URLS "https://www.lua.org/ftp/lua-5.1.5.tar.gz"
    FILENAME "lua-5.1.5.tar.gz"
    SHA512 0142fefcbd13afcd9b201403592aa60620011cc8e8559d4d2db2f92739d18186860989f48caa45830ff4f99bfc7483287fd3ff3a16d4dec928e2767ce4d542a9
)
vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
# patch disabled as it doesn't apply cleanly and doesn't seem related to the macos build
#    PATCHES vs2015-impl-c99.patch
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        -DCOMPILE_AS_CPP=OFF
    OPTIONS_DEBUG
        -DSKIP_INSTALL_HEADERS=ON
        -DSKIP_INSTALL_TOOLS=ON
)

vcpkg_install_cmake()

if("cpp" IN_LIST FEATURES)
    vcpkg_configure_cmake(
        SOURCE_PATH ${SOURCE_PATH}
        PREFER_NINJA
        OPTIONS
            -DCOMPILE_AS_CPP=ON
        OPTIONS_DEBUG
            -DSKIP_INSTALL_HEADERS=ON
            -DSKIP_INSTALL_TOOLS=ON
    )

    vcpkg_install_cmake()
endif()

if(VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
    if(NOT VCPKG_CMAKE_SYSTEM_NAME OR VCPKG_CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
        file(READ ${CURRENT_PACKAGES_DIR}/include/luaconf.h LUA_CONF_H)
        string(REPLACE "defined(LUA_BUILD_AS_DLL)" "1" LUA_CONF_H "${LUA_CONF_H}")
        file(WRITE ${CURRENT_PACKAGES_DIR}/include/luaconf.h "${LUA_CONF_H}")
    endif()
endif()

vcpkg_copy_tool_dependencies(${CURRENT_PACKAGES_DIR}/tools/lua)

# Handle copyright
file(COPY ${CMAKE_CURRENT_LIST_DIR}/COPYRIGHT DESTINATION ${CURRENT_PACKAGES_DIR}/share/lua/copyright)
# Handle post-build CMake instructions
file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/lua)
vcpkg_copy_pdbs()
