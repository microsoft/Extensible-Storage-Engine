// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSU_TIME_HXX_INCLUDED
#define _OSU_TIME_HXX_INCLUDED



typedef double DATESERIAL;


void UtilGetDateTime( DATESERIAL* pdt );



ERR ErrOSUTimeInit();


void OSUTimeTerm();


#endif


