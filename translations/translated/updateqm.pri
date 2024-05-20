# Credit: qbittorrent

TS_FILES += $$files(mudlet_*.ts)

# The American translation is plurals only and has to be done separately (by
# hand) so is not to be included in the built-in Qt TRANSLATIONS variable as the
# default processing of it will not have the needed extra `-pluralonly`
TS_FILES -= mudlet_en_US.ts

# need to use full path, otherwise running
# `lupdate` will generate *.ts files in project root directory
for(file, TS_FILES) {
    TRANSLATIONS += "$${PWD}/$${file}"
}

# Now put the file back
TS_FILES += mudlet_en_US.ts

isEmpty(QMAKE_LRELEASE) {
    win32 {
        equals(QT_MAJOR_VERSION, 5) {
            QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease-qt5.exe
        } else {
            QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease-qt6.exe
        }
        !exists($$QMAKE_LRELEASE) {
            QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
        }
    } else {
        equals(QT_MAJOR_VERSION, 5) {
            QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease-qt5
        } else {
            QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease-qt6
        }
        !exists($$QMAKE_LRELEASE) {
            QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
        }
    }
}

write_file(lrelease_output.txt)
message("Building translations")
TS_FILES_NOEXT = $$replace(TS_FILES, ".ts", "")
for(file, TS_FILES_NOEXT) {
    win32 {
        system("$$QMAKE_LRELEASE $${file}.ts -compress -qm $${file}.qm >> lrelease_output.txt")
    } else {
        system("LANG=C $$QMAKE_LRELEASE $${file}.ts -compress -qm $${file}.qm >> lrelease_output.txt")
    }
}
STATS_GENERATOR = $$system_path("$${PWD}/generate-translation-stats.lua")
# message("Will use \"$${STATS_GENERATOR}\" to parse translation statistics...")

win32 {
    CMD = "where"
    message("You can safely ignore one to three \"INFO: Could not find files for the given pattern(s).\" messages that may appear!")
} else {
    # OpenBSD reports the failure to find lua5.1 loudly enough to be seen;
    # so explictly bin the stderr output:
    CMD = "which 2>/dev/null"
}

win32 {
    # The '.1' in the file name confuses the QMake system() utility or the
    # shell used in the Windows case as it is considered to be an extension
    # unless we put in the actual 'exe' extension as well:
    LUA_SEARCH_OUT = $$system("$$CMD lua-5.1.exe")
} else {
    LUA_SEARCH_OUT = $$system("$$CMD lua-5.1")
}
isEmpty(LUA_SEARCH_OUT) {
    win32 {
        LUA_SEARCH_OUT = $$system("$$CMD lua5.1.exe")
    } else {
        LUA_SEARCH_OUT = $$system("$$CMD lua5.1")
    }
    isEmpty(LUA_SEARCH_OUT) {
        # This can be used for all OS since there is no '.'
        LUA_SEARCH_OUT = $$system("$$CMD lua51")
        isEmpty(LUA_SEARCH_OUT) {
            LUA_SEARCH_OUT = $$system("$$CMD lua")
            isEmpty(LUA_SEARCH_OUT) {
                error("no lua found in PATH")
            } else {
               # "lua" worked
               LUA_COMMAND = "lua"
            }

        } else {
            # "lua51" worked
            LUA_COMMAND = "lua51"
        }

    } else {
        # "lua5.1" worked
        win32 {
            LUA_COMMAND = "lua5.1.exe"
        } else {
            LUA_COMMAND = "lua5.1"
        }
    }

} else {
    # "lua-5.1" worked
    win32 {
        LUA_COMMAND = "lua-5.1.exe"
    } else {
        LUA_COMMAND = "lua-5.1"
    }
}

# message("Running: $$LUA_COMMAND $$STATS_GENERATOR ...")
system("$$LUA_COMMAND $$STATS_GENERATOR")
