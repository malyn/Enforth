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
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* AVR includes. */
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM
#define pgm_read_byte(p) (*(uint8_t*)(p))
#define pgm_read_word(p) (*(int *)(p))
#define memcpy_P memcpy
#define strlen_P strlen
#define strncasecmp_P strncasecmp
#endif

/* Enforth includes. */
#include "enforth.h"



/* -------------------------------------
 * Enforth definition types and tokens.
 */

enum EnforthDefinitionType
{
    kDefTypeCOLON = 0,
    kDefTypeIMMEDIATE,
    kDefTypeCONSTANT,
    kDefTypeCREATE,
    kDefTypeDOES,
    kDefTypeVARIABLE,
    kDefTypeFFI0,
    kDefTypeFFI1,
    kDefTypeFFI2,
    kDefTypeFFI3,
    kDefTypeFFI4,
    kDefTypeFFI5,
    kDefTypeFFI6,
    kDefTypeFFI7,
    kDefTypeFFI8
};

typedef enum EnforthToken
{
    /* $00 - $07 */
        SEMICOLON = 0x00,
    DUP,
        UDOT = 0x02,
    DROP,

        DOT = 0x04,
    PLUS,
    MINUS,
    ONEPLUS,

    /* $08 - $0F */
    ONEMINUS,
        CREATE = 0x09,
    SWAP,
    ABORT,

    INVERT,
    LESSTHAN,
    EMIT,
    FETCH,

    /* $10 - $17 */
    UGREATERTHAN,
    OR,
    TWOSWAP,
        COLON = 0x13,

    QDUP,
    UMSTAR,
        NUMSIGNS = 0x16,
unused_was_STATE,

    /* $18 - $1F */
        SIGN = 0x18,
    STORE,
        NUMSIGN = 0x1a,
unused_was_TOIN,

    TWODROP,
    MPLUS,
    ZERO,
        QUIT = 0x1f,

    /* $20 - $27 */
    ZEROEQUALS,
    BL,
    CFETCH,
    COUNT,

    DEPTH,
        INTERPRET = 0x25,
    BACKSLASH,
    HEX,

    /* $28 - $2F */
unused_was_HERE,
    TWODUP,
    COMMA,
    CCOMMA,

    TUCK,
    ALIGN,
    MOVE,
    CPLUSSTORE,

    /* $30 - $37 */
    ALLOT,
    NIP,
        ACCEPT = 0x32,
    WCOMMA,

    RTBRACKET,
    CSTORE,
    LTBRACKET,
    ABS,

    /* $38 - $3F */
    LESSNUMSIGN,
    ROT,
        PARSEWORD = 0x3a,
    NUMSIGNGRTR,

    ZEROLESS,
    HOLD,
unused_was_BASE,
    UDSLASHMOD,

    /* $40 - $47 */
    GREATERTHAN,
    AND,
    NOTEQUALS,
    OVER,

    TWOOVER,
    KEY,
    TOR,
        TYPE = 0x47,

    /* $48 - $4F */
    RFROM,
    RFETCH,
    EQUALS,
        SPACE = 0x4b,

        CR = 0x4c,
        TOCFA = 0x4d,
    TWOSTORE,
    SLASHSTRING,

    /* $50 - $57 */
        TOBODY = 0x50,
    PLUSSTORE,
    TWOFETCH,

        TOKENQ = 0x57,

    /* $58 - $5F */
        COMPILECOMMA = 0x59,
        EXECUTE = 0x5e,

    /* $60 - $67 */
        LITERAL = 0x63,

    /* $68 - $6F */
        DIGITQ = 0x68,

    /* $70 - $77 */
        TONUMBER = 0x73,

    /* $78 - $7F */
        NUMBERQ = 0x7c,
        SOURCE = 0x7f,

    /* $80 - $87 */
        BASE = 0x80,
        HERE = 0x81,
        LATEST = 0x82,
        TIB = 0x83,

        TIBSIZE = 0x84,
        TOIN = 0x85,
        STATE = 0x86,

    /* $88 - $8F */
    /* $90 - $97 */
    /* $98 - $9F */
    /* $A0 - $A7 */
    /* $A8 - $AF */
    /* $B0 - $B7 */
    /* $B8 - $BF */
    /* $C0 - $C7 */

    /* $C8 - $CF */
    /* ... */
    WLIT = 0xce,
    PSQUOTE,

    /* $D0 - $D7 */
    CHARLIT = 0xd0,
    REVEAL,
    ZBRANCH,
unused_was_LATEST,

    BRANCH,
    HIDE,
    INITRP,
unused_was_TIB,

    /* $D8 - $DF */
unused_was_TIBSIZE,
    PDOTQUOTE,
unused_was_TICKSOURCE,
unused_was_TICKSOURCELEN,

    FINDWORD,
    LIT,
    VMADDRLIT,
    PEXECUTE,

    /* Tokens 0xe0-0xee are reserved for jump labels to the "CFA"
     * primitives *after* the point where W (the Word Pointer) has
     * already been set.  This allows words like EXECUTE to jump to a
     * CFA without having to use a switch statement to convert
     * definition types to tokens.  The token names themselves do not
     * need to be defined because they are never referenced (we're just
     * reserving space in the token list and Address Interpreter jump
     * table, in other words), but we do list them here in order to make
     * it easier to turn raw tokens into enum values in the debugger. */
    PDOCOLON = 0xe0,
    PDOIMMEDIATE,
    PDOCONSTANT,
    PDOCREATE,
    PDODOES,
    PDOVARIABLE,
    PDOFFI0,
    PDOFFI1,
    PDOFFI2,
    PDOFFI3,
    PDOFFI4,
    PDOFFI5,
    PDOFFI6,
    PDOFFI7,
    PDOFFI8,

    /* Just like the above, these tokens are never used in code and this
     * list of enum values is only used to simplify debugging. */
    DOCOLON = 0xf0,
    DOIMMEDIATE,
    DOCONSTANT,
    DOCREATE,
    DODOES,
    DOVARIABLE,
    DOFFI0,
    DOFFI1,
    DOFFI2,
    DOFFI3,
    DOFFI4,
    DOFFI5,
    DOFFI6,
    DOFFI7,
    DOFFI8,

    /* This is a normal token and is placed at the end in order to make
     * it easier to identify in definitions. */
    EXIT = 0xff,
} EnforthToken;



/* -------------------------------------
 * Enforth definition names.
 */

/* A length with a high bit set indicates that the word is immediate. */
static const char kDefinitionNames[] PROGMEM =
    /* $00 - $07 */
    "\x81" ";"
    "\x03" "DUP"
    "\x02" "U."
    "\x04" "DROP"

    "\x01" "."
    "\x01" "+"
    "\x01" "-"
    "\x02" "1+"

    /* $08 - $0F */
    "\x02" "1-"
    "\x06" "CREATE"
    "\x04" "SWAP"
    "\x05" "ABORT"

    "\x06" "INVERT"
    "\x01" "<"
    "\x04" "EMIT"
    "\x01" "@"

    /* $10 - $17 */
    "\x02" "U>"
    "\x02" "OR"
    "\x05" "2SWAP"
    "\x01" ":"

    "\x04" "?DUP"
    "\x03" "UM*"
    "\x02" "#S"
"\x00" /* UNUSED */

    /* $18 - $1F */
    "\x04" "SIGN"
    "\x01" "!"
    "\x01" "#"
"\x00" /* UNUSED */

    "\x05" "2DROP"
    "\x02" "M+"
    "\x01" "0"
    "\x04" "QUIT"

    /* $20 - $27 */
    "\x02" "0="
    "\x02" "BL"
    "\x02" "C@"
    "\x05" "COUNT"

    "\x05" "DEPTH"
    "\x00" /* INTERPRET */
    "\x01" "\\"
    "\x03" "HEX"

    /* $28 - $2F */
"\x00" /* UNUSED was HERE */
    "\x04" "2DUP"
    "\x01" ","
    "\x02" "C,"

    "\x04" "TUCK"
    "\x05" "ALIGN"
    "\x04" "MOVE"
    "\x03" "C+!"

    /* $30 - $37 */
    "\x05" "ALLOT"
    "\x03" "NIP"
    "\x06" "ACCEPT"
    "\x02" "W,"

    "\x01" "]"
    "\x02" "C!"
    "\x81" "["
    "\x03" "ABS"

    /* $38 - $3F */
    "\x02" "<#"
    "\x03" "ROT"
    "\x00" /* PARSE-WORD */
    "\x02" "#>"

    "\x02" "0<"
    "\x04" "HOLD"
