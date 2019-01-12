# Credit: qbittorrent

TS_FILES += $$files(mudlet_*.ts)

# need to use full path, otherwise running
# `lupdate` will generate *.ts files in project root directory
for(file, TS_FILES) {
    TRANSLATIONS += "$${PWD}/$${file}"
}

isEmpty(QMAKE_LRELEASE) {
    win32 {
        QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    } else {
        QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
        !exists($$QMAKE_LRELEASE) {
            equals(QT_MAJOR_VERSION, 5) {
                QMAKE_LRELEASE = lrelease-qt5
            } else {
                QMAKE_LRELEASE = lrelease
            }
        }
    }
}

write_file(lrelease_output.txt)
message("Building translations")
TS_FILES_NOEXT = $$replace(TS_FILES, ".ts", "")
for(file, TS_FILES_NOEXT) {
    system("$$QMAKE_LRELEASE $${file}.ts -compress -qm $${file}.qm >> lrelease_output.txt")
}
STATS_GENERATOR = $$shell_path("$${PWD}/generate-translation-stats.lua")

win32 {
    CMD = "where"
    message("You can safely ignore one or two \"INFO: Could not find files for the given pattern(s).\" messages that may appear!")
} else {
    CMD = "which"
}

LUA_SEARCH_OUT = $$system("$$CMD lua5.1")
isEmpty(LUA_SEARCH_OUT) {
    LUA_SEARCH_OUT = $$system("$$CMD lua51")
    isEmpty(LUA_SEARCH_OUT) {
        LUA_SEARCH_OUT = $$system("$$CMD lua")
        isEmpty(LUA_SEARCH_OUT) {
            error("no lua found in PATH")
        } else {
            LUA_COMMAND = "lua"
        }
    } else {
        LUA_COMMAND = "lua51"
    }
} else {
    LUA_COMMAND = "lua5.1"
}

system("$$LUA_COMMAND $$STATS_GENERATOR")
