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

/* Catch includes. */
#include "catch.hpp"



/* -------------------------------------
 * Harness-provided types and functions.
 */

typedef void EnforthVM;
extern "C" EnforthVM * const get_test_vm();
extern "C" void enforth_evaluate(EnforthVM * const vm, const char * const text);
extern "C" bool enforth_test(EnforthVM * const vm, const char * const text);



/* -------------------------------------
 * Additional (Enforth) tests.
 */

TEST_CASE( "Additional +LOOP Tests" ) {
    EnforthVM * const vm = get_test_vm();
    enforth_evaluate(vm, "TESTING +LOOP (Enforth)");

    /* The basic core tests do not confirm that +LOOP behaves correctly
     * if a positive increment lands right on the loop terminator. */
    REQUIRE( enforth_test(vm, "T{ : GD2 DO I 1 +LOOP ; -> }T") );
    REQUIRE( enforth_test(vm, "T{ 4 0 GD2 -> 0 1 2 3 }T") );
}

TEST_CASE( "FFI Tests" ) {
    EnforthVM * const vm = get_test_vm();
    enforth_evaluate(vm, "TESTING FFI (Enforth)");

    REQUIRE( enforth_test(vm, "T{ USE: twoseven -> }T") );
    REQUIRE( enforth_test(vm, "T{ USE: dubnum -> }T") );

    REQUIRE( enforth_test(vm, "T{ twoseven -> 1B }T") );
    REQUIRE( enforth_test(vm, "T{ twoseven dubnum -> 36 }T") );
}