"\x00" /* UNUSED was BASE */
    "\x06" "UD/MOD"

    /* $40 - $47 */
    "\x01" ">"
    "\x03" "AND"
    "\x02" "<>"
    "\x04" "OVER"

    "\x05" "2OVER"
    "\x03" "KEY"
    "\x02" ">R"
    "\x04" "TYPE"

    /* $48 - $4F */
    "\x02" "R>"
    "\x02" "R@"
    "\x01" "="
    "\x05" "SPACE"

    "\x02" "CR"
    "\x00" /* TOCFA */
    "\x02" "2!"
    "\x07" "/STRING"

    /* $50 - $57 */
    "\x05" ">BODY"
    "\x02" "+!"
    "\x02" "2@"
    "\x00" /* UNUSED */

    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* TOKENQ */

    /* $58 - $5F */
    "\x00" /* UNUSED */
    "\x08" "COMPILE,"
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */

    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x07" "EXECUTE"
    "\x00" /* UNUSED */

    /* $60 - $67 */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x07" "LITERAL"

    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */

    /* $68 - $6F */
    "\x00" /* DIGITQ */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */

    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */

    /* $70 - $77 */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x07" ">NUMBER"

    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */

    /* $78 - $7F */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */

    "\x00" /* NUMBERQ */
    "\x00" /* UNUSED */
    "\x00" /* UNUSED */
    "\x06" "SOURCE"
    "\x04" "BASE"
    "\x04" "HERE"
    "\x00" /* LATEST */
    "\x00" /* TIB */
    "\x00" /* TIBSIZE */
    "\x03" ">IN"
    "\x05" "STATE"

    /* End byte */
    "\xff"
;



/* -------------------------------------
 * Enforth definitions.
 */

