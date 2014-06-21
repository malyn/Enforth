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
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* AVR includes. */
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM
#define pgm_read_byte(p) (*(uint8_t*)(p))
#define pgm_read_word(p) (*(int *)(p))
#define memcpy_P memcpy
#define strlen_P strlen
#define strncasecmp_P strncasecmp
#endif

/* Enforth includes. */
#include "enforth.h"



/* -------------------------------------
 * Enforth definition types and tokens.
 */

enum EnforthDefinitionType
{
    kDefTypeFFI = 0,
    kDefTypeCOLONHIDDEN,
    kDefTypeCOLON,
    kDefTypeCOLONIMMEDIATE,
    kDefTypeCONSTANT,
    kDefTypeCREATE,
    kDefTypeDOES,
    kDefTypeVARIABLE,
};

typedef enum EnforthToken
{
#include "enforth_tokens.h"

    /* Tokens 0xe0-0xef are reserved for jump labels to the "CFA"
     * primitives *after* the point where W (the Word Pointer) has
     * already been set.  This allows words like EXECUTE to jump to a
     * CFA without having to use a switch statement to convert
     * definition types to tokens.  The token names themselves do not
     * need to be defined because they are never referenced (we're just
     * reserving space in the token list and Address Interpreter jump
     * table, in other words), but we do list them here in order to make
     * it easier to turn raw tokens into enum values in the debugger. */
    /* Don't need to map kDefTypeFFI because that will be mapped to one
     * of the arity-specific values. */
    /* Don't need to map kDefTYPECOLONHIDDEN because those definitions
     * can never be executed (except by RECURSE, which knows to compile
     * in a DOCOLON). */
    PDOCOLON = 0xe2,
    PDOCOLONIMMEDIATE,
    PDOCONSTANT,
    PDOCREATE,
    PDODOES,
    PDOVARIABLE,

    PDOFFI0 = 0xe8,
    PDOFFI1,
    PDOFFI2,
    PDOFFI3,
    PDOFFI4,
    PDOFFI5,
    PDOFFI6,
    PDOFFI7,

    /* Just like the above, these tokens are never used in code and this
     * list of enum values is only used to simplify debugging. */
    DOCOLON = 0xf2,
    DOCOLONIMMEDIATE,
    DOCONSTANT,
    DOCREATE,
    DODOES,
    DOVARIABLE,

    DOFFI0 = 0xf8,
    DOFFI1,
    DOFFI2,
    DOFFI3,
    DOFFI4,
    DOFFI5,
    DOFFI6,
    DOFFI7,
} EnforthToken;



/* -------------------------------------
 * Enforth definition names.
 */

/* A length with a high bit set indicates that the word is immediate. */
static const char kDefinitionNames[] PROGMEM =
#include "enforth_names.h"
;



/* -------------------------------------
 * Enforth definitions.
 */

static const int8_t definitions[] PROGMEM = {
#include "enforth_definitions.h"
};



/* -------------------------------------
 * Private function definitions.
 */

static int enforth_paren_find_word(EnforthVM * const vm, uint8_t * caddr, EnforthUnsigned u, EnforthXT * const xt, int * const isImmediate);



/* -------------------------------------
 * Private functions.
 */

