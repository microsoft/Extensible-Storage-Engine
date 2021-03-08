# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_custom_command(
    OUTPUT
        ${GEN_OUTPUT_DIRECTORY}/ese/jetapitable.cxx
    COMMAND
        perl ${ESE_DEV}/src/ese/genapitable.pl ${ESE_DEV}/src/inc/_jet.hxx ${GEN_OUTPUT_DIRECTORY}/ese/jetapitable.cxx
    DEPENDS
        ${ESE_DEV}/src/inc/_jet.hxx
        ${ESE_DEV}/src/ese/genapitable.pl
)

add_custom_target(gen_jetapitable DEPENDS ${GEN_OUTPUT_DIRECTORY}/ese/jetapitable.cxx)

add_compile_definitions(
    _HAS_EXCEPTIONS=0
    _MBCS
    DELAYLOAD_VERSION=0x0200
    FILETYPE_DLL
    PERFMON_SUPPORT
)

set(ESE_SOURCES
    ${ESE_DEV}/src/ese/backup.cxx
    ${ESE_DEV}/src/ese/bf.cxx
    ${ESE_DEV}/src/ese/bt.cxx
    ${ESE_DEV}/src/ese/callback.cxx
    ${ESE_DEV}/src/ese/cat.cxx
    ${ESE_DEV}/src/ese/comp.cxx
    ${ESE_DEV}/src/ese/compression.cxx
    ${ESE_DEV}/src/ese/cpage.cxx
    ${ESE_DEV}/src/ese/cresmgr.cxx
    ${ESE_DEV}/src/ese/dataserializer.cxx
    ${ESE_DEV}/src/ese/db.cxx
    ${ESE_DEV}/src/ese/dbdump.cxx
    ${ESE_DEV}/src/ese/dbscan.cxx
    ${ESE_DEV}/src/ese/dbshrink.cxx
    ${ESE_DEV}/src/ese/dbtask.cxx
    ${ESE_DEV}/src/ese/dbutil.cxx
    ${ESE_DEV}/src/ese/dir.cxx
    ${ESE_DEV}/src/ese/esedump.cxx
    ${ESE_DEV}/src/ese/fcb.cxx
    ${ESE_DEV}/src/ese/fcreate.cxx
    ${ESE_DEV}/src/ese/fileopen.cxx
    ${ESE_DEV}/src/ese/fldenum.cxx
    ${ESE_DEV}/src/ese/fldext.cxx
    ${ESE_DEV}/src/ese/fldmod.cxx
    ${ESE_DEV}/src/ese/fldnorm.cxx
    ${ESE_DEV}/src/ese/flushmap.cxx
    ${ESE_DEV}/src/ese/fmp.cxx
    ${ESE_DEV}/src/ese/fucb.cxx
    ${ESE_DEV}/src/ese/info.cxx
    ${ESE_DEV}/src/ese/io.cxx
    ${ESE_DEV}/src/ese/jetapi.cxx
    ${ESE_DEV}/src/ese/kvpstore.cxx
    ${ESE_DEV}/src/ese/logapi.cxx
    ${ESE_DEV}/src/ese/logdiff.cxx
    ${ESE_DEV}/src/ese/lv.cxx
    ${ESE_DEV}/src/ese/mem.cxx
    ${ESE_DEV}/src/ese/node.cxx
    ${ESE_DEV}/src/ese/old.cxx
    ${ESE_DEV}/src/ese/pib.cxx
    ${ESE_DEV}/src/ese/prl.cxx
    ${ESE_DEV}/src/ese/rbsdump.cxx
    ${ESE_DEV}/src/ese/rec.cxx
    ${ESE_DEV}/src/ese/recupd.cxx
    ${ESE_DEV}/src/ese/repair.cxx
    ${ESE_DEV}/src/ese/revertsnapshot.cxx
    ${ESE_DEV}/src/ese/scrub.cxx
    ${ESE_DEV}/src/ese/sort.cxx
    ${ESE_DEV}/src/ese/sortapi.cxx
    ${ESE_DEV}/src/ese/space.cxx
    ${ESE_DEV}/src/ese/stats.cxx
    ${ESE_DEV}/src/ese/sysinit.cxx
    ${ESE_DEV}/src/ese/sysver.cxx
    ${ESE_DEV}/src/ese/tagfld.cxx
    ${ESE_DEV}/src/ese/taskmgr.cxx
    ${ESE_DEV}/src/ese/tm.cxx
    ${ESE_DEV}/src/ese/ttmap.cxx
    ${ESE_DEV}/src/ese/upgrade.cxx
    ${ESE_DEV}/src/ese/util.cxx
    ${ESE_DEV}/src/ese/ver.cxx
)

set(ESE_LIBRARIES
    dht
    sync
    _errstr
    _xpress
    _perfctrs
    _log
    _osu
    checksum
    checksum_avx
    oswinnt
)

add_subdirectory(_log)
add_subdirectory(_osu)
add_subdirectory(eselibwithtests)
add_subdirectory(server)