static const int8_t definitions[] PROGMEM = {
    /* SEMICOLON, Offset=0, Length=6
     *   : ; ( --)   ['] EXIT COMPILE,  REVEAL  [ ; IMMEDIATE */
    CHARLIT, EXIT, COMPILECOMMA, REVEAL, LTBRACKET, EXIT, 0, 0,

    /* UDOT, Offset=8, Length=7
     *   : U. ( u --)  0 <# #S #> TYPE SPACE ; */
    ZERO, LESSNUMSIGN, NUMSIGNS, NUMSIGNGRTR, TYPE, SPACE, EXIT, 0,

    /* DOT, Offset=16, Length=20
     * : . ( n -- )
     *   BASE @ 10 <>  IF U. EXIT THEN
     *   DUP ABS 0 <# #S ROT SIGN #> TYPE SPACE ; */
    BASE, FETCH, CHARLIT, 10, NOTEQUALS, ZBRANCH, 3, UDOT, EXIT,
    DUP, ABS, ZERO, LESSNUMSIGN, NUMSIGNS, ROT, SIGN,
        NUMSIGNGRTR, TYPE, SPACE,
    EXIT,

    /* -------------------------------------------------------------
     * CREATE [CORE] 6.1.1000 ( "<spaces>name" -- )
     *
     * Skip leading space delimiters.  Parse name delimited by a
     * space.  Create a definition for name with the execution
     * semantics defined below.  If the data-space pointer is not
     * aligned, reserve enough data space to align it.  The new
     * data-space pointer defines name's data field.  CREATE does
     * not allocate data space in name's data field.
     *
     *   name Execution:    ( -- a-addr )
     *       a-addr is the address of name's data field.  The
     *       execution semantics of name may be extended by using
     *       DOES>.
     *
     * : TERMINATE-NAME ( ca u -- ca u)  2DUP 1- +  $80 SWAP C+! ;
     * : S, ( ca u --)  TUCK  HERE SWAP MOVE  ALLOT ;
     * : NAME, ( ca u --)  TERMINATE-NAME S, ;
     * : >LATEST-OFFSET ( addr -- u)  LATEST @ DUP IF - ELSE NIP THEN ;
     * : CREATE ( "<spaces>name" -- )
     *   BL PARSE-WORD DUP 0= IF ABORT THEN ( ca u)
     *   HERE  DUP >LATEST-OFFSET W,  LATEST ! ( ca u)
     *   CFADOCREATE C, ( ca u)  NAME,  ALIGN ;
     *
     * Offset=36, Length=38 */
    BL, PARSEWORD, DUP, ZEROEQUALS, ZBRANCH, 2, ABORT,
    HERE, DUP,
    /* >LATEST-OFFSET */
        LATEST, FETCH, DUP, ZBRANCH, 4,
            MINUS, BRANCH, 2,
            NIP,
    WCOMMA, LATEST, STORE,
    CHARLIT, kDefTypeCREATE, CCOMMA,
    /* NAME, */
        /* TERMINATE-NAME */
            TWODUP, ONEMINUS, PLUS, CHARLIT, 0x80, SWAP, CPLUSSTORE,
        /* S, */
            TUCK, HERE, SWAP, MOVE, ALLOT,
    ALIGN,
    EXIT, 0, 0,

    /* -------------------------------------------------------------
     * : [CORE] 6.1.0450 "colon" ( C: "<spaces>name" -- colon-sys )
     *
     * Skip leading space delimiters.  Parse name delimited by a
     * space.  Create a definition for name, called a "colon
     * definition".  Enter compilation state and start the
     * current definition, producing colon-sys.  Append the
     * initiation semantics given below to the current definition.
     *
     * The execution semantics of name will be determined by the
     * words compiled into the body of the definition.  The current
     * definition shall not be findable in the dictionary until it
     * is ended (or until the execution of DOES> in some systems).
     *
     * Initiation: ( i*x -- i*x ) ( R: -- nest-sys )
     *   Save implementation-dependent information nest-sys about
     *   the calling definition.  The stack effects i*x represent
     *   arguments to name.
     *
     * name Execution: ( i*x -- j*x )
     *       Execute the definition name.  The stack effects i*x and
     *       j*x represent arguments to and results from name,
     *       respectively.
     *
     * : LFA>CFA ( addr -- addr)  1+ 1+ ;
     * : : ( "<spaces>name" -- )
     *   CREATE  HIDE  CFADOCOLON LATEST @ LFA>CFA C!  ] ;
     *
     * Offset=76, Length=11 */
    CREATE, HIDE, CHARLIT, kDefTypeCOLON, LATEST, FETCH,
    /* LFA>CFA */
        ONEPLUS, ONEPLUS,
    CSTORE, RTBRACKET,
    EXIT, 0,

    /* -------------------------------------------------------------
     * #S [CORE] 6.1.0050 "number-sign-s" ( ud1 -- ud2 )
     *
     * Convert one digit of ud1 according to the rule for #.
     * Continue conversion until the quotient is zero.  ud2 is zero.
     * An ambiguous condition exists if #S executes outside of a <#
     * #> delimited number conversion.
     *
     * ---
     * : #S ( ud1 -- 0 )   BEGIN # 2DUP OR WHILE REPEAT ;
     *
     * Offset=88, Length=8 */
    NUMSIGN, TWODUP, OR, ZBRANCH, 3, BRANCH, -6, EXIT,

    /* -------------------------------------------------------------
     * SIGN [CORE] 6.1.2210 ( n -- )
     *
     * If n is negative, add a minus sign to the beginning of the
     * pictured numeric output string.  An ambiguous condition
     * exists if SIGN executes outside of a <# #> delimited number
     * conversion.
     *
     * ---
     * : SIGN ( n -- )   0< IF [CHAR] - HOLD THEN ;
     *
     * Offset=96, Length=7 */
    ZEROLESS, ZBRANCH, 4, CHARLIT, '-', HOLD, EXIT, 0,

    /* -------------------------------------------------------------
     * # [CORE] 6.1.0030 "number-sign" ( ud1 -- ud2 )
     *
     * Divide ud1 by the number in BASE giving the quotient ud2 and
     * the remainder n.  (n is the least-significant digit of ud1.)
     * Convert n to external form and add the resulting character to
     * the beginning of the pictured numeric output string.  An
     * ambiguous condition exists if # executes outside of a <# #>
     * delimited number conversion.
     *
     * ---
     * : >DIGIT ( u -- c ) DUP 9 > 7 AND + 48 + ;
     * : # ( ud1 -- ud2 )   BASE @ UD/MOD ROT >digit HOLD ;
     *
     * Offset=104, Length=17 */
    BASE, FETCH, UDSLASHMOD, ROT,
    /* TODIGIT */
        DUP, CHARLIT, 9, GREATERTHAN, CHARLIT, 7, AND,
        PLUS, CHARLIT, 48, PLUS,
    HOLD,
    EXIT, 0, 0, 0,

    /* -------------------------------------------------------------
     * QUIT [CORE] 6.1.2050 ( -- ) ( R:  i*x -- )
     *
     * Empty the return stack, store zero in SOURCE-ID if it is
     * present, make the user input device the input source, and
     * enter interpretation state.  Do not display a message.
     * Repeat the following:
     *   - Accept a line from the input source into the input
     *     buffer, set >IN to zero, and interpret.
     *   - Display the implementation-defined system prompt if in
     *     interpretation state, all processing has been completed,
     *     and no ambiguous condition exists.
     *
     * ---
     * : QUIT  ( --; R: i*x --)
     *   INITRP  0 STATE !
     *   BEGIN
     *       TIB  DUP TIBSIZE ACCEPT  SPACE
     *       INTERPRET
     *       CR  STATE @ 0= IF ." ok " THEN
     *   AGAIN ;
     *
     * Offset=124, Length=24 */
    INITRP, ZERO, STATE, STORE,
    TIB, DUP, TIBSIZE, ACCEPT, SPACE,
    INTERPRET,
    CR, STATE, FETCH, ZEROEQUALS, ZBRANCH, 6,
    PDOTQUOTE, 3, 'o', 'k', ' ',
    BRANCH, -18,
    0,

    /* : INTERPRET ( i*x c-addr u -- j*x )
     *   'SOURCELEN 2!  0 >IN !
     *   BEGIN  BL PARSE-WORD  DUP WHILE
     *       FIND-WORD ( ca u 0=notfound | xt 1=imm | xt -1=interp)
     *       ?DUP IF ( xt 1=imm | xt -1=interp)
     *           1+  STATE @ 0=  OR ( xt 2=imm | xt 0=interp)
     *           IF EXECUTE ELSE COMPILE, THEN
     *       ELSE
     *           NUMBER? IF
     *               STATE @ IF POSTPONE LITERAL THEN
     *               -- Interpreting; leave number on stack.
     *           ELSE
     *               TYPE  SPACE  [CHAR] ? EMIT  CR  ABORT
     *           THEN
     *       THEN
     *   REPEAT ( j*x ca u) 2DROP ;
     *
     * Offset=148, Length=49 */
    VMADDRLIT, offsetof(EnforthVM, source_len), TWOSTORE,
    ZERO, TOIN, STORE,
    BL, PARSEWORD, DUP, ZBRANCH, 37,
    FINDWORD, QDUP, ZBRANCH, 14,
    ONEPLUS, STATE, FETCH, ZEROEQUALS, OR, ZBRANCH, 4,
    EXECUTE, BRANCH, 21,
    COMPILECOMMA, BRANCH, 18,
    NUMBERQ, ZBRANCH, 8,
    STATE, FETCH, ZBRANCH, 11,
    LITERAL, BRANCH, 8,
    TYPE, SPACE, CHARLIT, '?', EMIT, CR, ABORT,
    BRANCH, -40,
    TWODROP,
    EXIT, 0, 0, 0,

    /* -------------------------------------------------------------
     * ACCEPT [CORE] 6.1.0695 ( c-addr +n1 -- +n2 )
     *
     * Receive a string of at most +n1 characters.  An ambiguous
     * condition exists if +n1 is zero or greater than 32,767.
     * Display graphic characters as they are received.  A program
     * that depends on the presence or absence of non-graphic
     * characters in the string has an environmental dependency.
     * The editing functions, if any, that the system performs in
     * order to construct the string are implementation-defined.
     *
     * Input terminates when an implementation-defined line
     * terminator is received.  When input terminates, nothing is
     * appended to the string, and the display is maintained in an
     * implementation-defined way.
     *
     * +n2 is the length of the string stored at c-addr.
     * ---
     * TODO Deal with backspace.
     * : ACCEPT ( c-addr max -- n)
     *   OVER + OVER ( ca-start ca-end ca-dest)
     *   BEGIN  KEY  DUP 10 <> WHILE
     *      DUP 2OVER ( cas cae cad c c cae cad) <> IF
     *         EMIT OVER C! ( cas cae cad) 1+
     *      ELSE
     *         ( cas cae cad c c) 2DROP
     *      THEN
     *   REPEAT
     *   ( ca-start ca-end ca-dest c) DROP NIP SWAP - ;
     *
     * Offset=200, Length=29 */
    OVER, PLUS, OVER,
    KEY, DUP, CHARLIT, 10, NOTEQUALS, ZBRANCH, 15,
        DUP, TWOOVER, NOTEQUALS, ZBRANCH, 7,
            EMIT, OVER, CSTORE, ONEPLUS, BRANCH, 2,
            TWODROP,
        BRANCH, -20,
    DROP, NIP, SWAP, MINUS,
    EXIT, 0, 0, 0,

    /* -------------------------------------------------------------
     * PARSE-WORD [MFORTH] "parse-word" ( char "ccc<char>" -- c-addr u )
     *
     * Parse ccc delimited by the delimiter char, skipping leading
     * delimiters.
     *
     * c-addr is the address (within the input buffer) and u is the
     * length of the parsed string.  If the parse area was empty,
     * the resulting string has a zero length.
     * ---
     * : SKIP-DELIM ( c-addr1 u1 c -- c-addr2 u2)
     *   >R  BEGIN  OVER C@  R@ =  ( ca u f R:c) OVER  AND WHILE
     *      1 /STRING REPEAT  R> DROP ;
     * : FIND-DELIM ( c-addr1 u1 c -- c-addr2)
     *   >R  BEGIN  OVER C@  R@ <>  ( ca u f R:c) OVER  AND WHILE
     *      1 /STRING REPEAT  R> 2DROP ;
     * : PARSE-WORD ( c -- c-addr u)
     *   >R  SOURCE >IN @ /STRING ( ca-parse u-parse R:c)
     *   R@ SKIP-DELIM ( ca u R:c)  OVER SWAP ( ca-word ca-word u R:c)
     *   R> FIND-DELIM ( ca-word ca-delim)
     *   DUP SOURCE DROP ( caw cad cad cas) - >IN ! ( caw cad)
     *   OVER - ;
     *
     * Offset=232, Length=50 */
    TOR, SOURCE, TOIN, FETCH, SLASHSTRING,
    RFETCH,
        /* SKIP-DELIM */
        TOR,
        OVER, CFETCH, RFETCH, EQUALS, OVER, AND, ZBRANCH, 6,
            CHARLIT, 1, SLASHSTRING, BRANCH, -12,
        RFROM, DROP,
    OVER, SWAP,
    RFROM,
        /* FIND-DELIM */
        TOR,
        OVER, CFETCH, RFETCH, NOTEQUALS, OVER, AND, ZBRANCH, 6,
            CHARLIT, 1, SLASHSTRING, BRANCH, -12,
        RFROM, TWODROP,
    DUP, SOURCE, DROP, MINUS, TOIN, STORE,
    OVER, MINUS,
    EXIT, 0, 0,

    /* : TYPE ( c-addr u --)
     *   OVER + SWAP  ( ca-end ca-next)
     *   BEGIN 2DUP <> WHILE DUP C@ EMIT 1+ REPEAT 2DROP ;
     *
     * Offset=284, Length=15 */
    OVER, PLUS, SWAP,
    TWODUP, NOTEQUALS, ZBRANCH, 7,
        DUP, CFETCH, EMIT, ONEPLUS, BRANCH, -9,
    TWODROP,
    EXIT, 0,

    /* : SPACE ( --)  BL EMIT ;
     *
     * Offset=300, Length=3 */
    BL, EMIT, EXIT, 0,

    /* : CR ( --)  BL EMIT ;
     *
     * Offset=304, Length=4 */
    CHARLIT, 10, EMIT, EXIT,

    /* : +LFA ( addr1 -- addr2)  1+ 1+ ;
     * : >CFA ( xt -- addr)  $7FFF AND  'DICT +  +LFA ;
     *
     * Offset=308, Length=11 */
    WLIT, 0xff, 0x7f,
    AND,
    /* TICKDICT */
        VMADDRLIT, offsetof(EnforthVM, dictionary), FETCH,
    PLUS,
    /* PLUSLFA */
        ONEPLUS, ONEPLUS,
    EXIT, 0,

    /* : FFI? ( xt -- f)  >CFA C@ kDefTypeFFI0 1- > ;
     * : >BODY ( xt -- a-addr)
     *   DUP >CFA 1+  SWAP FFI? IF EXIT THEN
     *   BEGIN DUP C@ $80 AND 0= WHILE 1+ REPEAT 1+ ;
     *
     * Offset=320, Length=25 */
    DUP, TOCFA, ONEPLUS, SWAP,
    /* FFI? */
        TOCFA, CFETCH, CHARLIT, kDefTypeFFI0-1, GREATERTHAN,
    ZBRANCH, 2, EXIT,
    DUP, CFETCH, CHARLIT, 0x80, AND, ZEROEQUALS, ZBRANCH, 4,
        ONEPLUS, BRANCH, -10,
    ONEPLUS,
    EXIT, 0, 0, 0,

    /* : TOKEN? ( xt -- f)  $8000 AND 0= ;
     *
     * Offset=348, Length=6/8 */
    WLIT, 0x00, 0x80,
    AND, ZEROEQUALS,
    EXIT, 0, 0,

    /* : CFA>TOKEN ( def-type -- token)  $F0 OR ;
     * : COMPILE, ( xt --)
     *   DUP TOKEN? IF C, EXIT THEN
     *   DUP >CFA C@ CFA>[TOKEN] C,  >BODY 'DICT - W, ;
     *
     * Offset=356, Length=20 */
    DUP, TOKENQ, ZBRANCH, 3,
        CCOMMA, EXIT,
    DUP, TOCFA, CFETCH,
    /* CFA>[TOKEN] */
        CHARLIT, 0xF0, OR,
    CCOMMA, TOBODY,
    /* TICKDICT */
        VMADDRLIT, offsetof(EnforthVM, dictionary), FETCH,
    MINUS, WCOMMA,
    EXIT,

    /* -------------------------------------------------------------
     * EXECUTE [CORE] 6.1.1370 ( i*x xt -- j*x )
     *
     * Remove xt from the stack and perform the semantics identified
     * by it.  Other stack effects are due to the word EXECUTEd.
     * ---
     * : (EXECUTE) ( i*x token w -- j*x) ... ;
     * : EXECUTE ( i*x xt -- j*x)
     *   DUP TOKEN? IF 0
     *   ELSE DUP >CFA C@ CFA>TOKEN  SWAP >BODY THEN
     *   (EXECUTE) ;
     *
     * Offset=376, Length=17 */
    DUP, TOKENQ, ZBRANCH, 4,
        ZERO, BRANCH, 9,
    DUP, TOCFA, CFETCH,
    /* CFA>TOKEN */
        CHARLIT, 0xE0, OR,
    SWAP, TOBODY,
    PEXECUTE,
    EXIT, 0, 0, 0,

    /* : LITERAL ( x --)
     *   DUP $FF INVERT AND 0= IF CHARLIT C, C,
     *   ELSE LIT C, , THEN ;
     *
     * Offset=396, Length=19 */
    DUP, CHARLIT, 0xff, INVERT, AND, ZEROEQUALS, ZBRANCH, 7,
        CHARLIT, CHARLIT, CCOMMA, CCOMMA, BRANCH, 5,
        CHARLIT, LIT, CCOMMA, COMMA,
    EXIT, 0,

    /* -------------------------------------------------------------
     * DIGIT? [MFORTH] "digit-question" ( char -- u -1 | 0 )
     *
     * Attempts to convert char to a numeric value using the current
     * BASE.  Pushes the numeric value and -1 to the stack if the
     * value was converted, otherwise pushes 0 to the stack.
     * ---
     * : DIGIT? ( char -- u -1 | 0)
     *   [CHAR] 0 -           DUP 0< IF DROP 0 EXIT THEN
     *   DUP 9 > IF DUP 16 < IF DROP 0 EXIT ELSE 7 - THEN THEN
     *   DUP 1+ BASE @ > IF DROP FALSE ELSE TRUE THEN ;
     *
     * Offset=416, Length=41 */
    CHARLIT, '0', MINUS,
    DUP, ZEROLESS, ZBRANCH, 4, DROP, ZERO, EXIT,
    DUP, CHARLIT, 9, GREATERTHAN, ZBRANCH, 13,
        DUP, CHARLIT, 17, LESSTHAN, ZBRANCH, 4,
            DROP, ZERO, EXIT,
            CHARLIT, 7, MINUS,
    DUP, ONEPLUS, BASE, FETCH, UGREATERTHAN, ZBRANCH, 4,
        DROP, ZERO, EXIT, /* TODO: Replace with FALSE */
        ZERO, INVERT, /* TODO: Replace with TRUE */
    EXIT, 0, 0, 0,

    /* -------------------------------------------------------------
     * >NUMBER [CORE] 6.1.0567 "to-number" ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )
     *
     * ud2 is the unsigned result of converting the characters
     * within the string specified by c-addr1 u1 into digits, using
     * the number in BASE, and adding each into ud1 after
     * multiplying ud1 by the number in BASE.  Conversion continues
     * left-to-right until a character that is not convertible,
     * including any "+" or "-", is encountered or the string is
     * entirely converted.  c-addr2 is the location of the first
     * unconverted character or the first character past the end of
     * the string if the string was entirely converted.  u2 is the
     * number of unconverted characters in the string.  An ambiguous
     * condition exists if ud2 overflows during the conversion.
     *
     * ---
     * : UD* ( ud1 u1 -- ud2)   DUP >R UM* DROP  SWAP R> UM* ROT + ;
     * : >NUMBER ( ud1 c-addr1 u1 -- ud2 c-addr2 u2)
     *   BEGIN DUP WHILE
     *      OVER C@ DIGIT?  0= IF DROP EXIT THEN
     *      >R 2SWAP BASE @ UD* R> M+ 2SWAP
     *      1 /STRING
     *   REPEAT ;
     *
     * Offset=460, Length=33 */
    DUP, ZBRANCH, 30,
        OVER, CFETCH, DIGITQ, ZEROEQUALS, ZBRANCH, 3, DROP, EXIT,
        TOR, TWOSWAP, BASE, FETCH,
        /* UDSTAR */
            DUP, TOR, UMSTAR, DROP, SWAP, RFROM, UMSTAR, ROT, PLUS,
        RFROM, MPLUS, TWOSWAP,
        CHARLIT, 1, SLASHSTRING,
        BRANCH, -31,
    EXIT, 0, 0, 0,

    /* -------------------------------------------------------------
     * NUMBER? [ENFORTH] "number-question" ( c-addr u -- c-addr u 0 | n -1 )
     *
     * Attempt to convert a string at c-addr of length u into digits,
     * using the radix in BASE.  The number and -1 is returned if the
     * conversion was successful, otherwise 0 is returned.
     * ---
     * TODO Implement this for real.
     * : NUMBER? ( c-addr u -- c-addr u 0 | n -1)
     *   0 0 2SWAP >NUMBER 2DROP DROP -1 ;
     *
     * Offset=496, Length=9 */
    /* TODO Replace ZERO INVERT with TRUE. */
    ZERO, ZERO, TWOSWAP, TONUMBER, TWODROP, DROP, ZERO, INVERT,
    EXIT, 0, 0, 0,

    /* : SOURCE ( -- c-addr u)
     *   'SOURCELEN 2@ ;
     *
     * Offset=508, Length=4 */
    VMADDRLIT, offsetof(EnforthVM, source_len), TWOFETCH,
    EXIT,

    /* : BASE ( -- a-addr)  'BASE ;
     *
     * Offset=512, Length=3 */
    VMADDRLIT, offsetof(EnforthVM, base),
    EXIT, 0,

    /* : HERE ( -- addr)  'DP @ ;
     *
     * Offset=516, Length=4 */
    VMADDRLIT, offsetof(EnforthVM, dp), FETCH,
    EXIT,

    /* : LATEST ( -- a-addr)  'LATEST ;
     *
     * Offset=520, Length=3 */
    VMADDRLIT, offsetof(EnforthVM, latest),
    EXIT, 0,

    /* : TIB ( -- c-addr)  'TIB ;
     *
     * Offset=524, Length=3 */
    VMADDRLIT, offsetof(EnforthVM, tib),
    EXIT, 0,

    /* : TIBSIZE ( -- u)  ... ;
     *
     * Offset=528, Length=2 */
    /* TODO Put TIBSIZE in a constant. */
    CHARLIT, 80,
    EXIT, 0,

    /* : >IN ( -- c-addr)  '>IN ;
     *
     * Offset=532, Length=3 */
    VMADDRLIT, offsetof(EnforthVM, to_in),
    EXIT, 0,

    /* : STATE ( -- a-addr)  'STATE ;
     *
     * Offset=536, Length=3 */
    VMADDRLIT, offsetof(EnforthVM, state),
    EXIT, 0,
};