static int enforth_paren_find_word(EnforthVM * const vm, uint8_t * caddr, EnforthUnsigned u, EnforthXT * const xt, int * const isImmediate)
{
    int searchLen = u;
    char * searchName = (char *)caddr;

    /* Search the dictionary.*/
    uint8_t * curWord;
    for (curWord = vm->latest.ram;
         curWord != NULL;
         curWord = *(uint16_t *)curWord == 0 ? NULL : curWord - *(uint16_t *)curWord)
    {
        /* Get the definition type. */
        uint8_t definitionFlags = *(curWord + 2);
        uint8_t definitionType = definitionFlags & 0x7;
        uint8_t definitionNameLength = (definitionFlags >> 3) & 0x1f;

        /* Ignore smudged words. */
        if (definitionType == kDefTypeCOLONHIDDEN)
        {
            continue;
        }

        /* Compare name lengths. */
        if (definitionNameLength != searchLen)
        {
            continue;
        }

        /* Compare the strings, which happens differently depending on
         * if this is a user-defined word or an FFI trampoline. */
        int nameMatch;
        if (definitionType != kDefTypeFFI)
        {
            char * dictName = (char *)(curWord + 3);
            nameMatch = (strncasecmp(searchName, dictName, searchLen) == 0) ? -1 : 0;
        }
        else
        {
            /* Get a reference to the FFI definition. */
            const EnforthFFIDef * ffi = *(EnforthFFIDef **)(curWord + 3);
            const char * ffiName = (char *)pgm_read_word(&ffi->name);

            /* See if this FFI matches our search word. */
            nameMatch = (strncasecmp_P(searchName, ffiName, searchLen) == 0) ? -1 : 0;
        }

        /* Is this a match?  If so, we're done. */
        if (nameMatch != 0)
        {
            *xt = 0x8000 | (uint16_t)(curWord - vm->dictionary.ram);
            *isImmediate = (definitionType == kDefTypeCOLONIMMEDIATE) ? 1 : -1;
            return -1;
        }
    }

    /* Search through the primitives for a match against this search
     * name. */
    const char * pPrimitives = kDefinitionNames;
    uint8_t token;
    for (token = 0; /* below */; ++token)
    {
        /* Get the length of this primitive; we're done if this is the
         * end value (0xff). */
        uint8_t primitiveFlags = (uint8_t)pgm_read_byte(pPrimitives++);
        if (primitiveFlags == 0xff)
        {
            break;
        }

        uint8_t primitiveLength = primitiveFlags & 0x7f;
        int primitiveIsImmediate = (primitiveFlags & 0x80) != 0 ? 1 : -1;

        /* Is this a match?  If so, return the XT. */
        if ((primitiveLength == searchLen)
            && (strncasecmp_P(searchName, pPrimitives, searchLen) == 0))
        {
            *xt = token;
            *isImmediate = primitiveIsImmediate;
            return -1;
        }

        /* No match; skip over the name. */
        pPrimitives += primitiveLength;
    }

    /* No match. */
    return 0;
}



/* -------------------------------------
 * Public functions.
 */

void enforth_init(
        EnforthVM * const vm,
        uint8_t * const dictionary, int dictionary_size,
        const EnforthFFIDef * const last_ffi,
        int (*keyq)(void), char (*key)(void), void (*emit)(char))
{
    vm->last_ffi = last_ffi;

    vm->keyq = keyq;
    vm->key = key;
    vm->emit = emit;

    vm->dictionary.ram = dictionary;
    vm->dictionary_size = dictionary_size;

    vm->dp = vm->dictionary;
    vm->latest.ram = NULL;

    vm->hld = NULL;

    vm->state = 0;
}

void enforth_add_definition(EnforthVM * const vm, const uint8_t * const def, int defSize)
{
    /* Get the address of the start of this definition and the address
     * of the start of the last definition. */
    const uint8_t * const prevLFA = vm->latest.ram;
    uint8_t * newLFA = vm->dp.ram;

    /* Add the LFA link. */
    if (vm->latest.ram != NULL)
    {
        *vm->dp.ram++ = ((newLFA - prevLFA)     ) & 0xff; /* LFAlo */
        *vm->dp.ram++ = ((newLFA - prevLFA) >> 8) & 0xff; /* LFAhi */
    }
    else
    {
        *vm->dp.ram++ = 0x00;
        *vm->dp.ram++ = 0x00;
    }

    /* Copy the definition itself. */
    memcpy(vm->dp.ram, def, defSize);
    vm->dp.ram += defSize;

    /* Update latest. */
    vm->latest.ram = newLFA;
}

