// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



do {
    const UInt8    *_pCompare;
    uxint           _uCandidate;
#if DEEP_LOOKUP
    uxint           _uMaxDepth = (uMaxDepth);
#endif 

     xint           _iOffset;
    _uCandidate = pNext[uPosition];
    pNext[0]    = (UInt32) uPosition;

    for (;;)
    {
        uxint           _uCandidate2;
        {
            TAIL_T          _uTailComperand;

            _pCompare       = pData + uBestLength - (sizeof (TAIL_T) - 1);
            _uTailComperand = * (TAIL_T *) (_pCompare + uPosition);

            for (;;)
            {
#if DEEP_LOOKUP
                if (_uMaxDepth == 0)
                    goto L_Done;
                --_uMaxDepth;
#endif 


                _uCandidate2 = pNext[_uCandidate];
                if (* (const TAIL_T *) (_pCompare + _uCandidate) == _uTailComperand)
                    goto L_Same1;

                _uCandidate = pNext[_uCandidate2];
                if (* (const TAIL_T *) (_pCompare + _uCandidate2) == _uTailComperand)
                    goto L_Same2;

                _uCandidate2 = pNext[_uCandidate];
                if (* (const TAIL_T *) (_pCompare + _uCandidate) == _uTailComperand)
                    goto L_Same1;

                _uCandidate = pNext[_uCandidate2];
                if (* (const TAIL_T *) (_pCompare + _uCandidate2) == _uTailComperand)
                    goto L_Same2;

                _uCandidate2 = pNext[_uCandidate];
                if (* (const TAIL_T *) (_pCompare + _uCandidate) == _uTailComperand)
                    goto L_Same1;

                _uCandidate = pNext[_uCandidate2];
                if (* (const TAIL_T *) (_pCompare + _uCandidate2) == _uTailComperand)
                    goto L_Same2;

                _uCandidate2 = pNext[_uCandidate];
                if (* (const TAIL_T *) (_pCompare + _uCandidate) == _uTailComperand)
                    goto L_Same1;

#if DEEP_LOOKUP
                _uCandidate = pNext[_uCandidate2];
                if (* (const TAIL_T *) (_pCompare + _uCandidate2) == _uTailComperand)
                    goto L_Same2;
#else
                if (* (const TAIL_T *) (_pCompare + _uCandidate2) == _uTailComperand)
                    goto L_Same2;
                goto L_Done;
#endif 
            }                                                                           
        }
    L_Same2:
        _uCandidate = _uCandidate2;
    L_Same1:

        if (_uCandidate >= uPosition)
            goto L_Done;

        pData += uPosition;

        {
            _iOffset  = _uCandidate - uPosition;

            _pCompare = pData;
            while (*_pCompare == _pCompare[_iOffset])
            {
                ++_pCompare;
                if (_pCompare >= pEndData)
                    goto L_DoneEof;
            }
                
            _uCandidate2 = (uxint) (_pCompare - pData);
            if (
                _uCandidate2 > uBestLength &&
                (_uCandidate2 > sizeof (s_iMaxOffsetByLength) / sizeof (s_iMaxOffsetByLength[0]) - 1 
                  || _iOffset > s_iMaxOffsetByLength[_uCandidate2])
            )
            {
                uBestLength = _uCandidate2;
                iBestOffset = _iOffset;
            }
        }

        pData -= uPosition;

#if DEEP_LOOKUP
        _uCandidate = pNext[_uCandidate];
#else
        goto L_Done;
#endif 
    }
L_DoneEof:
    uBestLength = (uxint) (_pCompare - pData);
    iBestOffset = _iOffset;
    pData      -= uPosition;

L_Done:;
} while (0);