/* -------------------------------------
 * Private function definitions.
 */

static int enforth_paren_find_word(EnforthVM * const vm, uint8_t * caddr, EnforthUnsigned u, EnforthXT * const xt, int * const isImmediate);



/* -------------------------------------
 * Private functions.
 */

static int enforth_paren_find_word(EnforthVM * const vm, uint8_t * caddr, EnforthUnsigned u, EnforthXT * const xt, int * const isImmediate)
{
    int searchLen = u;
    char * searchName = (char *)caddr;

    /* Search the dictionary.*/
    uint8_t * curWord;
    for (curWord = vm->latest;
         curWord != NULL;
         curWord = *(uint16_t *)curWord == 0 ? NULL : curWord - *(uint16_t *)curWord)
    {
        /* Get the definition type. */
        uint8_t definitionType = *(curWord + 2);

        /* Ignore smudged words. */
        if ((definitionType & 0x80) != 0)
        {
            continue;
        }

        /* Compare the strings, which happens differently depending on
         * if this is a user-defined word or an FFI trampoline. */
        int nameMatch;
        if (definitionType < kDefTypeFFI0)
        {
            char * pSearchName = searchName;
            char * searchEnd = searchName + searchLen-1;
            char * pDictName = (char *)(curWord + 3);
            for (;;)
            {
                /* Try to match the characters and fail if they don't
                 * match. */
                if (toupper(*pSearchName) != toupper(*pDictName & 0x7f))
                {
                    nameMatch = 0;
                    break;
                }

                /* We're done if this was the last character. */
                if ((*pDictName & 0x80) != 0)
                {
                    /* It's only a match if we simultaneously hit the
                     * end of the search name. */
                    nameMatch = pSearchName == searchEnd ? -1 : 0;
                    break;
                }

                /* Next character. */
                pSearchName++;
                pDictName++;
            }
        }
        else
        {
            /* Get a reference to the FFI definition. */
            const EnforthFFIDef * ffi = *(EnforthFFIDef **)(curWord + 3);
            const char * ffiName = (char *)pgm_read_word(&ffi->name);

            /* See if this FFI matches our search word. */
            if ((strlen_P(ffiName) == searchLen)
                    && (strncasecmp_P(searchName, ffiName, searchLen) == 0))
            {
                nameMatch = -1;
            }
            else
            {
                nameMatch = 0;
            }
        }

        /* Is this a match?  If so, we're done. */
        if (nameMatch != 0)
        {
            *xt = 0x8000 | (uint16_t)(curWord - vm->dictionary);
            *isImmediate = *(curWord + 2) == kDefTypeIMMEDIATE ? 1 : -1;
            return -1;
        }
    }

    /* Search through the primitives for a match against this search
     * name. */
    const char * pPrimitives = kDefinitionNames;
    uint8_t token;
    for (token = 0; /* below */; ++token)
    {
        /* Get the length of this primitive; we're done if this is the
         * end value (0xff). */
        uint8_t primitiveFlags = (uint8_t)pgm_read_byte(pPrimitives++);
        if (primitiveFlags == 0xff)
        {
            break;
        }

        uint8_t primitiveLength = primitiveFlags & 0x7f;
        int primitiveIsImmediate = (primitiveFlags & 0x80) != 0 ? 1 : -1;

        /* Is this a match?  If so, return the XT. */
        if ((primitiveLength == searchLen)
            && (strncasecmp_P(searchName, pPrimitives, searchLen) == 0))
        {
            *xt = token;
            *isImmediate = primitiveIsImmediate;
            return -1;
        }

        /* No match; skip over the name. */
        pPrimitives += primitiveLength;
    }

    /* No match. */
    return 0;
}



