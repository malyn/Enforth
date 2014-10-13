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

/* Enforth includes. */
#include "enforth.h"

/* Test includes. */
#include "enforthtesthelper.h"

/* Catch includes. */
#define CATCH_CONFIG_MAIN
#include "catch.hpp"



/* -------------------------------------
 * Harness-provided functions.
 */

extern "C" EnforthVM * const get_test_vm();
extern "C" void enforth_evaluate(EnforthVM * const vm, const char * const text);
extern "C" bool enforth_test(EnforthVM * const vm, const char * const text);



/* -------------------------------------
 * FFI definitions for tests.
 */

static int twoSeven()
{
    return 27;
}

static int doubleNumber(int num)
{
    return num + num;
}

/* Externs */
ENFORTH_EXTERN(twoseven, twoSeven, 0)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(twoseven)

ENFORTH_EXTERN(dubnum, doubleNumber, 1)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(dubnum)



/* -------------------------------------
 * Enforth I/O primitives.
 */

static int enforthSimpleKeyQuestion(void)
{
    return -1;
}

static char enforthSimpleKey(void)
{
    return getchar();
}

static void enforthSimpleEmit(char ch)
{
    putchar(ch);
}



/* -------------------------------------
 * Test harness functions.
 */

EnforthVM * const get_test_vm()
{
    /* Globals. */
    static EnforthVM enforthVM;
    static unsigned char enforthDict[8192];


    /* Clear out the dictionary and VM structure. */
    memset(&enforthVM, sizeof(enforthVM), 0);
    memset(&enforthDict, sizeof(enforthDict), 0);

    /* Initialize Enforth. */
    enforth_init(
            &enforthVM,
            enforthDict, sizeof(enforthDict),
            LAST_FFI,
            enforthSimpleKeyQuestion, enforthSimpleKey, enforthSimpleEmit);

    /* Compile the tester words. */
    compile_tester(&enforthVM);

    /* Enable verbose mode. */
    enforth_evaluate(&enforthVM, "TRUE VERBOSE !");

    /* Return the VM. */
    return &enforthVM;
}

static EnforthCell enforth_push(EnforthVM * const vm, const EnforthCell cell)
{
    vm->data_stack[31 - vm->saved_sp.u++] = cell;
}

static EnforthCell enforth_pop(EnforthVM * const vm)
{
    return vm->data_stack[31 - vm->saved_sp.u--];
}

bool enforth_test(EnforthVM * const vm, const char * const text)
{
    /* Run the test. */
    enforth_evaluate(vm, text);

    /* Check the stack. */
    if (vm->saved_sp.u < 3)
    {
        return false;
    }

    /* Pop IP and RSP so that we can look at the stack itself. */
    EnforthCell ip = enforth_pop(vm);
    EnforthCell rsp = enforth_pop(vm);

    /* Pop the test result flag. */
    bool success = enforth_pop(vm).u == -1;

    /* Push RSP and IP back onto the stack. */
    enforth_push(vm, rsp);
    enforth_push(vm, ip);

    /* Return the test result flag. */
    return success;
}
