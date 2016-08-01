#include "ltfat.h"
#include "ltfat/types.h"
#include "ltfat/macros.h"

// These are non-public function header templates
typedef int LTFAT_NAME(realtocomplextransform)(void* userdata,
        const LTFAT_REAL* in, const ltfatInt, LTFAT_COMPLEX* out);

typedef int LTFAT_NAME(complextorealtransform)(void* userdata,
        const LTFAT_COMPLEX* in, const ltfatInt W, LTFAT_REAL* out);

struct LTFAT_NAME(rtdgtreal_plan)
{
    LTFAT_REAL* g; //!< Window
    ltfatInt gl; //!< Window length
    ltfatInt M; //!< Number of FFT channels
    rtdgt_phasetype ptype; //!< Phase convention
    LTFAT_REAL* fftBuf; //!< Internal buffer
    ltfatInt fftBufLen; //!< Internal buffer length
    LTFAT_FFTW(plan) pfft; //!< FFTW plan
};


int
LTFAT_NAME(rtdgtreal_commoninit)(const LTFAT_REAL* g, const ltfatInt gl,
                                 const ltfatInt M, const rtdgt_phasetype ptype,
                                 const ltfat_transformdirection tradir,
                                 LTFAT_NAME(rtdgtreal_plan)** pout)
{
    LTFAT_NAME(rtdgtreal_plan)* p = NULL;

    int status = LTFATERR_SUCCESS;
    CHECK(LTFATERR_NOTPOSARG, gl > 0, "gl must be positive");
    CHECK(LTFATERR_NOTPOSARG, M > 0, "M must be positive");

    CHECKMEM( p = ltfat_calloc(1, sizeof * p) );

    ltfatInt M2 = M / 2 + 1;
    p->fftBufLen = gl > 2 * M2 ? gl : 2 * M2;

    CHECKMEM( p->g = ltfat_malloc(gl * sizeof * p->g));
    CHECKMEM( p->fftBuf = ltfat_malloc(p->fftBufLen * sizeof * p->fftBuf));
    p->gl = gl;
    p->M = M;
    p->ptype = ptype;

    LTFAT_NAME_REAL(fftshift)(g, gl, p->g);

    if (LTFAT_FORWARD == tradir)
        p->pfft = LTFAT_FFTW(plan_dft_r2c_1d)(M, p->fftBuf, (LTFAT_COMPLEX*)p->fftBuf,
                                              FFTW_MEASURE);
    else if (LTFAT_INVERSE == tradir)
        p->pfft = LTFAT_FFTW(plan_dft_c2r_1d)(M, (LTFAT_COMPLEX*)p->fftBuf, p->fftBuf,
                                              FFTW_MEASURE);
    else
        CHECKCANTHAPPEN("Unknown transform direction.");

    CHECKINIT(p->pfft, "FFTW plan creation failed.");

    *pout = p;
    return status;
error:
    if (p)
    {
        if (p->g) ltfat_free(p->g);
        if (p->fftBuf) ltfat_free(p->fftBuf);
        if (p->pfft) LTFAT_FFTW(destroy_plan)(p->pfft);
        ltfat_free(p);
    }
    *pout = NULL;
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_init)(const LTFAT_REAL* g, const ltfatInt gl,
                           const ltfatInt M, const rtdgt_phasetype ptype,
                           LTFAT_NAME(rtdgtreal_plan)** p)
{
    return LTFAT_NAME(rtdgtreal_commoninit)(g, gl, M, ptype, LTFAT_FORWARD, p);
}

LTFAT_EXTERN int
LTFAT_NAME(rtidgtreal_init)(const LTFAT_REAL* g, const ltfatInt gl,
                            const ltfatInt M, const rtdgt_phasetype ptype,
                            LTFAT_NAME(rtdgtreal_plan)** p)
{
    return LTFAT_NAME(rtdgtreal_commoninit)(g, gl, M, ptype, LTFAT_INVERSE, p);
}

LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_execute)(const LTFAT_NAME(rtdgtreal_plan)* p,
                              const LTFAT_REAL* f, const ltfatInt W,
                              LTFAT_COMPLEX* c)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(f); CHECKNULL(c);
    CHECK(LTFATERR_NOTPOSARG, W > 0, "W must be positive");

    ltfatInt M = p->M;
    ltfatInt M2 = M / 2 + 1;
    ltfatInt gl = p->gl;
    LTFAT_REAL* fftBuf = p->fftBuf;

    for (ltfatInt w = 0; w < W; w++)
    {
        const LTFAT_REAL* fchan = f + w * gl;
        LTFAT_COMPLEX* cchan = c + w * M2;

        if (p->g)
            for (ltfatInt ii = 0; ii < gl; ii++)
                fftBuf[ii] = fchan[ii] * p->g[ii];

        if (M > gl)
            memset(fftBuf + gl, 0, (M - gl) * sizeof * fftBuf);

        if (gl > M)
            LTFAT_NAME_REAL(fold_array)(fftBuf, gl, M, 0, fftBuf);

        if (p->ptype == LTFAT_RTDGTPHASE_ZERO)
            LTFAT_NAME_REAL(circshift)(fftBuf, M, -(gl / 2), fftBuf );

        LTFAT_FFTW(execute)(p->pfft);

        memcpy(cchan, fftBuf, M2 * sizeof * c);
    }

error:
    return status;
}

int
LTFAT_NAME(rtdgtreal_execute_wrapper)(void* p,
                                      const LTFAT_REAL* f, const ltfatInt W,
                                      LTFAT_COMPLEX* c)
{
    return LTFAT_NAME(rtdgtreal_execute)(p, f, W, c);
}

LTFAT_EXTERN int
LTFAT_NAME(rtidgtreal_execute)(const LTFAT_NAME(rtidgtreal_plan)* p,
                               const LTFAT_COMPLEX* c, const ltfatInt W,
                               LTFAT_REAL* f)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(c); CHECKNULL(f);
    CHECK(LTFATERR_NOTPOSARG, W > 0, "W must be positive");

    ltfatInt M = p->M;
    ltfatInt M2 = M / 2 + 1;
    ltfatInt gl = p->gl;
    LTFAT_REAL* fftBuf = p->fftBuf;

    for (ltfatInt w = 0; w < W; w++)
    {
        const LTFAT_COMPLEX* cchan = c + w * M2;
        LTFAT_REAL* fchan = f + w * gl;

        memcpy(fftBuf, cchan, M2 * sizeof * cchan);

        LTFAT_FFTW(execute)(p->pfft);

        if (p->ptype == LTFAT_RTDGTPHASE_ZERO)
            LTFAT_NAME_REAL(circshift)(fftBuf, M, gl / 2, fftBuf );

        if (gl > M)
            LTFAT_NAME_REAL(periodize_array)(fftBuf, M , gl, fftBuf);

        if (p->g)
            for (ltfatInt ii = 0; ii < gl; ii++)
                fftBuf[ii] *= p->g[ii];

        memcpy(fchan, fftBuf, gl * sizeof * fchan);
    }
error:
    return status;
}

int
LTFAT_NAME(rtidgtreal_execute_wrapper)(void* p,
                                       const LTFAT_COMPLEX* c, const ltfatInt W,
                                       LTFAT_REAL* f)
{
    return LTFAT_NAME(rtidgtreal_execute)(p, c, W, f);
}


LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_done)(LTFAT_NAME(rtdgtreal_plan)** p)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(*p);

    LTFAT_NAME(rtdgtreal_plan)* pp = *p;
    ltfat_free(pp->g);
    ltfat_free(pp->fftBuf);
    LTFAT_FFTW(destroy_plan)(pp->pfft);
    ltfat_free(pp);
    pp = NULL;
error:
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtidgtreal_done)(LTFAT_NAME(rtidgtreal_plan)** p)
{
    return LTFAT_NAME(rtdgtreal_done)(p);
}