/* -------------------------------------
 * Public functions.
 */

void enforth_init(
        EnforthVM * const vm,
        uint8_t * const dictionary, int dictionary_size,
        const EnforthFFIDef * const last_ffi,
        int (*keyq)(void), char (*key)(void), void (*emit)(char))
{
    vm->last_ffi = last_ffi;

    vm->keyq = keyq;
    vm->key = key;
    vm->emit = emit;

    vm->dictionary = dictionary;
    vm->dictionary_size = dictionary_size;

    vm->dp = vm->dictionary;
    vm->latest = NULL;

    vm->hld = NULL;

    vm->state = 0;
}

void enforth_add_definition(EnforthVM * const vm, const uint8_t * const def, int defSize)
{
    /* Get the address of the start of this definition and the address
     * of the start of the last definition. */
    const uint8_t * const prevLFA = vm->latest;
    uint8_t * newLFA = vm->dp;

    /* Add the LFA link. */
    if (vm->latest != NULL)
    {
        *vm->dp++ = ((newLFA - prevLFA)     ) & 0xff; /* LFAlo */
        *vm->dp++ = ((newLFA - prevLFA) >> 8) & 0xff; /* LFAhi */
    }
    else
    {
        *vm->dp++ = 0x00;
        *vm->dp++ = 0x00;
    }

    /* Copy the definition itself. */
    memcpy(vm->dp, def, defSize);
    vm->dp += defSize;

    /* Update latest. */
    vm->latest = newLFA;
}

void enforth_go(EnforthVM * const vm)
{
    register uint8_t *ip;
    register EnforthCell tos;
    register EnforthCell *restDataStack; /* Points at the second item on the stack. */
    register uint8_t *w;
    register EnforthCell *returnTop;

#ifdef __AVR__
    register int inProgramSpace;
#endif

#if ENABLE_STACK_CHECKING
    /* Check for available stack space and abort with a message if this
     * operation would run out of space. */
    /* TODO The overflow logic seems slightly too aggressive -- it
     * probably needs a "- 1" in there given that we store TOS in a
     * register. */
#define CHECK_STACK(numArgs, numResults) \
    { \
        if ((&vm->data_stack[32] - restDataStack) < numArgs) { \
            goto STACK_UNDERFLOW; \
        } else if (((&vm->data_stack[32] - restDataStack) - numArgs) + numResults > 32) { \
            goto STACK_OVERFLOW; \
        } \
    }
#else
#define CHECK_STACK(numArgs, numResults)
#endif

    /* Print the banner. */
    if (vm->emit != NULL)
    {
        vm->emit('H');
        vm->emit('i');
        vm->emit('\n');
    }

    static const void * const primitive_table[256] PROGMEM = {
        /* $00 - $07 */
            &&DOCOLONROM, /* SEMICOLON */
        &&DUP,
            &&DOCOLONROM, /* UDOT */
        &&DROP,

            &&DOCOLONROM, /* DOT */
        &&PLUS,
        &&MINUS,
        &&ONEPLUS,

        /* $08 - $0F */
        &&ONEMINUS,
            &&DOCOLONROM, /* CREATE */
        &&SWAP,
        &&ABORT,

        &&INVERT,
        &&LESSTHAN,
        &&EMIT,
        &&FETCH,

        /* $10 - $17 */
        &&UGREATERTHAN,
        &&OR,
        &&TWOSWAP,
            &&DOCOLONROM, /* COLON */

        &&QDUP,
        &&UMSTAR,
            &&DOCOLONROM, /* NUMSIGNS */
    0, /* UNUSED */

        /* $18 - $1F */
            &&DOCOLONROM, /* SIGN */
        &&STORE,
            &&DOCOLONROM, /* NUMSIGN */
    0, /* UNUSED */

        &&TWODROP,
        &&MPLUS,
        &&ZERO,
            &&DOCOLONROM, /* QUIT */

        /* $20 - $27 */
        &&ZEROEQUALS,
        &&BL,
        &&CFETCH,
        &&COUNT,

        &&DEPTH,
            &&DOCOLONROM, /* INTERPRET */
        &&BACKSLASH,
        &&HEX,

        /* $28 - $2F */
    0, /* UNUSED */
        &&TWODUP,
        &&COMMA,
        &&CCOMMA,

        &&TUCK,
        &&ALIGN,
        &&MOVE,
        &&CPLUSSTORE,

        /* $30 - $37 */
        &&ALLOT,
        &&NIP,
            &&DOCOLONROM, /* ACCEPT */
        &&WCOMMA,

        &&RTBRACKET,
        &&CSTORE,
        &&LTBRACKET,
        &&ABS,

        /* $38 - $3F */
        &&LESSNUMSIGN,
        &&ROT,
            &&DOCOLONROM, /* PARSEWORD */
        &&NUMSIGNGRTR,

        &&ZEROLESS,
        &&HOLD,
    0, /* UNUSED */
        &&UDSLASHMOD,

        /* $40 - $47 */
        &&GREATERTHAN,
        &&AND,
        &&NOTEQUALS,
        &&OVER,

        &&TWOOVER,
        &&KEY,
        &&TOR,
            &&DOCOLONROM, /* TYPE */

        /* $48 - $4F */
        &&RFROM,
        &&RFETCH,
        &&EQUALS,
            &&DOCOLONROM, /* SPACE */

            &&DOCOLONROM, /* CR */
            &&DOCOLONROM, /* TOCFA */
        &&TWOSTORE,
        &&SLASHSTRING,

        /* $50 - $57 */
            &&DOCOLONROM, /* TOBODY */
        &&PLUSSTORE,
        &&TWOFETCH,
        0,

        0, 0, 0,
            &&DOCOLONROM, /* TOKENQ */

        /* $58 - $5F */
        0,
            &&DOCOLONROM, /* COMPILECOMMA */
        0, 0,

        0, 0,
            &&DOCOLONROM, /* EXECUTE */
        0,

        /* $60 - $67 */
        0, 0, 0,
            &&DOCOLONROM, /* LITERAL */

        0, 0, 0, 0,

        /* $68 - $6F */
            &&DOCOLONROM, /* DIGITQ */
        0, 0, 0,

        0, 0, 0, 0,

        /* $70 - $77 */
        0, 0, 0,
            &&DOCOLONROM, /* TONUMBER */

        0, 0, 0, 0,

        /* $78 - $7F */
        0, 0, 0, 0,

            &&DOCOLONROM, /* NUMBERQ */
        0, 0,
            &&DOCOLONROM, /* SOURCE */

        /* $80 - $87 */
            &&DOCOLONROM, /* BASE */
            &&DOCOLONROM, /* HERE */
            &&DOCOLONROM, /* LATEST */
            &&DOCOLONROM, /* TIB */

            &&DOCOLONROM, /* TIBSIZE */
            &&DOCOLONROM, /* >IN */
            &&DOCOLONROM, /* STATE */
        0,

        /* $88 - $8F */
        0,0,0,0, 0,0,0,0,
        /* $90 - $97 */
        0,0,0,0, 0,0,0,0,
        /* $98 - $9F */
        0,0,0,0, 0,0,0,0,
        /* $A0 - $A7 */
        0,0,0,0, 0,0,0,0,
        /* $A8 - $AF */
        0,0,0,0, 0,0,0,0,
        /* $B0 - $B7 */
        0,0,0,0, 0,0,0,0,
        /* $B8 - $BF */
        0,0,0,0, 0,0,0,0,
        /* $C0 - $C7 */
        0,0,0,0, 0,0,0,0,

        /* $C8 - $CF */
        0, 0, 0, 0,

        0, 0,
        &&WLIT,
        &&PSQUOTE,

        /* $D0 - $D7 */
        &&CHARLIT,
        &&REVEAL,
        &&ZBRANCH,
    0, /* UNUSED */

        &&BRANCH,
        &&HIDE,
        &&INITRP,
    0, /* UNUSED */

        /* $D8 - $DF */
    0, /* UNUSED */
        &&PDOTQUOTE,
    0, /* UNUSED */
    0, /* UNUSED */

        &&FINDWORD,
        &&LIT,
        &&VMADDRLIT,
        &&PEXECUTE,

        /* $E0 - $EF */
        &&PDOCOLON,
        0, /* &&PDOIMMEDIATE, */
        0, /* &&PDOCONSTANT, */
        0, /* &&PDOCREATE, */

        0, /* &&PDODOES, */
        0, /* &&PDOVARIABLE, */
        &&PDOFFI0,
        &&PDOFFI1,

        &&PDOFFI2,
        0, /* &&PDOFFI3, */
        0, /* &&PDOFFI4, */
        0, /* &&PDOFFI5, */

        0, /* &&PDOFFI6, */
        0, /* &&PDOFFI7, */
        0, /* &&PDOFFI8, */
        0,

        /* $F0 - $FF */
        &&DOCOLON,
        0, /* &&DOIMMEDIATE, */
        0, /* &&DOCONSTANT, */
        0, /* &&DOCREATE, */

        0, /* &&DODOES, */
        0, /* &&DOVARIABLE, */
        &&DOFFI0,
        &&DOFFI1,

        &&DOFFI2,
        &&DOFFI3,
        &&DOFFI4,
        &&DOFFI5,

        &&DOFFI6,
        &&DOFFI7,
        &&DOFFI8,
        &&EXIT,
    };

    /* Initialize RP so that we can use threading to get to QUIT from
     * ABORT.  QUIT will immediately reset RP as well, of course. */
    returnTop = (EnforthCell *)&vm->return_stack[32];

    /* Jump to ABORT, which initializes the IP, our stacks, etc. */
    goto ABORT;

    /* The inner interpreter. */
    for (;;)
    {
        /* Get the next token and dispatch to the label. */
        uint8_t token;

#ifdef __AVR__
        if (inProgramSpace)
        {
            token = pgm_read_byte(ip++);
        }
        else
#endif
        {
            token = *ip++;
        }

DISPATCH_TOKEN:
        goto *(void *)pgm_read_word(&primitive_table[token]);

        DOFFI0:
        {
            CHECK_STACK(0, 1);

            /* The IP is pointing at the dictionary-relative offset of
             * the PFA of the FFI trampoline.  Convert that to a pointer
             * and store it in W. */
            w = (uint8_t*)(vm->dictionary + *(uint16_t*)ip);
            ip += 2;

        PDOFFI0:
            {
                /* W contains a pointer to the PFA of the FFI
                 * definition; get the FFI definition pointer and then
                 * use that to get the FFI function pointer. */
                ZeroArgFFI fn = (ZeroArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);

                *--restDataStack = tos;
                tos = (*fn)();
            }
        }
        continue;

        DOFFI1:
        {
            CHECK_STACK(1, 1);

            /* The IP is pointing at the dictionary-relative offset of
             * the PFA of the FFI trampoline.  Convert that to a pointer
             * and store it in W. */
            w = (uint8_t*)(vm->dictionary + *(uint16_t*)ip);
            ip += 2;

        PDOFFI1:
            {
                /* W contains a pointer to the PFA of the FFI
                 * definition; get the FFI definition pointer and then
                 * use that to get the FFI function pointer. */
                OneArgFFI fn = (OneArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);

                tos = (*fn)(tos);
            }
        }
        continue;

        DOFFI2:
        {
            CHECK_STACK(2, 1);

            /* The IP is pointing at the dictionary-relative offset of
             * the PFA of the FFI trampoline.  Convert that to a pointer
             * and store it in W. */
            w = (uint8_t*)(vm->dictionary + *(uint16_t*)ip);
            ip += 2;

        PDOFFI2:
            {
                TwoArgFFI fn = (TwoArgFFI)pgm_read_word(&(*(EnforthFFIDef**)w)->fn);

                EnforthCell arg2 = tos;
                EnforthCell arg1 = *restDataStack++;
                tos = (*fn)(arg1, arg2);
            }
        }
        continue;

        DOFFI3:
            CHECK_STACK(3, 1);
        continue;

        DOFFI4:
            CHECK_STACK(4, 1);
        continue;

        DOFFI5:
            CHECK_STACK(5, 1);
        continue;

        DOFFI6:
            CHECK_STACK(6, 1);
        continue;

        DOFFI7:
            CHECK_STACK(7, 1);
        continue;

        DOFFI8:
            CHECK_STACK(8, 1);
        continue;

        LIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
#ifdef __AVR__
            if (inProgramSpace)
            {
                tos.i = pgm_read_word(ip);
            }
            else
#endif
            {
                tos = *(EnforthCell*)ip;
            }

            ip += kEnforthCellSize;
        }
        continue;

        DUP:
        {
            CHECK_STACK(1, 2);
            *--restDataStack = tos;
        }
        continue;

        DROP:
        {
            CHECK_STACK(1, 0);
            tos = *restDataStack++;
        }
        continue;

        PLUS:
        {
            CHECK_STACK(2, 1);
            tos.i += restDataStack++->i;
        }
        continue;

        MINUS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i - tos.i;
        }
        continue;

        ONEPLUS:
        {
            CHECK_STACK(1, 1);
            tos.i++;
        }
        continue;

        ONEMINUS:
        {
            CHECK_STACK(1, 1);
            tos.i--;
        }
        continue;

        SWAP:
        {
            CHECK_STACK(2, 2);
            EnforthCell swap = restDataStack[0];
            restDataStack[0] = tos;
            tos = swap;
        }
        continue;

        /* TODO Add docs re: Note that (branch) and (0branch) offsets
         * are 8-bit relative offsets.  UNLIKE word addresses, the IP
         * points at the offset in BRANCH/ZBRANCH.  These offsets can be
         * positive or negative because branches can go both forwards
         * and backwards. */
        BRANCH:
        {
            CHECK_STACK(0, 0);

            /* Relative, because these are entirely within a single word
             * and so we want it to be relocatable without us having to
             * do anything.  Note that the offset cannot be larger than
             * +/- 127 bytes! */
            int8_t relativeOffset;
#ifdef __AVR__
            if (inProgramSpace)
            {
                relativeOffset = pgm_read_byte(ip);
            }
            else
#endif
            {
                relativeOffset = *(int8_t*)ip;
            }

            ip = ip + relativeOffset;
        }
        continue;

