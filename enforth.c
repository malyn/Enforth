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
    DOVARIABLE,
    /* Unused */
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

/* Some compilers have issues with these things being constants (static
 * const int), which is why they are #define'd instead. */
#define kNFAtoCFA (1 /* PSF+namelen */ + 2 /* LFA */)
#define kNFAtoPFA (1 /* PSF+namelen */ + 2 /* LFA */ + 2 /* CFA */)

#define kTaskUserVariableSize 8
#define kTaskReturnStackSize 32
#define kTaskDataStackSize 24

#define kTaskStartToReturnTop ((kEnforthCellSize * kTaskUserVariableSize) + (kEnforthCellSize * (kTaskReturnStackSize - 1)))
#define kTaskStartToDataTop ((kEnforthCellSize * kTaskUserVariableSize) + (kEnforthCellSize * kTaskReturnStackSize) + (kEnforthCellSize * (kTaskDataStackSize - 1)))



/* -------------------------------------
 * Enforth globals.
 */

#if ENABLE_TRACING
static int gTraceLevel = 0;
#endif



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
        int (*keyq)(void), char (*key)(void), void (*emit)(char),
        int (*load)(uint8_t*, int), int (*save)(uint8_t*, int))
{
    vm->last_ffi = last_ffi;

    vm->keyq = keyq;
    vm->key = key;
    vm->emit = emit;
    vm->load = load;
    vm->save = save;

    vm->dictionary.ram = dictionary;
    vm->dictionary_size.u = dictionary_size;

    enforth_reset(vm);
}

void enforth_reset(EnforthVM * const vm)
{
    /* Initialize the dictionary, which contains the DP, LATEST, and
     * LASTTASK cells, and the default task.  The user-accessible
     * portion of the dictionary starts after that block of data.  DP is
     * reset to point after that block, LATEST points at ROMDEF_LAST,
     * and LASTTASK points to the default task. */
    /* TODO The DP and LASTTASK cells should be dictionary-relative so
     * that the dictionary can be loaded into RAM at a different
     * location across SAVE/LOADs. */
    ((EnforthCell*)vm->dictionary.ram)[0].ram
        = vm->dictionary.ram
        + kEnforthCellSize /* DP */
        + kEnforthCellSize /* LATEST */
        + kEnforthCellSize /* LASTTASK */
        + (kEnforthCellSize * 64); /* Task Control Block */

    ((EnforthCell*)vm->dictionary.ram)[1].u
        = ROMDEF_LAST;

    vm->cur_task.ram
        = vm->dictionary.ram
        + (kEnforthCellSize * 3);
    ((EnforthCell*)vm->dictionary.ram)[2].ram
        = vm->cur_task.ram;

    /* Reset the globals. */
    vm->hld = NULL;
    vm->state = 0;

    /* Reset the task. */
    /* TODO These should be dictionary-relative so that they can
     * relocate with the dictionary. */
    ((EnforthCell*)vm->cur_task.ram)[0].u = 0; /* User: PREVTASK */
    ((EnforthCell*)vm->cur_task.ram)[1].u = 0; /* User: SAVEDSP */
    ((EnforthCell*)vm->cur_task.ram)[2].u = 10; /* User: BASE */

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
    EnforthCell * sp = (EnforthCell*)(vm->cur_task.ram + kTaskStartToDataTop - kEnforthCellSize);
    EnforthCell * rsp = (EnforthCell*)(vm->cur_task.ram + kTaskStartToReturnTop - kEnforthCellSize);

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
    /* TODO This should be dictionary-relative so that it can relocate
     * with the dictionary. */
    ((EnforthCell*)vm->cur_task.ram)[1].ram = (uint8_t*)sp;
}

void enforth_evaluate(EnforthVM * const vm, const char * const text)
{
    /* Clear the return stack. */
    EnforthCell * rsp = (EnforthCell*)(vm->cur_task.ram + kTaskStartToReturnTop - kEnforthCellSize);

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
    EnforthCell * sp = (EnforthCell*)((EnforthCell*)vm->cur_task.ram)[1].ram;

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
    /* TODO This should be dictionary-relative so that it can relocate
     * with the dictionary. */
    ((EnforthCell*)vm->cur_task.ram)[1].ram = (uint8_t*)sp;

    /* Resume the interpreter. */
    enforth_resume(vm);
}

