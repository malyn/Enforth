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