void enforth_go(EnforthVM * const vm)
{
    register uint8_t *ip;
    register EnforthCell tos;
    register EnforthCell *restDataStack; /* Points at the second item on the stack. */
    register uint8_t *w;
    register EnforthCell *returnTop;

#ifdef __AVR__
    register int inProgramSpace;
#endif

#if ENABLE_STACK_CHECKING
    /* Check for available stack space and abort with a message if this
     * operation would run out of space. */
    /* TODO The overflow logic seems slightly too aggressive -- it
     * probably needs a "- 1" in there given that we store TOS in a
     * register. */
#define CHECK_STACK(numArgs, numResults) \
    { \
        if ((&vm->data_stack[32] - restDataStack) < numArgs) { \
            goto STACK_UNDERFLOW; \
        } else if (((&vm->data_stack[32] - restDataStack) - numArgs) + numResults > 32) { \
            goto STACK_OVERFLOW; \
        } \
    }
#else
#define CHECK_STACK(numArgs, numResults)
#endif

    /* Print the banner. */
    if (vm->emit != NULL)
    {
        vm->emit('H');
        vm->emit('i');
        vm->emit('\n');
    }

    static const void * const primitive_table[256] PROGMEM = {
#include "enforth_jumptable.h"

        /* $E0 - $E7 */
        0, /* Not needed */
        0, /* Not needed */
        &&PDOCOLON,
        &&PDOCOLON, /* Immediate word */

        0, /* &&PDOCONSTANT, */
        0, /* &&PDOCREATE, */
        0, /* &&PDODOES, */
        0, /* &&PDOVARIABLE, */

        /* $E8 - $EF */
        &&PDOFFI0,
        &&PDOFFI1,
        &&PDOFFI2,
        0, /* &&PDOFFI3, */

        0, /* &&PDOFFI4, */
        0, /* &&PDOFFI5, */
        0, /* &&PDOFFI6, */
        0, /* &&PDOFFI7, */

        /* $F0 - $F7 */
        0, /* Not needed */
        0, /* Not needed */
        &&DOCOLON,
        &&DOCOLON, /* Immediate word */

        0, /* &&DOCONSTANT, */
        0, /* &&DOCREATE, */
        0, /* &&DODOES, */
        0, /* &&DOVARIABLE, */

        /* $F8 - $FF */
        &&DOFFI0,
        &&DOFFI1,
        &&DOFFI2,
        &&DOFFI3,

        &&DOFFI4,
        &&DOFFI5,
        &&DOFFI6,
        &&DOFFI7,
    };

    /* Initialize RP so that we can use threading to get to QUIT from
     * ABORT.  QUIT will immediately reset RP as well, of course. */
    returnTop = (EnforthCell *)&vm->return_stack[32];

    /* Jump to ABORT, which initializes the IP, our stacks, etc. */
    goto ABORT;

    /* The inner interpreter. */
    for (;;)
    {
        /* Get the next token and dispatch to the label. */
        uint8_t token;

#ifdef __AVR__
        if (inProgramSpace)
        {
            token = pgm_read_byte(ip++);
        }
        else
#endif
        {
            token = *ip++;
        }

DISPATCH_TOKEN:
        goto *(void *)pgm_read_word(&primitive_table[token]);

        /* TODO Move this to Forth once we have C@C */
        FFIARITY:
        {
            /* TOS contains the XT (dictionary-relative offset with the
             * high bit set) of the FFI trampoline.  Get the pointer to
             * the trampoline -- which is a pointer to the definition in
             * code space -- and then put the arity into TOS. */
            EnforthFFIDef** trampoline = (EnforthFFIDef**)(
                    vm->dictionary.ram
                        + (tos.u & 0x7fff)
                        + 2 /* LFA */
                        + 1 /* Flags */);
            tos.u = pgm_read_byte(&(*trampoline)->arity);
        }
        continue;

        DOFFI0:
        {
            CHECK_STACK(0, 1);

            /* The IP is pointing at the dictionary-relative offset of
             * the PFA of the FFI trampoline.  Convert that to a pointer
             * and store it in W. */
            w = (uint8_t*)(vm->dictionary.ram + *(uint16_t*)ip);
            ip += 2;

        PDOFFI0:
            {
                /* W contains a pointer to the PFA of the FFI
                 * definition; get the FFI definition pointer and then
                 * use that to get the FFI function pointer. */
                ZeroArgFFI fn = (ZeroArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);

                *--restDataStack = tos;
                tos = (*fn)();
            }
        }
        continue;

        DOFFI1:
        {
            CHECK_STACK(1, 1);

            /* The IP is pointing at the dictionary-relative offset of
             * the PFA of the FFI trampoline.  Convert that to a pointer
             * and store it in W. */
            w = (uint8_t*)(vm->dictionary.ram + *(uint16_t*)ip);
            ip += 2;

        PDOFFI1:
            {
                /* W contains a pointer to the PFA of the FFI
                 * definition; get the FFI definition pointer and then
                 * use that to get the FFI function pointer. */
                OneArgFFI fn = (OneArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);

                tos = (*fn)(tos);
            }
        }
        continue;

        DOFFI2:
        {
            CHECK_STACK(2, 1);

            /* The IP is pointing at the dictionary-relative offset of
             * the PFA of the FFI trampoline.  Convert that to a pointer
             * and store it in W. */
            w = (uint8_t*)(vm->dictionary.ram + *(uint16_t*)ip);
            ip += 2;

        PDOFFI2:
            {
                TwoArgFFI fn = (TwoArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);

                EnforthCell arg2 = tos;
                EnforthCell arg1 = *restDataStack++;
                tos = (*fn)(arg1, arg2);
            }
        }
        continue;

        DOFFI3:
            CHECK_STACK(3, 1);
        continue;

        DOFFI4:
            CHECK_STACK(4, 1);
        continue;

        DOFFI5:
            CHECK_STACK(5, 1);
        continue;

        DOFFI6:
            CHECK_STACK(6, 1);
        continue;

        DOFFI7:
            CHECK_STACK(7, 1);
        continue;

        WLIT:
#ifdef __AVR__
        /* Fall through, since literals are already 16-bits on the AVR. */
#else
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            /* No need for pgm_read_word here since this code is never
             * compiled on the AVR. */
            tos.i = (EnforthInt)*(uint16_t*)ip;
            ip += 2;
        }
        continue;
#endif

        LIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
#ifdef __AVR__
            if (inProgramSpace)
            {
                tos.i = pgm_read_word(ip);
            }
            else
#endif
            {
                tos = *(EnforthCell*)ip;
            }

            ip += kEnforthCellSize;
        }
        continue;

        DUP:
        {
            CHECK_STACK(1, 2);
            *--restDataStack = tos;
        }
        continue;

        DROP:
        {
            CHECK_STACK(1, 0);
            tos = *restDataStack++;
        }
        continue;

        PLUS:
        {
            CHECK_STACK(2, 1);
            tos.i += restDataStack++->i;
        }
        continue;

        MINUS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i - tos.i;
        }
        continue;

        ONEPLUS:
        {
            CHECK_STACK(1, 1);
            tos.i++;
        }
        continue;

        ONEMINUS:
        {
            CHECK_STACK(1, 1);
            tos.i--;
        }
        continue;

        SWAP:
        {
            CHECK_STACK(2, 2);
            EnforthCell swap = restDataStack[0];
            restDataStack[0] = tos;
            tos = swap;
        }
        continue;

        /* TODO Add docs re: Note that (branch) and (0branch) offsets
         * are 8-bit relative offsets.  UNLIKE word addresses, the IP
         * points at the offset in BRANCH/ZBRANCH.  These offsets can be
         * positive or negative because branches can go both forwards
         * and backwards. */
        BRANCH:
        {
            CHECK_STACK(0, 0);

            /* Relative, because these are entirely within a single word
             * and so we want it to be relocatable without us having to
             * do anything.  Note that the offset cannot be larger than
             * +/- 127 bytes! */
            int8_t relativeOffset;
#ifdef __AVR__
            if (inProgramSpace)
            {
                relativeOffset = pgm_read_byte(ip);
            }
            else
#endif
            {
                relativeOffset = *(int8_t*)ip;
            }

            ip = ip + relativeOffset;
        }
        continue;