#if ENABLE_STACK_CHECKING
        STACK_OVERFLOW:
        {
            if (vm->emit != NULL)
            {
                vm->emit('\n');
                vm->emit('!');
                vm->emit('O');
                vm->emit('V');
                vm->emit('\n');
            }
            goto ABORT;
        }
        continue;

        STACK_UNDERFLOW:
        {
            if (vm->emit != NULL)
            {
                vm->emit('\n');
                vm->emit('!');
                vm->emit('U');
                vm->emit('N');
                vm->emit('\n');
            }
            goto ABORT;
        }
        continue;
#endif
        /* -------------------------------------------------------------
         * ABORT [CORE] 6.1.0670 ( i*x -- ) ( R: j*x -- )
         *
         * Empty the data stack and perform the function of QUIT, which
         * includes emptying the return stack, without displaying a
         * message. */
        ABORT:
        {
            tos.i = 0;
            restDataStack = (EnforthCell*)&vm->data_stack[32];
            vm->base = 10;
            token = QUIT;
            goto DISPATCH_TOKEN;
        }
        continue;

        CHARLIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
#ifdef __AVR__
            if (inProgramSpace)
            {
                tos.i = pgm_read_byte(ip);
            }
            else
#endif
            {
                tos.i = *ip;
            }

            ip++;
        }
        continue;

        EMIT:
        {
            CHECK_STACK(1, 0);

            if (vm->emit != NULL)
            {
                vm->emit(tos.i);
            }

            tos = *restDataStack++;
        }
        continue;

        PEXECUTE:
        {
            w = tos.ram;
            token = restDataStack++->u;
            tos = *restDataStack++;
            goto DISPATCH_TOKEN;
        }
        continue;

        FETCH:
        {
            CHECK_STACK(1, 1);
            tos = *(EnforthCell*)tos.ram;
        }
        continue;

        OR:
        {
            CHECK_STACK(2, 1);
            tos.i |= restDataStack++->i;
        }
        continue;

        /* -------------------------------------------------------------
         * FIND-WORD [ENFORTH] "paren-find-paren" ( c-addr u -- c-addr u 0 | xt 1 | xt -1 )
         *
         * Find the definition named in the string at c-addr with length
         * u in the word list whose latest definition is pointed to by
         * nfa.  If the definition is not found, return the string and
         * zero.  If the definition is found, return its execution token
         * xt.  If the definition is immediate, also return one (1),
         * otherwise also return minus-one (-1).  For a given string,
         * the values returned by FIND-WORD while compiling may differ
         * from those returned while not compiling. */
        FINDWORD:
        {
            CHECK_STACK(2, 3);

            uint8_t * caddr = (uint8_t *)restDataStack->ram;
            EnforthUnsigned u = tos.u;

            EnforthXT xt;
            int isImmediate;
            if (enforth_paren_find_word(vm, caddr, u, &xt, &isImmediate) == -1)
            {
                /* Stack still contains c-addr u; rewrite to xt 1|-1. */
                restDataStack->u = xt;
                tos.i = isImmediate;
            }
            else
            {
                /* Stack still contains c-addr u, so just push 0. */
                *--restDataStack = tos;
                tos.i = 0;
            }
        }
        continue;

        QDUP:
        {
            CHECK_STACK(1, 2);

            if (tos.i != 0)
            {
                *--restDataStack = tos;
            }
        }
        continue;

        /* -------------------------------------------------------------
         * ! [CORE] 6.1.0010 "store" ( x a-addr -- )
         *
         * Store x at a-addr. */
        STORE:
        {
            CHECK_STACK(2, 0);
            *(EnforthCell*)tos.ram = *restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        TWODROP:
        {
            CHECK_STACK(2, 0);
            restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        ZBRANCH:
        {
            CHECK_STACK(1, 0);

            if (tos.i == 0)
            {
                int8_t relativeOffset;
#ifdef __AVR__
                if (inProgramSpace)
                {
                    relativeOffset = pgm_read_byte(ip);
                }
                else
#endif
                {
                    relativeOffset = *(int8_t*)ip;
                }

                ip = ip + relativeOffset;
            }
            else
            {
                ip++;
            }

            tos = *restDataStack++;
        }
        continue;

        ZERO:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = 0;
        }
        continue;

        ZEROEQUALS:
        {
            CHECK_STACK(1, 1);
            tos.i = tos.i == 0 ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
         * (s") [ENFORTH] "paren-s-quote-paren" ( -- c-addr u )
         *
         * Runtime behavior of S": return c-addr and u.
         * NOTE: Cannot be used in program space! */
        PSQUOTE:
        {
            CHECK_STACK(0, 2);

            /* Push existing TOS onto the stack. */
            *--restDataStack = tos;

            /* Instruction stream contains the length of the string as a
             * byte and then the string itself.  Start out by pushing
             * the address of the string (ip+1) onto the stack. */
            (--restDataStack)->ram = ip + 1;

            /* Now get the string length into TOS. */
            tos.i = *ip++;

            /* Advance IP over the string. */
            ip = ip + tos.i;
        }
        continue;

        BL:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.u = ' ';
        }
        continue;

        CFETCH:
        {
            CHECK_STACK(1, 1);
            tos.u = *(uint8_t*)tos.ram;
        }
        continue;

        COUNT:
        {
            CHECK_STACK(1, 2);
            (--restDataStack)->ram = (uint8_t*)tos.ram + 1;
            tos.u = *(uint8_t*)tos.ram;
        }
        continue;

        /* -------------------------------------------------------------
         * DEPTH [CORE] 6.1.1200 ( -- +n )
         *
         * +n is the number of single-cell values contained in the data
         * stack before +n was placed on the stack. */
        DEPTH:
        {
            CHECK_STACK(0, 1);

            /* Save TOS, then calculate the stack depth.  The return
             * value should be the number of items on the stack *before*
             * DEPTH was called and so we have to subtract one from the
             * count given that we calculate the depth *after* pushing
             * the old TOS onto the stack. */
            *--restDataStack = tos;
            tos.i = &vm->data_stack[32] - restDataStack - 1;
        }
        continue;

        /* -------------------------------------------------------------
         * (.") [ENFORTH] "paren-dot-quote-paren" ( -- )
         *
         * Prints the string that was compiled into the definition. */
        PDOTQUOTE:
        {
            /* Instruction stream contains the length of the string as a
             * byte and then the string itself.  Get and skip over both
             * values */
            uint8_t * caddr;
            uint8_t u;

#ifdef __AVR__
            if (inProgramSpace)
            {
                u = pgm_read_byte(ip);
                ip++;
            }
            else
#endif
            {
                u = *ip++;
            }

            caddr = ip;

            /* Advance IP over the string. */
            ip = ip + u;

            /* Print out the string. */
            if (vm->emit != NULL)
            {
                int i;
                for (i = 0; i < u; i++)
                {
                    uint8_t ch;
#ifdef __AVR__
                    if (inProgramSpace)
                    {
                        ch = pgm_read_byte(caddr);
                    }
                    else
#endif
                    {
                        ch = *caddr;
                    }

                    vm->emit(ch);

                    caddr++;
                }
            }
        }
        continue;

        /* -------------------------------------------------------------
         * \ [CORE-EXT] 6.2.2535 "backslash"
         *
         * Compilation:
         *   Perform the execution semantics given below.
         *
         * Execution: ( "ccc<eol>" -- )
         *   Parse and discard the remainder of the parse area.  \ is an
         *   immediate word. */
        BACKSLASH:
        {
            vm->to_in = vm->source_len.i;
        }
        continue;

        /* -------------------------------------------------------------
         * HEX [CORE EXT] 6.2.1660 ( -- )
         *
         * Set contents of BASE to sixteen. */
        HEX:
        {
            vm->base = 16;
        }
        continue;

        TWODUP:
        {
            CHECK_STACK(2, 4);
            EnforthCell second = *restDataStack;
            *--restDataStack = tos;
            *--restDataStack = second;
        }
        continue;

        COMMA:
        {
            CHECK_STACK(1, 0);
            *((EnforthCell*)vm->dp) = tos;
            vm->dp += kEnforthCellSize;
            tos = *restDataStack++;
        }
        continue;

        CCOMMA:
        {
            CHECK_STACK(1, 0);
            *vm->dp++ = tos.u & 0xff;
            tos = *restDataStack++;
        }
        continue;

        TUCK:
        {
            EnforthCell second = *restDataStack;
            *restDataStack = tos;
            *--restDataStack = second;
        }
        continue;

        ALIGN:
        {
            /* No alignment necessary (unless we want to expand the
             * range of the dictionary-relative offsets at some point). */
        }
        continue;

        MOVE:
        {
            CHECK_STACK(3, 0);
            EnforthCell arg3 = tos;
            EnforthCell arg2 = *restDataStack++;
            EnforthCell arg1 = *restDataStack++;
            memcpy(arg2.ram, arg1.ram, arg3.u);
            tos = *restDataStack++;
        }
        continue;

        CPLUSSTORE:
        {
            CHECK_STACK(2, 0);
            uint8_t c = *(uint8_t*)tos.ram;
            c += (restDataStack++)->u & 0xff;
            *(uint8_t*)tos.ram = c;
            tos = *restDataStack++;
        }
        continue;

        ALLOT:
        {
            CHECK_STACK(1, 0);
            vm->dp += tos.u;
            tos = *restDataStack++;
        }
        continue;

        NIP:
        {
            CHECK_STACK(2, 1);
            restDataStack++;
        }
        continue;

        WCOMMA:
        {
            CHECK_STACK(1, 0);
            *((uint16_t*)vm->dp) = (uint16_t)(tos.u & 0xffff);
            vm->dp += 2;
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
         * HIDE [ENFORTH] ( -- )
         *
         * Prevent the most recent definition from being found in the
         * dictionary. */
        HIDE:
        {
            *(vm->latest + 2) |= 0x80;
        }
        continue;

        /* -------------------------------------------------------------
         * ] [CORE] 6.1.2540 "right-bracket" ( -- )
         *
         * Enter compilation state. */
        RTBRACKET:
        {
            vm->state = -1;
        }
        continue;

        /* -------------------------------------------------------------
         * C! [CORE] 6.1.0850 "c-store" ( char c-addr -- )
         *
         * Store char at c-addr.  When character size is smaller than
         * cell size, only the number of low-order bits corresponding to
         * character size are transferred. */
        CSTORE:
        {
            CHECK_STACK(2, 0);
            *(uint8_t*)tos.ram = restDataStack++->u;
            tos = *restDataStack++;
        }
        continue;

        REVEAL:
        {
            *(vm->latest + 2) &= 0x7f;
        }
        continue;

        LTBRACKET:
        {
            vm->state = 0;
        }
        continue;

        ABS:
        {
            CHECK_STACK(1, 1);
            tos.i = abs(tos.i);
        }
        continue;

        LESSNUMSIGN:
        {
            vm->hld = vm->dp + (kEnforthCellSize*8*3);
        }
        continue;

        /* -------------------------------------------------------------
         * ROT [CORE] 6.1.2160 "rote" ( x1 x2 x3 -- x2 x3 x1 )
         *
         * Rotate the top three stack entries. */
        ROT:
        {
            CHECK_STACK(3, 3);
            EnforthCell x3 = tos;
            EnforthCell x2 = *restDataStack++;
            EnforthCell x1 = *restDataStack++;
            *--restDataStack = x2;
            *--restDataStack = x3;
            tos = x1;
        }
        continue;

        /* -------------------------------------------------------------
         * #> [CORE] 6.1.0040 "number-sign-greater" ( xd -- c-addr u )
         *
         * Drop xd.  Make the pictured numeric output string available
         * as a character string.  c-addr and u specify the resulting
         * character string.  A program may replace characters within
         * the string.
         *
         * ---
         * : #> ( xd -- c-addr u ) DROP DROP  HLD @  HERE HLDEND +  OVER - ;
         */
        NUMSIGNGRTR:
        {
            restDataStack->ram = vm->hld;
            tos.u = (vm->dp + (kEnforthCellSize*8*3)) - vm->hld;
        }
        continue;

        /* -------------------------------------------------------------
         * 0< [CORE] 6.1.0250 "zero-less" ( b -- flag )
         *
         * flag is true if and only if n is less than zero. */
        ZEROLESS:
        {
            CHECK_STACK(1, 0);
            tos.i = tos.i < 0 ? -1 : 0;
        }
        continue;

        /* -------------------------------------------------------------
         * HOLD [CORE] 6.1.1670 ( char -- )
         *
         * Add char to the beginning of the pictured numeric output
         * string.  An ambiguous condition exists if HOLD executes
         * outside of a <# #> delimited number conversion. */
        HOLD:
        {
            CHECK_STACK(1, 0);
            *--vm->hld = tos.u;
            tos = *restDataStack++;
        }
        continue;

        /* -------------------------------------------------------------
         * UD/MOD [ENFORTH] "u-d-slash-mod" ( ud1 u1 -- n ud2 )
         *
         * Divide ud1 by u1 giving the quotient ud2 and the remainder n. */
        UDSLASHMOD:
        {
            CHECK_STACK(3, 3);
#ifdef __AVR__
            uint16_t u1 = tos.u;
            uint16_t ud1_msb = restDataStack++->u;
            uint16_t ud1_lsb = restDataStack++->u;
            uint32_t ud1 = ((uint32_t)ud1_msb << 16) | ud1_lsb;
            ldiv_t result = ldiv(ud1, u1);
            (--restDataStack)->u = result.rem;
            (--restDataStack)->u = result.quot;
            tos.u = (uint16_t)(result.quot >> 16);
#else
            uint32_t u1 = tos.u;
            uint32_t ud1_msb = restDataStack++->u;
            uint32_t ud1_lsb = restDataStack++->u;
            uint64_t ud1 = ((uint64_t)ud1_msb << 32) | ud1_lsb;
            lldiv_t result = lldiv(ud1, u1);
            (--restDataStack)->u = result.rem;
            (--restDataStack)->u = result.quot;
            tos.u = (uint32_t)(result.quot >> 32);
#endif
        }
        continue;

        /* -------------------------------------------------------------
         * > [CORE] 6.1.0540 "greater-than" ( n1 n2 -- flag )
         *
         * flag is true if and only if n1 is greater than n2. */
        GREATERTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i > tos.i ? -1 : 0;
        }
        continue;

        UGREATERTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->u > tos.u ? -1 : 0;
        }
        continue;

        AND:
        {
            CHECK_STACK(2, 1);
            tos.i &= restDataStack++->i;
        }
        continue;

        NOTEQUALS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i != tos.i ? -1 : 0;
        }
        continue;

        INITRP:
        {
            CHECK_STACK(0, 0);
            returnTop = (EnforthCell *)&vm->return_stack[32];
        }
        continue;

        OVER:
        {
            CHECK_STACK(2, 3);
            EnforthCell second = restDataStack[0];
            *--restDataStack = tos;
            tos = second;
        }
        continue;

        TWOOVER:
        {
            CHECK_STACK(4, 6);
            *--restDataStack = tos;
            EnforthCell x2 = restDataStack[2];
            EnforthCell x1 = restDataStack[3];
            *--restDataStack = x1;
            tos = x2;
        }
        continue;

        KEY:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = vm->key();
        }
        continue;

        TOR:
        {
            CHECK_STACK(1, 0);
            *--returnTop = tos;
            tos = *restDataStack++;
        }
        continue;

        RFROM:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos = *returnTop++;
        }
        continue;

        RFETCH:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos = returnTop[0];
        }
        continue;

        EQUALS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i == tos.i ? -1 : 0;
        }
        continue;

        SLASHSTRING:
        {
            CHECK_STACK(3, 2);
            EnforthCell adjust = tos;
            tos.u = restDataStack++->u - adjust.i;
            restDataStack[0].u += adjust.i;
        }
        continue;

        PLUSSTORE:
        {
            CHECK_STACK(2, 0);
            ((EnforthCell*)tos.ram)->i += restDataStack++->i;
            tos = *restDataStack++;
        }
        continue;

        VMADDRLIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;