/* FWD FIFO */

struct LTFAT_NAME(rtdgtreal_fifo_state)
{
    ltfatInt gl; //!< Window length
    ltfatInt a; //!< Hop size
    LTFAT_REAL* buf; //!< Ring buffer array
    ltfatInt bufLen; //!< Length of the previous
    ltfatInt readIdx; //!< Read pos.
    ltfatInt writeIdx; //!< Write pos.
    ltfatInt Wmax;
};


LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_fifo_init)(ltfatInt fifoLen, ltfatInt procDelay,
                                ltfatInt gl, ltfatInt a, ltfatInt Wmax,
                                LTFAT_NAME(rtdgtreal_fifo_state)** pout)
{
    LTFAT_NAME(rtdgtreal_fifo_state)* p = NULL;

    int status = LTFATERR_SUCCESS;
    CHECK(LTFATERR_NOTPOSARG, fifoLen > 0, "fifoLen must be positive");
    CHECK(LTFATERR_NOTPOSARG, gl > 0, "gl must be positive");
    CHECK(LTFATERR_NOTPOSARG, a > 0, "a must be positive");
    CHECK(LTFATERR_NOTPOSARG, Wmax > 0, "Wmax must be positive");
    CHECK(LTFATERR_BADARG, procDelay >= gl - 1 , "procDelay must be positive");
    CHECK(LTFATERR_BADARG , fifoLen > gl + 1, "fifoLen must be bugger than gl+1");

    CHECKMEM(p = ltfat_calloc(1, sizeof * p));
    CHECKMEM(p->buf = ltfat_calloc( Wmax * (fifoLen + 1), sizeof * p->buf));

    p->bufLen = fifoLen + 1;
    p->a = a; p->gl = gl; p->readIdx = fifoLen + 1 - (procDelay); p->Wmax = Wmax;
    /* LTFAT_NAME(rtdgtreal_fifo_state) retloc = { .bufLen = fifoLen + 1, */
    /*                                            .a = a, .gl = gl, */
    /*                                            .readIdx =  fifoLen + 1 - (procDelay), */
    /*                                            .writeIdx = 0, .Wmax = Wmax */
    /*                                           }; */

    *pout = p;
    return status;
error:
    if (p)
    {
        if (p->buf) ltfat_free(p->buf);
        ltfat_free(p);
    }
    *pout = NULL;
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_fifo_done)(LTFAT_NAME(rtdgtreal_fifo_state)** p)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(*p);
    LTFAT_NAME(rtdgtreal_fifo_state)* pp = *p;
    ltfat_free(pp->buf);
    ltfat_free(pp);
    pp = NULL;
error:
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_fifo_write)(LTFAT_NAME(rtdgtreal_fifo_state)* p,
                                 const LTFAT_REAL** buf,
                                 const ltfatInt bufLen, const ltfatInt W)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(buf);
    CHECK(LTFATERR_NOTPOSARG, W > 0, "W must be positive.");
    for (ltfatInt w = 0; w < W; w++)
        CHECKNULL(buf[w]);

    CHECK(LTFATERR_NOTPOSARG, bufLen > 0, "bufLen must be positive.");

    ltfatInt freeSpace = p->readIdx - p->writeIdx - 1;
    if (freeSpace < 0) freeSpace += p->bufLen;

    // CHECK(LTFATERR_OVERFLOW, freeSpace, "FIFO owerflow");

    ltfatInt Wact = p->Wmax < W ? p->Wmax : W;

    ltfatInt toWrite = bufLen > freeSpace ? freeSpace : bufLen;
    ltfatInt valid = toWrite;
    ltfatInt over = 0;

    ltfatInt endWriteIdx = p->writeIdx + toWrite;

    if (endWriteIdx > p->bufLen)
    {
        valid = p->bufLen - p->writeIdx;
        over = endWriteIdx - p->bufLen;
    }

    if (valid > 0)
    {
        for (ltfatInt w = 0; w < p->Wmax; w++)
        {
            LTFAT_REAL* pbufchan = p->buf + w * p->bufLen + p->writeIdx;
            if (w < Wact)
                memcpy(pbufchan, buf[w], valid * sizeof * p->buf );
            else
                memset(pbufchan, 0, valid * sizeof * p->buf );
        }
    }
    if (over > 0)
    {
        for (ltfatInt w = 0; w < Wact; w++)
        {
            LTFAT_REAL* pbufchan = p->buf + w * p->bufLen;
            if (w < Wact)
                memcpy(pbufchan, buf[w] + valid, over * sizeof * p->buf);
            else
                memset(pbufchan, 0,  over * sizeof * p->buf);
        }
    }
    p->writeIdx = ( p->writeIdx + toWrite ) % p->bufLen;

    return toWrite;
