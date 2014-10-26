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
#if ENABLE_TRACING
#include <stdio.h>
#endif
#include <stddef.h>
#include <string.h>

/* AVR includes. */
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM
#define pgm_read_byte(p) (*(uint8_t*)(p))
#define pgm_read_word(p) (*(int *)(p))
#endif

/* Enforth includes. */
#include "enforth.h"



/* -------------------------------------
 * Enforth tokens.
 */

typedef enum EnforthToken
{
#include "enforth_tokens.h"

    /* Tokens 0x70-0x7f are reserved for jump labels to the "CFA"
     * primitives.  The token names themselves do not need to be defined
     * because they are never referenced (we're just reserving space in
     * the token list and Address Interpreter jump table, in other
     * words), but we do list them here in order to make it easier to
     * turn raw tokens into enum values in the debugger. */
    DOCOLON = 0x70,
    DOCOLONROM,
    DOCONSTANT,
    DOCREATE,
    DODOES,
    DOVARIABLE,
    /* Unused */
    /* Unused */

    DOFFI0 = 0x78,
    DOFFI1,
    DOFFI2,
    DOFFI3,
    DOFFI4,
    DOFFI5,
    DOFFI6,
    DOFFI7,
} EnforthToken;



/* -------------------------------------
 * Enforth constants
 */

static const int kNFAtoCFA = 1 /* PSF+namelen */ + 2 /* LFA */;
static const int kNFAtoPFA = 1 /* PSF+namelen */ + 2 /* LFA */ + 1; /* CFA */



/* -------------------------------------
 * Enforth globals.
 */

#if ENABLE_TRACING
static int gTraceLevel = 0;
#endif



/* -------------------------------------
 * Enforth definition names.
 */

/* This must be stored in near memory on large AVRs since it is accessed
 * using instruction space words, all of which require that their target
 * addresses be able to fit in a cell (which is 16 bits on the AVR). */
static const char kDefinitionNames[] PROGMEM =
#include "enforth_names.h"
;



/* -------------------------------------
 * Enforth definitions.
 */

/* This must be stored in near memory on large AVRs since it is accessed
 * using instruction space words, all of which require that their target
 * addresses be able to fit in a cell (which is 16 bits on the AVR). */
static const int8_t definitions[] PROGMEM = {
#include "enforth_definitions.h"
};



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
    vm->dictionary_size.u = dictionary_size;

    enforth_reset(vm);
}

void enforth_reset(EnforthVM * const vm)
{
    /* Reset the globals. */
    vm->dp = vm->dictionary;
    vm->latest.u = 0;

    vm->hld = NULL;

    vm->state = 0;

    vm->saved_sp.u = 0;
    vm->base = 10;

    /* TODO This entire block below isn't really necessary since people
     * aren't allowed to call enforth_resume on their own.  Instead,
     * they should always call enforth_evaluate or enforth_go.  But if
     * they call enforth_evaluate then enforth_evaluate needs to have an
     * IP and RSP to pop from the stack and so enforth_reset needs to
     * prepare the stack.
     *
     * What we really need is some sort of vm->status enum that tells us
     * if the VM is halted and can be resumed (and therefore has IP and
     * RSP stack items) or has never been run and therefore
     * enforth_evaluate doesn't need to clean anything up. */

    /* Clear both stacks. */
    EnforthCell * sp = (EnforthCell*)&vm->data_stack[31];
    EnforthCell * rsp = (EnforthCell*)&vm->return_stack[31];

    /* Set the IP to the beginning of COLD. */
#ifdef __AVR__
    uint8_t* ip = (void *)(0x8000 | (unsigned int)((uint8_t*)definitions + (ROMDEF_COLD&0x3FFF) + kNFAtoPFA));
#else
    uint8_t* ip = (uint8_t*)definitions + (ROMDEF_COLD&0x3FFF) + kNFAtoPFA;
#endif

    /* Push RSP and IP to the stack. */
    (--sp)->ram = (uint8_t*)rsp;
    (--sp)->ram = ip;

    /* Save the stack pointer. */
    vm->saved_sp.u = &vm->data_stack[31] - sp;
}