#if ENABLE_STACK_CHECKING
        STACK_OVERFLOW:
        {
            if (vm->emit != NULL)
            {
                vm->emit('\n');
                vm->emit('!');
                vm->emit('O');
                vm->emit('V');
                vm->emit('\n');
            }
            goto ABORT;
        }
        continue;

        STACK_UNDERFLOW:
        {
            if (vm->emit != NULL)
            {
                vm->emit('\n');
                vm->emit('!');
                vm->emit('U');
                vm->emit('N');
                vm->emit('\n');
            }
            goto ABORT;
        }
        continue;
#endif
        /* -------------------------------------------------------------
         * ABORT [CORE] 6.1.0670 ( i*x -- ) ( R: j*x -- )
         *
         * Empty the data stack and perform the function of QUIT, which
         * includes emptying the return stack, without displaying a
         * message. */
        ABORT:
        {
            tos.i = 0;
            restDataStack = (EnforthCell*)&vm->data_stack[32];
            vm->base = 10;
            token = QUIT;
            goto DISPATCH_TOKEN;
        }
        continue;

        CHARLIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
#ifdef __AVR__
            if (inProgramSpace)
            {
                tos.i = pgm_read_byte(ip);
            }
            else
#endif
            {
                tos.i = *ip;
            }

            ip++;
        }
        continue;

        EMIT:
        {
            CHECK_STACK(1, 0);

            if (vm->emit != NULL)
            {
                vm->emit(tos.i);
            }

            tos = *restDataStack++;
        }
        continue;

        PEXECUTE:
        {
            w = tos.ram;
            token = restDataStack++->u;
            tos = *restDataStack++;
            goto DISPATCH_TOKEN;
        }
        continue;

        FETCH:
        {
            CHECK_STACK(1, 1);
            tos = *(EnforthCell*)tos.ram;
        }
        continue;

        OR:
        {
            CHECK_STACK(2, 1);
            tos.i |= restDataStack++->i;
        }
        continue;

        /* -------------------------------------------------------------
         * FIND-WORD [ENFORTH] "paren-find-paren" ( c-addr u -- c-addr u 0 | xt 1 | xt -1 )
         *
         * Find the definition named in the string at c-addr with length
         * u in the word list whose latest definition is pointed to by
         * nfa.  If the definition is not found, return the string and
         * zero.  If the definition is found, return its execution token
         * xt.  If the definition is immediate, also return one (1),
         * otherwise also return minus-one (-1).  For a given string,
         * the values returned by FIND-WORD while compiling may differ
         * from those returned while not compiling. */
        FINDWORD:
        {
            CHECK_STACK(2, 3);

            uint8_t * caddr = (uint8_t *)restDataStack->ram;
            EnforthUnsigned u = tos.u;

            EnforthXT xt;
            int isImmediate;
            if (enforth_paren_find_word(vm, caddr, u, &xt, &isImmediate) == -1)
            {
                /* Stack still contains c-addr u; rewrite to xt 1|-1. */
                restDataStack->u = xt;
                tos.i = isImmediate;
            }
            else
            {
                /* Stack still contains c-addr u, so just push 0. */
                *--restDataStack = tos;
                tos.i = 0;
            }
        }
        continue;

        QDUP:
        {
            CHECK_STACK(1, 2);

            if (tos.i != 0)
            {
                *--restDataStack = tos;
            }
        }
        continue;

        /* -------------------------------------------------------------
         * ! [CORE] 6.1.0010 "store" ( x a-addr -- )
         *
         * Store x at a-addr. */
        STORE:
        {
            CHECK_STACK(2, 0);
            *(EnforthCell*)tos.ram = *restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        TWODROP:
        {
            CHECK_STACK(2, 0);
            restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        ZBRANCH:
        {
            CHECK_STACK(1, 0);

            if (tos.i == 0)
            {
                int8_t relativeOffset;
#ifdef __AVR__
                if (inProgramSpace)
                {
                    relativeOffset = pgm_read_byte(ip);
                }
                else
#endif
                {
                    relativeOffset = *(int8_t*)ip;
                }

                ip = ip + relativeOffset;
            }
            else
            {
                ip++;
            }

            tos = *restDataStack++;
        }
        continue;

        TRUE:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = -1;
        }
        continue;

        FALSE:
        ZERO:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = 0;
        }
        continue;

        ZEROEQUALS:
        {
            CHECK_STACK(1, 1);
            tos.i = tos.i == 0 ? -1 : 0;
        }
        continue;

        ZERONOTEQUALS:
        {
            CHECK_STACK(1, 1);
            tos.i = tos.i != 0 ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
         * (s") [ENFORTH] "paren-s-quote-paren" ( -- c-addr u )
         *
         * Runtime behavior of S": return c-addr and u.
         * NOTE: Cannot be used in program space! */
        PSQUOTE:
        {
            CHECK_STACK(0, 2);

            /* Push existing TOS onto the stack. */
            *--restDataStack = tos;

            /* Instruction stream contains the length of the string as a
             * byte and then the string itself.  Start out by pushing
             * the address of the string (ip+1) onto the stack. */
            (--restDataStack)->ram = ip + 1;

            /* Now get the string length into TOS. */
            tos.i = *ip++;

            /* Advance IP over the string. */
            ip = ip + tos.i;
        }
        continue;

        CFETCH:
        {
            CHECK_STACK(1, 1);
            tos.u = *(uint8_t*)tos.ram;
        }
        continue;

        COUNT:
        {
            CHECK_STACK(1, 2);
            (--restDataStack)->ram = (uint8_t*)tos.ram + 1;
            tos.u = *(uint8_t*)tos.ram;
        }
        continue;

        /* -------------------------------------------------------------
         * DEPTH [CORE] 6.1.1200 ( -- +n )
         *
         * +n is the number of single-cell values contained in the data
         * stack before +n was placed on the stack. */
        DEPTH:
        {
            CHECK_STACK(0, 1);

            /* Save TOS, then calculate the stack depth.  The return
             * value should be the number of items on the stack *before*
             * DEPTH was called and so we have to subtract one from the
             * count given that we calculate the depth *after* pushing
             * the old TOS onto the stack. */
            *--restDataStack = tos;
            tos.i = &vm->data_stack[32] - restDataStack - 1;
        }
        continue;

        /* -------------------------------------------------------------
         * (.") [ENFORTH] "paren-dot-quote-paren" ( -- )
         *
         * Prints the string that was compiled into the definition. */
        PDOTQUOTE:
        {
            /* Instruction stream contains the length of the string as a
             * byte and then the string itself.  Get and skip over both
             * values */
            uint8_t * caddr;
            uint8_t u;

#ifdef __AVR__
            if (inProgramSpace)
            {
                u = pgm_read_byte(ip);
                ip++;
            }
            else
#endif
            {
                u = *ip++;
            }

            caddr = ip;

            /* Advance IP over the string. */
            ip = ip + u;

            /* Print out the string. */
            if (vm->emit != NULL)
            {
                int i;
                for (i = 0; i < u; i++)
                {
                    uint8_t ch;
#ifdef __AVR__
                    if (inProgramSpace)
                    {
                        ch = pgm_read_byte(caddr);
                    }
                    else
#endif
                    {
                        ch = *caddr;
                    }

                    vm->emit(ch);

                    caddr++;
                }
            }
        }
        continue;

        /* -------------------------------------------------------------
         * \ [CORE-EXT] 6.2.2535 "backslash"
         *
         * Compilation:
         *   Perform the execution semantics given below.
         *
         * Execution: ( "ccc<eol>" -- )
         *   Parse and discard the remainder of the parse area.  \ is an
         *   immediate word. */
        BACKSLASH:
        {
            vm->to_in = vm->source_len.i;
        }
        continue;

        TWODUP:
        {
            CHECK_STACK(2, 4);
            EnforthCell second = *restDataStack;
            *--restDataStack = tos;
            *--restDataStack = second;
        }
        continue;

        WCOMMA:
#ifdef __AVR__
        /* Fall through, since cells are already 16-bits on the AVR. */
#else
        {
            CHECK_STACK(1, 0);
            *((uint16_t*)vm->dp.ram) = (uint16_t)(tos.u & 0xffff);
            vm->dp.ram += 2;
            tos = *restDataStack++;
        }
        continue;
#endif

        COMMA:
        {
            CHECK_STACK(1, 0);
            *((EnforthCell*)vm->dp.ram) = tos;
            vm->dp.ram += kEnforthCellSize;
            tos = *restDataStack++;
        }
        continue;

        CCOMMA:
        {
            CHECK_STACK(1, 0);
            *vm->dp.ram++ = tos.u & 0xff;
            tos = *restDataStack++;
        }
        continue;

        TUCK:
        {
            EnforthCell second = *restDataStack;
            *restDataStack = tos;
            *--restDataStack = second;
        }
        continue;

        ALIGN:
        {
            /* No alignment necessary (unless we want to expand the
             * range of the dictionary-relative offsets at some point). */
        }
        continue;

        MOVE:
        {
            CHECK_STACK(3, 0);
            EnforthCell arg3 = tos;
            EnforthCell arg2 = *restDataStack++;
            EnforthCell arg1 = *restDataStack++;
            memcpy(arg2.ram, arg1.ram, arg3.u);
            tos = *restDataStack++;
        }
        continue;

        CPLUSSTORE:
        {
            CHECK_STACK(2, 0);
            uint8_t c = *(uint8_t*)tos.ram;
            c += (restDataStack++)->u & 0xff;
            *(uint8_t*)tos.ram = c;
            tos = *restDataStack++;
        }
        continue;

        ALLOT:
        {
            CHECK_STACK(1, 0);
            vm->dp.ram += tos.u;
            tos = *restDataStack++;
        }
        continue;

        NIP:
        {
            CHECK_STACK(2, 1);
            restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
         * C! [CORE] 6.1.0850 "c-store" ( char c-addr -- )
         *
         * Store char at c-addr.  When character size is smaller than
         * cell size, only the number of low-order bits corresponding to
         * character size are transferred. */
        CSTORE:
        {
            CHECK_STACK(2, 0);
            *(uint8_t*)tos.ram = restDataStack++->u;
            tos = *restDataStack++;
        }
        continue;

        ABS:
        {
            CHECK_STACK(1, 1);
            tos.i = abs(tos.i);
        }
        continue;

        /* -------------------------------------------------------------
         * ROT [CORE] 6.1.2160 "rote" ( x1 x2 x3 -- x2 x3 x1 )
         *
         * Rotate the top three stack entries. */
        ROT:
        {
            CHECK_STACK(3, 3);
            EnforthCell x3 = tos;
            EnforthCell x2 = *restDataStack++;
            EnforthCell x1 = *restDataStack++;
            *--restDataStack = x2;
            *--restDataStack = x3;
            tos = x1;
        }
        continue;

        /* -------------------------------------------------------------
         * 0< [CORE] 6.1.0250 "zero-less" ( b -- flag )
         *
         * flag is true if and only if n is less than zero. */
        ZEROLESS:
        {
            CHECK_STACK(1, 0);
            tos.i = tos.i < 0 ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
         * UD/MOD [ENFORTH] "u-d-slash-mod" ( ud1 u1 -- n ud2 )
         *
         * Divide ud1 by u1 giving the quotient ud2 and the remainder n. */
        UDSLASHMOD:
        {
            CHECK_STACK(3, 3);
#ifdef __AVR__
            uint16_t u1 = tos.u;
            uint16_t ud1_msb = restDataStack++->u;
            uint16_t ud1_lsb = restDataStack++->u;
            uint32_t ud1 = ((uint32_t)ud1_msb << 16) | ud1_lsb;
            ldiv_t result = ldiv(ud1, u1);
            (--restDataStack)->u = result.rem;
            (--restDataStack)->u = result.quot;
            tos.u = (uint16_t)(result.quot >> 16);
#else
            uint32_t u1 = tos.u;
            uint32_t ud1_msb = restDataStack++->u;
            uint32_t ud1_lsb = restDataStack++->u;
            uint64_t ud1 = ((uint64_t)ud1_msb << 32) | ud1_lsb;
            lldiv_t result = lldiv(ud1, u1);
            (--restDataStack)->u = result.rem;
            (--restDataStack)->u = result.quot;
            tos.u = (uint32_t)(result.quot >> 32);
#endif
        }
        continue;

        /* -------------------------------------------------------------
         * > [CORE] 6.1.0540 "greater-than" ( n1 n2 -- flag )
         *
         * flag is true if and only if n1 is greater than n2. */
        GREATERTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i > tos.i ? -1 : 0;
        }
        continue;

        UGREATERTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->u > tos.u ? -1 : 0;
        }
        continue;

        AND:
        {
            CHECK_STACK(2, 1);
            tos.i &= restDataStack++->i;
        }
        continue;

        NOTEQUALS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i != tos.i ? -1 : 0;
        }
        continue;

        INITRP:
        {
            CHECK_STACK(0, 0);
            returnTop = (EnforthCell *)&vm->return_stack[32];
        }
        continue;

        OVER:
        {
            CHECK_STACK(2, 3);
            EnforthCell second = restDataStack[0];
            *--restDataStack = tos;
            tos = second;
        }
        continue;

        TWOOVER:
        {
            CHECK_STACK(4, 6);
            *--restDataStack = tos;
            EnforthCell x2 = restDataStack[2];
            EnforthCell x1 = restDataStack[3];
            *--restDataStack = x1;
            tos = x2;
        }
        continue;

        KEY:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = vm->key();
        }
        continue;

        TOR:
        {
            CHECK_STACK(1, 0);
            *--returnTop = tos;
            tos = *restDataStack++;
        }
        continue;

        RFROM:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos = *returnTop++;
        }
        continue;

        RFETCH:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos = returnTop[0];
        }
        continue;

        EQUALS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i == tos.i ? -1 : 0;
        }
        continue;

        SLASHSTRING:
        {
            CHECK_STACK(3, 2);
            EnforthCell adjust = tos;
            tos.u = restDataStack++->u - adjust.i;
            restDataStack[0].u += adjust.i;
        }
        continue;

        PLUSSTORE:
        {
            CHECK_STACK(2, 0);
            ((EnforthCell*)tos.ram)->i += restDataStack++->i;
            tos = *restDataStack++;
        }
        continue;

        VMADDRLIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;