error:
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_fifo_read)(LTFAT_NAME(rtdgtreal_fifo_state)* p,
                                LTFAT_REAL* buf)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(buf);

    ltfatInt available = p->writeIdx - p->readIdx;
    if (available < 0) available += p->bufLen;

    // CHECK(LTFATERR_UNDERFLOW, available >= p->gl, "FIFO underflow");
    if (available < p->gl) return 0;

    ltfatInt toRead = p->gl;

    ltfatInt valid = toRead;
    ltfatInt over = 0;

    ltfatInt endReadIdx = p->readIdx + valid;

    if (endReadIdx > p->bufLen)
    {
        valid = p->bufLen - p->readIdx;
        over = endReadIdx - p->bufLen;
    }

    if (valid > 0)
    {
        for (ltfatInt w = 0; w < p->Wmax; w++)
        {
            LTFAT_REAL* pbufchan = p->buf + w * p->bufLen + p->readIdx;
            memcpy(buf + w * p->gl, pbufchan, valid * sizeof * p->buf );
        }
    }
    if (over > 0)
    {
        for (ltfatInt w = 0; w < p->Wmax; w++)
        {
            memcpy(buf + valid + w * p->gl, p->buf + w * p->bufLen, over * sizeof * p->buf);
        }
    }

    // Only advance by a
    p->readIdx = ( p->readIdx + p->a ) % p->bufLen;

    return toRead;
error:
    return status;
}

/* BACK FIFO */

struct LTFAT_NAME(rtidgtreal_fifo_state)
{
    ltfatInt gl; //!< Window length
    ltfatInt a; //!< Hop size
    LTFAT_REAL* buf; //!< Ring buffer array
    ltfatInt bufLen; //!< Length of the previous
    ltfatInt readIdx; //!< Read pos.
    ltfatInt writeIdx; //!< Write pos.
    ltfatInt Wmax;
};


LTFAT_EXTERN int
LTFAT_NAME(rtidgtreal_fifo_init)(ltfatInt fifoLen, ltfatInt gl,
                                 ltfatInt a, ltfatInt Wmax,
                                 LTFAT_NAME(rtidgtreal_fifo_state)** pout)
{
    LTFAT_NAME(rtidgtreal_fifo_state)* p = NULL;

    int status = LTFATERR_SUCCESS;
    CHECK(LTFATERR_NOTPOSARG, fifoLen > 0, "fifoLen must be positive");
    CHECK(LTFATERR_NOTPOSARG, gl > 0, "gl must be positive");
    CHECK(LTFATERR_NOTPOSARG, a > 0, "a must be positive");
    CHECK(LTFATERR_NOTPOSARG, Wmax > 0, "Wmax must be positive");
    CHECK(LTFATERR_BADARG , fifoLen > gl + 1, "fifoLen must be bugger than gl+1");

    CHECKMEM( p = ltfat_calloc( 1, sizeof * p));
    CHECKMEM( p->buf = ltfat_calloc( Wmax * (fifoLen + gl + 1), sizeof * p->buf));
    p->a = a; p->gl = gl; p->Wmax = Wmax; p->bufLen = fifoLen + gl + 1;

    /* LTFAT_NAME(rtidgtreal_fifo_state) retloc = {.bufLen = fifoLen + gl + 1, */
    /*                                             .a = a, .gl = gl, */
    /*                                             .readIdx = 0, */
    /*                                             .writeIdx = 0, .Wmax = Wmax */
    /*                                            }; */

    *pout = p;
    return status;
error:
    if (p)
    {
        if (p->buf) ltfat_free(p->buf);
        ltfat_free(p);
    }
    *pout = NULL;
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtidgtreal_fifo_done)(LTFAT_NAME(rtidgtreal_fifo_state)** p)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(*p);
    LTFAT_NAME(rtidgtreal_fifo_state)* pp = *p;
    ltfat_free(pp->buf);
    ltfat_free(pp);
    pp = NULL;