void enforth_resume(EnforthVM * const vm)
{
    register uint8_t *ip;
    register uint16_t xt;
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
        if (((EnforthCell*)(vm->cur_task.ram + kTaskStartToDataTop) - restDataStack) < numArgs) { \
            goto STACK_UNDERFLOW; \
        } else if ((((EnforthCell*)(vm->cur_task.ram + kTaskStartToDataTop) - restDataStack) - numArgs) + numResults > 32) { \
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

        &&DOVARIABLE,
        0, /* Unused */
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

UNPAUSE:

    /* Restore the stack pointer. */
    restDataStack = (EnforthCell*)((EnforthCell*)vm->cur_task.ram)[1].ram;

    /* Pop IP and RSP from the stack. */
    ip = (restDataStack++)->ram;
#ifdef __AVR__
    if (((unsigned int)ip & 0x8000) != 0)
    {
        /* This IP address points at a ROM definition; strip off that
         * flag as part of popping the address. */
        ip = (uint8_t*)((unsigned int)ip & 0x7FFF);
        inProgramSpace = -1;
    }
    else
    {
        inProgramSpace = 0;
    }
#endif

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
            w = NULL;
            goto DISPATCH_TOKEN;
        }
        else
        {
            /* Not a token, which means that this is a two-byte XT that
             * points at the NFA of the word to be called. */
            xt = token << 8;

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

DISPATCH_XT:
            /* This is a two-byte XT that points at another word.  We
             * need to find the word that is being targeted by this XT
             * and set W to that word's PFA.  We then need to dispatch
             * to the word's Code Field.  The Code Field will almost
             * always contain a DO* token, *unless the word was modified
             * by a defining word.*  In that case, the Code Field will
             * contain a full, two-byte XT and that XT will point to the
             * runtime behavior of the defining word (the code after
             * DOES> in the defining word).
             *
             * Note that ROM Definitions never use DOES> and so we can
             * assume that the Code Field in a ROM Definition will
             * always contain a token. */
            if ((xt & 0xC000) == 0xC000) /* ROM Definition: 0xCxxx */
            {
                /* Advance past the empty first byte in the Code Field,
                 * then read the token in the second byte. */
                /* TODO We could probably eliminate the first byte in
                 * ROM Definitions. */
                w = (uint8_t*)((uint8_t*)definitions + (xt & 0x3FFF) + kNFAtoCFA + 1);
#ifdef __AVR__
                token = pgm_read_byte(w++);
#else
                token = *w++;
#endif
            }
            else /* User Definition: 0x8xxx */
            {
                /* Load the Code Field. */
                w = (uint8_t*)(vm->dictionary.ram + (xt & 0x3FFF) + kNFAtoCFA);
                xt = *w++ << 8;
                xt |= *w++;

                /* We're done if this is a token. */
                if (xt < 0x80)
                {
                    token = xt;
                    goto DISPATCH_TOKEN;
                }
                else
                {
                    /* Not a token, which means that this word was
                     * defined by DOES>; jump to DODOES to perform the
                     * runtime behavior of the defining word. */
                    goto DODOES;
                }
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

            /* FIXME Needs to get the XT of the token and then use that
             * to retrieve the name. */
#if false
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
#endif

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


        /* =============================================================
         * KERNEL PRIMITIVES
         */

        DOCOLON:
        {
            /* IP points to the next word in the PFA and that is the
             * location to which we should return once this new word has
             * executed. */
#ifdef __AVR__
            if (inProgramSpace)
            {
                /* FIXME Can this ever happen?  How would a ROM word
                 * know about a RAM word? */
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

        DOCONSTANT:
        {
            /* W points at the PFA of this word; push the address in
             * that location onto the stack. */
            *--restDataStack = tos;
            tos = *(EnforthCell*)w;
        }
        continue;

        DODOES:
        {
            /* We're currently sitting in a word that is calling a word
             * defined with DOES>.  IP has been advanced beyond that
             * call and is still in the calling word.  XT contains the
             * Code Field of the defined word (and thus points to the
             * DOES> part of the defining word's thread).  We need to
             * push IP to the stack, push the PFA of the defined word to
             * the stack, and then continue execution at XT (the
             * defining word's runtime behavior). */
#ifdef __AVR__
            if (inProgramSpace)
            {
                /* FIXME Can this ever happen?  How would a ROM word
                 * know about a RAM word? */
                /* Set the high bit on the return address so that we
                 * know that this address is in ROM. */
                ip = (uint8_t*)((unsigned int)ip | 0x8000);

                /* We are no longer in program space since, by design,
                 * DODOES is only ever used for user-defined words in
                 * RAM. */
                inProgramSpace = 0;
            }
#endif
            (--returnTop)->ram = ip;

            /* W points at the PFA of the defined word; push that to the
             * stack per the runtime behavior of DOES>. */
            *--restDataStack = tos;
            tos.ram = w;

            /* Point IP at the DOES> portion of the defining word (which
             * is the target of the defined word's Code Field and is
             * thus in XT). */
            ip = (uint8_t*)(vm->dictionary.ram + (xt & 0x3FFF));
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

        DOFFI0:
        {
            CHECK_STACK(0, 1);

            /* W contains a pointer to the PFA of the FFI definition;
             * get the FFI definition pointer and then use that to get
             * the FFI function pointer.  Get the value of the is_void
             * flag as well.*/
            ZeroArgFFI fn = (ZeroArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);
            uint8_t is_void = pgm_read_byte(&(*(EnforthFFIDef**)w)->is_void);

            if (is_void == 0)
            {
                *--restDataStack = tos;
                tos = (*fn)();
            }
            else
            {
                (*fn)();
            }
        }
        continue;

        DOFFI1:
        {
            CHECK_STACK(1, 1);

            /* W contains a pointer to the PFA of the FFI definition;
             * get the FFI definition pointer and then use that to get
             * the FFI function pointer.  Get the value of the is_void
             * flag as well. */
            OneArgFFI fn = (OneArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);
            uint8_t is_void = pgm_read_byte(&(*(EnforthFFIDef**)w)->is_void);

            tos = (*fn)(tos);

            if (is_void == 1)
            {
                tos = *restDataStack++;
            }
        }
        continue;

        DOFFI2:
        {
            CHECK_STACK(2, 1);
            TwoArgFFI fn = (TwoArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);
            uint8_t is_void = pgm_read_byte(&(*(EnforthFFIDef**)w)->is_void);

            EnforthCell arg2 = tos;
            EnforthCell arg1 = *restDataStack++;

            tos = (*fn)(arg1, arg2);

            if (is_void == 1)
            {
                tos = *restDataStack++;
            }
        }
        continue;

        DOFFI3:
        {
            CHECK_STACK(3, 1);
            ThreeArgFFI fn = (ThreeArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);
            uint8_t is_void = pgm_read_byte(&(*(EnforthFFIDef**)w)->is_void);

            EnforthCell arg3 = tos;
            EnforthCell arg2 = *restDataStack++;
            EnforthCell arg1 = *restDataStack++;

            tos = (*fn)(arg1, arg2, arg3);

            if (is_void == 1)
            {
                tos = *restDataStack++;
            }
        }
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

        /* -------------------------------------------------------------
        ***{:token :ibranch
        *** :flags #{:headerless}}
         */
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

        /* -------------------------------------------------------------
        ***{:token :branch
        *** :flags #{:headerless}}
         */
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

        /* -------------------------------------------------------------
        ***{:token :icharlit
        *** :args [[] [:char]]
        *** :flags #{:headerless}}
         */
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

        /* -------------------------------------------------------------
        ***{:token :charlit
        *** :flags #{:headerless}}
         */
        CHARLIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = *ip++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :exit}
         */
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

        /* -------------------------------------------------------------
        ***{:token :lit
        *** :name "(LIT)"
        *** :args [[] [:x]]
        *** :flags #{:headerless}}
         */
        /* Cannot be used in ROM definitions! */
        LIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos = *(EnforthCell*)ip;
            ip += kEnforthCellSize;
        }
        continue;

        /* -------------------------------------------------------------
         * PAUSE [Enforth] ( -- )
         *
         * Suspend the current task and resume execution of the next
         * task in the task list.  PAUSE will return to the caller when
         * all of the tasks in the task list have had a chance to
         * execute (and called PAUSE in order to relinquish execution to
         * their next task).
         *
        ***{:token :pause}
         */
        PAUSE:
        {
            /* Push TOS onto the stack. */
            *--restDataStack = tos;

            /* Push RSP and IP to the stack. */
            /* TODO Both of these need to be relative addresses. */
            (--restDataStack)->ram = (uint8_t*)returnTop;

#ifdef __AVR__
            if (inProgramSpace)
            {
                /* Set the high bit on the current IP so that we know
                 * that this address is in ROM. */
                ip = (uint8_t*)((unsigned int)ip | 0x8000);
            }
#endif

            (--restDataStack)->ram = ip;

            /* Save the stack pointer. */
            ((EnforthCell*)vm->cur_task.ram)[1].ram = (uint8_t*)restDataStack;

            /* Make the previous task the current task. */
            vm->cur_task.ram = ((EnforthCell*)vm->cur_task.ram)[0].ram;

            /* Did we hit the beginning of the list?  If so, wrap around
             * to the last task. */
            if (vm->cur_task.ram == 0)
            {
                vm->cur_task.ram = ((EnforthCell*)vm->dictionary.ram)[2].ram;
            }

            /* Unpause the interpreter. */
            goto UNPAUSE;
        }

        /* -------------------------------------------------------------
         * (HALT) [Enforth] ( i*x -- i*x ) ( R: j*x -- j*x )
         *
         * Stops and exits the VM.  State is preserved on the stack,
         * allowing the VM to be resumed by enforth_resume.
         *
        ***{:token :phalt
        *** :name "(HALT)"
        *** :args [[] []]
        *** :flags #{:headerless}}
         */
        PHALT:
        {
            /* Push TOS onto the stack. */
            *--restDataStack = tos;

            /* Push RSP and IP to the stack. */
            /* TODO Both of these need to be relative addresses. */
            (--restDataStack)->ram = (uint8_t*)returnTop;
            (--restDataStack)->ram = ip;

            /* Save the stack pointer. */
            ((EnforthCell*)vm->cur_task.ram)[1].ram = (uint8_t*)restDataStack;

            /* Exit the interpreter. */
            return;
        }

        /* -------------------------------------------------------------
        ***{:token :izbranch
        *** :flags #{:headerless}}
         */
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

        /* -------------------------------------------------------------
        ***{:token :zbranch
        *** :name "0BRANCH"
        *** :flags #{:headerless}}
         */
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



        /* =============================================================
         * CORE PRIMITIVES
         */

        /* -------------------------------------------------------------
         * ABORT [CORE] 6.1.0670 ( i*x -- ) ( R: j*x -- )
         *
         * Empty the data stack and perform the function of QUIT, which
         * includes emptying the return stack, without displaying a
         * message.
         *
        ***{:token :abort}
         */
        ABORT:
        {
            tos.i = 0;
            restDataStack = (EnforthCell*)(vm->cur_task.ram + kTaskStartToDataTop);
            ((EnforthCell*)vm->cur_task.ram)[2].u = 10; /* User: BASE */

            /* Set the IP to the beginning of QUIT */
#ifdef __AVR__
            ip = (void *)(0x8000 | (unsigned int)((uint8_t*)definitions + (ROMDEF_QUIT&0x3FFF) + kNFAtoPFA));
#else
            ip = (uint8_t*)definitions + (ROMDEF_QUIT&0x3FFF) + kNFAtoPFA;
#endif
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :abs}
         */
        ABS:
        {
            CHECK_STACK(1, 1);
            tos.u = abs(tos.i);
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :and}
         */
        AND:
        {
            CHECK_STACK(2, 1);
            tos.i &= restDataStack++->i;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :base}
         */
        BASE:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.ram = (uint8_t*)&((EnforthCell*)vm->cur_task.ram)[2];
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :icfetch
        *** :name "IC@"
        *** :args [[:c-addr] [:c]]
        *** :flags #{:headerless}}
         */
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
        /* -------------------------------------------------------------
        ***{:token :cfetch
        *** :name "C@"}
         */
        CFETCH:
        {
            CHECK_STACK(1, 1);
            tos.u = *(uint8_t*)tos.ram;
        }
        continue;

        /* -------------------------------------------------------------
         * C! [CORE] 6.1.0850 "c-store" ( char c-addr -- )
         *
         * Store char at c-addr.  When character size is smaller than
         * cell size, only the number of low-order bits corresponding to
         * character size are transferred.
         *
        ***{:token :cstore
        *** :name "C!"}
         */
        CSTORE:
        {
            CHECK_STACK(2, 0);
            *(uint8_t*)tos.ram = restDataStack++->u;
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
         * DEPTH [CORE] 6.1.1200 ( -- +n )
         *
         * +n is the number of single-cell values contained in the data
         * stack before +n was placed on the stack.
         *
        ***{:token :depth}
         */
        DEPTH:
        {
            CHECK_STACK(0, 1);

            /* Save TOS, then calculate the stack depth.  The return
             * value should be the number of items on the stack *before*
             * DEPTH was called and so we have to subtract one from the
             * count given that we calculate the depth *after* pushing
             * the old TOS onto the stack. */
            *--restDataStack = tos;
            tos.i = (EnforthCell*)(vm->cur_task.ram + kTaskStartToDataTop) - restDataStack - 1;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :drop
        *** :args [[:x1 :x2] [:x1]]}
         */
        DROP:
        {
            CHECK_STACK(1, 0);
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :dup
        *** :args [[:x] [:x :x]]}
         */
        DUP:
        {
            CHECK_STACK(1, 2);
            *--restDataStack = tos;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :emit}
         */
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

        /* -------------------------------------------------------------
        ***{:token :equals
        *** :name "="}
         */
        EQUALS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i == tos.i ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
         * EXECUTE [CORE] 6.1.1370 ( i*x xt -- j*x )
         *
         * Remove xt from the stack and perform the semantics identified
         * by it.  Other stack effects are due to the word EXECUTEd.
         *
        ***{:token :execute
        *** :args [[:xt] []]}
         */
        EXECUTE:
        {
            xt = tos.u;
            tos = *restDataStack++;
            goto DISPATCH_XT;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :ifetch
        *** :name "I@"
        *** :args [[:addr] [:x]]
        *** :flags #{:headerless}}
         */
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
        /* -------------------------------------------------------------
        ***{:token :fetch
        *** :name "@"}
         */
        FETCH:
        {
            CHECK_STACK(1, 1);
            tos = *(EnforthCell*)tos.ram;
        }
        continue;

        /* -------------------------------------------------------------
         * > [CORE] 6.1.0540 "greater-than" ( n1 n2 -- flag )
         *
         * flag is true if and only if n1 is greater than n2.
         *
        ***{:token :greaterthan
        *** :name ">"}
         */
        GREATERTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i > tos.i ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :invert}
         */
        INVERT:
        {
            CHECK_STACK(1, 1);
            tos.i = ~tos.i;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :j
        *** :args [[] [:n]]}
         */
        J:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos = returnTop[2];
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :lessthan
        *** :name "<"}
         */
        LESSTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i < tos.i ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :lshift
        *** :args [[:x1 :u] [:x2]]}
         */
        LSHIFT:
        {
            CHECK_STACK(2, 1);
            tos.u = restDataStack++->u << tos.u;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :max
        *** :args [[:n1 :n2] [:n3]]}
         */
        MAX:
        {
            CHECK_STACK(2, 1);
            tos.i = tos.i > restDataStack->i ? tos.i : restDataStack->i;
            ++restDataStack;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :min
        *** :args [[:n1 :n2] [:n3]]}
         */
        MIN:
        {
            CHECK_STACK(2, 1);
            tos.i = tos.i < restDataStack->i ? tos.i : restDataStack->i;
            ++restDataStack;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :minus
        *** :name "-"}
         */
        MINUS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i - tos.i;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :move}
         */
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

        /* -------------------------------------------------------------
        ***{:token :negate}
         */
        NEGATE:
        {
            CHECK_STACK(1, 1);
            tos.i = -tos.i;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :oneminus
        *** :name "1-"}
         */
        ONEMINUS:
        {
            CHECK_STACK(1, 1);
            tos.i--;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :oneplus
        *** :name "1+"}
         */
        ONEPLUS:
        {
            CHECK_STACK(1, 1);
            tos.i++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :or}
         */
        OR:
        {
            CHECK_STACK(2, 1);
            tos.i |= restDataStack++->i;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :over}
         */
        OVER:
        {
            CHECK_STACK(2, 3);
            EnforthCell second = restDataStack[0];
            *--restDataStack = tos;
            tos = second;
        }
        continue;

        /* -------------------------------------------------------------
         * (DO) [Enforth] "paren-do-paren" ( n1|u1 n2|u2 -- ) ( R: -- loop-sys )
         *
         * Set up loop control parameters with index n2|u2 and limit
         * n1|u1. An ambiguous condition exists if n1|u1 and n2|u2 are
         * not both the same type.  Anything already on the return stack
         * becomes unavailable until the loop-control parameters are
         * discarded.
         *
        ***{:token :pdo
        *** :name "(DO)"
        *** :args [[:n1 :n2] []]
        *** :flags #{:headerless}}
         */
        PDO:

        /* -------------------------------------------------------------
        ***{:token :twotor
        *** :name "2>R"}
         */
        TWOTOR:
        {
            CHECK_STACK(2, 0);
            *--returnTop = *restDataStack++;
            *--returnTop = tos;
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :piloop
        *** :name "(ILOOP)"
        *** :flags #{:headerless}}
         */
        /* TODO We could remove this if we refactor DUMP, but I feel
         * like it could be useful for other definitions later on. */
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
         * loop.
         *
        ***{:token :ploop
        *** :name "(LOOP)"
        *** :flags #{:headerless}}
         */
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

        /* -------------------------------------------------------------
        ***{:token :pplusloop
        *** :name "(+LOOP)"
        *** :flags #{:headerless}}
         */
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

        /* -------------------------------------------------------------
        ***{:token :plus
        *** :name "+"}
         */
        PLUS:
        {
            CHECK_STACK(2, 1);
            tos.i += restDataStack++->i;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :plusstore
        *** :name "+!"}
         */
        PLUSSTORE:
        {
            CHECK_STACK(2, 0);
            ((EnforthCell*)tos.ram)->i += restDataStack++->i;
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :pqdo
        *** :name "(?DO)"
        *** :flags #{:headerless}}
         */
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

        /* -------------------------------------------------------------
        ***{:token :pisquote
        *** :name "(IS\")"
        *** :args [[] [:icaddr :u]]
        *** :flags #{:headerless}}
         */
        /* TODO Should probably just remove this since it is only used
         * in COLD... */
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
         * NOTE: Cannot be used in program space!
         *
        ***{:token :psquote
        *** :name "(S\")"
        *** :flags #{:headerless}}
         */
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

        /* -------------------------------------------------------------
        ***{:token :qdup
        *** :name "?DUP"}
         */
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
        ***{:token :i
        *** :args [[] [:n]]}
         */
        I:

        /* -------------------------------------------------------------
        ***{:token :rfetch
        *** :name "R@"}
         */
        RFETCH:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos = returnTop[0];
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :rfrom
        *** :name "R>"}
         */
        RFROM:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos = *returnTop++;
        }
        continue;

        /* -------------------------------------------------------------
         * ROT [CORE] 6.1.2160 "rote" ( x1 x2 x3 -- x2 x3 x1 )
         *
         * Rotate the top three stack entries.
         *
        ***{:token :rot}
         */
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
        ***{:token :rshift
        *** :args [[:x1 :u] [:x2]]}
         */
        RSHIFT:
        {
            CHECK_STACK(2, 1);
            tos.u = restDataStack++->u >> tos.u;
        }
        continue;

        /* -------------------------------------------------------------
         * ! [CORE] 6.1.0010 "store" ( x a-addr -- )
         *
         * Store x at a-addr.
         *
        ***{:token :store
        *** :name "!"}
         */
        STORE:
        {
            CHECK_STACK(2, 0);
            *(EnforthCell*)tos.ram = *restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :swap}
         */
        SWAP:
        {
            CHECK_STACK(2, 2);
            EnforthCell swap = restDataStack[0];
            restDataStack[0] = tos;
            tos = swap;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :tor
        *** :name ">R"}
         */
        TOR:
        {
            CHECK_STACK(1, 0);
            *--returnTop = tos;
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :twodrop
        *** :name "2DROP"}
         */
        TWODROP:
        {
            CHECK_STACK(2, 0);
            restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :twodup
        *** :name "2DUP"}
         */
        TWODUP:
        {
            CHECK_STACK(2, 4);
            EnforthCell second = *restDataStack;
            *--restDataStack = tos;
            *--restDataStack = second;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :twofetch
        *** :name "2@"}
         */
        TWOFETCH:
        {
            CHECK_STACK(1, 2);
            *--restDataStack = *(EnforthCell*)(tos.ram + kEnforthCellSize);
            tos = *(EnforthCell*)tos.ram;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :twoover
        *** :name "2OVER"}
         */
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

        /* -------------------------------------------------------------
        ***{:token :twoslash
        *** :name "2/"
        *** :args [[:x1] [:x2]]}
         */
        TWOSLASH:
        {
            CHECK_STACK(1, 1);
            tos.i = tos.i >> 1;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :twostar
        *** :name "2*"
        *** :args [[:x1] [:x2]]}
         */
        TWOSTAR:
        {
            CHECK_STACK(1, 1);
            tos.u = tos.u << 1;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :twostore
        *** :name "2!"}
         */
        TWOSTORE:
        {
            CHECK_STACK(3, 0);
            EnforthCell * addr = (EnforthCell*)tos.ram;
            *addr++ = *restDataStack++;
            *addr = *restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :twoswap
        *** :name "2SWAP"}
         */
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
        ***{:token :ulessthan
        *** :name "U<"}
         */
        ULESSTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->u < tos.u ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
         * UM/MOD [CORE] 6.1.2370 "u-m-slash-mod" ( ud u1 -- u2 u3 )
         *
         * Divide ud by u1, giving the quotient u3 and the remainder u2.
         * All values and arithmetic are unsigned.  An ambiguous
         * condition exists if u1 is zero or if the quotient lies
         * outside the range of a single-cell unsigned integer.
         *
        ***{:token :umslashmod
        *** :name "UM/MOD"
        *** :args [[:ud :u1] [:u2 :u3]]}
         */
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
         * UM* [CORE] 6.1.2360 "u-m-star" ( u1 u2 -- ud )
         *
         * Multiply u1 by u2, giving the unsigned double-cell product
         * ud.  All values and arithmetic are unsigned
         *
        ***{:token :umstar
        *** :name "UM*"}
         */
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
        ***{:token :unloop}
         */
        UNLOOP:
        {
            CHECK_STACK(0, 0);
            returnTop++;
            returnTop++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :xor}
         */
        XOR:
        {
            CHECK_STACK(2, 1);
            tos.i ^= restDataStack++->i;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :zeroequals
        *** :name "0="}
         */
        ZEROEQUALS:
        {
            CHECK_STACK(1, 1);
            tos.i = tos.i == 0 ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
         * 0< [CORE] 6.1.0250 "zero-less" ( b -- flag )
         *
         * flag is true if and only if n is less than zero.
         *
        ***{:token :zeroless
        *** :name "0<"}
         */
        ZEROLESS:
        {
            CHECK_STACK(1, 0);
            tos.i = tos.i < 0 ? -1 : 0;
        }
        continue;



        /* =============================================================
         * CORE-EXT PRIMITIVES
         */

        /* -------------------------------------------------------------
        ***{:token :nip}
         */
        NIP:
        {
            CHECK_STACK(2, 1);
            restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :notequals
        *** :name "<>"}
         */
        NOTEQUALS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i != tos.i ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :true}
         */
        TRUE:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = -1;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :tuck}
         */
        TUCK:
        {
            EnforthCell second = *restDataStack;
            *restDataStack = tos;
            *--restDataStack = second;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :tworfetch
        *** :name "2R@"}
         */
        TWORFETCH:
        {
            CHECK_STACK(0, 2);
            *--restDataStack = tos;
            tos = returnTop[0];
            *--restDataStack = returnTop[1];
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :tworfrom
        *** :name "2R>"}
         */
        TWORFROM:
        {
            CHECK_STACK(0, 2);
            *--restDataStack = tos;
            tos = *returnTop++;
            *--restDataStack = *returnTop++;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :ugreaterthan
        *** :name "U>"}
         */
        UGREATERTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->u > tos.u ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :false}
         */
        FALSE:

        /* -------------------------------------------------------------
        ***{:token :zero
        *** :name "0"}
         */
        ZERO:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = 0;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :zeronotequals
        *** :name "0<>"
        *** :args [[:x] [:f]]}
         */
        ZERONOTEQUALS:
        {
            CHECK_STACK(1, 1);
            tos.i = tos.i != 0 ? -1 : 0;
        }
        continue;



        /* =============================================================
         * DOUBLE PRIMITIVES
         */

        /* -------------------------------------------------------------
         * M+ [DOUBLE] 8.6.1.1830 "m-plus" ( d1|ud1 n -- d2|ud2 )
         *
         * Add n to d1|ud1, giving the sum d2|ud2.
         *
        ***{:token :mplus
        *** :name "M+"}
         */
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



        /* =============================================================
         * FACILITY PRIMITIVES
         */

        /* -------------------------------------------------------------
        ***{:token :keyq
        *** :name "KEY?"}
         */
        KEYQ:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = vm->keyq();
        }
        continue;



        /* =============================================================
         * ENFORTH PRIMITIVES
         */

        /* -------------------------------------------------------------
        ***{:token :initrp
        *** :flags #{:headerless}}
         */
        INITRP:
        {
            CHECK_STACK(0, 0);
            returnTop = (EnforthCell*)(vm->cur_task.ram + kTaskStartToReturnTop);
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :load}
         */
        LOAD:
        {
            CHECK_STACK(0, 1);

            *--restDataStack = tos;

            if (vm->load != NULL)
            {
                tos.i = vm->load(vm->dictionary.ram, vm->dictionary_size.u);
            }
            else
            {
                tos.i = 0;
            }
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :pkey
        *** :name "(KEY)"
        *** :flags #{:headerless}}
         */
        PKEY:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = vm->key();
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :save}
         */
        SAVE:
        {
            CHECK_STACK(0, 1);

            *--restDataStack = tos;

            if (vm->save != NULL)
            {
                tos.i = vm->save(vm->dictionary.ram, vm->dictionary_size.u);
            }
            else
            {
                tos.i = 0;
            }
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :tickromdef
        *** :name "'ROMDEF"
        *** :args [[] [:c-addr]]
        *** :flags #{:headerless}}
         */
        TICKROMDEF:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.ram = (uint8_t*)definitions;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :twonip
        *** :name "2NIP"
        *** :args [[:x1 :x2 :x3 :x4] [:x3 :x4]]}
         */
        /* TODO Can probably remove this since it is only used in one
         * place (and can just be replaced by 2SWAP 2DROP). */
        TWONIP:
        {
            CHECK_STACK(4, 2);
            EnforthCell x3 = *restDataStack++;
            *++restDataStack = x3;
        }
        continue;

        /* -------------------------------------------------------------
        ***{:token :vm
        *** :args [[] [:addr]]
        *** :flags #{:headerless}}
         */
        VM:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.ram = (uint8_t*)vm;
        }
        continue;
    }
}

void enforth_go(EnforthVM * const vm)
{
    /* Clear both stacks. */
    EnforthCell * sp = (EnforthCell*)(vm->cur_task.ram + kTaskStartToDataTop - kEnforthCellSize);
    EnforthCell * rsp = (EnforthCell*)(vm->cur_task.ram + kTaskStartToReturnTop - kEnforthCellSize);

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
    /* TODO This should be dictionary-relative so that it can relocate
     * with the dictionary. */
    ((EnforthCell*)vm->cur_task.ram)[1].ram = (uint8_t*)sp;

    /* Resume the interpreter. */
    enforth_resume(vm);
}
