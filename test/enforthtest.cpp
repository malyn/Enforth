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

/* Catch includes. */
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

/* Enforth includes. */
#include "enforth.h"



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
 * Helper functions.
 */

static void compile_tester(EnforthVM * const vm)
{
    /* Compile tester.fr, copyrighted as follows:
     *
     * (C) 1995 JOHNS HOPKINS UNIVERSITY / APPLIED PHYSICS LABORATORY
     * MAY BE DISTRIBUTED FREELY AS LONG AS THIS COPYRIGHT NOTICE
     * REMAINS. */
    enforth_evaluate(vm, "HEX");

    /* SET THE FOLLOWING FLAG TO TRUE FOR MORE VERBOSE OUTPUT; THIS MAY
     * ALLOW YOU TO TELL WHICH TEST CAUSED YOUR SYSTEM TO HANG. */
    enforth_evaluate(vm, "VARIABLE VERBOSE");
    enforth_evaluate(vm, "    FALSE VERBOSE !");
    /* enforth_evaluate(vm, "    TRUE VERBOSE !"); */

    /* EMPTY STACK: HANDLES UNDERFLOWED STACK TOO. */
    enforth_evaluate(vm, ": EMPTY-STACK");
    enforth_evaluate(vm, "   DEPTH ?DUP IF DUP 0< IF NEGATE 0 DO 0 LOOP ELSE 0 DO DROP LOOP THEN THEN ;");

    /* DISPLAY AN ERROR MESSAGE FOLLOWED BY THE LINE THAT HAD THE ERROR. */
    enforth_evaluate(vm, ": ERROR");
    enforth_evaluate(vm, "   TYPE SOURCE TYPE CR");
    enforth_evaluate(vm, "   EMPTY-STACK");
    /* Comment out the following line to continue after an error */
    enforth_evaluate(vm, "   QUIT");
    enforth_evaluate(vm, ";");

    /* STACK RECORD */
    enforth_evaluate(vm, "VARIABLE ACTUAL-DEPTH");
    enforth_evaluate(vm, "CREATE ACTUAL-RESULTS 20 CELLS ALLOT");

    /* SYNTACTIC SUGAR. */
    enforth_evaluate(vm, ": T{ ;");

    /* RECORD DEPTH AND CONTENT OF STACK. */
    enforth_evaluate(vm, ": ->");
    enforth_evaluate(vm, "   DEPTH DUP ACTUAL-DEPTH !");
    enforth_evaluate(vm, "   ?DUP IF");
    enforth_evaluate(vm, "      0 DO ACTUAL-RESULTS I CELLS + ! LOOP");
    enforth_evaluate(vm, "   THEN ;");

    /* COMPARE STACK (EXPECTED) CONTENTS WITH SAVED (ACTUAL) CONTENTS. */
    enforth_evaluate(vm, ": }T");
    enforth_evaluate(vm, "   DEPTH ACTUAL-DEPTH @ = IF");
    enforth_evaluate(vm, "      DEPTH ?DUP IF");
    enforth_evaluate(vm, "         0  DO");
    enforth_evaluate(vm, "            ACTUAL-RESULTS I CELLS + @");
    enforth_evaluate(vm, "            <> IF S\" INCORRECT RESULT: \" ERROR LEAVE THEN");
    enforth_evaluate(vm, "         LOOP");
    enforth_evaluate(vm, "      THEN");
    enforth_evaluate(vm, "   ELSE");
    enforth_evaluate(vm, "      S\" WRONG NUMBER OF RESULTS: \" ERROR");
    enforth_evaluate(vm, "   THEN ;");

    /* TALKING COMMENT. */
    enforth_evaluate(vm, ": TESTING");
    enforth_evaluate(vm, "  SOURCE VERBOSE @");
    enforth_evaluate(vm, "   IF DUP >R TYPE CR R> >IN !");
    enforth_evaluate(vm, "   ELSE >IN ! DROP [CHAR] * EMIT");
    enforth_evaluate(vm, "   THEN ;");
}

