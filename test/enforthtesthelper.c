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

#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------
 * Harness-provided types and functions.
 */

typedef void EnforthVM;
extern "C" void enforth_evaluate(EnforthVM * const vm, const char * const text);



/* -------------------------------------
 * Helper functions.
 */

void compile_tester(EnforthVM * const vm)
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
    enforth_evaluate(vm, "   FALSE"); /* Test failed */
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
    enforth_evaluate(vm, "            <> IF S\" INCORRECT RESULT: \" ERROR UNLOOP EXIT THEN");
    enforth_evaluate(vm, "         LOOP");
    enforth_evaluate(vm, "      THEN");
    enforth_evaluate(vm, "      TRUE"); /* Test was successful */
    enforth_evaluate(vm, "   ELSE");
    enforth_evaluate(vm, "      S\" WRONG NUMBER OF RESULTS: \" ERROR");
    enforth_evaluate(vm, "   THEN ;");

    /* TALKING COMMENT. */
    enforth_evaluate(vm, ": TESTING");
    enforth_evaluate(vm, "  SOURCE VERBOSE @");
    enforth_evaluate(vm, "   IF DUP >R TYPE CR R> >IN !");
    enforth_evaluate(vm, "   ELSE >IN ! DROP");
    enforth_evaluate(vm, "   THEN ;");
}


#ifdef __cplusplus
}
#endif