void enforth_evaluate(EnforthVM * const vm, const char * const text)
{
    /* Clear the return stack. */
    EnforthCell * rsp = (EnforthCell*)&vm->return_stack[31];

    /* Push the address of HALT onto the return stack so that we exit
     * the interpreter after EVALUATE is done. */
#ifdef __AVR__
    (--rsp)->ram = (void *)(0x8000 | (unsigned int)((uint8_t*)definitions + (ROMDEF_HALT&0x3FFF) + kNFAtoPFA));
#else
    (--rsp)->ram = (uint8_t*)definitions + (ROMDEF_HALT&0x3FFF) + kNFAtoPFA;
#endif

    /* Set the IP to the beginning of EVALUATE. */
#ifdef __AVR__
    uint8_t* ip = (void *)(0x8000 | (unsigned int)((uint8_t*)definitions + (ROMDEF_EVALUATE&0x3FFF) + kNFAtoPFA));
#else
    uint8_t* ip = (uint8_t*)definitions + (ROMDEF_EVALUATE&0x3FFF) + kNFAtoPFA;
#endif

    /* Restore the stack pointer. */
    EnforthCell * sp = (EnforthCell*)&vm->data_stack[31 - vm->saved_sp.u];

    /* Pop the previous IP and RSP; we're about to replace them. */
    ++sp; /* IP */
    ++sp; /* RSP */

    /* Push the text and text length onto the stack. */
    (--sp)->ram = (uint8_t*)text;
    (--sp)->u = strlen(text);

    /* Push the new RSP and IP to the stack. */
    (--sp)->ram = (uint8_t*)rsp;
    (--sp)->ram = ip;

    /* Update the saved the stack pointer now that we have modified the
     * stack. */
    vm->saved_sp.u = &vm->data_stack[31] - sp;

    /* Resume the interpreter. */
    enforth_resume(vm);
}

