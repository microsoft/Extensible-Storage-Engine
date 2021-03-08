# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set(MSVC_ESE_LINK_OPTIONS
    # Incremental link option is incompatible with the `/DEBUGTYPE:CV,PDATA,FIXUP` option below
    /INCREMENTAL:NO
    /WX
    /VERSION:"6.0"
    /SECTION:"INIT,d"
    /NODEFAULTLIB
    /DELAYLOAD:WER.DLL
    /DEBUG:FULL
    /MAP
    /SUBSYSTEM:CONSOLE
    # /STACK:<reserve>,<commit>. "Reserve" and "commit" refer to the allocated stack sizes in virtual and physical memory
    /STACK:"0x40000","0x1000"
    /OPT:REF
    /OPT:ICF
    /RELEASE
    /DYNAMICBASE
    /NXCOMPAT
    /MERGE:"_PAGE=PAGE"
    /DEBUGTYPE:CV,PDATA,FIXUP
    /MERGE:_TEXT=.text
    /pdbcompress
    /FULLBUILD
    /OSVERSION:6.0
    # LNK4199: on `eselibwithtests`, `link.exe` warns that there are no imports from `WER.DLL` (treated as error)
    /IGNORE:4010,4013,4037,4039,4065,4070,4078,4087,4089,4221,4198,4199
)

add_compile_definitions(
    _ATL_STATIC_REGISTRY
    DEBUGGER_EXTENSION;
    JET_VERSION=0xFFFF;
    WSZVERSIONNAME=L"ESE";

    # These are expected to be set in, e.g., the Exchange-specific `bldver.h` or the Windows-specific `ntverp.h`.
    # The values of `BUILD_MAJOR` and `BUILD_MINOR` would eventually be based on Azure pipelines' build numbers
    # and read from the environment, e.g., from `$ENV{BUILD_MINOR}`
    PRODUCT_MAJOR="10"
    PRODUCT_MINOR="0"
    BUILD_MAJOR="0"
    BUILD_MINOR="0"
)

include_directories(oslite PRIVATE
    ${ESE_DEV}/published/inc/
    ${ESE_DEV}/published/inc/bf/
    ${ESE_DEV}/published/inc/os/
    ${ESE_DEV}/src/inc/
)

add_subdirectory(published)
add_subdirectory(src)