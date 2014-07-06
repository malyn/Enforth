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
 * CORE tests.
 */

TEST_CASE( "TESTING CORE WORDS" ) {
    /* Get the test VM. */
    EnforthVM * const vm = get_test_vm();

    /* Configure the environment. */
    enforth_evaluate(vm, "HEX");

    SECTION( "TESTING BASIC ASSUMPTIONS" ) {
        /* START WITH CLEAN SLATE */
        REQUIRE( enforth_test(vm, "T{ -> }T") );

        /* TEST IF ANY BITS ARE SET; ANSWER IN BASE 1 */
        REQUIRE( enforth_test(vm, "T{ : BITSSET? IF 0 0 ELSE 0 THEN ; -> }T") );

        /* ZERO IS ALL BITS CLEAR */
        REQUIRE( enforth_test(vm, "T{  0 BITSSET? -> 0 }T") );

        /* OTHER NUMBERS HAVE AT LEAST ONE BIT */
        REQUIRE( enforth_test(vm, "T{  1 BITSSET? -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 BITSSET? -> 0 0 }T") );
    }

    SECTION( "TESTING BOOLEANS: INVERT AND OR XOR" ) {
        REQUIRE( enforth_test(vm, "T{ 0 0 AND -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 1 AND -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 AND -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 AND -> 1 }T") );

        REQUIRE( enforth_test(vm, "T{ 0 INVERT 1 AND -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 INVERT 1 AND -> 0 }T") );

             enforth_evaluate(vm, "0        CONSTANT 0S");
             enforth_evaluate(vm, "0 INVERT CONSTANT 1S");

        REQUIRE( enforth_test(vm, "T{ 0S INVERT -> 1S }T") );
        REQUIRE( enforth_test(vm, "T{ 1S INVERT -> 0S }T") );

        REQUIRE( enforth_test(vm, "T{ 0S 0S AND -> 0S }T") );
        REQUIRE( enforth_test(vm, "T{ 0S 1S AND -> 0S }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 0S AND -> 0S }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 1S AND -> 1S }T") );

        REQUIRE( enforth_test(vm, "T{ 0S 0S OR -> 0S }T") );
        REQUIRE( enforth_test(vm, "T{ 0S 1S OR -> 1S }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 0S OR -> 1S }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 1S OR -> 1S }T") );

        REQUIRE( enforth_test(vm, "T{ 0S 0S XOR -> 0S }T") );
        REQUIRE( enforth_test(vm, "T{ 0S 1S XOR -> 1S }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 0S XOR -> 1S }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 1S XOR -> 0S }T") );
    }

    SECTION( "TESTING 2* 2/ LSHIFT RSHIFT ") {
        /* From previous sections. */
             enforth_evaluate(vm, "0        CONSTANT 0S");
             enforth_evaluate(vm, "0 INVERT CONSTANT 1S");
             enforth_evaluate(vm, ": BITSSET? IF 0 0 ELSE 0 THEN ;");

        /* WE TRUST 1S, INVERT, AND BITSSET?; WE WILL CONFIRM RSHIFT
         * LATER */
             enforth_evaluate(vm, "1S 1 RSHIFT INVERT CONSTANT MSB");
        REQUIRE( enforth_test(vm, "T{ MSB BITSSET? -> 0 0 }T") );

        REQUIRE( enforth_test(vm, "T{ 0S 2* -> 0S }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2* -> 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 4000 2* -> 8000 }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 2* 1 XOR -> 1S }T") );
        REQUIRE( enforth_test(vm, "T{ MSB 2* -> 0S }T") );

        REQUIRE( enforth_test(vm, "T{ 0S 2/ -> 0S }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2/ -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 4000 2/ -> 2000 }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 2/ -> 1S }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 1 XOR 2/ -> 1S }T") );
        REQUIRE( enforth_test(vm, "T{ MSB 2/ MSB AND -> MSB }T") );

        REQUIRE( enforth_test(vm, "T{ 1 0 LSHIFT -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 LSHIFT -> 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 LSHIFT -> 4 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 F LSHIFT -> 8000 }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 1 LSHIFT 1 XOR -> 1S }T") );
        REQUIRE( enforth_test(vm, "T{ MSB 1 LSHIFT -> 0 }T") );

        REQUIRE( enforth_test(vm, "T{ 1 0 RSHIFT -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 RSHIFT -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 RSHIFT -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 4 2 RSHIFT -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 8000 F RSHIFT -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ MSB 1 RSHIFT MSB AND -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MSB 1 RSHIFT 2* -> MSB }T") );
    }

    SECTION( "TESTING COMPARISONS: 0= = 0< < > U< MIN MAX" ) {
        // TODO Consider defining all of these at the test case level
        // since they are used everywhere and just waste dictionary
        // space (which is at a premium on the Arduino).
             enforth_evaluate(vm, "0        CONSTANT 0S");
             enforth_evaluate(vm, "0 INVERT CONSTANT 1S");

             enforth_evaluate(vm, "0 INVERT                 CONSTANT MAX-UINT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT        CONSTANT MAX-INT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT INVERT CONSTANT MIN-INT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT        CONSTANT MID-UINT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT INVERT CONSTANT MID-UINT+1");

             enforth_evaluate(vm, "0S CONSTANT <FALSE>");
             enforth_evaluate(vm, "1S CONSTANT <TRUE>");

        REQUIRE( enforth_test(vm, "T{ 0 0= -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0= -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 2 0= -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ -1 0= -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT 0= -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 0= -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 0= -> <FALSE> }T") );

        REQUIRE( enforth_test(vm, "T{ 0 0 = -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 = -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ -1 -1 = -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 = -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ -1 0 = -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 1 = -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -1 = -> <FALSE> }T") );

        REQUIRE( enforth_test(vm, "T{ 0 0< -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ -1 0< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 0< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0< -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 0< -> <FALSE> }T") );

        REQUIRE( enforth_test(vm, "T{ 0 1 < -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 < -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ -1 0 < -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1 < -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 0 < -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MAX-INT < -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MAX-INT < -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -1 < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 -1 < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MIN-INT < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MIN-INT < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 0 < -> <FALSE> }T") );

        REQUIRE( enforth_test(vm, "T{ 0 1 > -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 > -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ -1 0 > -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1 > -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 0 > -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MAX-INT > -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MAX-INT > -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 > -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 > -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 > -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 > -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -1 > -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 -1 > -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MIN-INT > -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MIN-INT > -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 0 > -> <TRUE> }T") );

        REQUIRE( enforth_test(vm, "T{ 0 1 U< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 U< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MID-UINT U< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MAX-UINT U< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT MAX-UINT U< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 U< -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 U< -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 U< -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 U< -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT 0 U< -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT 0 U< -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT MID-UINT U< -> <FALSE> }T") );

        REQUIRE( enforth_test(vm, "T{ 0 1 MIN -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 MIN -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 0 MIN -> -1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1 MIN -> -1 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 0 MIN -> MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MAX-INT MIN -> MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MAX-INT MIN -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 MIN -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 MIN -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 MIN -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 MIN -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -1 MIN -> -1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 -1 MIN -> -1 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MIN-INT MIN -> MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MIN-INT MIN -> MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 0 MIN -> 0 }T") );

        REQUIRE( enforth_test(vm, "T{ 0 1 MAX -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 MAX -> 2 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 0 MAX -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1 MAX -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 0 MAX -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MAX-INT MAX -> MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MAX-INT MAX -> MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 MAX -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 MAX -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 MAX -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 MAX -> 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -1 MAX -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 -1 MAX -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MIN-INT MAX -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MIN-INT MAX -> MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 0 MAX -> MAX-INT }T") );
    }

    SECTION( "TESTING STACK OPS: 2DROP 2DUP 2OVER 2SWAP ?DUP DEPTH DROP DUP OVER ROT SWAP" ) {
        REQUIRE( enforth_test(vm, "T{ 1 2 2DROP -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 2DUP -> 1 2 1 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 3 4 2OVER -> 1 2 3 4 1 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 3 4 2SWAP -> 3 4 1 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 ?DUP -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 ?DUP -> 1 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 ?DUP -> -1 -1 }T") );
        REQUIRE( enforth_test(vm, "T{ DEPTH -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 DEPTH -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 1 DEPTH -> 0 1 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 DROP -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 DROP -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 DUP -> 1 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 OVER -> 1 2 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 3 ROT -> 2 3 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 SWAP -> 2 1 }T") );
    }

    SECTION( "TESTING >R R> R@" ) {
             enforth_evaluate(vm, "0 INVERT CONSTANT 1S");

        REQUIRE( enforth_test(vm, "T{ : GR1 >R R> ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ : GR2 >R R@ R> DROP ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ 123 GR1 -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ 123 GR2 -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ 1S GR1 -> 1S }T") );
    }

    SECTION( "TESTING ADD/SUBTRACT: + - 1+ 1- ABS NEGATE" ) {
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT INVERT CONSTANT MIN-INT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT        CONSTANT MID-UINT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT INVERT CONSTANT MID-UINT+1");

        REQUIRE( enforth_test(vm, "T{ 0 5 + -> 5 }T") );
        REQUIRE( enforth_test(vm, "T{ 5 0 + -> 5 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -5 + -> -5 }T") );
        REQUIRE( enforth_test(vm, "T{ -5 0 + -> -5 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 + -> 3 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 -2 + -> -1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 2 + -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 -2 + -> -3 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1 + -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT 1 + -> MID-UINT+1 }T") );

        REQUIRE( enforth_test(vm, "T{ 0 5 - -> -5 }T") );
        REQUIRE( enforth_test(vm, "T{ 5 0 - -> 5 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -5 - -> 5 }T") );
        REQUIRE( enforth_test(vm, "T{ -5 0 - -> -5 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 - -> -1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 -2 - -> 3 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 2 - -> -3 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 -2 - -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 1 - -> -1 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT+1 1 - -> MID-UINT }T") );

        REQUIRE( enforth_test(vm, "T{ 0 1+ -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1+ -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1+ -> 2 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT 1+ -> MID-UINT+1 }T") );

        REQUIRE( enforth_test(vm, "T{ 2 1- -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1- -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 1- -> -1 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT+1 1- -> MID-UINT }T") );

        REQUIRE( enforth_test(vm, "T{ 0 NEGATE -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 NEGATE -> -1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 NEGATE -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 NEGATE -> -2 }T") );
        REQUIRE( enforth_test(vm, "T{ -2 NEGATE -> 2 }T") );

        REQUIRE( enforth_test(vm, "T{ 0 ABS -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 ABS -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 ABS -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT ABS -> MID-UINT+1 }T") );
    }

    SECTION( "TESTING MULTIPLY: S>D * M* UM*") {
             enforth_evaluate(vm, "0 INVERT CONSTANT 1S");

             enforth_evaluate(vm, "1S 1 RSHIFT INVERT CONSTANT MSB");

             enforth_evaluate(vm, "0 INVERT                 CONSTANT MAX-UINT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT        CONSTANT MAX-INT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT INVERT CONSTANT MIN-INT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT        CONSTANT MID-UINT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT INVERT CONSTANT MID-UINT+1");

        REQUIRE( enforth_test(vm, "T{ 0 S>D -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 S>D -> 1 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 S>D -> 2 0 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 S>D -> -1 -1 }T") );
        REQUIRE( enforth_test(vm, "T{ -2 S>D -> -2 -1 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT S>D -> MIN-INT -1 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT S>D -> MAX-INT 0 }T") );

        REQUIRE( enforth_test(vm, "T{ 0 0 M* -> 0 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 0 1 M* -> 0 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 M* -> 0 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 M* -> 2 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 M* -> 2 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 3 3 M* -> 9 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ -3 3 M* -> -9 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 3 -3 M* -> -9 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ -3 -3 M* -> 9 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MIN-INT M* -> 0 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 1 MIN-INT M* -> MIN-INT S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MIN-INT M* -> 0 1S }T") );
        REQUIRE( enforth_test(vm, "T{ 0 MAX-INT M* -> 0 S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 1 MAX-INT M* -> MAX-INT S>D }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MAX-INT M* -> MAX-INT 1 LSHIFT 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MIN-INT M* -> 0 MSB 1 RSHIFT }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MIN-INT M* -> MSB MSB 2/ }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MAX-INT M* -> 1 MSB 2/ INVERT }T") );

        REQUIRE( enforth_test(vm, "T{ 0 0 * -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 1 * -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 * -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 * -> 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 * -> 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 3 3 * -> 9 }T") );
        REQUIRE( enforth_test(vm, "T{ -3 3 * -> -9 }T") );
        REQUIRE( enforth_test(vm, "T{ 3 -3 * -> -9 }T") );
        REQUIRE( enforth_test(vm, "T{ -3 -3 * -> 9 }T") );

        REQUIRE( enforth_test(vm, "T{ MID-UINT+1 1 RSHIFT 2 * -> MID-UINT+1 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT+1 2 RSHIFT 4 * -> MID-UINT+1 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT+1 1 RSHIFT MID-UINT+1 OR 2 * -> MID-UINT+1 }T") );

        REQUIRE( enforth_test(vm, "T{ 0 0 UM* -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 1 UM* -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 UM* -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 UM* -> 2 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 UM* -> 2 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 3 3 UM* -> 9 0 }T") );

        REQUIRE( enforth_test(vm, "T{ MID-UINT+1 1 RSHIFT 2 UM* -> MID-UINT+1 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT+1 2 UM* -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT+1 4 UM* -> 0 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 2 UM* -> 1S 1 LSHIFT 1 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT MAX-UINT UM* -> 1 1 INVERT }T") );
    }

    SECTION( "TESTING DIVIDE: FM/MOD SM/REM UM/MOD */ */MOD / /MOD MOD" ) {
             enforth_evaluate(vm, "0 INVERT CONSTANT 1S");

             enforth_evaluate(vm, "0 INVERT                 CONSTANT MAX-UINT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT        CONSTANT MAX-INT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT INVERT CONSTANT MIN-INT");

        REQUIRE( enforth_test(vm, "T{ 0 S>D 1 FM/MOD -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 S>D 1 FM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 S>D 1 FM/MOD -> 0 2 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 S>D 1 FM/MOD -> 0 -1 }T") );
        REQUIRE( enforth_test(vm, "T{ -2 S>D 1 FM/MOD -> 0 -2 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 S>D -1 FM/MOD -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 S>D -1 FM/MOD -> 0 -1 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 S>D -1 FM/MOD -> 0 -2 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 S>D -1 FM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -2 S>D -1 FM/MOD -> 0 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 S>D 2 FM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 S>D -1 FM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -2 S>D -2 FM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{  7 S>D  3 FM/MOD -> 1 2 }T") );
        REQUIRE( enforth_test(vm, "T{  7 S>D -3 FM/MOD -> -2 -3 }T") );
        REQUIRE( enforth_test(vm, "T{ -7 S>D  3 FM/MOD -> 2 -3 }T") );
        REQUIRE( enforth_test(vm, "T{ -7 S>D -3 FM/MOD -> -1 2 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT S>D 1 FM/MOD -> 0 MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT S>D 1 FM/MOD -> 0 MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT S>D MAX-INT FM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT S>D MIN-INT FM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 1 4 FM/MOD -> 3 MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 1 MIN-INT M* 1 FM/MOD -> 0 MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 1 MIN-INT M* MIN-INT FM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MIN-INT M* 2 FM/MOD -> 0 MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MIN-INT M* MIN-INT FM/MOD -> 0 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 MAX-INT M* 1 FM/MOD -> 0 MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 1 MAX-INT M* MAX-INT FM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MAX-INT M* 2 FM/MOD -> 0 MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MAX-INT M* MAX-INT FM/MOD -> 0 2 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MIN-INT M* MIN-INT FM/MOD -> 0 MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MAX-INT M* MIN-INT FM/MOD -> 0 MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MAX-INT M* MAX-INT FM/MOD -> 0 MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MAX-INT M* MAX-INT FM/MOD -> 0 MAX-INT }T") );

        REQUIRE( enforth_test(vm, "T{ 0 S>D 1 SM/REM -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 S>D 1 SM/REM -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 S>D 1 SM/REM -> 0 2 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 S>D 1 SM/REM -> 0 -1 }T") );
        REQUIRE( enforth_test(vm, "T{ -2 S>D 1 SM/REM -> 0 -2 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 S>D -1 SM/REM -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 S>D -1 SM/REM -> 0 -1 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 S>D -1 SM/REM -> 0 -2 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 S>D -1 SM/REM -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -2 S>D -1 SM/REM -> 0 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 S>D 2 SM/REM -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 S>D -1 SM/REM -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -2 S>D -2 SM/REM -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{  7 S>D  3 SM/REM -> 1 2 }T") );
        REQUIRE( enforth_test(vm, "T{  7 S>D -3 SM/REM -> 1 -2 }T") );
        REQUIRE( enforth_test(vm, "T{ -7 S>D  3 SM/REM -> -1 -2 }T") );
        REQUIRE( enforth_test(vm, "T{ -7 S>D -3 SM/REM -> -1 2 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT S>D 1 SM/REM -> 0 MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT S>D 1 SM/REM -> 0 MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT S>D MAX-INT SM/REM -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT S>D MIN-INT SM/REM -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 1 4 SM/REM -> 3 MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MIN-INT M* 2 SM/REM -> 0 MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MIN-INT M* MIN-INT SM/REM -> 0 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MAX-INT M* 2 SM/REM -> 0 MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ 2 MAX-INT M* MAX-INT SM/REM -> 0 2 }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MIN-INT M* MIN-INT SM/REM -> 0 MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MAX-INT M* MIN-INT SM/REM -> 0 MAX-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MAX-INT M* MAX-INT SM/REM -> 0 MIN-INT }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MAX-INT M* MAX-INT SM/REM -> 0 MAX-INT }T") );

        REQUIRE( enforth_test(vm, "T{ 0 0 1 UM/MOD -> 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 1 UM/MOD -> 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 2 UM/MOD -> 1 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 3 0 2 UM/MOD -> 1 1 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT 2 UM* 2 UM/MOD -> 0 MAX-UINT }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT 2 UM* MAX-UINT UM/MOD -> 0 2 }T") );
        // TODO Enable this once it works correctly on the Arduino.
        // REQUIRE( enforth_test(vm, "T{ MAX-UINT MAX-UINT UM* MAX-UINT UM/MOD -> 0 MAX-UINT }T") );

             enforth_evaluate(vm, ": IFFLOORED");
             enforth_evaluate(vm, "   [ -3 2 / -2 = INVERT ] LITERAL IF POSTPONE \\ THEN ;");

             enforth_evaluate(vm, ": IFSYM");
             enforth_evaluate(vm, "   [ -3 2 / -1 = INVERT ] LITERAL IF POSTPONE \\ THEN ;");

        /* THE SYSTEM MIGHT DO EITHER FLOORED OR SYMMETRIC DIVISION.
         * SINCE WE HAVE ALREADY TESTED M*, FM/MOD, AND SM/REM WE CAN
         * USE THEM IN TEST. */
             enforth_evaluate(vm, "IFFLOORED : T/MOD  >R S>D R> FM/MOD ;");
             enforth_evaluate(vm, "IFFLOORED : T/     T/MOD SWAP DROP ;");
             enforth_evaluate(vm, "IFFLOORED : TMOD   T/MOD DROP ;");
             enforth_evaluate(vm, "IFFLOORED : T*/MOD >R M* R> FM/MOD ;");
             enforth_evaluate(vm, "IFFLOORED : T*/    T*/MOD SWAP DROP ;");
             enforth_evaluate(vm, "IFSYM     : T/MOD  >R S>D R> SM/REM ;");
             enforth_evaluate(vm, "IFSYM     : T/     T/MOD SWAP DROP ;");
             enforth_evaluate(vm, "IFSYM     : TMOD   T/MOD DROP ;");
             enforth_evaluate(vm, "IFSYM     : T*/MOD >R M* R> SM/REM ;");
             enforth_evaluate(vm, "IFSYM     : T*/    T*/MOD SWAP DROP ;");

        REQUIRE( enforth_test(vm, "T{ 0 1 /MOD -> 0 1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 /MOD -> 1 1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 /MOD -> 2 1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1 /MOD -> -1 1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -2 1 /MOD -> -2 1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -1 /MOD -> 0 -1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 1 -1 /MOD -> 1 -1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 2 -1 /MOD -> 2 -1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -1 -1 /MOD -> -1 -1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -2 -1 /MOD -> -2 -1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 2 2 /MOD -> 2 2 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -1 -1 /MOD -> -1 -1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -2 -2 /MOD -> -2 -2 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 7 3 /MOD -> 7 3 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 7 -3 /MOD -> 7 -3 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -7 3 /MOD -> -7 3 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -7 -3 /MOD -> -7 -3 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 1 /MOD -> MAX-INT 1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 1 /MOD -> MIN-INT 1 T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MAX-INT /MOD -> MAX-INT MAX-INT T/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MIN-INT /MOD -> MIN-INT MIN-INT T/MOD }T") );

        REQUIRE( enforth_test(vm, "T{ 0 1 / -> 0 1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 / -> 1 1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 / -> 2 1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1 / -> -1 1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ -2 1 / -> -2 1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -1 / -> 0 -1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ 1 -1 / -> 1 -1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ 2 -1 / -> 2 -1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ -1 -1 / -> -1 -1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ -2 -1 / -> -2 -1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ 2 2 / -> 2 2 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ -1 -1 / -> -1 -1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ -2 -2 / -> -2 -2 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ 7 3 / -> 7 3 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ 7 -3 / -> 7 -3 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ -7 3 / -> -7 3 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ -7 -3 / -> -7 -3 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 1 / -> MAX-INT 1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 1 / -> MIN-INT 1 T/ }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MAX-INT / -> MAX-INT MAX-INT T/ }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MIN-INT / -> MIN-INT MIN-INT T/ }T") );

        REQUIRE( enforth_test(vm, "T{ 0 1 MOD -> 0 1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1 MOD -> 1 1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 MOD -> 2 1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1 MOD -> -1 1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ -2 1 MOD -> -2 1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ 0 -1 MOD -> 0 -1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ 1 -1 MOD -> 1 -1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ 2 -1 MOD -> 2 -1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ -1 -1 MOD -> -1 -1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ -2 -1 MOD -> -2 -1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ 2 2 MOD -> 2 2 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ -1 -1 MOD -> -1 -1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ -2 -2 MOD -> -2 -2 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ 7 3 MOD -> 7 3 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ 7 -3 MOD -> 7 -3 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ -7 3 MOD -> -7 3 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ -7 -3 MOD -> -7 -3 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 1 MOD -> MAX-INT 1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 1 MOD -> MIN-INT 1 TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT MAX-INT MOD -> MAX-INT MAX-INT TMOD }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT MIN-INT MOD -> MIN-INT MIN-INT TMOD }T") );

        REQUIRE( enforth_test(vm, "T{ 0 2 1 */ -> 0 2 1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 1 */ -> 1 2 1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ 2 2 1 */ -> 2 2 1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ -1 2 1 */ -> -1 2 1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ -2 2 1 */ -> -2 2 1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ 0 2 -1 */ -> 0 2 -1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 -1 */ -> 1 2 -1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ 2 2 -1 */ -> 2 2 -1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ -1 2 -1 */ -> -1 2 -1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ -2 2 -1 */ -> -2 2 -1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ 2 2 2 */ -> 2 2 2 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ -1 2 -1 */ -> -1 2 -1 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ -2 2 -2 */ -> -2 2 -2 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ 7 2 3 */ -> 7 2 3 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ 7 2 -3 */ -> 7 2 -3 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ -7 2 3 */ -> -7 2 3 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ -7 2 -3 */ -> -7 2 -3 T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 2 MAX-INT */ -> MAX-INT 2 MAX-INT T*/ }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 2 MIN-INT */ -> MIN-INT 2 MIN-INT T*/ }T") );

        REQUIRE( enforth_test(vm, "T{ 0 2 1 */MOD -> 0 2 1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 1 */MOD -> 1 2 1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 2 2 1 */MOD -> 2 2 1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -1 2 1 */MOD -> -1 2 1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -2 2 1 */MOD -> -2 2 1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 0 2 -1 */MOD -> 0 2 -1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 1 2 -1 */MOD -> 1 2 -1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 2 2 -1 */MOD -> 2 2 -1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -1 2 -1 */MOD -> -1 2 -1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -2 2 -1 */MOD -> -2 2 -1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 2 2 2 */MOD -> 2 2 2 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -1 2 -1 */MOD -> -1 2 -1 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -2 2 -2 */MOD -> -2 2 -2 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 7 2 3 */MOD -> 7 2 3 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ 7 2 -3 */MOD -> 7 2 -3 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -7 2 3 */MOD -> -7 2 3 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ -7 2 -3 */MOD -> -7 2 -3 T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-INT 2 MAX-INT */MOD -> MAX-INT 2 MAX-INT T*/MOD }T") );
        REQUIRE( enforth_test(vm, "T{ MIN-INT 2 MIN-INT */MOD -> MIN-INT 2 MIN-INT T*/MOD }T") );
    }

    SECTION( "TESTING HERE , @ ! CELL+ CELLS C, C@ C! CHARS 2@ 2! ALIGN ALIGNED +! ALLOT") {
             enforth_evaluate(vm, "0        CONSTANT 0S");
             enforth_evaluate(vm, "0 INVERT CONSTANT 1S");
             enforth_evaluate(vm, "0S CONSTANT <FALSE>");
             enforth_evaluate(vm, "1S CONSTANT <TRUE>");
             enforth_evaluate(vm, "1S 1 RSHIFT INVERT CONSTANT MSB");

             enforth_evaluate(vm, "HERE 1 ALLOT");
             enforth_evaluate(vm, "HERE");
             enforth_evaluate(vm, "CONSTANT 2NDA");
             enforth_evaluate(vm, "CONSTANT 1STA");
        REQUIRE( enforth_test(vm, "T{ 1STA 2NDA U< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1STA 1+ -> 2NDA }T") );
        /* MISSING TEST: NEGATIVE ALLOT */

             enforth_evaluate(vm, "HERE 1 ,");
             enforth_evaluate(vm, "HERE 2 ,");
             enforth_evaluate(vm, "CONSTANT 2ND");
             enforth_evaluate(vm, "CONSTANT 1ST");
        REQUIRE( enforth_test(vm, "T{ 1ST 2ND U< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1ST CELL+ -> 2ND }T") );
        REQUIRE( enforth_test(vm, "T{ 1ST 1 CELLS + -> 2ND }T") );
        REQUIRE( enforth_test(vm, "T{ 1ST @ 2ND @ -> 1 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 5 1ST ! -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1ST @ 2ND @ -> 5 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 6 2ND ! -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1ST @ 2ND @ -> 5 6 }T") );
        REQUIRE( enforth_test(vm, "T{ 1ST 2@ -> 6 5 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 1 1ST 2! -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1ST 2@ -> 2 1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1S 1ST !  1ST @ -> 1S }T") );

             enforth_evaluate(vm, "HERE 1 C,");
             enforth_evaluate(vm, "HERE 2 C,");
             enforth_evaluate(vm, "CONSTANT 2NDC");
             enforth_evaluate(vm, "CONSTANT 1STC");
        REQUIRE( enforth_test(vm, "T{ 1STC 2NDC U< -> <TRUE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1STC CHAR+ -> 2NDC }T") );
        REQUIRE( enforth_test(vm, "T{ 1STC 1 CHARS + -> 2NDC }T") );
        REQUIRE( enforth_test(vm, "T{ 1STC C@ 2NDC C@ -> 1 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 3 1STC C! -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1STC C@ 2NDC C@ -> 3 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 4 2NDC C! -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1STC C@ 2NDC C@ -> 3 4 }T") );

             enforth_evaluate(vm, "ALIGN 1 ALLOT HERE ALIGN HERE 3 CELLS ALLOT");
             enforth_evaluate(vm, "CONSTANT A-ADDR  CONSTANT UA-ADDR");
        REQUIRE( enforth_test(vm, "T{ UA-ADDR ALIGNED -> A-ADDR }T") );
        REQUIRE( enforth_test(vm, "T{    1 A-ADDR C!  A-ADDR C@ ->    1 }T") );
        REQUIRE( enforth_test(vm, "T{ 1234 A-ADDR  !  A-ADDR  @ -> 1234 }T") );
        REQUIRE( enforth_test(vm, "T{ 123 456 A-ADDR 2!  A-ADDR 2@ -> 123 456 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 A-ADDR CHAR+ C!  A-ADDR CHAR+ C@ -> 2 }T") );
        REQUIRE( enforth_test(vm, "T{ 3 A-ADDR CELL+ C!  A-ADDR CELL+ C@ -> 3 }T") );
        REQUIRE( enforth_test(vm, "T{ 1234 A-ADDR CELL+ !  A-ADDR CELL+ @ -> 1234 }T") );
        REQUIRE( enforth_test(vm, "T{ 123 456 A-ADDR CELL+ 2!  A-ADDR CELL+ 2@ -> 123 456 }T") );

             enforth_evaluate(vm, ": BITS ( X -- U )");
             enforth_evaluate(vm, "   0 SWAP BEGIN DUP WHILE DUP MSB AND IF >R 1+ R> THEN 2* REPEAT DROP ;");
        /* CHARACTERS >= 1 AU, <= SIZE OF CELL, >= 8 BITS */
        REQUIRE( enforth_test(vm, "T{ 1 CHARS 1 < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 CHARS 1 CELLS > -> <FALSE> }T") );
        /* TBD: HOW TO FIND NUMBER OF BITS? */

        /* CELLS >= 1 AU, INTEGRAL MULTIPLE OF CHAR SIZE, >= 16 BITS */
        REQUIRE( enforth_test(vm, "T{ 1 CELLS 1 < -> <FALSE> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 CELLS 1 CHARS MOD -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 1S BITS 10 < -> <FALSE> }T") );

        REQUIRE( enforth_test(vm, "T{ 0 1ST ! -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 1ST +! -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1ST @ -> 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 1ST +! 1ST @ -> 0 }T") );
    }

    SECTION( "TESTING CHAR [CHAR] [ ] BL S\"" ) {
        REQUIRE( enforth_test(vm, "T{ BL -> 20 }T") );
        REQUIRE( enforth_test(vm, "T{ CHAR X -> 58 }T") );
        REQUIRE( enforth_test(vm, "T{ CHAR HELLO -> 48 }T") );
        REQUIRE( enforth_test(vm, "T{ : GC1 [CHAR] X ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ : GC2 [CHAR] HELLO ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ GC1 -> 58 }T") );
        REQUIRE( enforth_test(vm, "T{ GC2 -> 48 }T") );
        REQUIRE( enforth_test(vm, "T{ : GC3 [ GC1 ] LITERAL ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ GC3 -> 58 }T") );
        REQUIRE( enforth_test(vm, "T{ : GC4 S\" XY\" ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ GC4 SWAP DROP -> 2 }T") );
        REQUIRE( enforth_test(vm, "T{ GC4 DROP DUP C@ SWAP CHAR+ C@ -> 58 59 }T") );
    }

    SECTION( "TESTING ' ['] FIND EXECUTE IMMEDIATE COUNT LITERAL POSTPONE STATE") {
             enforth_evaluate(vm, "0        CONSTANT 0S");
             enforth_evaluate(vm, "0 INVERT CONSTANT 1S");
             enforth_evaluate(vm, "0S CONSTANT <FALSE>");
             enforth_evaluate(vm, "1S CONSTANT <TRUE>");

        REQUIRE( enforth_test(vm, "T{ : GT1 123 ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ ' GT1 EXECUTE -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ : GT2 ['] GT1 ; IMMEDIATE -> }T") );
        REQUIRE( enforth_test(vm, "T{ GT2 EXECUTE -> 123 }T") );
                 enforth_evaluate(vm, "HERE 3 C, CHAR G C, CHAR T C, CHAR 1 C, CONSTANT GT1STRING");
                 enforth_evaluate(vm, "HERE 3 C, CHAR G C, CHAR T C, CHAR 2 C, CONSTANT GT2STRING");
        REQUIRE( enforth_test(vm, "T{ GT1STRING FIND -> ' GT1 -1 }T") );
        REQUIRE( enforth_test(vm, "T{ GT2STRING FIND -> ' GT2 1 }T") );
        /* HOW TO SEARCH FOR NON-EXISTENT WORD? */
        REQUIRE( enforth_test(vm, "T{ : GT3 GT2 LITERAL ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ GT3 -> ' GT1 }T") );
        REQUIRE( enforth_test(vm, "T{ GT1STRING COUNT -> GT1STRING CHAR+ 3 }T") );

        REQUIRE( enforth_test(vm, "T{ : GT4 POSTPONE GT1 ; IMMEDIATE -> }T") );
        REQUIRE( enforth_test(vm, "T{ : GT5 GT4 ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ GT5 -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ : GT6 345 ; IMMEDIATE -> }T") );
        REQUIRE( enforth_test(vm, "T{ : GT7 POSTPONE GT6 ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ GT7 -> 345 }T") );

        REQUIRE( enforth_test(vm, "T{ : GT8 STATE @ ; IMMEDIATE -> }T") );
        REQUIRE( enforth_test(vm, "T{ GT8 -> 0 }T") );
        REQUIRE( enforth_test(vm, "T{ : GT9 GT8 LITERAL ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ GT9 0= -> <FALSE> }T") );
    }

    SECTION( "TESTING IF ELSE THEN BEGIN WHILE REPEAT UNTIL RECURSE") {
        REQUIRE( enforth_test(vm, "T{ : GI1 IF 123 THEN ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ : GI2 IF 123 ELSE 234 THEN ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 GI1 -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 GI1 -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 GI1 -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 GI2 -> 234 }T") );
        REQUIRE( enforth_test(vm, "T{ 1 GI2 -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 GI1 -> 123 }T") );

        REQUIRE( enforth_test(vm, "T{ : GI3 BEGIN DUP 5 < WHILE DUP 1+ REPEAT ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ 0 GI3 -> 0 1 2 3 4 5 }T") );
        REQUIRE( enforth_test(vm, "T{ 4 GI3 -> 4 5 }T") );
        REQUIRE( enforth_test(vm, "T{ 5 GI3 -> 5 }T") );
        REQUIRE( enforth_test(vm, "T{ 6 GI3 -> 6 }T") );

        // TODO Enable this once have UNTIL.
        // REQUIRE( enforth_test(vm, "T{ : GI4 BEGIN DUP 1+ DUP 5 > UNTIL ; -> }T") );
        // REQUIRE( enforth_test(vm, "T{ 3 GI4 -> 3 4 5 6 }T") );
        // REQUIRE( enforth_test(vm, "T{ 5 GI4 -> 5 6 }T") );
        // REQUIRE( enforth_test(vm, "T{ 6 GI4 -> 6 7 }T") );

        REQUIRE( enforth_test(vm, "T{ : GI5 BEGIN DUP 2 > WHILE DUP 5 < WHILE DUP 1+ REPEAT 123 ELSE 345 THEN ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 GI5 -> 1 345 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 GI5 -> 2 345 }T") );
        REQUIRE( enforth_test(vm, "T{ 3 GI5 -> 3 4 5 123 }T") );
        REQUIRE( enforth_test(vm, "T{ 4 GI5 -> 4 5 123 }T") );
        REQUIRE( enforth_test(vm, "T{ 5 GI5 -> 5 123 }T") );

        // TODO Enable this once have RECURSE.
        // REQUIRE( enforth_test(vm, "T{ : GI6 ( N -- 0,1,..N ) DUP IF DUP >R 1- RECURSE R> THEN ; -> }T") );
        // REQUIRE( enforth_test(vm, "T{ 0 GI6 -> 0 }T") );
        // REQUIRE( enforth_test(vm, "T{ 1 GI6 -> 0 1 }T") );
        // REQUIRE( enforth_test(vm, "T{ 2 GI6 -> 0 1 2 }T") );
        // REQUIRE( enforth_test(vm, "T{ 3 GI6 -> 0 1 2 3 }T") );
        // REQUIRE( enforth_test(vm, "T{ 4 GI6 -> 0 1 2 3 4 }T") );
    }

    SECTION( "TESTING DO LOOP +LOOP I J UNLOOP LEAVE EXIT" ) {
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT        CONSTANT MID-UINT");
             enforth_evaluate(vm, "0 INVERT 1 RSHIFT INVERT CONSTANT MID-UINT+1");

        REQUIRE( enforth_test(vm, "T{ : GD1 DO I LOOP ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ 4 1 GD1 -> 1 2 3 }T") );
        REQUIRE( enforth_test(vm, "T{ 2 -1 GD1 -> -1 0 1 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT+1 MID-UINT GD1 -> MID-UINT }T") );

        REQUIRE( enforth_test(vm, "T{ : GD2 DO I -1 +LOOP ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 4 GD2 -> 4 3 2 1 }T") );
        REQUIRE( enforth_test(vm, "T{ -1 2 GD2 -> 2 1 0 -1 }T") );
        REQUIRE( enforth_test(vm, "T{ MID-UINT MID-UINT+1 GD2 -> MID-UINT+1 MID-UINT }T") );

        // TODO Enable this once have J.
        // REQUIRE( enforth_test(vm, "T{ : GD3 DO 1 0 DO J LOOP LOOP ; -> }T") );
        // REQUIRE( enforth_test(vm, "T{ 4 1 GD3 -> 1 2 3 }T") );
        // REQUIRE( enforth_test(vm, "T{ 2 -1 GD3 -> -1 0 1 }T") );
        // REQUIRE( enforth_test(vm, "T{ MID-UINT+1 MID-UINT GD3 -> MID-UINT }T") );

        // REQUIRE( enforth_test(vm, "T{ : GD4 DO 1 0 DO J LOOP -1 +LOOP ; -> }T") );
        // REQUIRE( enforth_test(vm, "T{ 1 4 GD4 -> 4 3 2 1 }T") );
        // REQUIRE( enforth_test(vm, "T{ -1 2 GD4 -> 2 1 0 -1 }T") );
        // REQUIRE( enforth_test(vm, "T{ MID-UINT MID-UINT+1 GD4 -> MID-UINT+1 MID-UINT }T") );

        REQUIRE( enforth_test(vm, "T{ : GD5 123 SWAP 0 DO I 4 > IF DROP 234 LEAVE THEN LOOP ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ 1 GD5 -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ 5 GD5 -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ 6 GD5 -> 234 }T") );

        // TODO Enable this once have J.
        //      enforth_evaluate(vm, "T{ : GD6  ( PAT: T{0 0}T,T{0 0}TT{1 0}TT{1 1}T,T{0 0}TT{1 0}TT{1 1}TT{2 0}TT{2 1}TT{2 2}T )");
        //      enforth_evaluate(vm, "   0 SWAP 0 DO");
        //      enforth_evaluate(vm, "      I 1+ 0 DO I J + 3 = IF I UNLOOP I UNLOOP EXIT THEN 1+ LOOP");
        // REQUIRE( enforth_test(vm, "    LOOP ; -> }T") );
        // REQUIRE( enforth_test(vm, "T{ 1 GD6 -> 1 }T") );
        // REQUIRE( enforth_test(vm, "T{ 2 GD6 -> 3 }T") );
        // REQUIRE( enforth_test(vm, "T{ 3 GD6 -> 4 1 2 }T") );
    }

    SECTION( "TESTING DEFINING WORDS: : ; CONSTANT VARIABLE CREATE DOES> >BODY") {
        REQUIRE( enforth_test(vm, "T{ 123 CONSTANT X123 -> }T") );
        REQUIRE( enforth_test(vm, "T{ X123 -> 123 }T") );
        REQUIRE( enforth_test(vm, "T{ : EQU CONSTANT ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ X123 EQU Y123 -> }T") );
        REQUIRE( enforth_test(vm, "T{ Y123 -> 123 }T") );

        REQUIRE( enforth_test(vm, "T{ VARIABLE V1 -> }T") );
        REQUIRE( enforth_test(vm, "T{ 123 V1 ! -> }T") );
        REQUIRE( enforth_test(vm, "T{ V1 @ -> 123 }T") );

        REQUIRE( enforth_test(vm, "T{ : NOP : POSTPONE ; ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ NOP NOP1 NOP NOP2 -> }T") );
        REQUIRE( enforth_test(vm, "T{ NOP1 -> }T") );
        REQUIRE( enforth_test(vm, "T{ NOP2 -> }T") );

        // TODO Enable this once have DOES>.
        // REQUIRE( enforth_test(vm, "T{ : DOES1 DOES> @ 1 + ; -> }T") );
        // REQUIRE( enforth_test(vm, "T{ : DOES2 DOES> @ 2 + ; -> }T") );
        REQUIRE( enforth_test(vm, "T{ CREATE CR1 -> }T") );
        REQUIRE( enforth_test(vm, "T{ CR1 -> HERE }T") );
        REQUIRE( enforth_test(vm, "T{ ' CR1 >BODY -> HERE }T") );
        REQUIRE( enforth_test(vm, "T{ 1 , -> }T") );
        // REQUIRE( enforth_test(vm, "T{ CR1 @ -> 1 }T") );
        // REQUIRE( enforth_test(vm, "T{ DOES1 -> }T") );
        // REQUIRE( enforth_test(vm, "T{ CR1 -> 2 }T") );
        // REQUIRE( enforth_test(vm, "T{ DOES2 -> }T") );
        // REQUIRE( enforth_test(vm, "T{ CR1 -> 3 }T") );

        // REQUIRE( enforth_test(vm, "T{ : WEIRD: CREATE DOES> 1 + DOES> 2 + ; -> }T") );
        // REQUIRE( enforth_test(vm, "T{ WEIRD: W1 -> }T") );
        // REQUIRE( enforth_test(vm, "T{ ' W1 >BODY -> HERE }T") );
        // REQUIRE( enforth_test(vm, "T{ W1 -> HERE 1 + }T") );
        // REQUIRE( enforth_test(vm, "T{ W1 -> HERE 2 + }T") );
    }

    // TODO Enable this once we have Input Control Blocks.
    // SECTION( "TESTING EVALUATE") {
    //          enforth_evaluate(vm, ": GE1 S\" 123\" ; IMMEDIATE");
    //          enforth_evaluate(vm, ": GE2 S\" 123 1+\" ; IMMEDIATE");
    //          enforth_evaluate(vm, ": GE3 S\" : GE4 345 ;\" ;");
    //          enforth_evaluate(vm, ": GE5 EVALUATE ; IMMEDIATE");

    //     REQUIRE( enforth_test(vm, "T{ GE1 EVALUATE -> 123 }T") );
    //     REQUIRE( enforth_test(vm, "T{ GE2 EVALUATE -> 124 }T") );
    //     REQUIRE( enforth_test(vm, "T{ GE3 EVALUATE -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ GE4 -> 345 }T") );

    //     REQUIRE( enforth_test(vm, "T{ : GE6 GE1 GE5 ; -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ GE6 -> 123 }T") );
    //     REQUIRE( enforth_test(vm, "T{ : GE7 GE2 GE5 ; -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ GE7 -> 124 }T") );
    // }

    SECTION( "TESTING SOURCE >IN WORD") {
        // TODO Enable this once have EVALUATE.
        //      enforth_evaluate(vm, ": GS1 S\" SOURCE\" 2DUP EVALUATE");
        //      enforth_evaluate(vm, "       >R SWAP >R = R> R> = ;");
        // REQUIRE( enforth_test(vm, "T{ GS1 -> <TRUE> <TRUE> }T") );

             enforth_evaluate(vm, "VARIABLE SCANS");
             enforth_evaluate(vm, ": RESCAN?  -1 SCANS +! SCANS @ IF 0 >IN ! THEN ;");

             enforth_evaluate(vm, "T{ 2 SCANS !");
             enforth_evaluate(vm, "345 RESCAN?");
        REQUIRE( enforth_test(vm, "-> 345 345 }T") );

        // TODO Enable this once have EVALUATE.
        //      enforth_evalute(vm, ": GS2  5 SCANS ! S\" 123 RESCAN?\" EVALUATE ;");
        // REQUIRE( enforth_test(vm, "T{ GS2 -> 123 123 123 123 123 }T") );

        // These tests are disabled; Enforth doesn't support WORD due to
        // its use of a transient area for storing a counted string.
        //         enforth_evaluate(vm, ": GS3 WORD COUNT SWAP C@ ;");
        // REQUIRE( enforth_test(vm, "T{ BL GS3 HELLO -> 5 CHAR H }T") );
        // REQUIRE( enforth_test(vm, "T{ CHAR \" GS3 GOODBYE\" -> 7 CHAR G }T") );
        //      enforth_evaluate(vm, "T{ BL GS3");
        // REQUIRE( enforth_test(vm, "DROP -> 0 }T") );

             enforth_evaluate(vm, ": GS4 SOURCE >IN ! DROP ;");
             enforth_evaluate(vm, "T{ GS4 123 456");
        REQUIRE( enforth_test(vm, "-> }T") );
    }

    SECTION( "TESTING <# # #S #> HOLD SIGN BASE >NUMBER HEX DECIMAL" ) {
             enforth_evaluate(vm, "0        CONSTANT 0S");
             enforth_evaluate(vm, "0 INVERT CONSTANT 1S");
             enforth_evaluate(vm, "0S CONSTANT <FALSE>");
             enforth_evaluate(vm, "1S CONSTANT <TRUE>");
             enforth_evaluate(vm, "0 INVERT                 CONSTANT MAX-UINT");

             enforth_evaluate(vm, ": S=");
             enforth_evaluate(vm, "   >R SWAP R@ = IF");
             enforth_evaluate(vm, "      R> ?DUP IF");
             enforth_evaluate(vm, "         0 DO");
             enforth_evaluate(vm, "            OVER C@ OVER C@ - IF 2DROP <FALSE> UNLOOP EXIT THEN");
             enforth_evaluate(vm, "            SWAP CHAR+ SWAP CHAR+");
             enforth_evaluate(vm, "         LOOP");
             enforth_evaluate(vm, "      THEN");
             enforth_evaluate(vm, "      2DROP <TRUE>");
             enforth_evaluate(vm, "   ELSE");
             enforth_evaluate(vm, "      R> DROP 2DROP <FALSE>");
             enforth_evaluate(vm, "   THEN ;");

             enforth_evaluate(vm, ": GP1  <# 41 HOLD 42 HOLD 0 0 #> S\" BA\" S= ;");
        REQUIRE( enforth_test(vm, "T{ GP1 -> <TRUE> }T") );

             enforth_evaluate(vm, ": GP2  <# -1 SIGN 0 SIGN -1 SIGN 0 0 #> S\" --\" S= ;");
        REQUIRE( enforth_test(vm, "T{ GP2 -> <TRUE> }T") );

             enforth_evaluate(vm, ": GP3  <# 1 0 # # #> S\" 01\" S= ;");
        REQUIRE( enforth_test(vm, "T{ GP3 -> <TRUE> }T") );

             enforth_evaluate(vm, ": GP4  <# 1 0 #S #> S\" 1\" S= ;");
        REQUIRE( enforth_test(vm, "T{ GP4 -> <TRUE> }T") );

             enforth_evaluate(vm, "24 CONSTANT MAX-BASE");
             enforth_evaluate(vm, ": COUNT-BITS");
             enforth_evaluate(vm, "   0 0 INVERT BEGIN DUP WHILE >R 1+ R> 2* REPEAT DROP ;");
             enforth_evaluate(vm, "COUNT-BITS 2* CONSTANT #BITS-UD");

             enforth_evaluate(vm, ": GP5");
             enforth_evaluate(vm, "   BASE @ <TRUE>");
             enforth_evaluate(vm, "   MAX-BASE 1+ 2 DO");
             enforth_evaluate(vm, "      I BASE !");
             enforth_evaluate(vm, "      I 0 <# #S #> S\" 10\" S= AND");
             enforth_evaluate(vm, "   LOOP");
             enforth_evaluate(vm, "   SWAP BASE ! ;");
        REQUIRE( enforth_test(vm, "T{ GP5 -> <TRUE> }T") );

             enforth_evaluate(vm, ": GP6");
             enforth_evaluate(vm, "   BASE @ >R  2 BASE !");
             enforth_evaluate(vm, "   MAX-UINT MAX-UINT <# #S #>");
             enforth_evaluate(vm, "   R> BASE !");
             enforth_evaluate(vm, "   DUP #BITS-UD = SWAP");
             enforth_evaluate(vm, "   0 DO");
             enforth_evaluate(vm, "      OVER C@ [CHAR] 1 = AND");
             enforth_evaluate(vm, "      >R CHAR+ R>");
             enforth_evaluate(vm, "   LOOP SWAP DROP ;");
        REQUIRE( enforth_test(vm, "T{ GP6 -> <TRUE> }T") );

             enforth_evaluate(vm, ": GP7");
             enforth_evaluate(vm, "   BASE @ >R    MAX-BASE BASE !");
             enforth_evaluate(vm, "   <TRUE>");
             enforth_evaluate(vm, "   A 0 DO");
             enforth_evaluate(vm, "      I 0 <# #S #>");
             enforth_evaluate(vm, "      1 = SWAP C@ I 30 + = AND AND");
             enforth_evaluate(vm, "   LOOP");
             enforth_evaluate(vm, "   MAX-BASE A DO");
             enforth_evaluate(vm, "      I 0 <# #S #>");
             enforth_evaluate(vm, "      1 = SWAP C@ 41 I A - + = AND AND");
             enforth_evaluate(vm, "   LOOP");
             enforth_evaluate(vm, "   R> BASE ! ;");
        REQUIRE( enforth_test(vm, "T{ GP7 -> <TRUE> }T") );

        /* >NUMBER TESTS */
             enforth_evaluate(vm, "CREATE GN-BUF 0 C,");
             enforth_evaluate(vm, ": GN-STRING     GN-BUF 1 ;");
             enforth_evaluate(vm, ": GN-CONSUMED   GN-BUF CHAR+ 0 ;");
        // Modified for Enforth to use PARSE-WORD instead of WORD.
        //   enforth_evaluate(vm, ": GN'           [CHAR] ' WORD CHAR+ C@ GN-BUF C!  GN-STRING ;");
             enforth_evaluate(vm, ": GN'           [CHAR] ' PARSE-WORD DROP C@ GN-BUF C!  GN-STRING ;");

        REQUIRE( enforth_test(vm, "T{ 0 0 GN' 0' >NUMBER -> 0 0 GN-CONSUMED }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 GN' 1' >NUMBER -> 1 0 GN-CONSUMED }T") );
        REQUIRE( enforth_test(vm, "T{ 1 0 GN' 1' >NUMBER -> BASE @ 1+ 0 GN-CONSUMED }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 GN' -' >NUMBER -> 0 0 GN-STRING }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 GN' +' >NUMBER -> 0 0 GN-STRING }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 GN' .' >NUMBER -> 0 0 GN-STRING }T") );

             enforth_evaluate(vm, ": >NUMBER-BASED");
             enforth_evaluate(vm, "   BASE @ >R BASE ! >NUMBER R> BASE ! ;");

        REQUIRE( enforth_test(vm, "T{ 0 0 GN' 2' 10 >NUMBER-BASED -> 2 0 GN-CONSUMED }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 GN' 2'  2 >NUMBER-BASED -> 0 0 GN-STRING }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 GN' F' 10 >NUMBER-BASED -> F 0 GN-CONSUMED }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 GN' G' 10 >NUMBER-BASED -> 0 0 GN-STRING }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 GN' G' MAX-BASE >NUMBER-BASED -> 10 0 GN-CONSUMED }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 GN' Z' MAX-BASE >NUMBER-BASED -> 23 0 GN-CONSUMED }T") );

             enforth_evaluate(vm, ": GN1");
             enforth_evaluate(vm, "   BASE @ >R BASE !");
             enforth_evaluate(vm, "   <# #S #>");
             enforth_evaluate(vm, "   0 0 2SWAP >NUMBER SWAP DROP");
             enforth_evaluate(vm, "   R> BASE ! ;");
        REQUIRE( enforth_test(vm, "T{ 0 0 2 GN1 -> 0 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT 0 2 GN1 -> MAX-UINT 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT DUP 2 GN1 -> MAX-UINT DUP 0 }T") );
        REQUIRE( enforth_test(vm, "T{ 0 0 MAX-BASE GN1 -> 0 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT 0 MAX-BASE GN1 -> MAX-UINT 0 0 }T") );
        REQUIRE( enforth_test(vm, "T{ MAX-UINT DUP MAX-BASE GN1 -> MAX-UINT DUP 0 }T") );

             enforth_evaluate(vm, ": GN2");
             enforth_evaluate(vm, "   BASE @ >R  HEX BASE @  DECIMAL BASE @  R> BASE ! ;");
        REQUIRE( enforth_test(vm, "T{ GN2 -> 10 A }T") );
    }

    // TODO Enable this once have FILL.
    // SECTION( "TESTING FILL MOVE" ) {
    //          enforth_evaluate(vm, "CREATE FBUF 00 C, 00 C, 00 C,");
    //          enforth_evaluate(vm, "CREATE SBUF 12 C, 34 C, 56 C,");
    //          enforth_evaluate(vm, ": SEEBUF FBUF C@  FBUF CHAR+ C@  FBUF CHAR+ CHAR+ C@ ;");

    //     REQUIRE( enforth_test(vm, "T{ FBUF 0 20 FILL -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ SEEBUF -> 00 00 00 }T") );

    //     REQUIRE( enforth_test(vm, "T{ FBUF 1 20 FILL -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ SEEBUF -> 20 00 00 }T") );

    //     REQUIRE( enforth_test(vm, "T{ FBUF 3 20 FILL -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ SEEBUF -> 20 20 20 }T") );

    //     REQUIRE( enforth_test(vm, "T{ FBUF FBUF 3 CHARS MOVE -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ SEEBUF -> 20 20 20 }T") );

    //     REQUIRE( enforth_test(vm, "T{ SBUF FBUF 0 CHARS MOVE -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ SEEBUF -> 20 20 20 }T") );

    //     REQUIRE( enforth_test(vm, "T{ SBUF FBUF 1 CHARS MOVE -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ SEEBUF -> 12 20 20 }T") );

    //     REQUIRE( enforth_test(vm, "T{ SBUF FBUF 3 CHARS MOVE -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ SEEBUF -> 12 34 56 }T") );

    //     REQUIRE( enforth_test(vm, "T{ FBUF FBUF CHAR+ 2 CHARS MOVE -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ SEEBUF -> 12 12 34 }T") );

    //     REQUIRE( enforth_test(vm, "T{ FBUF CHAR+ FBUF 2 CHARS MOVE -> }T") );
    //     REQUIRE( enforth_test(vm, "T{ SEEBUF -> 12 34 34 }T") );
    // }

    SECTION( "TESTING OUTPUT: . .\" CR EMIT SPACE SPACES TYPE U.") {
    // TODO Enable the descriptive text once have .".
             enforth_evaluate(vm, ": OUTPUT-TEST");
             // enforth_evaluate(vm, "   .\" YOU SHOULD SEE THE STANDARD GRAPHIC CHARACTERS:\" CR");
             enforth_evaluate(vm, "   41 BL DO I EMIT LOOP CR");
             enforth_evaluate(vm, "   61 41 DO I EMIT LOOP CR");
             enforth_evaluate(vm, "   7F 61 DO I EMIT LOOP CR");
             // enforth_evaluate(vm, "   .\" YOU SHOULD SEE 0-9 SEPARATED BY A SPACE:\" CR");
             enforth_evaluate(vm, "   9 1+ 0 DO I . LOOP CR");
             // enforth_evaluate(vm, "   .\" YOU SHOULD SEE 0-9 (WITH NO SPACES):\" CR");
             enforth_evaluate(vm, "   [CHAR] 9 1+ [CHAR] 0 DO I 0 SPACES EMIT LOOP CR");
             // enforth_evaluate(vm, "   .\" YOU SHOULD SEE A-G SEPARATED BY A SPACE:\" CR");
             enforth_evaluate(vm, "   [CHAR] G 1+ [CHAR] A DO I EMIT SPACE LOOP CR");
             // enforth_evaluate(vm, "   .\" YOU SHOULD SEE 0-5 SEPARATED BY TWO SPACES:\" CR");
             enforth_evaluate(vm, "   5 1+ 0 DO I [CHAR] 0 + EMIT 2 SPACES LOOP CR");
             // enforth_evaluate(vm, "   .\" YOU SHOULD SEE TWO SEPARATE LINES:\" CR");
             enforth_evaluate(vm, "   S\" LINE 1\" TYPE CR S\" LINE 2\" TYPE CR");
             // enforth_evaluate(vm, "   .\" YOU SHOULD SEE THE NUMBER RANGES OF SIGNED AND UNSIGNED NUMBERS:\" CR");
             // enforth_evaluate(vm, "   .\"   SIGNED: \" MIN-INT . MAX-INT . CR");
             // enforth_evaluate(vm, "   .\" UNSIGNED: \" 0 U. MAX-UINT U. CR");
             enforth_evaluate(vm, ";");
        REQUIRE( enforth_test(vm, "T{ OUTPUT-TEST -> }T") );
    }

    SECTION( "TESTING DICTIONARY SEARCH RULES" ) {
        REQUIRE( enforth_test(vm, "T{ : GDX   123 ; : GDX   GDX 234 ; -> }T") );

        REQUIRE( enforth_test(vm, "T{ GDX -> 123 234 }T") );
    }
}