void enforth_resume(EnforthVM * const vm)
{
    register uint8_t *ip;
    register EnforthCell tos;
    register EnforthCell *restDataStack; /* Points at the second item on the stack. */
    register uint8_t *w;
    register EnforthCell *returnTop;

#ifdef __AVR__
    register int8_t inProgramSpace;
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

    static const void * const primitive_table[128] PROGMEM = {
#include "enforth_jumptable.h"

        /* $70 - $77 */
        &&DOCOLON,
        &&DOCOLONROM,
        &&DOCONSTANT,
        &&DOCREATE,

        0, /* &&DODOES, */
        &&DOVARIABLE,
        0, /* Unused */
        0, /* Unused */

        /* $78 - $7F */
        &&DOFFI0,
        &&DOFFI1,
        &&DOFFI2,
        &&DOFFI3,

        &&DOFFI4,
        &&DOFFI5,
        &&DOFFI6,
        &&DOFFI7,
    };

    /* Restore the stack pointer. */
    restDataStack = (EnforthCell*)&vm->data_stack[31 - vm->saved_sp.u];

    /* Pop IP and RSP from the stack. */
    ip = (restDataStack++)->ram;
    returnTop = (EnforthCell *)(restDataStack++)->ram;

    /* Pop TOS into our register. */
    tos = *restDataStack++;

    /* The inner interpreter. */
    for (;;)
    {
        /* Get the next instruction, which could be one or two bytes
         * depending on if this is a Code Primitive (one byte) or a
         * Definition (two bytes).  The W ("Word") pointer needs to be
         * set if this is a Definition. */
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

        /* Is this actually a token (< 128)?  If so, dispatch the
         * token. */
        if (token < 128)
        {
            goto DISPATCH_TOKEN;
        }
        else
        {

            /* Not a token, which means that this is a two-byte XT that
             * points at the NFA of the word to be called. */
            uint16_t xt = token << 8;

#ifdef __AVR__
            if (inProgramSpace)
            {
                xt |= pgm_read_byte(ip++);
            }
            else
#endif
            {
                xt |= *ip++;
            }

            /* Convert the XT into a Word Pointer depending on the type
             * of XT (ROM definition or user definition) and then read
             * the CFA. */
            if ((xt & 0xC000) == 0xC000) /* ROM Definition: 0xCxxx */
            {
                w = (uint8_t*)((uint8_t*)definitions + (xt & 0x3FFF) + kNFAtoCFA);
#ifdef __AVR__
                token = pgm_read_byte(w++);
#else
                token = *w++;
#endif
            }
            else /* User Definition: 0x8xxx */
            {
                w = (uint8_t*)(vm->dictionary.ram + (xt & 0x3FFF) + kNFAtoCFA);
                token = *w++;
            }
        }

        /* W now points at the PFA; fall through to dispatch the CFA
         * token. */

DISPATCH_TOKEN:
#if ENABLE_TRACING == 2
        {
            int i;
            for (i = 0; i < gTraceLevel; i++)
            {
                printf(".");
            }

            printf(" [%02x] ", token);

            if (token < 0x70) /* No names for DO* tokens */
            {
                const char * curDef = kDefinitionNames;
                for (i = 0; i < token; i++)
                {
                    curDef += 1 + (((unsigned int)*curDef) & 0x1f);
                }

                for (i = 1; i <= (((unsigned int)*curDef) & 0x1f); i++)
                {
                    printf("%c", *(curDef + i));
                }
            }

            for (i = 0; i < &vm->data_stack[32] - restDataStack - 1; i++)
            {
                printf(" %x", vm->data_stack[30 - i].u);
            }

            if ((&vm->data_stack[32] - restDataStack) > 0)
            {
                printf(" %x", tos.u);
            }

            printf("\n");
        }
#endif

        goto *(void *)pgm_read_word(&primitive_table[token]);

        PHALT:
        {
            /* Push TOS onto the stack. */
            *--restDataStack = tos;

            /* Push RSP and IP to the stack. */
            /* TODO Both of these need to be relative addresses. */
            (--restDataStack)->ram = (uint8_t*)returnTop;
            (--restDataStack)->ram = ip;

            /* Save the stack pointer. */
            vm->saved_sp.u = &vm->data_stack[31] - restDataStack;

            /* Exit the interpreter. */
            return;
        }

        DOFFI0:
        {
            CHECK_STACK(0, 1);

            /* W contains a pointer to the PFA of the FFI definition;
             * get the FFI definition pointer and then use that to get
             * the FFI function pointer. */
            ZeroArgFFI fn = (ZeroArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);

            *--restDataStack = tos;
            tos = (*fn)();
        }
        continue;

        DOFFI1:
        {
            CHECK_STACK(1, 1);

            /* W contains a pointer to the PFA of the FFI definition;
             * get the FFI definition pointer and then use that to get
             * the FFI function pointer. */
            OneArgFFI fn = (OneArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);

            tos = (*fn)(tos);
        }
        continue;

        DOFFI2:
        {
            CHECK_STACK(2, 1);
            TwoArgFFI fn = (TwoArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);

            EnforthCell arg2 = tos;
            EnforthCell arg1 = *restDataStack++;
            tos = (*fn)(arg1, arg2);
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

        /* Cannot be used in ROM definitions! */
        LIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos = *(EnforthCell*)ip;
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
        IBRANCH:
#ifdef __AVR__
        {
            CHECK_STACK(0, 0);
            ip += (int8_t)pgm_read_byte(ip);
        }
        continue;
#else
        /* Fall through, since the other architectures use shared
         * instruction and data space. */
#endif

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
            ip += *(int8_t*)ip;
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

            /* Set the IP to the beginning of QUIT */
#ifdef __AVR__
            ip = (void *)(0x8000 | (unsigned int)((uint8_t*)definitions + (ROMDEF_QUIT&0x3FFF) + kNFAtoPFA));
#else
            ip = (uint8_t*)definitions + (ROMDEF_QUIT&0x3FFF) + kNFAtoPFA;
#endif
        }
        continue;

        ICHARLIT:
#ifdef __AVR__
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = pgm_read_byte(ip);
            ip++;
        }
        continue;
#else
        /* Fall through, since the other architectures use shared
         * instruction and data space. */
#endif

        CHARLIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = *ip++;
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

        IFETCH:
#ifdef __AVR__
        {
            CHECK_STACK(1, 1);
            tos.u = pgm_read_word(tos.ram);
        }
        continue;
#else
        /* Fall through, since the other architectures use shared
         * instruction and data space. */
#endif
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

        IZBRANCH:
#ifdef __AVR__
        {
            CHECK_STACK(1, 0);

            if (tos.i == 0)
            {
                ip += (int8_t)pgm_read_byte(ip);
            }
            else
            {
                ip++;
            }

            tos = *restDataStack++;
        }
        continue;
#else
        /* Fall through, since the other architectures use shared
         * instruction and data space. */
#endif

        ZBRANCH:
        {
            CHECK_STACK(1, 0);

            if (tos.i == 0)
            {
                ip += *(int8_t*)ip;
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

        PISQUOTE:
#ifdef __AVR__
        {
            CHECK_STACK(0, 2);

            /* Push existing TOS onto the stack. */
            *--restDataStack = tos;

            /* Instruction stream contains the length of the string as a
             * byte and then the string itself.  Start out by getting
             * the length into TOS. */
            tos.u = pgm_read_byte(ip);
            ip++;

            /* Now push the address of the string (the newly-incremented
             * IP) onto the stack. */
            (--restDataStack)->ram = ip;

            /* Advance IP over the string. */
            ip = ip + tos.u;
        }
        continue;
#else
        /* Fall through, since the other architectures use shared
         * instruction and data space. */
#endif

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

        ICFETCH:
#ifdef __AVR__
        {
            CHECK_STACK(1, 1);
            tos.u = pgm_read_byte(tos.ram);
        }
        continue;
#else
        /* Fall through, since the other architectures use shared
         * instruction and data space. */
#endif
        CFETCH:
        {
            CHECK_STACK(1, 1);
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

        TWODUP:
        {
            CHECK_STACK(2, 4);
            EnforthCell second = *restDataStack;
            *--restDataStack = tos;
            *--restDataStack = second;
        }
        continue;

        TUCK:
        {
            EnforthCell second = *restDataStack;
            *restDataStack = tos;
            *--restDataStack = second;
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
            tos.u = abs(tos.i);
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
         * UM/MOD [CORE] 6.1.2370 "u-m-slash-mod" ( ud u1 -- u2 u3 )
         *
         * Divide ud by u1, giving the quotient u3 and the remainder u2.
         * All values and arithmetic are unsigned.  An ambiguous
         * condition exists if u1 is zero or if the quotient lies
         * outside the range of a single-cell unsigned integer. */
        UMSLASHMOD:
        {
            CHECK_STACK(3, 2);
#ifdef __AVR__
            uint16_t u1 = tos.u;
            uint16_t ud_msb = restDataStack++->u;
            uint16_t ud_lsb = restDataStack++->u;
            uint32_t ud = ((uint32_t)ud_msb << 16) | ud_lsb;

            // FIXME Doesn't work on Arduino; maybe a compiler
            // difference?  Meanwhile, ldiv seems to work in the vast
            // majority of cases; only extreme situations ((65535 *
            // 65535) / 65535) return the incorrect results.
            //
            // (--restDataStack)->u = ud % u1;
            // tos.u = ud / u1;

            ldiv_t result = ldiv(ud, u1);
            (--restDataStack)->u = result.rem;
            tos.u = result.quot;
#else
            uint32_t u1 = tos.u;
            uint32_t ud_msb = restDataStack++->u;
            uint32_t ud_lsb = restDataStack++->u;
            uint64_t ud = ((uint64_t)ud_msb << 32) | ud_lsb;
            (--restDataStack)->u = ud % u1;
            tos.u = ud / u1;
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

        ULESSTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->u < tos.u ? -1 : 0;
        }
        continue;

        AND:
        {
            CHECK_STACK(2, 1);
            tos.i &= restDataStack++->i;
        }
        continue;

        XOR:
        {
            CHECK_STACK(2, 1);
            tos.i ^= restDataStack++->i;
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

        I:
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

        PLUSSTORE:
        {
            CHECK_STACK(2, 0);
            ((EnforthCell*)tos.ram)->i += restDataStack++->i;
            tos = *restDataStack++;
        }
        continue;

        VM:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.ram = (uint8_t*)vm;
        }
        continue;

        TICKNAMES:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.ram = (uint8_t*)kDefinitionNames;
        }
        continue;

        TICKROMDEF:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.ram = (uint8_t*)definitions;
        }
        continue;

        LASTROMDEF:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.u = ROMDEF_LAST;
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
            uint16_t n = tos.u;
            uint16_t d1_msb = restDataStack++->u;
            uint16_t d1_lsb = restDataStack++->u;
            uint32_t ud1 = ((uint32_t)d1_msb << 16) | d1_lsb;
            uint32_t result = ud1 + n;
            (--restDataStack)->u = (uint16_t)result;
            tos.u = (uint16_t)(result >> 16);
#else
            uint32_t n = tos.u;
            uint32_t d1_msb = restDataStack++->u;
            uint32_t d1_lsb = restDataStack++->u;
            uint64_t ud1 = ((uint64_t)d1_msb << 32) | d1_lsb;
            uint64_t result = ud1 + n;
            (--restDataStack)->u = (uint32_t)result;
            tos.u = (uint32_t)(result >> 32);
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

        TWOSTAR:
        {
            CHECK_STACK(1, 1);
            tos.u = tos.u << 1;
        }
        continue;

        TWOSLASH:
        {
            CHECK_STACK(1, 1);
            tos.i = tos.i >> 1;
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

        TWORFETCH:
        {
            CHECK_STACK(0, 2);
            *--restDataStack = tos;
            tos = returnTop[0];
            *--restDataStack = returnTop[1];
        }
        continue;

        TWORFROM:
        {
            CHECK_STACK(0, 2);
            *--restDataStack = tos;
            tos = *returnTop++;
            *--restDataStack = *returnTop++;
        }
        continue;

        PDO:
        TWOTOR:
        {
            CHECK_STACK(2, 0);
            *--returnTop = *restDataStack++;
            *--returnTop = tos;
            tos = *restDataStack++;
        }
        continue;

        PIQDO:
#ifdef __AVR__
        {
            CHECK_STACK(2, 0);
            EnforthCell limit = *restDataStack++;
            EnforthCell index = tos;
            tos = *restDataStack++;

            if (index.u == limit.u)
            {
                ip += (int8_t)pgm_read_byte(ip);
            }
            else
            {
                *--returnTop = limit;
                *--returnTop = index;
                ++ip;
            }
        }
        continue;
#else
        /* Fall through, since the other architectures use shared
         * instruction and data space. */
#endif

        PQDO:
        {
            CHECK_STACK(2, 0);
            EnforthCell limit = *restDataStack++;
            EnforthCell index = tos;
            tos = *restDataStack++;

            if (index.u == limit.u)
            {
                ip += *(int8_t*)ip;
            }
            else
            {
                *--returnTop = limit;
                *--returnTop = index;
                ++ip;
            }
        }
        continue;

        UNLOOP:
        {
            CHECK_STACK(0, 0);
            returnTop++;
            returnTop++;
        }
        continue;

        TWONIP:
        {
            CHECK_STACK(4, 2);
            EnforthCell x3 = *restDataStack++;
            *++restDataStack = x3;
        }
        continue;

        NEGATE:
        {
            CHECK_STACK(1, 1);
            tos.i = -tos.i;
        }
        continue;

        PILOOP:
#ifdef __AVR__
        {
            ++(returnTop[0]).i;
            if (returnTop[0].i == returnTop[1].i)
            {
                ++returnTop;
                ++returnTop;
                ++ip;
            }
            else
            {
                ip += (int8_t)pgm_read_byte(ip);
            }
        }
        continue;
#else
        /* Fall through, since the other architectures use shared
         * instruction and data space. */
#endif

        /* (LOOP) [Enforth] "paren-loop-paren" ( -- ) ( R: loop-sys1 -- | loop-sys2 )
         *
         * An ambiguous condition exists if the loop control parameters
         * are unavailable.  Add one to the loop index.  If the loop
         * index is then equal to the loop limit, discard the loop
         * parameters and continue execution immediately following the
         * loop.  Otherwise continue execution at the beginning of the
         * loop. */
        PLOOP:
        {
            ++(returnTop[0]).i;
            if (returnTop[0].i == returnTop[1].i)
            {
                ++returnTop;
                ++returnTop;
                ++ip;
            }
            else
            {
                ip += *(int8_t*)ip;
            }
        }
        continue;

        PIPLUSLOOP:
#ifdef __AVR__
        {
            CHECK_STACK(1, 0);

            returnTop[0].i += tos.i;
            tos = *restDataStack++;

            if (returnTop[0].i >= returnTop[1].i)
            {
                ++returnTop;
                ++returnTop;
                ++ip;
            }
            else
            {
                ip += (int8_t)pgm_read_byte(ip);
            }
        }
        continue;
#else
        /* Fall through, since the other architectures use shared
         * instruction and data space. */
#endif

        PPLUSLOOP:
        {
            CHECK_STACK(1, 0);

            returnTop[0].i += tos.i;

            if (((tos.i >= 0) && (returnTop[0].i >= returnTop[1].i))
                    || ((tos.i < 0) && (returnTop[0].i < returnTop[1].i)))
            {
                ++returnTop;
                ++returnTop;
                ++ip;
            }
            else
            {
                ip += *(int8_t*)ip;
            }

            tos = *restDataStack++;
        }
        continue;

        MAX:
        {
            CHECK_STACK(2, 1);
            tos.i = tos.i > restDataStack->i ? tos.i : restDataStack->i;
            ++restDataStack;
        }
        continue;

        MIN:
        {
            CHECK_STACK(2, 1);
            tos.i = tos.i < restDataStack->i ? tos.i : restDataStack->i;
            ++restDataStack;
        }
        continue;

        DOCOLON:
        {
            /* IP points to the next word in the PFA and that is the
             * location to which we should return once this new word has
             * executed. */
#ifdef __AVR__
            if (inProgramSpace)
            {
                /* Set the high bit on the return address so that we
                 * know that this address is in ROM. */
                ip = (uint8_t*)((unsigned int)ip | 0x8000);

                /* We are no longer in program space since, by design,
                 * DOCOLON is only ever used for user-defined words in
                 * RAM. */
                inProgramSpace = 0;
            }
#endif
            (--returnTop)->ram = ip;

            /* Now set the IP to the PFA of the word that is being
             * called and continue execution inside of that word. */
            ip = w;
        }
        continue;

        DOCREATE:
        DOVARIABLE:
        {
            /* W points at the PFA of this word; push that location onto
             * the stack. */
            *--restDataStack = tos;
            tos.ram = w;
        }
        continue;

        DOCONSTANT:
        {
            /* W points at the PFA of this word; push the address in
             * that location onto the stack. */
            *--restDataStack = tos;
            tos = *(EnforthCell*)w;
        }
        continue;

        DOCOLONROM:
        {
#if ENABLE_TRACING
            gTraceLevel++;

            int i;
            for (i = 0; i < gTraceLevel; i++)
            {
                printf(">");
            }

            printf(" [%02x] ", token);

            /* W points at the PFA; back up to the NFA. */
            const char * curDef = (const char *)w - (1 + 2 + 1);

            /* Is this a hidden definition (the name is length zero)?
             * If so, output the XT instead of the name. */
            if ((*curDef & 0x1f) == 0)
            {
                printf("<pfa=0x%04X>", 0xC000 | (uint16_t)((uint8_t*)curDef - (uint8_t*)definitions));
            }
            else
            {
                /* Normal definition; walk backwards and output the
                 * name. */
                for (i = 1; i <= (((unsigned int)*curDef) & 0x1f); --i)
                {
                    printf("%c", *(curDef + i) & 0x7f);
                }
            }

            /* Print the data stack and top-of-stack. */
            for (i = 0; i < &vm->data_stack[32] - restDataStack - 1; i++)
            {
                printf(" %x", vm->data_stack[30 - i].u);
            }

            if ((&vm->data_stack[32] - restDataStack) > 0)
            {
                printf(" %x", tos.u);
            }

            printf("\n");
            fflush(stdout);
#endif
            /* IP points to the next word in the PFA and that is the
             * location to which we should return once this new word has
             * executed. */
#ifdef __AVR__
            if (inProgramSpace)
            {
                /* Set the high bit on the return address so that we
                 * know that this address is in ROM. */
                ip = (uint8_t*)((unsigned int)ip | 0x8000);
            }
#endif
            (--returnTop)->ram = ip;

            /* Now set the IP to the PFA of the word that is being
             * called and continue execution inside of that word. */
            ip = w;

#ifdef __AVR__
            /* We are obviously now in program space since, by design,
             * DOCOLONROM is only ever used for ROM definitions. */
            inProgramSpace = -1;
#endif
        }
        continue;

        EXIT:
        {
#if ENABLE_TRACING
            int i;
            for (i = 0; i < gTraceLevel; i++)
            {
                printf("<");
            }

            printf(" (");
            for (i = 0; i < &vm->data_stack[32] - restDataStack - 1; i++)
            {
                printf("%x ", vm->data_stack[30 - i].u);
            }

            if ((&vm->data_stack[32] - restDataStack) > 0)
            {
                printf("%x", tos.u);
            }
            printf(")");

            printf(" (R:");
            for (i = 0; i < &vm->return_stack[32] - returnTop; i++)
            {
                printf(" %x", vm->return_stack[31 - i].u);
            }
            printf(")");

            printf("\n");

            gTraceLevel--;
#endif

            ip = (uint8_t *)((returnTop++)->ram);

#ifdef __AVR__
            if (((unsigned int)ip & 0x8000) != 0)
            {
                /* This return address points at a ROM definition; strip
                 * off that flag as part of popping the address. */
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

void enforth_go(EnforthVM * const vm)
{
    /* Clear both stacks. */
    EnforthCell * sp = (EnforthCell*)&vm->data_stack[31];
    EnforthCell * rsp = (EnforthCell*)&vm->return_stack[31];

    /* Set the IP to the beginning of COLD. */
#ifdef __AVR__
    uint8_t* ip = (void *)(0x8000 | (unsigned int)((uint8_t*)definitions + (ROMDEF_COLD&0x3FFF) + kNFAtoPFA));
#else
    uint8_t* ip = (uint8_t*)definitions + (ROMDEF_COLD&0x3FFF) + kNFAtoPFA;
#endif

    /* Push RSP and IP to the stack. */
    (--sp)->ram = (uint8_t*)rsp;
    (--sp)->ram = ip;

    /* Save the stack pointer. */
    vm->saved_sp.u = &vm->data_stack[31] - sp;

    /* Resume the interpreter. */
    enforth_resume(vm);
}