#ifdef __AVR__
            if (inProgramSpace)
            {
                tos.ram = (uint8_t*)vm + (uint8_t)pgm_read_byte(ip);
            }
            else
#endif
            {
                tos.ram = (uint8_t*)vm + (uint8_t)*ip;
            }

            ip++;
        }
        continue;

        LESSTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i < tos.i ? -1 : 0;
        }
        continue;

        INVERT:
        {
            CHECK_STACK(1, 1);
            tos.i = ~tos.i;
        }
        continue;

        TWOSWAP:
        {
            CHECK_STACK(4, 4);
            EnforthCell x4 = tos;
            EnforthCell x3 = restDataStack[0];
            EnforthCell x2 = restDataStack[1];
            EnforthCell x1 = restDataStack[2];

            tos = x2;
            restDataStack[0] = x1;
            restDataStack[1] = x4;
            restDataStack[2] = x3;
        }
        continue;

        /* -------------------------------------------------------------
         * UM* [CORE] 6.1.2360 "u-m-star" ( u1 u2 -- ud )
         *
         * Multiply u1 by u2, giving the unsigned double-cell product
         * ud.  All values and arithmetic are unsigned */
        UMSTAR:
        {
            CHECK_STACK(2, 2);
#ifdef __AVR__
            uint32_t result = (uint32_t)tos.u * (uint32_t)restDataStack[0].u;
            restDataStack[0].u = (uint16_t)result;
            tos.u = (uint16_t)(result >> 16);
#else
            uint64_t result = (uint64_t)tos.u * (uint64_t)restDataStack[0].u;
            restDataStack[0].u = (uint32_t)result;
            tos.u = (uint32_t)(result >> 32);
#endif
        }
        continue;

        /* -------------------------------------------------------------
         * M+ [DOUBLE] 8.6.1.1830 "m-plus" ( d1|ud1 n -- d2|ud2 )
         *
         * Add n to d1|ud1, giving the sum d2|ud2. */
        MPLUS:
        {
            CHECK_STACK(3, 2);
#ifdef __AVR__
            int16_t n = tos.i;
            int16_t d1_msb = restDataStack++->i;
            int16_t d1_lsb = restDataStack++->i;
            int32_t ud1 = ((uint32_t)d1_msb << 16) | d1_lsb;
            int32_t result = ud1 + n;
            (--restDataStack)->i = (int16_t)result;
            tos.i = (int16_t)(result >> 16);
#else
            int32_t n = tos.i;
            int32_t d1_msb = restDataStack++->i;
            int32_t d1_lsb = restDataStack++->i;
            int64_t ud1 = ((uint64_t)d1_msb << 32) | d1_lsb;
            int64_t result = ud1 + n;
            (--restDataStack)->i = (int32_t)result;
            tos.i = (int32_t)(result >> 32);
#endif
        }
        continue;

        WLIT:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
