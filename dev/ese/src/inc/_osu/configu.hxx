// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSU_CONFIG_HXX_INCLUDED
#define _OSU_CONFIG_HXX_INCLUDED


//  Persistent Configuration

//  init config subsystem

ERR ErrOSUConfigInit();

//  terminate config subsystem

void OSUConfigTerm();

//  loads a extra diagnostics option.  note: does not change variable if null setting.

ERR ErrLoadDiagOption_( _In_ const INST * const pinst, _In_z_ PCWSTR wszOverrideName, _Out_ BOOL * pfResult );
#define ErrLoadDiagOption( pinst, fVariable )       ErrLoadDiagOption_( pinst, L#fVariable, &fVariable )

#endif  //  _OSU_CONFIG_HXX_INCLUDED

