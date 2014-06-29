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

#ifndef ENFORTH_H_
#define ENFORTH_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------
 * Includes.
 */

/* ANSI C includes. */
#include <stdlib.h>
#include <inttypes.h>

/* AVR includes. */
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM
#endif



/* -------------------------------------
 * Basic types.
 */

#ifdef __AVR__
typedef int16_t EnforthInt;
typedef uint16_t EnforthUnsigned;
#else
typedef int32_t EnforthInt;
typedef uint32_t EnforthUnsigned;
#endif

/* Execution Tokens (XTs) are always 16-bits, even on 32-bit processors
 * or processors with more than 16 bits of program space.  Enforth
 * handles this constraint by ensuring that all XTs are relative to the
 * start of the dictionary. */
typedef uint16_t EnforthXT;

typedef union
{
    EnforthInt i;
    EnforthUnsigned u;

    /* Pointer to RAM.  Why not just a generic pointer?  Because Enforth
     * works on processors that have more program space than can be
     * referenced by a cell-sized pointer.  The name of this field is
     * designed to make it clear that cells can only reference addresses
     * in RAM. */
    uint8_t * ram;
} EnforthCell;

#define kEnforthCellSize (sizeof(EnforthCell))



/* -------------------------------------
 * Foreign-function-interface types.
 */

typedef EnforthCell (*ZeroArgFFI)(
        void);
typedef EnforthCell (*OneArgFFI)(
        EnforthCell a);
typedef EnforthCell (*TwoArgFFI)(
        EnforthCell a, EnforthCell b);
typedef EnforthCell (*ThreeArgFFI)(
        EnforthCell a, EnforthCell b, EnforthCell c);
typedef EnforthCell (*FourArgFFI)(
        EnforthCell a, EnforthCell b, EnforthCell c, EnforthCell d);
typedef EnforthCell (*FiveArgFFI)(
        EnforthCell a, EnforthCell b, EnforthCell c, EnforthCell d,
        EnforthCell e);
typedef EnforthCell (*SixArgFFI)(
        EnforthCell a, EnforthCell b, EnforthCell c, EnforthCell d,
        EnforthCell e, EnforthCell f);
typedef EnforthCell (*SevenArgFFI)(
        EnforthCell a, EnforthCell b, EnforthCell c, EnforthCell d,
        EnforthCell e, EnforthCell f, EnforthCell g);
typedef EnforthCell (*EightArgFFI)(
        EnforthCell a, EnforthCell b, EnforthCell c, EnforthCell d,
        EnforthCell e, EnforthCell f, EnforthCell g, EnforthCell h);

typedef struct EnforthFFIDef
{
    const struct EnforthFFIDef * const prev;
    const char * const name;
    uint8_t arity;
    void * fn;
} EnforthFFIDef;

static const int kEnforthFFIProcPtrSize = sizeof(void*);

/* FFI Macros */
#define LAST_FFI NULL

/* NOTE that ENFORTH_EXTERN needs to be in near (<64KB) program memory
 * on large AVR processors since we access the definition using 16-bit
 * references in Forth.  The code itself can (and probably should?) be
 * located in extended program memory though since the function pointer
 * is only ever used in C and thus can use the full address space
 * resolution of the processor. */
#define ENFORTH_EXTERN(name, fn, arity) \
    static const char FFIDEF_ ## name ## _NAME[] PROGMEM = #name; \
    static const EnforthFFIDef FFIDEF_##name PROGMEM = { LAST_FFI, FFIDEF_ ## name ## _NAME, arity, (void*)fn };

#define GET_LAST_FFI(name) &FFIDEF_ ## name



/* -------------------------------------
 * Enforth Virtual Machine type.
 */

typedef struct
{
    /* VM constants */
    const EnforthFFIDef * last_ffi;

    int (*keyq)(void);
    char (*key)(void);
    void (*emit)(char);

    EnforthCell dictionary;
    EnforthCell dictionary_size;

    /* Dictionary vars */
    EnforthCell dp;
    EnforthCell latest; /* XT of the latest word; 0 means empty dictionary */

    /* Transient (dictionary-based) vars */
    uint8_t * hld;

    /* Text Interpreter vars */
    EnforthInt state;

    /* TODO Put TIBSIZE in a constant. */
    uint8_t tib[80];
    EnforthCell source_len;
    EnforthCell source;
    EnforthInt to_in;

    uint8_t * prev_leave;

    /* Task vars */
    EnforthCell saved_sp; /* Index of the SP from the base of the stack */
    EnforthUnsigned base;
    EnforthCell data_stack[32];
    EnforthCell return_stack[32];
} EnforthVM;



/* -------------------------------------
 * Functions definitions.
 */

void enforth_init(
        EnforthVM * const vm,
        uint8_t * const dictionary, int dictionary_size,
        const EnforthFFIDef * const last_ffi,
        /* TODO Remove this and just have it be in the FFI (which we
         * look up in the constructor and then stash into our private
         * function pointers). */
        int (*keyq)(void), char (*key)(void), void (*emit)(char));

void enforth_reset(
        EnforthVM * const vm);

void enforth_evaluate(
        EnforthVM * const vm, const char * const text);

void enforth_resume(
        EnforthVM * const vm);

void enforth_go(
        EnforthVM * const vm);

#ifdef __cplusplus
}
#endif

#endif /* ENFORTH_H_ */
