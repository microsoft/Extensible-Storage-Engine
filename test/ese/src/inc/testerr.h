// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  This is a range of errors that ESE test defines for it's own usage in the JET ERR number
//  space.

#define JET_errTestError                    -64   /* Generic test failure. */
#define JET_errTestFailure                  -65   /* Specifically a test's explicit test check failed. */
#define JET_errJetApiLoadFailThunk          -68   /* This is used for the virtual JET_API pfn layer. */


#define JET_errSuccess                       0    /* Successful Operation */
#define JET_errInvalidParameter             -1003 /* Invalid API parameter */
#define JET_errOutOfMemory                  -1011 /* Out of Memory */