error:
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtidgtreal_fifo_write)(LTFAT_NAME(rtidgtreal_fifo_state)* p,
                                  const LTFAT_REAL* buf)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(buf);

    ltfatInt freeSpace = p->readIdx - p->writeIdx - 1;
    if (freeSpace < 0) freeSpace += p->bufLen;

    // CHECK(LTFATERR_OVERFLOW, freeSpace >= p->gl, "FIFO overflow");
    if (freeSpace < p->gl) return 0;

    ltfatInt toWrite = p->gl;
    ltfatInt valid = toWrite;
    ltfatInt over = 0;

    ltfatInt endWriteIdx = p->writeIdx + toWrite;

    if (endWriteIdx > p->bufLen)
    {
        valid = p->bufLen - p->writeIdx;
        over = endWriteIdx - p->bufLen;
    }

    if (valid > 0)
    {
        for (ltfatInt w = 0; w < p->Wmax; w++)
        {
            LTFAT_REAL* pbufchan = p->buf + p->writeIdx + w * p->bufLen;
            const LTFAT_REAL* bufchan = buf + w * p->gl;
            for (ltfatInt ii = 0; ii < valid; ii++)
                pbufchan[ii] += bufchan[ii];
        }
    }
    if (over > 0)
    {
        for (ltfatInt w = 0; w < p->Wmax; w++)
        {
            LTFAT_REAL* pbufchan = p->buf + w * p->bufLen;
            const LTFAT_REAL* bufchan = buf + valid + w * p->gl;
            for (ltfatInt ii = 0; ii < over; ii++)
                pbufchan[ii] += bufchan[ii];
        }
    }

    p->writeIdx = ( p->writeIdx + p->a ) % p->bufLen;

    return toWrite;
error:
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtidgtreal_fifo_read)(LTFAT_NAME(rtidgtreal_fifo_state)* p,
                                 const ltfatInt bufLen, const ltfatInt W,
                                 LTFAT_REAL** buf)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(buf);
    CHECK(LTFATERR_NOTPOSARG, W > 0, "W must be positive.");

    for (ltfatInt w = 0; w < W; w++)
        CHECKNULL(buf[w]);

    CHECK(LTFATERR_NOTPOSARG, bufLen > 0, "bufLen must be positive.");

    ltfatInt available = p->writeIdx - p->readIdx;
    if (available < 0) available += p->bufLen;

    // CHECK(LTFATERR_UNDERFLOW, available, "FIFO underflow");

    ltfatInt toRead = available < bufLen ? available : bufLen;

    ltfatInt valid = toRead;
    ltfatInt over = 0;

    ltfatInt endReadIdx = p->readIdx + valid;

    if (endReadIdx > p->bufLen)
    {
        valid = p->bufLen - p->readIdx;
        over = endReadIdx - p->bufLen;
    }

    // Set the just read samples to zero so that the values are not used in
    // write again
    if (valid > 0)
    {
        for (ltfatInt w = 0; w < W; w++)
        {
            LTFAT_REAL* pbufchan = p->buf + p->readIdx + w * p->bufLen;
            memcpy(buf[w], pbufchan, valid * sizeof * p->buf);
            memset(pbufchan, 0, valid * sizeof * p->buf);
        }
    }
    if (over > 0)
    {
        for (ltfatInt w = 0; w < W; w++)
        {
            LTFAT_REAL* pbufchan = p->buf + w * p->bufLen;
            memcpy(buf[w] + valid, pbufchan, over * sizeof * p->buf);
            memset(pbufchan, 0, over * sizeof * p->buf);
        }
    }

    p->readIdx = ( p->readIdx + toRead ) % p->bufLen;

    return toRead;
