/* Copyright (c) 2008-2014, Michael Alyn Miller <malyn@strangeGizmo.com>.
 * All rights reserved.
 * vi:ts=4:sts=4:et:sw=4:sr:et:tw=72:fo=tcrq
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice unmodified, this list of conditions, and the following
 *     disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 3.  Neither the name of Michael Alyn Miller nor the names of the
 *     contributors to this software may be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* -------------------------------------
 * Includes.
 */

/* ANSI C includes. */
#include <stdio.h>
#include <stdlib.h>

/* MFORTH includes. */
#include "MFORTH.h"



/* -------------------------------------
 * Sample FFI definitions.
 */

// Externs
MFORTH_EXTERN(rand, rand, 0)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(rand)

MFORTH_EXTERN(srand, srand, 1)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(srand)



/* -------------------------------------
 * MFORTH I/O primitives.
 */

static bool mforthSimpleKeyQuestion(void)
{
    return true;
}

/* 6.1.1750 KEY
 *
 * ( -- char )
 *
 * Receive one character char, a member of the implementation-defined
 * character set. Keyboard events that do not correspond to such
 * characters are discarded until a valid character is received, and
 * those events are subsequently unavailable.

 * All standard characters can be received. Characters received by KEY
 * are not displayed.

 * Any standard character returned by KEY has the numeric value
 * specified in 3.1.2.1 Graphic characters. Programs that require the
 * ability to receive control characters have an environmental
 * dependency.

 * See: 10.6.2.1305 EKEY , 10.6.1.1755 KEY?
 */
static char mforthSimpleKey(void)
{
    return getchar();
}

static void mforthSimpleEmit(char ch)
{
    putchar(ch);
}



/* -------------------------------------
 * main()
 */

int main(int argc, char **argv)
{
    /* Initial dictionary with a couple of hand-coded definitions. */
    unsigned char mforthDict[512];
    unsigned char * here = mforthDict;

    unsigned char * favnumLFA = here;
    *here++ = 0x00; // LFAlo; bogus LFA offset
    *here++ = 0x00; // LFAhi; bogus LFA offset
    *here++ = 0x00; // DOCOLON
    *here++ = 'F';
    *here++ = 'A';
    *here++ = 'V';
    *here++ = 'N';
    *here++ = 'U';
    *here++ = 0x80 | 'M';
    unsigned char * favnumPFA = here;
    *here++ = 0x0b; // CHARLIT
    *here++ = 27;
    *here++ = 0x7f; // EXIT

    unsigned char * twoxLFA = here;
    *here++ = ((twoxLFA - favnumLFA)     ) & 0xff; // LFAlo
    *here++ = ((twoxLFA - favnumLFA) >> 8) & 0xff; // LFAhi
    *here++ = 0x00; // DOCOLON
    *here++ = '2';
    *here++ = 0x80 | 'X';
    unsigned char * twoxPFA = here;
    *here++ = 0x02; // DUP
    *here++ = 0x04; // +
    *here++ = 0x7f; // EXIT

    unsigned char * twoxfavnumLFA = here;
    *here++ = ((twoxfavnumLFA - twoxLFA)     ) & 0xff; // LFAlo
    *here++ = ((twoxfavnumLFA - twoxLFA) >> 8) & 0xff; // LFAhi
    *here++ = 0x00; // DOCOLON
    *here++ = '2';
    *here++ = 'X';
    *here++ = 'F';
    *here++ = 'A';
    *here++ = 'V';
    *here++ = 'N';
    *here++ = 'U';
    *here++ = 0x80 | 'M';
    *here++ = 0x70; // DOCOLON
    *(here+0) = ((here - favnumPFA)     ) & 0xff; // offset LSB
    *(here+1) = ((here - favnumPFA) >> 8) & 0xff; // offset MSB
    here += 2;
    *here++ = 0x70; // DOCOLON
    *(here+0) = ((here - twoxPFA)     ) & 0xff; // offset LSB
    *(here+1) = ((here - twoxPFA) >> 8) & 0xff; // offset MSB
    here += 2;
    *here++ = 0x7f; // EXIT

    unsigned char * randLFA = here;
    *here++ = ((randLFA - twoxfavnumLFA)     ) & 0xff; // LFAlo
    *here++ = ((randLFA - twoxfavnumLFA) >> 8) & 0xff; // LFAhi
    *here++ = 0x06; // DOFFI0
    *here++ = ((uint32_t)&FFIDEF_rand      ) & 0xff; // FFIdef LSB
    *here++ = ((uint32_t)&FFIDEF_rand >>  8) & 0xff; // FFIdef
    *here++ = ((uint32_t)&FFIDEF_rand >> 16) & 0xff; // FFIdef
    *here++ = ((uint32_t)&FFIDEF_rand >> 24) & 0xff; // FFIdef MSB

    unsigned char * srandLFA = here;
    *here++ = ((srandLFA - randLFA)     ) & 0xff; // LFAlo
    *here++ = ((srandLFA - randLFA) >> 8) & 0xff; // LFAhi
    *here++ = 0x07; // DOFFI1
    *here++ = ((uint32_t)&FFIDEF_srand      ) & 0xff; // FFIdef LSB
    *here++ = ((uint32_t)&FFIDEF_srand >>  8) & 0xff; // FFIdef
    *here++ = ((uint32_t)&FFIDEF_srand >> 16) & 0xff; // FFIdef
    *here++ = ((uint32_t)&FFIDEF_srand >> 24) & 0xff; // FFIdef MSB

    /* MFORTH VM */
    MFORTH mforth(mforthDict, sizeof(mforthDict),
            srandLFA - mforthDict, here - mforthDict,
            LAST_FFI,
            mforthSimpleKeyQuestion, mforthSimpleKey, mforthSimpleEmit);


    /* Launch the MFORTH interpreter. */
    mforth.go();

    /* Exit the application. */
    return 0;
}
