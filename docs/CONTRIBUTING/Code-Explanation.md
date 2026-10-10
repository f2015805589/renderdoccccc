# Code Explanation

This is a rough 'table of contents' overview of how the main code components are organised:

    renderccccc/
        CMakeLists.txt           ; The cmake file, will recurse into subdirectories to build them
        renderccccc.sln            ; VS2015 solution for windows building
        renderccccc/
            3rdparty/            ; third party utilities & libraries included
            drivers/             ; API-specific back-ends, can be individually skipped/removed
            ...                  ; everything else in here consists of the core renderccccc runtime
        renderccccccmd/            ; A small C++ utility program that runs to do various little tasks
        rendercccccshim/           ; A tiny C DLL using only kernel32.dll that is used for global hooking
        qrenderccccc/              ; The Qt UI layer built on top of renderccccc/
        docs/                    ; source documentation for the .chm file or http://docs.renderccccc.org/
        util/                    ; folder for utility/support files - e.g. build scripts, installers, CI config