#ifdef __AVR__
            if (inProgramSpace)
            {
                tos.ram = (uint8_t*)vm + (uint8_t)pgm_read_byte(ip);
            }
            else
#endif
            {
                tos.ram = (uint8_t*)vm + (uint8_t)*ip;
            }

            ip++;
        }
        continue;

        LESSTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i < tos.i ? -1 : 0;
        }
        continue;

        INVERT:
        {
            CHECK_STACK(1, 1);
            tos.i = ~tos.i;
        }
        continue;

        TWOSWAP:
        {
            CHECK_STACK(4, 4);
            EnforthCell x4 = tos;
            EnforthCell x3 = restDataStack[0];
            EnforthCell x2 = restDataStack[1];
            EnforthCell x1 = restDataStack[2];

            tos = x2;
            restDataStack[0] = x1;
            restDataStack[1] = x4;
            restDataStack[2] = x3;
        }
        continue;

        /* -------------------------------------------------------------
         * UM* [CORE] 6.1.2360 "u-m-star" ( u1 u2 -- ud )
         *
         * Multiply u1 by u2, giving the unsigned double-cell product
         * ud.  All values and arithmetic are unsigned */
        UMSTAR:
        {
            CHECK_STACK(2, 2);
#ifdef __AVR__
            uint32_t result = (uint32_t)tos.u * (uint32_t)restDataStack[0].u;
            restDataStack[0].u = (uint16_t)result;
            tos.u = (uint16_t)(result >> 16);
#else
            uint64_t result = (uint64_t)tos.u * (uint64_t)restDataStack[0].u;
            restDataStack[0].u = (uint32_t)result;
            tos.u = (uint32_t)(result >> 32);
#endif
        }
        continue;

        /* -------------------------------------------------------------
         * M+ [DOUBLE] 8.6.1.1830 "m-plus" ( d1|ud1 n -- d2|ud2 )
         *
         * Add n to d1|ud1, giving the sum d2|ud2. */
        MPLUS:
        {
            CHECK_STACK(3, 2);
#ifdef __AVR__
            int16_t n = tos.i;
            int16_t d1_msb = restDataStack++->i;
            int16_t d1_lsb = restDataStack++->i;
            int32_t ud1 = ((uint32_t)d1_msb << 16) | d1_lsb;
            int32_t result = ud1 + n;
            (--restDataStack)->i = (int16_t)result;
            tos.i = (int16_t)(result >> 16);
#else
            int32_t n = tos.i;
            int32_t d1_msb = restDataStack++->i;
            int32_t d1_lsb = restDataStack++->i;
            int64_t ud1 = ((uint64_t)d1_msb << 32) | d1_lsb;
            int64_t result = ud1 + n;
            (--restDataStack)->i = (int32_t)result;
            tos.i = (int32_t)(result >> 32);
#endif
        }
        continue;

        TWOFETCH:
        {
            CHECK_STACK(1, 2);
            *--restDataStack = *(EnforthCell*)(tos.ram + kEnforthCellSize);
            tos = *(EnforthCell*)tos.ram;
        }
        continue;

        TWOSTORE:
        {
            CHECK_STACK(3, 0);
            EnforthCell * addr = (EnforthCell*)tos.ram;
            *addr++ = *restDataStack++;
            *addr = *restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        RSHIFT:
        {
            CHECK_STACK(2, 1);
            tos.u = restDataStack++->u >> tos.u;
        }
        continue;

        LSHIFT:
        {
            CHECK_STACK(2, 1);
            tos.u = restDataStack++->u << tos.u;
        }
        continue;

        DOCOLON:
        {
            /* IP currently points to the relative offset of the PFA of
             * the target word.  Read that offset and advance IP to the
             * token after the offset. */
            w = vm->dictionary.ram + *(uint16_t*)ip;
            ip += 2;

        PDOCOLON:
            /* IP now points to the next word in the PFA and that is the
             * location to which we should return once this new word has
             * executed. */
#ifdef __AVR__
            if (inProgramSpace)
            {
                /* TODO Needs to be relative to the ROM definition
                 * block. */
                ip = (uint8_t*)((unsigned int)ip | 0x8000);

                /* We are no longer in program space since, by design,
                 * DOCOLON is only ever used for user-defined words in
                 * RAM. */
                inProgramSpace = 0;
            }
#endif
            (--returnTop)->ram = (void *)ip;

            /* Now set the IP to the PFA of the word that is being
             * called and continue execution inside of that word. */
            ip = w;
        }
        continue;

        DOCOLONROM:
        {
            /* Push IP to the return stack, marking IP as
             * in-program-space as necessary. */
#ifdef __AVR__
            if (inProgramSpace)
            {
                ip = (uint8_t*)((unsigned int)ip | 0x8000);
            }
#endif
            (--returnTop)->ram = (void *)ip;

            /* Calculate the offset of the definition and set the IP to
             * the absolute address. */
            int definitionOffset = token << 2;
            ip = (uint8_t*)definitions + definitionOffset;
#ifdef __AVR__
            inProgramSpace = -1;
#endif
        }
        continue;

        EXIT:
        {
            ip = (uint8_t *)((returnTop++)->ram);

#ifdef __AVR__
            if (((unsigned int)ip & 0x8000) != 0)
            {
                /* TODO Needs to be relative to the ROM definition
                 * block. */
                ip = (uint8_t*)((unsigned int)ip & 0x7FFF);
                inProgramSpace = -1;
            }
            else
            {
                inProgramSpace = 0;
            }
#endif
        }
        continue;
    }
}
