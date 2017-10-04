/* Copyright 2008-2017 Michael Alyn Miller
 * vi:ts=4:sts=4:et:sw=4:sr:et:tw=72:fo=tcrq
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.  See the License for the specific language governing
 * permissions and limitations under the License.
 */

/* -------------------------------------
 * Includes.
 */

/* ANSI C includes. */
#include <stdlib.h>

/* AVR includes. */
#include <avr/pgmspace.h>

/* enforth includes. */
#include "..\enforth.h"



/* -------------------------------------
 * Sample FFI definitions.
 */

ENFORTH_EXTERN(rand, rand, 0)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(rand)

ENFORTH_EXTERN_VOID(srand, srand, 1)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(srand)



/* -------------------------------------
 * Enforth I/O primitives.
 */

unsigned int inputOffset = 0;
static const uint8_t input[] PROGMEM = {
    '2', '7', ' ', '5', '4', ' ', '+', ' ', '.', '\n'
    //'f','a','v','n','u','m', ' ', '2','x', ' ', '+', ' ', '.', '\n'
    //'r','a','n','d', ' ', '.', '\n'
};

static int enforthStaticKeyQuestion(void)
{
    return inputOffset < sizeof(input) ? -1 : 0;
}

static char enforthStaticKey(void)
{
    if (inputOffset < sizeof(input))
    {
        return pgm_read_byte(&input[inputOffset++]);
    }
    else
    {
        for (;;)
        {
            // Block forever.
        }
    }
}

static char lastCh = 0;
static void enforthStaticEmit(char ch)
{
    lastCh = ch;
}



/* -------------------------------------
 * Globals.
 */

static EnforthVM enforthVM;
static unsigned char enforthDict[512];


/* -------------------------------------
 * main()
 */

int main(void)
{
    /* Initialize Enforth. */
    enforth_init(
            &enforthVM,
            enforthDict, sizeof(enforthDict),
            LAST_FFI,
            enforthStaticKeyQuestion, enforthStaticKey, enforthStaticEmit,
            NULL, NULL);

    /* Add a couple of definitions. */
    enforth_evaluate(&enforthVM, ": favnum 27 ;");
    enforth_evaluate(&enforthVM, ": 2x dup + ;");

    /* Launch the Enforth interpreter. */
    enforth_go(&enforthVM);
}