error:
    return status;
}

/* DGTREAL processor */
struct LTFAT_NAME(rtdgtreal_processor_state)
{
    LTFAT_NAME(rtdgtreal_processor_callback)*
    processorCallback; //!< Custom processor callback
    void* userdata; //!< Callback data
    LTFAT_NAME(rtdgtreal_fifo_state)* fwdfifo;
    LTFAT_NAME(rtidgtreal_fifo_state)* backfifo;
    LTFAT_NAME(rtdgtreal_plan)* fwdplan;
    LTFAT_NAME(rtidgtreal_plan)* backplan;
    LTFAT_NAME(realtocomplextransform)* fwdtra;
    LTFAT_NAME(complextorealtransform)* backtra;
    LTFAT_REAL* buf;
    LTFAT_COMPLEX* fftbufIn;
    LTFAT_COMPLEX* fftbufOut;
    void** garbageBin;
    int garbageBinSize;
};


LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_processor_init)(const LTFAT_REAL* ga, const ltfatInt gal,
                                     const LTFAT_REAL* gs, const ltfatInt gsl,
                                     const ltfatInt a, const ltfatInt M, const ltfatInt Wmax,
                                     LTFAT_NAME(rtdgtreal_processor_callback)* callback, void* userdata,
                                     LTFAT_NAME(rtdgtreal_processor_state)** pout)
{
    LTFAT_NAME(rtdgtreal_processor_state)* p = NULL;

    int status = LTFATERR_SUCCESS;
    CHECKNULL(pout);
    CHECK(LTFATERR_BADSIZE, gal > 0, "gla must be positive");
    CHECK(LTFATERR_BADSIZE, gsl > 0, "gls must be positive");
    CHECK(LTFATERR_NOTPOSARG, a > 0, "a must be positive");
    CHECK(LTFATERR_NOTPOSARG, M > 0, "M must be positive");
    CHECK(LTFATERR_NOTPOSARG, Wmax > 0, "Wmax must be positive");
    CHECKMEM( p = ltfat_calloc(1, sizeof * p));

    CHECKMEM(
        p->fftbufIn = ltfat_malloc( Wmax * (M / 2 + 1) * sizeof * p->fftbufIn));
    CHECKMEM(
        p->fftbufOut = ltfat_malloc( Wmax * (M / 2 + 1) * sizeof * p->fftbufOut));
    CHECKMEM( p->buf = ltfat_malloc( Wmax * gal * sizeof * p->buf));

    CHECKSTATUS(
        LTFAT_NAME(rtdgtreal_fifo_init)(11 * gal, gal > gsl ? gal - 1 : gsl - 1 , gal,
                                        a, Wmax, &p->fwdfifo), "fwd fifo init failed");

    CHECKSTATUS(
        LTFAT_NAME(rtidgtreal_fifo_init)(11 * gsl, gsl, a, Wmax, &p->backfifo),
        "back fifo init failed");

    CHECKSTATUS( LTFAT_NAME(rtdgtreal_init)(ga, gal, M, LTFAT_RTDGTPHASE_ZERO,
                                            &p->fwdplan), "fwd plan failed");

    CHECKSTATUS( LTFAT_NAME(rtidgtreal_init)(gs, gsl, M,
                 LTFAT_RTDGTPHASE_ZERO, &p->backplan), "back plan failed");

    p->processorCallback = callback;
    p->userdata = userdata;
    p->fwdtra = &LTFAT_NAME(rtdgtreal_execute_wrapper);
    p->backtra = &LTFAT_NAME(rtidgtreal_execute_wrapper);

    /* LTFAT_NAME(rtdgtreal_processor_state) retLoc = */
    /* { */
    /*     .processorCallback = callback, .userdata = userdata, */
    /*     .fwdfifo = fwdfifo, .backfifo = backfifo, */
    /*     .fwdplan = fwdplan, .backplan = backplan, */
    /*     .buf = buf, .fftbufIn = fftbufIn, .fftbufOut = fftbufOut, */
    /*     .fwdtra = LTFAT_NAME(rtdgtreal_execute_wrapper), */
    /*     .backtra = LTFAT_NAME(rtidgtreal_execute_wrapper), */
    /*     .garbageBin = NULL, .garbageBinSize = 0 */
    /* }; */

    *pout = p;
    return status;
error:
    if (p)
    {
        LTFAT_SAFEFREEALL(p->buf, p->fftbufIn, p->fftbufOut);
        if (p->fwdfifo) LTFAT_NAME(rtdgtreal_fifo_done)(&p->fwdfifo);
        if (p->backfifo) LTFAT_NAME(rtidgtreal_fifo_done)(&p->backfifo);
        if (p->fwdplan) LTFAT_NAME(rtdgtreal_done)(&p->fwdplan);
        if (p->backplan) LTFAT_NAME(rtidgtreal_done)(&p->backplan);
        if (p) ltfat_free(p);
    }
    *pout = NULL;
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_processor_init_win)(LTFAT_FIRWIN win,
        const ltfatInt gl, const ltfatInt a, const ltfatInt M,
        const ltfatInt Wmax,
        LTFAT_NAME(rtdgtreal_processor_callback)* callback,
        void* userdata,
        LTFAT_NAME(rtdgtreal_processor_state)** pout)
{
    LTFAT_REAL* g = NULL;
    LTFAT_REAL* gd = NULL;
    void** garbageBin = NULL;

    int status = LTFATERR_SUCCESS;
    CHECKNULL(pout);
    CHECK(LTFATERR_BADSIZE, gl > 0, "gl must be positive");
    CHECK(LTFATERR_NOTPOSARG, a > 0,  "a must be positive");
    CHECK(LTFATERR_NOTPOSARG, M > 0,  "M must be positive");
    CHECK(LTFATERR_NOTPOSARG, Wmax > 0, "Wmax must be positive");

    CHECKMEM(g = ltfat_malloc(gl * sizeof * g));
    CHECKMEM(gd = ltfat_malloc(gl * sizeof * gd));
    CHECKMEM(garbageBin = ltfat_malloc(2 * sizeof(void*)));

    CHECKSTATUS(LTFAT_NAME_REAL(firwin)(win, gl, g), "Call to firwin failed");
    CHECKSTATUS(LTFAT_NAME_REAL(gabdual_painless)(g, gl, a, M, gd),
                "Call to gabdual_painless failed");

    CHECKSTATUS(LTFAT_NAME(rtdgtreal_processor_init)(g, gl, gd, gl, a, M, Wmax,
                callback, userdata, pout), "processor_init failed");

    LTFAT_NAME(rtdgtreal_processor_state)* p = *pout;
    p->garbageBinSize = 2;
    p->garbageBin = garbageBin;
    p->garbageBin[0] = g;
    p->garbageBin[1] = gd;

    return status;
error:
    if (g) ltfat_free(g);
    if (gd) ltfat_free(gd);
    if (garbageBin) ltfat_free(garbageBin);
    // Also status is now set to the proper value
    return status;
}

LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_processor_setcallback)(
    LTFAT_NAME( rtdgtreal_processor_state)* p,
    LTFAT_NAME(rtdgtreal_processor_callback)* callback,
    void* userdata)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p);
    p->processorCallback = callback;
    p->userdata = userdata;
error:
    return status;
}


LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_processor_execute_compact)(
    LTFAT_NAME(rtdgtreal_processor_state)* p,
    const LTFAT_REAL* in,
    const ltfatInt len, const ltfatInt chanNo,
    LTFAT_REAL* out)
{
    // Stack allocated (tiny) VLAs
    const LTFAT_REAL* inTmp[chanNo];
    LTFAT_REAL* outTmp[chanNo];

    for (ltfatInt w = 0; w < chanNo; w++)
    {
        inTmp[w] = &in[w * len];
        outTmp[w] = &out[w * len];
    }

    return LTFAT_NAME(rtdgtreal_processor_execute)( p, inTmp, len, chanNo, outTmp);
}

LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_processor_execute)(
    LTFAT_NAME(rtdgtreal_processor_state)* p,
    const LTFAT_REAL** in,
    const ltfatInt len, const ltfatInt chanNo,
    LTFAT_REAL** out)
{
    // Get default processor if none was set
    LTFAT_NAME(rtdgtreal_processor_callback)* processorCallback =
        p->processorCallback;

    if (!processorCallback)
        processorCallback = &LTFAT_NAME(default_rtdgtreal_processor_callback);

    // Write new data
    ltfatInt samplesWritten = LTFAT_NAME(rtdgtreal_fifo_write)(p->fwdfifo, in, len,
                              chanNo);

    // While there is new data in the input fifo
    while ( LTFAT_NAME(rtdgtreal_fifo_read)(p->fwdfifo, p->buf) > 0 )
    {
        // Transform
        p->fwdtra((void*)p->fwdplan, p->buf, p->fwdfifo->Wmax,
                  p->fftbufIn);

        // Process
        processorCallback(p->userdata, p->fftbufIn, p->fwdplan->M / 2 + 1,
                          p->fwdfifo->Wmax, p->fftbufOut);

        // Reconstruct
        p->backtra((void*)p->backplan, p->fftbufOut, p->backfifo->Wmax, p->buf);

        // Write (and overlap) to out fifo
        LTFAT_NAME(rtidgtreal_fifo_write)(p->backfifo, p->buf);
    }

    // Read sampples for output
    ltfatInt samplesRead = LTFAT_NAME(rtidgtreal_fifo_read)(p->backfifo, len,
                           chanNo, out);

    if ( samplesWritten != len ) return LTFATERR_OVERFLOW;
    else if ( samplesRead != len ) return LTFATERR_UNDERFLOW;
    return LTFATERR_SUCCESS;
}

