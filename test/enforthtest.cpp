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
    memset(&enforthVM, 0, sizeof(enforthVM));
    memset(&enforthDict, 0, sizeof(enforthDict));

    /* Initialize Enforth. */
    enforth_init(
            &enforthVM,
            enforthDict, sizeof(enforthDict),
            LAST_FFI,
            enforthSimpleKeyQuestion, enforthSimpleKey, enforthSimpleEmit,
            NULL, NULL);

    /* Compile the tester words. */
    compile_tester(&enforthVM);

    /* Enable verbose mode. */
    enforth_evaluate(&enforthVM, "TRUE VERBOSE !");

    /* Return the VM. */
    return &enforthVM;
}

static EnforthCell enforth_push(EnforthVM * const vm, const EnforthCell cell)
{
    EnforthCell * saved_sp = (EnforthCell*)(vm->cur_task.ram + kEnforthCellSize);
    EnforthCell * sp = (EnforthCell*)(saved_sp->ram);

    *(--sp) = cell;

    saved_sp->ram = (uint8_t*)sp;
}

static EnforthCell enforth_pop(EnforthVM * const vm)
{
    EnforthCell * saved_sp = (EnforthCell*)(vm->cur_task.ram + kEnforthCellSize);
    EnforthCell * sp = (EnforthCell*)(saved_sp->ram);

    EnforthCell tos = *sp++;

    saved_sp->ram = (uint8_t*)sp;

    return tos;
}

bool enforth_test(EnforthVM * const vm, const char * const text)
{
    /* Run the test. */
    enforth_evaluate(vm, text);

    /* Check the stack. */
    EnforthCell * saved_sp = (EnforthCell*)(vm->cur_task.ram + kEnforthCellSize);
    EnforthCell * sp = (EnforthCell*)(saved_sp->ram);
    int depth = (EnforthCell*)(vm->cur_task.ram + 256) - sp;
    if (depth < 3)
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
