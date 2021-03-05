// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#pragma warning( push, 3 )


#include <vector>
#pragma warning( pop ) 
#include "esetest.h"

class ESEFreeze {
public:
    typedef std::vector<JET_INSTANCE> JINSTVEC;
    typedef enum { stNone, stFreezing, stFrozen, stThawing, stThawed } FROZEN_STATE ;

    static JINSTVEC GetJInstVec( void ) { return s_jInstVec; }
    static void SetJInstVec( JINSTVEC jInstVec ) { s_jInstVec = jInstVec; }
    static FROZEN_STATE GetFrozenState( void ) { return s_stFrozen; }
    static void SetFrozenState( FROZEN_STATE stFrozen ) { s_stFrozen = stFrozen; }

protected:
    static JINSTVEC s_jInstVec;
    static FROZEN_STATE s_stFrozen;
};