static EnforthVM * const get_test_vm()
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

    /* Return the VM. */
    return &enforthVM;
}

static EnforthCell enforth_pop(EnforthVM * const vm)
{
    return vm->data_stack[31 - vm->saved_sp.u--];
}



/* -------------------------------------
 * Basic tests.
 */

TEST_CASE( "DUP works", "[enforth]" ) {
    /* Get the test VM. */
    EnforthVM * const vm = get_test_vm();

    /* Evaluate DUP. */
    enforth_evaluate(vm, "DECIMAL");
    enforth_evaluate(vm, "27 DUP");

    /* Pop IP and RSP so that we can look at the stack itself. */
    enforth_pop(vm); /* IP */
    enforth_pop(vm); /* RSP */

    /* Check the stack. */
    REQUIRE( vm->saved_sp.u == 2 );
    REQUIRE( enforth_pop(vm).u == 27 );
    REQUIRE( enforth_pop(vm).u == 27 );
}

TEST_CASE( "TESTING CORE WORDS" ) {
    /* Get the test VM. */
    EnforthVM * const vm = get_test_vm();

    /* Configure the environment. */
    enforth_evaluate(vm, "CR");
    enforth_evaluate(vm, "HEX");

    SECTION( "TESTING BASIC ASSUMPTIONS" ) {
        /* START WITH CLEAN SLATE */
        enforth_evaluate(vm, "T{ -> }T");

        /* TEST IF ANY BITS ARE SET; ANSWER IN BASE 1 */
        enforth_evaluate(vm, "T{ : BITSSET? IF 0 0 ELSE 0 THEN ; -> }T");

        /* ZERO IS ALL BITS CLEAR */
        enforth_evaluate(vm, "T{  0 BITSSET? -> 0 }T");

        /* OTHER NUMBERS HAVE AT LEAST ONE BIT */
        enforth_evaluate(vm, "T{  1 BITSSET? -> 0 0 }T");
        enforth_evaluate(vm, "T{ -1 BITSSET? -> 0 0 }T");
    }

    SECTION( "TESTING BOOLEANS: INVERT AND OR XOR" ) {
        enforth_evaluate(vm, "T{ 0 0 AND -> 0 }T");
        enforth_evaluate(vm, "T{ 0 1 AND -> 0 }T");
        enforth_evaluate(vm, "T{ 1 0 AND -> 0 }T");
        enforth_evaluate(vm, "T{ 1 1 AND -> 1 }T");

        enforth_evaluate(vm, "T{ 0 INVERT 1 AND -> 1 }T");
        enforth_evaluate(vm, "T{ 1 INVERT 1 AND -> 0 }T");

        enforth_evaluate(vm, "0        CONSTANT 0S");
        enforth_evaluate(vm, "0 INVERT CONSTANT 1S");

        enforth_evaluate(vm, "T{ 0S INVERT -> 1S }T");
        enforth_evaluate(vm, "T{ 1S INVERT -> 0S }T");

        enforth_evaluate(vm, "T{ 0S 0S AND -> 0S }T");
        enforth_evaluate(vm, "T{ 0S 1S AND -> 0S }T");
        enforth_evaluate(vm, "T{ 1S 0S AND -> 0S }T");
        enforth_evaluate(vm, "T{ 1S 1S AND -> 1S }T");

        enforth_evaluate(vm, "T{ 0S 0S OR -> 0S }T");
        enforth_evaluate(vm, "T{ 0S 1S OR -> 1S }T");
        enforth_evaluate(vm, "T{ 1S 0S OR -> 1S }T");
        enforth_evaluate(vm, "T{ 1S 1S OR -> 1S }T");

        enforth_evaluate(vm, "T{ 0S 0S XOR -> 0S }T");
        enforth_evaluate(vm, "T{ 0S 1S XOR -> 1S }T");
        enforth_evaluate(vm, "T{ 1S 0S XOR -> 1S }T");
        enforth_evaluate(vm, "T{ 1S 1S XOR -> 0S }T");
    }
}
