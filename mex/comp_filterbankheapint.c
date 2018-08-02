#ifndef _LTFAT_MEX_FILE
#define _LTFAT_MEX_FILE

#define ISNARGINEQ 11
#define TYPEDEPARGS 0, 1, 2, 4, 5
#define SINGLEARGS
#define REALARGS

#endif /* _LTFAT_MEX_FILE */

#define MEX_FILE __BASE_FILE__
#include "ltfat_mex_template_helper.h"

#if defined(LTFAT_SINGLE) || defined(LTFAT_DOUBLE)
#include "ltfat_types.h"

// Calling convention:
//                              0     1     2     3       4     5 6 7 8         9  10        11
// phase=comp_filterbankheapint(s,tgrad,fgrad,neigh,posInfo,cfreq,a,M,N,chanStart,tol,phasetype);

// phasetype defines how to adjust tgrad and fgrad such that
// phase corresponds to:
// phasetype  1:  timeinv
// phasetype  2:  do not adjust the gradient, it is already correct

void LTFAT_NAME(ltfatMexFnc)( int UNUSED(nlhs), mxArray* plhs[],
                              int UNUSED(nrhs), const mxArray* prhs[] )
{
    // Get inputs
    const mxArray* mxs  = prhs[0];
    const mxArray* mxneigh  = prhs[3];
    const mxArray* mxposinfo  = prhs[4];

    const LTFAT_REAL* s = mxGetData(mxs);

    const LTFAT_REAL* tgrad = mxGetData(prhs[1]);
    const LTFAT_REAL* fgrad = mxGetData(prhs[2]);
    const LTFAT_REAL* cfreq = mxGetData(prhs[5]);

    double* a = mxGetData(prhs[6]);
    mwSize M = (mwSize)mxGetScalar(prhs[7]);

    ltfatInt NPtr[M];

    double* N  = mxGetData(prhs[8]);

    for (mwSize ii = 0; ii < M; ++ii)
        NPtr[ii] = (ltfatInt) N[ii];

    LTFAT_REAL tol = (LTFAT_REAL) mxGetScalar(prhs[9]);
    int phasetype = (int)mxGetScalar(prhs[10]);

    const ltfatInt Nsum = mxGetM(mxs);

    mwSize neighLen = mxGetNumberOfElements(mxneigh);
    ltfatInt* neighPtr = ltfat_malloc(neighLen * sizeof * neighPtr);

    const double* neighDoublePtr = mxGetData(mxneigh);
    for (mwSize ii = 0; ii < neighLen; ++ii)
        neighPtr[ii] = (ltfatInt) neighDoublePtr[ii];


    const LTFAT_REAL* posinfoPtr = mxGetData(mxposinfo);

    // Get matrix dimensions.
    ltfatInt W = mxGetN(mxs);

    // Create output matrix and zero it.
    plhs[0] = ltfatCreateNdimArray(mxGetNumberOfDimensions(mxs),
                                   mxGetDimensions(mxs),
                                   LTFAT_MX_CLASSID, mxREAL);

    // Get pointer to output
    LTFAT_REAL* phase = mxGetData(plhs[0]);

    //mexPrintf("L=%i, n[0] = %i, a = %f, M = %i \n",L,neighPtr[0],a[0],M);

    if (phasetype == 1)
        LTFAT_NAME(filterbankheapint)(s, tgrad, fgrad, neighPtr, posinfoPtr, cfreq,
                               a, M, NPtr, Nsum, W, tol, phase);
    else
        LTFAT_NAME(filterbankheapint_relgrad)(s , tgrad, fgrad, neighPtr, posinfoPtr, cfreq,
                                       a, M, NPtr, Nsum, W, tol, phase);

    ltfat_free(neighPtr);
}
#endif /* LTFAT_SINGLE or LTFAT_DOUBLE */
