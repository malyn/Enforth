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
#include <stdlib.h>

/* AVR includes. */
#include <avr/pgmspace.h>

/* MFORTH includes. */
#include "..\MFORTH.h"



/* -------------------------------------
 * Sample FFI definitions.
 */

MFORTH_EXTERN(rand, rand, 0)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(rand)

MFORTH_EXTERN(srand, srand, 1)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(srand)



/* -------------------------------------
 * MFORTH I/O primitives.
 */

unsigned int inputOffset = 0;
static const uint8_t input[] PROGMEM = {
    '2', '7', ' ', '5', '4', ' ', '+', ' ', '.', '\n'
    //'f','a','v','n','u','m', ' ', '2','x', ' ', '+', ' ', '.', '\n'
    //'r','a','n','d', ' ', '.', '\n'
};

static bool mforthStaticKeyQuestion(void)
{
    return inputOffset < sizeof(input);
}

static char mforthStaticKey(void)
{
    if (inputOffset < sizeof(input))
    {
        return pgm_read_byte(&input[inputOffset++]);
    }
    else
    {
        while (true)
        {
            // Block forever.
        }
    }
}

static char lastCh = 0;
static void mforthStaticEmit(char ch)
{
    lastCh = ch;
}



/* -------------------------------------
 * Globals.
 */

static unsigned char mforthDict[512];
static MFORTH mforth(
        mforthDict, sizeof(mforthDict),
        LAST_FFI,
        mforthStaticKeyQuestion, mforthStaticKey, mforthStaticEmit);


/* -------------------------------------
 * main()
 */

int main(void)
{
    /* Add a couple of hand-coded definitions. */
    const uint8_t favnumDef[] = {
        0x00, // DOCOLON
        'F',
        'A',
        'V',
        'N',
        'U',
        0x80 | 'M',
        0x0b,   // CHARLIT
        27,
        0x7f }; // EXIT
    mforth.addDefinition(favnumDef, sizeof(favnumDef));

    const uint8_t twoxDef[] = {
        0x00, // DOCOLON
        '2',
        0x80 | 'X',
        0x02,   // DUP
        0x04,   // +
        0x7f }; // EXIT
    mforth.addDefinition(twoxDef, sizeof(twoxDef));

    const uint8_t randDef[] = {
        0x06, // DOFFI0
        (uint8_t)(((uint16_t)&FFIDEF_rand      ) & 0xff),  // FFIdef LSB
        (uint8_t)(((uint16_t)&FFIDEF_rand >>  8) & 0xff)}; // FFIdef MSB
    mforth.addDefinition(randDef, sizeof(randDef));

    const uint8_t srandDef[] = {
        0x07, // DOFFI1
        (uint8_t)(((uint16_t)&FFIDEF_srand      ) & 0xff),  // FFIdef LSB
        (uint8_t)(((uint16_t)&FFIDEF_srand >>  8) & 0xff)}; // FFIdef MSB
    mforth.addDefinition(srandDef, sizeof(srandDef));

    /* Launch the MFORTH interpreter. */
    mforth.go();
}
