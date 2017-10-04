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

#ifndef ENFORTH_H_
#define ENFORTH_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------
 * Includes.
 */

/* ANSI C includes. */
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
    uint8_t is_void;
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
    static const EnforthFFIDef FFIDEF_##name PROGMEM = { LAST_FFI, FFIDEF_ ## name ## _NAME, arity, 0, (void*)fn };

#define ENFORTH_EXTERN_VOID(name, fn, arity) \
    static const char FFIDEF_ ## name ## _NAME[] PROGMEM = #name; \
    static const EnforthFFIDef FFIDEF_##name PROGMEM = { LAST_FFI, FFIDEF_ ## name ## _NAME, arity, 1, (void*)fn };

#define ENFORTH_EXTERN_METHOD(name, fnbody, arity) \
    static void * FFIMETHODCALL_ ## name (EnforthCell a, EnforthCell b, EnforthCell c) fnbody \
    static const char FFIDEF_ ## name ## _NAME[] PROGMEM = #name; \
    static const EnforthFFIDef FFIDEF_##name PROGMEM = { LAST_FFI, FFIDEF_ ## name ## _NAME, arity, 0, (void*)FFIMETHODCALL_ ## name };

#define ENFORTH_EXTERN_VOID_METHOD(name, fnbody, arity) \
    static void * FFIMETHODCALL_ ## name (EnforthCell a, EnforthCell b, EnforthCell c) fnbody \
    static const char FFIDEF_ ## name ## _NAME[] PROGMEM = #name; \
    static const EnforthFFIDef FFIDEF_##name PROGMEM = { LAST_FFI, FFIDEF_ ## name ## _NAME, arity, 1, (void*)FFIMETHODCALL_ ## name };

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
    int (*load)(uint8_t*, int);
    int (*save)(uint8_t*, int);

    EnforthCell dictionary;
    EnforthCell dictionary_size;

    /* Transient (dictionary-based) vars */
    uint8_t * hld;

    /* Text Interpreter vars */
    EnforthInt state;
    EnforthCell prev_leave;

    /* TODO Put TIBSIZE in a constant. */
    uint8_t tib[80];
    EnforthCell source_len;
    EnforthCell source;
    EnforthInt to_in;

    EnforthCell cur_task;
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
        int (*keyq)(void), char (*key)(void), void (*emit)(char),
        int (*load)(uint8_t*, int), int (*save)(uint8_t*, int));

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