LTFAT_EXTERN int
LTFAT_NAME(rtdgtreal_processor_done)(LTFAT_NAME(rtdgtreal_processor_state)** p)
{
    int status = LTFATERR_SUCCESS;
    CHECKNULL(p); CHECKNULL(*p);

    LTFAT_NAME(rtdgtreal_processor_state)* pp = *p;

    LTFAT_NAME(rtdgtreal_fifo_done)(&pp->fwdfifo);
    LTFAT_NAME(rtidgtreal_fifo_done)(&pp->backfifo);
    LTFAT_NAME(rtdgtreal_done)(&pp->fwdplan);
    LTFAT_NAME(rtidgtreal_done)(&pp->backplan);
    ltfat_free(pp->buf);
    ltfat_free(pp->fftbufIn);
    ltfat_free(pp->fftbufOut);

    if (pp->garbageBinSize)
    {
        for (int ii = 0; ii < pp->garbageBinSize; ii++)
            ltfat_free(pp->garbageBin[ii]);

        ltfat_free(pp->garbageBin);
    }

    ltfat_free(pp);
    pp = NULL;
error:
    return status;
}

LTFAT_EXTERN void
LTFAT_NAME(default_rtdgtreal_processor_callback)(void* UNUSED(userdata),
        const LTFAT_COMPLEX* in, const int M2, const int W, LTFAT_COMPLEX* out)
{
    memcpy(out, in, W * M2 * sizeof * in);
}
