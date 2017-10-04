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