#ifdef __AVR__
            if (inProgramSpace)
            {
                tos.i = pgm_read_word(ip);
            }
            else
#endif
            {
                tos.i = (EnforthInt)*(uint16_t*)ip;
            }

            ip += 2;
        }
        continue;

        TWOFETCH:
        {
            CHECK_STACK(1, 2);
            *--restDataStack = *(EnforthCell*)(tos.ram + kEnforthCellSize);
            tos = *(EnforthCell*)tos.ram;
        }
        continue;

        TWOSTORE:
        {
            CHECK_STACK(3, 0);
            EnforthCell * addr = (EnforthCell*)tos.ram;
            *addr++ = *restDataStack++;
            *addr = *restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        DOCOLON:
        {
            /* IP currently points to the relative offset of the PFA of
             * the target word.  Read that offset and advance IP to the
             * token after the offset. */
            w = vm->dictionary + *(uint16_t*)ip;
            ip += 2;

        PDOCOLON:
            /* IP now points to the next word in the PFA and that is the
             * location to which we should return once this new word has
             * executed. */
#ifdef __AVR__
            if (inProgramSpace)
            {
                /* TODO Needs to be relative to the ROM definition
                 * block. */
                ip = (uint8_t*)((unsigned int)ip | 0x8000);

                /* We are no longer in program space since, by design,
                 * DOCOLON is only ever used for user-defined words in
                 * RAM. */
                inProgramSpace = 0;
            }
#endif
            (--returnTop)->ram = (void *)ip;

            /* Now set the IP to the PFA of the word that is being
             * called and continue execution inside of that word. */
            ip = w;
        }
        continue;

        DOCOLONROM:
        {
            /* Push IP to the return stack, marking IP as
             * in-program-space as necessary. */
#ifdef __AVR__
            if (inProgramSpace)
            {
                ip = (uint8_t*)((unsigned int)ip | 0x8000);
            }
#endif
            (--returnTop)->ram = (void *)ip;

            /* Calculate the offset of the definition and set the IP to
             * the absolute address. */
            int definitionOffset = token << 2;
            ip = (uint8_t*)definitions + definitionOffset;
#ifdef __AVR__
            inProgramSpace = -1;
#endif
        }
        continue;

        EXIT:
        {
            ip = (uint8_t *)((returnTop++)->ram);

#ifdef __AVR__
            if (((unsigned int)ip & 0x8000) != 0)
            {
                /* TODO Needs to be relative to the ROM definition
                 * block. */
                ip = (uint8_t*)((unsigned int)ip & 0x7FFF);
                inProgramSpace = -1;
            }
            else
            {
                inProgramSpace = 0;
            }
#endif
        }
        continue;
    }
}
