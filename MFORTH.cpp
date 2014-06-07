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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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

#include "MFORTH.h"

// TODO Should move all of the internal words (BRANCH, CHARLIT, etc.) to
// the end of the opcode table so that they don't need to waste space in
// the primitive list.
// A length with a high bit set indicates that the word is immediate.
static const char primitives[] PROGMEM =
    // $00 - $07
    "\x00"
    "\x00" // LIT
    "\x03" "DUP"
    "\x04" "DROP"

    "\x01" "+"
    "\x01" "-"
    "\x02" "1+"
    "\x02" "1-"

    // $08 - $0F
    "\x04" "SWAP"
    "\x00" // BRANCH
    "\x05" "ABORT"
    "\x00" // CHARLIT

    "\x08" "COMPILE,"
    "\x02" "CR"
    "\x04" "EMIT"
    "\x07" "EXECUTE"

    // $10 - $17
    "\x01" "@"
    "\x07" "LITERAL"
    "\x07" "NUMBER?"
    "\x02" "OR"

    "\x00" // PARSE-WORD
    "\x00" // FIND-WORD
    "\x04" "?DUP"
    "\x05" "SPACE"

    // $18 - $1F
    "\x05" "STATE"
    "\x01" "!"
    "\x03" ">IN"
    "\x05" "2DROP"

    "\x04" "TYPE"
    "\x00" // ZBRANCH
    "\x01" "0"
    "\x02" "0="

    // $20 - $27
    "\x00" // UNUSED
    "\x00" // TIB
    "\x00" // TIBSIZE
    "\x06" "ACCEPT"

    "\x00" // UNUSED
    "\x00" // (S")
    "\x02" "BL"
    "\x02" "C@"

    // $28 - $2F
    "\x05" "COUNT"
    "\x07" ">NUMBER"
    "\x05" "DEPTH"
    "\x00" // UNUSED

    "\x00" // (.")
    "\x01" "\\"
    "\x03" "HEX"
    "\x00" // UNUSED

    // $30 - $37
    "\x04" "HERE"
    "\x00" // LATEST
    "\x04" "2DUP"
    "\x01" ","

    "\x02" "C,"
    "\x04" "TUCK"
    "\x05" "ALIGN"
    "\x04" "MOVE"

    // $38 - $3F
    "\x03" "C+!"
    "\x05" "ALLOT"
    "\x03" "NIP"
    "\x02" "W,"

    "\x00" // UNUSED
    "\x00" // HIDE
    "\x01" "]"
    "\x02" "C!"

    // $40 - $47
    "\x00" // UNUSED
    "\x00" // REVEAL
    "\x81" "["
    "\x03" "ABS"

    "\x02" "<#"
    "\x00" // UNUSED
    "\x03" "ROT"
    "\x04" "SIGN"

    // $48 - $4F
    "\x02" "#>"
    "\x01" "#"
    "\x02" "0<"
    "\x04" "HOLD"

    "\x04" "BASE"
    "\x06" "UD/MOD"
    "\x01" ">"
    "\x03" "AND"

    // $50 - $57
    "\x02" "<>"
    "\x00"
    "\x00" // INITRP
    "\x00" // TICKSOURCE

    "\x00" // TICKSOURCELEN
    "\x00" "\x00" "\x00"

    // $58 - $5F
    "\x00" "\x00" "\x00" "\x00"
    "\x00" "\x00" "\x00" "\x00"

    // $60 - $67
    "\x00" "\x00" "\x00" "\x00"
    "\x00" "\x00" "\x00" "\x00"

    // $68 - $6F
    "\x00" "\x00" "\x00" "\x00"
    "\x00" "\x00" "\x00" "\x00"

    // $70 - $77
    "\x00" "\x00" "\x00" "\x00"
    "\x00" "\x00" "\x00" "\x00"

    // $78 - $7F
    "\x00" "\x00" "\x00" "\x00"
    "\x00" "\x00" "\x00" "\x00"

    // $80 - $87
    "\x81" ";"
    "\x00" // UNUSED
    "\x02" "U."
    "\x00" // UNUSED
    "\x01" "."
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x06" "CREATE"
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x01" ":"
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x02" "#S"
    "\x00" // UNUSED
    "\x04" "SIGN"
    "\x00" // UNUSED
    "\x01" "#"
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x04" "QUIT"
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // INTERPRET
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED
    "\x00" // UNUSED

    // End byte
    "\xff"
;

MFORTH::MFORTH(uint8_t * const dictionary, int dictionarySize,
        const FFIDef * const lastFFI,
        KeyQuestion keyQ, Key key, Emit emit)
 : lastFFI(lastFFI),
   keyQ(keyQ), key(key), emit(emit),
   dictionary(dictionary), dictionarySize(dictionarySize),
   dp(dictionary), latest(NULL), hld(NULL),
   state(0),
   toIn(0), prevLeave(NULL)
{
}

void MFORTH::addDefinition(const uint8_t * const def, int defSize)
{
    // Get the address of the start of this definition and the address
    // of the start of the last definition.
    const uint8_t * const prevLFA = this->latest;
    uint8_t * newLFA = this->dp;

    // Add the LFA link.
    if (this->latest != NULL)
    {
        *this->dp++ = ((newLFA - prevLFA)     ) & 0xff; // LFAlo
        *this->dp++ = ((newLFA - prevLFA) >> 8) & 0xff; // LFAhi
    }
    else
    {
        *this->dp++ = 0x00;
        *this->dp++ = 0x00;
    }

    // Copy the definition itself.
    memcpy(this->dp, def, defSize);
    this->dp += defSize;

    // Update latest.
    this->latest = newLFA;
}

MFORTH::Unsigned MFORTH::parenAccept(uint8_t * caddr, MFORTH::Unsigned n1)
{
    char * p = (char *)caddr;
    Unsigned n2 = 0;
    char ch;

    // Read characters until we get a linefeed.
    while ((ch = this->key()) != '\n')
    {
        // Is our buffer full?  If so, ignore this character.
        if (n2 == n1)
        {
            continue;
        }

        // TODO Deal with backspace.

        // Emit the character.
        this->emit(ch);

        // Append the character to our buffer.
        *p++ = ch;
        n2++;
    }

    // Return the number of characters that were read.
    return n2;
}

bool MFORTH::parenFindWord(uint8_t * caddr, MFORTH::Unsigned u, XT &xt, bool &isImmediate)
{
    int searchLen = u;
    char * searchName = (char *)caddr;

    // Search the dictionary.
    for (uint8_t * curWord = this->latest;
         curWord != NULL;
         curWord = *(uint16_t *)curWord == 0 ? NULL : curWord - *(uint16_t *)curWord)
    {
        // Get the definition type.
        uint8_t definitionType = *(curWord + 2);

        // Ignore smudged words.
        if ((definitionType & 0x80) != 0)
        {
            continue;
        }

        // Compare the strings, which happens differently depending on
        // if this is a user-defined word or an FFI trampoline.
        bool nameMatch;
        if (definitionType < CFADOFFI0)
        {
            char * pSearchName = searchName;
            char * searchEnd = searchName + searchLen-1;
            char * pDictName = (char *)(curWord + 3);
            while (true)
            {
                // Try to match the characters and fail if they don't match.
                if (toupper(*pSearchName) != toupper(*pDictName & 0x7f))
                {
                    nameMatch = false;
                    break;
                }

                // We're done if this was the last character.
                if ((*pDictName & 0x80) != 0)
                {
                    // It's only a match if we simultaneously hit the
                    // end of the search name.
                    nameMatch = pSearchName == searchEnd;
                    break;
                }

                // Next character.
                pSearchName++;
                pDictName++;
            }
        }
        else
        {
            // Get a reference to the FFI definition.
            const FFIDef * ffi = *(FFIDef **)(curWord + 3);
            const char * ffiName = (char *)pgm_read_word(&ffi->name);

            // See if this FFI matches our search word.
            nameMatch
                = (strlen_P(ffiName) == searchLen)
                && (strncasecmp_P(searchName, ffiName, searchLen) == 0);
        }

        // Is this a match?  If so, we're done.
        if (nameMatch)
        {
            xt = 0x8000 | (uint16_t)(curWord - this->dictionary);
            isImmediate = *(curWord + 2) == CFADOIMMEDIATE;
            return true;
        }
    }

    // Search through the opcodes for a match against this search name.
    const char * pPrimitives = primitives;
    for (uint8_t opcode = 0; /* below */; ++opcode)
    {
        // Get the length of this primitive; we're done if this is the
        // end value (0xff).
        uint8_t primitiveFlags = (uint8_t)pgm_read_byte(pPrimitives++);
        if (primitiveFlags == 0xff)
        {
            break;
        }

        uint8_t primitiveLength = primitiveFlags & 0x7f;
        bool primitiveIsImmediate = (primitiveFlags & 0x80) != 0;

        // Is this a match?  If so, return the XT.
        if ((primitiveLength == searchLen)
            && (strncasecmp_P(searchName, pPrimitives, searchLen) == 0))
        {
            xt = opcode;
            isImmediate = primitiveIsImmediate;
            return true;
        }

        // No match; skip over the name.
        pPrimitives += primitiveLength;
    }

    // No match.
    return false;
}

// NUMBER? [MFORTH] "number-question" ( c-addr u -- c-addr u 0 | n -1 )
//
// Attempt to convert a string at c-addr of length u into digits, using
// the radix in BASE.  The number and -1 is returned if the conversion
// was successful, otherwise 0 is returned.
bool MFORTH::parenNumberQ(uint8_t * caddr, MFORTH::Unsigned u, MFORTH::Int &n)
{
    // Store the sign as a value to be multipled against the final
    // number.
    Int sign = 1;
    if (*caddr == '-')
    {
        sign = -1;
        caddr++;
        u--;
    }

    // Try to convert the (rest of, if we found a sign) number.
    Unsigned ud = 0;
    this->parenToNumber(ud, caddr, u);
    if (u == 0)
    {
        n = (Int)ud * sign;
        return true;
    }
    else
    {
        return false;
    }
}

// >NUMBER [CORE] 6.1.0567 "to-number" ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )
//
// ud2 is the unsigned result of converting the characters within the
// string specified by c-addr1 u1 into digits, using the number in BASE,
// and adding each into ud1 after multiplying ud1 by the number in BASE.
// Conversion continues left-to-right until a character that is not
// convertible, including any "+" or "-", is encountered or the string
// is entirely converted.  c-addr2 is the location of the first
// unconverted character or the first character past the end of the
// string if the string was entirely converted.  u2 is the number of
// unconverted characters in the string.  An ambiguous condition exists
// if ud2 overflows during the conversion.
void MFORTH::parenToNumber(MFORTH::Unsigned &ud, uint8_t * &caddr, MFORTH::Unsigned &u)
{
    while (u > 0)
    {
        char ch = *(char *)caddr;
        if (ch < '0')
        {
            break;
        }

        Unsigned digit = ch - '0';
        if (digit > 9)
        {
            digit = tolower(ch) - 'a' + 10;
        }

        if (digit >= this->base)
        {
            break;
        }

        ud = (ud * this->base) + digit;
        caddr++;
        u--;
    }
}

void MFORTH::parenParseWord(uint8_t delim, uint8_t * &caddr, MFORTH::Unsigned &u)
{
    // Skip over the start of the string until we find a non-delimiter
    // character or we hit the end of the parse area.
    while ((*((uint8_t*)this->source.pRAM + this->toIn) == delim)
            && (this->toIn < this->sourceLen.i))
    {
        this->toIn++;
    }

    // We're either pointing to a non-delimiter character or we're at
    // the end of the parse area; point caddr at the current location
    // and then scan forwards until we hit another delimiter or we
    // exhaust the parse area.
    caddr = (uint8_t*)this->source.pRAM + this->toIn;
    char * pParse = (char *)caddr;
    while ((*pParse != delim) && (this->toIn < this->sourceLen.i))
    {
        pParse++;
        this->toIn++;
    }

    // Set the string length.
    u = (uint8_t*)pParse - caddr;
}

void MFORTH::go()
{
    register uint8_t *ip;
    register Cell tos;
    register Cell *restDataStack; // Points at the second item on the stack.
    register uint8_t *w;
    register Cell *returnTop;

#ifdef __AVR__
    register bool inProgramSpace;
#endif

#if ENABLE_STACK_CHECKING
    // Check for available stack space and abort with a message if this
    // operation would run out of space.
    // TODO The overflow logic seems slightly too aggressive -- it
    // probably needs a "- 1" in there given that we store TOS in a
    // register.
#define CHECK_STACK(numArgs, numResults) \
    { \
        if ((&this->dataStack[32] - restDataStack) < numArgs) { \
            goto STACK_UNDERFLOW; \
        } else if (((&this->dataStack[32] - restDataStack) - numArgs) + numResults > 32) { \
            goto STACK_OVERFLOW; \
        } \
    }
#else
#define CHECK_STACK(numArgs, numResults)
#endif

    // Print the banner.
    if (this->emit != NULL)
    {
        this->emit('H');
        this->emit('i');
        this->emit('\n');
    }

    static const void * const jtb[256] PROGMEM = {
        // $00 - $07
        0,
        &&LIT,
        &&DUP,
        &&DROP,

        &&PLUS,
        &&MINUS,
        &&ONEPLUS,
        &&ONEMINUS,

        // $08 - $0F
        &&SWAP,
        &&BRANCH,
        &&ABORT,
        &&CHARLIT,

        &&COMPILECOMMA,
        &&CR,
        &&EMIT,
        &&EXECUTE,

        // $10 - $17
        &&FETCH,
        &&LITERAL,
        &&NUMBERQ,
        &&OR,

        &&PARSEWORD,
        &&FINDWORD,
        &&QDUP,
        &&SPACE,

        // $18 - $1F
        &&STATE,
        &&STORE,
        &&TOIN,
        &&TWODROP,

        &&TYPE,
        &&ZBRANCH,
        &&ZERO,
        &&ZEROEQUALS,

        // $20 - $27
        0, // UNUSED
        &&TIB,
        &&TIBSIZE,
        &&ACCEPT,

        0, // UNUSED
        &&PSQUOTE,
        &&BL,
        &&CFETCH,

        // $28 - $2F
        &&COUNT,
        &&TONUMBER,
        &&DEPTH,
        0, // UNUSED

        &&PDOTQUOTE,
        &&BACKSLASH,
        &&HEX,
        0, // UNUSED

        // $30 - $37
        &&HERE,
        &&LATEST,
        &&TWODUP,
        &&COMMA,

        &&CCOMMA,
        &&TUCK,
        &&ALIGN,
        &&MOVE,

        // $38 - $3F
        &&CPLUSSTORE,
        &&ALLOT,
        &&NIP,
        &&WCOMMA,

        0, // UNUSED
        &&HIDE,
        &&RTBRACKET,
        &&CSTORE,

        // $40 - $47
        0, // UNUSED
        &&REVEAL,
        &&LTBRACKET,
        &&ABS,

        &&LESSNUMSIGN,
        0, // UNUSED
        &&ROT,
        0, // UNUSED

        // $48 - $4F
        &&NUMSIGNGRTR,
        0, // UNUSED
        &&ZEROLESS,
        &&HOLD,

        &&BASE,
        &&UDSLASHMOD,
        &&GREATERTHAN,
        &&AND,

        // $50 - $57
        &&NOTEQUALS,
        0, // UNUSED
        &&INITRP,
        &&TICKSOURCE,

        &&TICKSOURCELEN,
        0, 0, 0,

        // $58 - $5F
        0, 0, 0, 0,
        0, 0, 0, 0,

        // $60 - $67
        &&PDOCOLON,
        0, 0, 0,

        0, 0,
        &&PDOFFI0,
        &&PDOFFI1,

        // $68 - $6F
        &&PDOFFI2,
        0, 0, 0,

        0, 0, 0, 0,

        // $70 - $77
        &&DOCOLON,
        0, 0, 0,

        0, 0,
        &&DOFFI0,
        &&DOFFI1,

        // $78 - $7F
        &&DOFFI2,
        &&DOFFI3,
        &&DOFFI4,
        &&DOFFI5,

        &&DOFFI6,
        &&DOFFI7,
        &&DOFFI8,
        &&EXIT,

        // $80 - $87
        &&DOCOLONROM, // Offset=0 (SEMICOLON)
        0, // UNUSED, Offset=4
        &&DOCOLONROM, // Offset=8 (UDOT)
        0, // UNUSED, Offset=12
        &&DOCOLONROM, // Offset=16 (DOT)
        0, // UNUSED, Offset=20
        0, // UNUSED, Offset=24
        0, // UNUSED, Offset=28
        0, // UNUSED, Offset=32
        &&DOCOLONROM, // Offset=36 (CREATE)
        0, // UNUSED, Offset=40
        0, // UNUSED, Offset=44
        0, // UNUSED, Offset=48
        0, // UNUSED, Offset=52
        0, // UNUSED, Offset=56
        0, // UNUSED, Offset=60
        0, // UNUSED, Offset=64
        0, // UNUSED, Offset=68
        0, // UNUSED, Offset=72
        &&DOCOLONROM, // Offset=76 (COLON)
        0, // UNUSED, Offset=80
        0, // UNUSED, Offset=84
        &&DOCOLONROM, // Offset=88 (NUMSIGNS)
        0, // UNUSED, Offset=92
        &&DOCOLONROM, // Offset=96 (SIGN)
        0, // UNUSED, Offset=100
        &&DOCOLONROM, // Offset=104 (NUMSIGN)
        0, // UNUSED, Offset=108
        0, // UNUSED, Offset=112
        0, // UNUSED, Offset=116
        0, // UNUSED, Offset=120
        &&DOCOLONROM, // Offset=124 (QUIT)
        0, // UNUSED, Offset=128
        0, // UNUSED, Offset=132
        0, // UNUSED, Offset=136
        0, // UNUSED, Offset=140
        0, // UNUSED, Offset=144
        &&DOCOLONROM, // Offset=148 (INTERPRET)
        0, // UNUSED, Offset=152
        0, // UNUSED, Offset=156
        0, // UNUSED, Offset=160
        0, // UNUSED, Offset=164
        0, // UNUSED, Offset=168
        0, // UNUSED, Offset=172
        0, // UNUSED, Offset=176
        0, // UNUSED, Offset=180
        0, // UNUSED, Offset=184
        0, // UNUSED, Offset=188
        0, // UNUSED, Offset=192
        0, // UNUSED, Offset=196
    };

    static const int8_t primitiveDefinitions[] PROGMEM = {
        // SEMICOLON, Offset=0, Length=6
        //   : ; ( --)   ['] EXIT COMPILE,  REVEAL  [ ; IMMEDIATE
        CHARLIT, EXIT, COMPILECOMMA, REVEAL, LTBRACKET, EXIT, 0, 0,

        // UDOT, Offset=8, Length=7
        //   : U. ( u --)  0 <# #S #> TYPE SPACE ;
        ZERO, LESSNUMSIGN, NUMSIGNS, NUMSIGNGRTR, TYPE, SPACE, EXIT, 0,

        // DOT, Offset=16, Length=20
        // : . ( n -- )
        //   BASE @ 10 <>  IF U. EXIT THEN
        //   DUP ABS 0 <# #S ROT SIGN #> TYPE SPACE ;
        BASE, FETCH, CHARLIT, 10, NOTEQUALS, ZBRANCH, 3, UDOT, EXIT,
        DUP, ABS, ZERO, LESSNUMSIGN, NUMSIGNS, ROT, SIGN,
            NUMSIGNGRTR, TYPE, SPACE,
        EXIT,

        // -------------------------------------------------------------
        // CREATE [CORE] 6.1.1000 ( "<spaces>name" -- )
        //
        // Skip leading space delimiters.  Parse name delimited by a
        // space.  Create a definition for name with the execution
        // semantics defined below.  If the data-space pointer is not
        // aligned, reserve enough data space to align it.  The new
        // data-space pointer defines name's data field.  CREATE does
        // not allocate data space in name's data field.
        //
        //   name Execution:	( -- a-addr )
        //       a-addr is the address of name's data field.  The
        //       execution semantics of name may be extended by using
        //       DOES>.
        //
        // : TERMINATE-NAME ( ca u -- ca u)  2DUP 1- +  $80 SWAP C+! ;
        // : S, ( ca u --)  TUCK  HERE SWAP MOVE  ALLOT ;
        // : NAME, ( ca u --)  TERMINATE-NAME S, ;
        // : >LATEST-OFFSET ( addr -- u)  LATEST @ DUP IF - ELSE NIP THEN ;
        // : CREATE ( "<spaces>name" -- )
        //   PARSE-WORD DUP 0= IF ABORT THEN ( ca u)
        //   HERE  DUP >LATEST-OFFSET W,  LATEST ! ( ca u)
        //   CFADOCREATE C, ( ca u)  NAME,  ALIGN ;
        //
        // Offset=36, Length=37
        PARSEWORD, DUP, ZEROEQUALS, ZBRANCH, 2, ABORT,
        HERE, DUP,
        // >LATEST-OFFSET
            LATEST, FETCH, DUP, ZBRANCH, 4,
                MINUS, BRANCH, 2,
                NIP,
        WCOMMA, LATEST, STORE,
        CHARLIT, CFADOCREATE, CCOMMA,
        // NAME,
            // TERMINATE-NAME
                TWODUP, ONEMINUS, PLUS, CHARLIT, 0x80, SWAP, CPLUSSTORE,
            // S,
                TUCK, HERE, SWAP, MOVE, ALLOT,
        ALIGN,
        EXIT, 0, 0, 0,

        // -------------------------------------------------------------
        // : [CORE] 6.1.0450 "colon" ( C: "<spaces>name" -- colon-sys )
        //
        // Skip leading space delimiters.  Parse name delimited by a
        // space.  Create a definition for name, called a "colon
        // definition".  Enter compilation state and start the
        // current definition, producing colon-sys.  Append the
        // initiation semantics given below to the current definition.
        //
        // The execution semantics of name will be determined by the
        // words compiled into the body of the definition.  The current
        // definition shall not be findable in the dictionary until it
        // is ended (or until the execution of DOES> in some systems).
        //
        // Initiation: ( i*x -- i*x ) ( R: -- nest-sys )
        //   Save implementation-dependent information nest-sys about
        //   the calling definition.  The stack effects i*x represent
        //   arguments to name.
        //
        // name Execution: ( i*x -- j*x )
        //       Execute the definition name.  The stack effects i*x and
        //       j*x represent arguments to and results from name,
        //       respectively.
        //
        // : LFA>CFA ( addr -- addr)  1+ 1+ ;
        // : : ( "<spaces>name" -- )
        //   CREATE  HIDE  CFADOCOLON LATEST @ LFA>CFA C!  ] ;
        //
        // Offset=76, Length=11
        CREATE, HIDE, CHARLIT, CFADOCOLON, LATEST, FETCH,
        // LFA>CFA
            ONEPLUS, ONEPLUS,
        CSTORE, RTBRACKET,
        EXIT, 0,

        // -------------------------------------------------------------
        // #S [CORE] 6.1.0050 "number-sign-s" ( ud1 -- ud2 )
        //
        // Convert one digit of ud1 according to the rule for #.
        // Continue conversion until the quotient is zero.  ud2 is zero.
        // An ambiguous condition exists if #S executes outside of a <#
        // #> delimited number conversion.
        //
        // ---
        // : #S ( ud1 -- 0 )   BEGIN # 2DUP OR WHILE REPEAT ;
        //
        // Offset=88, Length=8
        NUMSIGN, TWODUP, OR, ZBRANCH, 3, BRANCH, -6, EXIT,

        // -------------------------------------------------------------
        // SIGN [CORE] 6.1.2210 ( n -- )
        //
        // If n is negative, add a minus sign to the beginning of the
        // pictured numeric output string.  An ambiguous condition
        // exists if SIGN executes outside of a <# #> delimited number
        // conversion.
        //
        // ---
        // : SIGN ( n -- )   0< IF [CHAR] - HOLD THEN ;
        //
        // Offset=96, Length=7
        ZEROLESS, ZBRANCH, 4, CHARLIT, '-', HOLD, EXIT, 0,

        // -------------------------------------------------------------
        // # [CORE] 6.1.0030 "number-sign" ( ud1 -- ud2 )
        //
        // Divide ud1 by the number in BASE giving the quotient ud2 and
        // the remainder n.  (n is the least-significant digit of ud1.)
        // Convert n to external form and add the resulting character to
        // the beginning of the pictured numeric output string.  An
        // ambiguous condition exists if # executes outside of a <# #>
        // delimited number conversion.
        //
        // ---
        // : >DIGIT ( u -- c ) DUP 9 > 7 AND + 48 + ;
        // : # ( ud1 -- ud2 )   BASE @ UD/MOD ROT >digit HOLD ;
        //
        // Offset=104, Length=17
        BASE, FETCH, UDSLASHMOD, ROT,
        // TODIGIT
            DUP, CHARLIT, 9, GREATERTHAN, CHARLIT, 7, AND,
            PLUS, CHARLIT, 48, PLUS,
        HOLD,
        EXIT, 0, 0, 0,

        // -------------------------------------------------------------
        // QUIT [CORE] 6.1.2050 ( -- ) ( R:  i*x -- )
        //
        // Empty the return stack, store zero in SOURCE-ID if it is
        // present, make the user input device the input source, and
        // enter interpretation state.  Do not display a message.
        // Repeat the following:
        //   - Accept a line from the input source into the input
        //     buffer, set >IN to zero, and interpret.
        //   - Display the implementation-defined system prompt if in
        //     interpretation state, all processing has been completed,
        //     and no ambiguous condition exists.
        //
        // ---
        // : QUIT  ( --; R: i*x --)
        //   INITRP  0 STATE !
        //   BEGIN
        //       TIB  DUP TIBSIZE ACCEPT  SPACE
        //       INTERPRET
        //       CR  STATE @ 0= IF ." ok " THEN
        //   AGAIN ;
        //
        // Offset=124, Length=24
        INITRP, ZERO, STATE, STORE,
        TIB, DUP, TIBSIZE, ACCEPT, SPACE,
        INTERPRET,
        CR, STATE, FETCH, ZEROEQUALS, ZBRANCH, 6,
        PDOTQUOTE, 3, 'o', 'k', ' ',
        BRANCH, -18,
        0,

        // : INTERPRET ( i*x c-addr u -- j*x )
        //   'SOURCELEN !  'SOURCE !  0 >IN !
        //   BEGIN  PARSE-WORD  DUP WHILE
        //       FIND-WORD ( ca u 0=notfound | xt 1=imm | xt -1=interp)
        //       ?DUP IF ( xt 1=imm | xt -1=interp)
        //           1+  STATE @ 0=  OR ( xt 2=imm | xt 0=interp)
        //           IF EXECUTE ELSE COMPILE, THEN
        //       ELSE
        //           NUMBER? IF
        //               STATE @ IF POSTPONE LITERAL THEN
        //               -- Interpreting; leave number on stack.
        //           ELSE
        //               TYPE  SPACE  [CHAR] ? EMIT  CR  ABORT
        //           THEN
        //       THEN
        //   REPEAT ( j*x ca u) 2DROP ;
        //
        // Offset=148, Length=49
        TICKSOURCELEN, STORE, TICKSOURCE, STORE,
        ZERO, TOIN, STORE,
        PARSEWORD, DUP, ZBRANCH, 37,
        FINDWORD, QDUP, ZBRANCH, 14,
        ONEPLUS, STATE, FETCH, ZEROEQUALS, OR, ZBRANCH, 4,
        EXECUTE, BRANCH, 21,
        COMPILECOMMA, BRANCH, 18,
        NUMBERQ, ZBRANCH, 8,
        STATE, FETCH, ZBRANCH, 11,
        LITERAL, BRANCH, 8,
        TYPE, SPACE, CHARLIT, '?', EMIT, CR, ABORT,
        BRANCH, -39,
        TWODROP,
        EXIT, 0, 0, 0,
    };

    // Initialize RP so that we can use threading to get to QUIT from
    // ABORT.  QUIT will immediately reset RP as well, of course.
    returnTop = (Cell *)&this->returnStack[32];

    // Jump to ABORT, which initializes the IP, our stacks, etc.
    goto ABORT;

    // The inner interpreter.
    while (true)
    {
        // Get the next opcode and dispatch to the label.
        uint8_t op;

#ifdef __AVR__
        if (inProgramSpace)
        {
            op = pgm_read_byte(ip++);
        }
        else
#endif
        {
            op = *ip++;
        }

DISPATCH_OPCODE:
        goto *(void *)pgm_read_word(&jtb[op]);

        DOFFI0:
        {
            CHECK_STACK(0, 1);

            // The IP is pointing at the dictionary-relative offset of
            // the PFA of the FFI trampoline.  Convert that to a pointer
            // and store it in W.
            w = (uint8_t*)(this->dictionary + *(uint16_t*)ip);
            ip += 2;

        PDOFFI0:
            // W contains a pointer to the PFA of the FFI definition;
            // get the FFI definition pointer and then use that to get
            // the FFI function pointer.
            ZeroArgFFI fn = (ZeroArgFFI)pgm_read_word(&(*(FFIDef**)w)->fn);

            *--restDataStack = tos;
            tos = (*fn)();
        }
        continue;

        DOFFI1:
        {
            CHECK_STACK(1, 1);

            // The IP is pointing at the dictionary-relative offset of
            // the PFA of the FFI trampoline.  Convert that to a pointer
            // and store it in W.
            w = (uint8_t*)(this->dictionary + *(uint16_t*)ip);
            ip += 2;

        PDOFFI1:
            // W contains a pointer to the PFA of the FFI definition;
            // get the FFI definition pointer and then use that to get
            // the FFI function pointer.
            OneArgFFI fn = (OneArgFFI)pgm_read_word(&(*(FFIDef**)w)->fn);

            tos = (*fn)(tos);
        }
        continue;

        DOFFI2:
        {
            CHECK_STACK(2, 1);

            // The IP is pointing at the dictionary-relative offset of
            // the PFA of the FFI trampoline.  Convert that to a pointer
            // and store it in W.
            w = (uint8_t*)(this->dictionary + *(uint16_t*)ip);
            ip += 2;

        PDOFFI2:
            TwoArgFFI fn = (TwoArgFFI)pgm_read_word(&(*(FFIDef**)w)->fn);

            Cell arg2 = tos;
            Cell arg1 = *restDataStack++;
            tos = (*fn)(arg1, arg2);
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
                tos = *(Cell*)ip;
            }

            ip += CellSize;
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
            Cell swap = restDataStack[0];
            restDataStack[0] = tos;
            tos = swap;
        }
        continue;

        // TODO Add docs re: Note that (branch) and (0branch) offsets
        // are 8-bit relative offsets.  UNLIKE word addresses, the IP
        // points at the offset in BRANCH/ZBRANCH.  These offsets can be
        // positive or negative because branches can go both forwards
        // and backwards.
        BRANCH:
        {
            CHECK_STACK(0, 0);

            // Relative, because these are entirely within a single word
            // and so we want it to be relocatable without us having to
            // do anything.  Note that the offset cannot be larger than
            // +/- 127 bytes!
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
            if (this->emit != NULL)
            {
                this->emit('\n');
                this->emit('!');
                this->emit('O');
                this->emit('V');
                this->emit('\n');
            }
            goto ABORT;
        }
        continue;

        STACK_UNDERFLOW:
        {
            if (this->emit != NULL)
            {
                this->emit('\n');
                this->emit('!');
                this->emit('U');
                this->emit('N');
                this->emit('\n');
            }
            goto ABORT;
        }
        continue;
#endif
        // -------------------------------------------------------------
        // ABORT [CORE] 6.1.0670 ( i*x -- ) ( R: j*x -- )
        //
        // Empty the data stack and perform the function of QUIT, which
        // includes emptying the return stack, without displaying a
        // message.
        ABORT:
        {
            tos.i = 0;
            restDataStack = (Cell*)&this->dataStack[32];
            this->base = 10;
            op = QUIT;
            goto DISPATCH_OPCODE;
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

        COMPILECOMMA:
        {
            CHECK_STACK(1, 0);

            // Does this XT reference a primitive word or a user-defined
            // word?  If the former, just compile the token.  If the
            // latter, look up the type of user-defined word and compile
            // the address of the PFA into the definition.
            if ((tos.u & 0x8000) == 0)
            {
                *this->dp++ = tos.u & 0xff;
                tos = *restDataStack++;
            }
            else
            {
                // TODO Most of this is >BODY and is duplicated in
                // EXECUTE; we should move it into a helper function
                // (and just rewrite this in Forth).

                // Calculate the absolute RAM address of the target
                // word's PFA (the "W" register).
                uint8_t * pTarget = (uint8_t*)this->dictionary + (tos.u & 0x7fff);
                pTarget += 2; // Skip LFA
                uint8_t definitionType = *pTarget++;

                // Compile the CFA opcode.  By design, the compilation
                // opcodes for each definition are 0x70 greater than the
                // definition type value, so we can just OR 0x70 with
                // the definition type and then use that as the opcode.
                *this->dp++ = 0x70 | definitionType;

                // Find the PFA.  We're already there for FFI
                // trampolines, but for user-defined words we need to
                // skip over the variable-length NFA.
                if (definitionType < CFADOFFI0)
                {
                    while ((*pTarget++ & 0x80) == 0)
                    {
                        // Loop
                    }
                }

                // Compile the relative offset to the PFA.
                uint16_t relativeOffset = pTarget - this->dictionary;
                *((uint16_t*)this->dp) = relativeOffset;
                this->dp += 2;

                // Drop the XT and get a new TOS.
                tos = *restDataStack++;
            }
        }
        continue;

        CR:
        {
            CHECK_STACK(0, 0);

            if (this->emit != NULL)
            {
                this->emit('\n');
            }
        }
        continue;

        EMIT:
        {
            CHECK_STACK(1, 0);

            if (this->emit != NULL)
            {
                this->emit(tos.i);
            }

            tos = *restDataStack++;
        }
        continue;

        // -------------------------------------------------------------
        // EXECUTE [CORE] 6.1.1370 ( i*x xt -- j*x )
        //
        // Remove xt from the stack and perform the semantics identified
        // by it.  Other stack effects are due to the word EXECUTEd.
        EXECUTE:
        {
            CHECK_STACK(1, 0);

            // Does this XT reference a primitive word or a user-defined
            // word?  If the former, just dispatch to the primitive.  If
            // the latter, look up the type of user-defined word, set W,
            // and jump to the appropriate DO* primitive.
            //
            // Note that XTs are always 16 bits, even on 32-bit
            // platforms.  This is because XTs are relative offsets from
            // the start of the dictionary so that they can be stored in
            // constants (for example) and yet still relocate with the
            // dictionary.
            if ((tos.u & 0x8000) == 0)
            {
                op = tos.u;
                tos = *restDataStack++;
                goto DISPATCH_OPCODE;
            }
            else
            {
                // Calculate the absolute RAM address of the target
                // word's PFA (the "W" register).
                uint8_t * pTarget = (uint8_t*)this->dictionary + (tos.u & 0x7fff);
                pTarget += 2; // Skip LFA
                uint8_t definitionType = *pTarget++;

                // Find the PFA.  We're already there for FFI
                // trampolines, but for user-defined words we need to
                // skip over the variable-length NFA.
                if (definitionType < CFADOFFI0)
                {
                    while ((*pTarget++ & 0x80) == 0)
                    {
                        // Loop
                    }
                }

                // Set W to the PFA.
                w = pTarget;

                // Drop the XT and get a new TOS.
                tos = *restDataStack++;

                // Dispatch to the opcode that handles this type of
                // definition.  By design, the EXECUTE opcodes for each
                // definition are 0x60 greater than the definition type
                // value, so we can just OR 0x60 with the definition
                // type and then use that as the opcode.
                op = 0x60 | definitionType;
                goto DISPATCH_OPCODE;
            }
        }
        continue;

        FETCH:
        {
            CHECK_STACK(1, 1);
            tos = *(Cell*)tos.pRAM;
        }
        continue;

        LITERAL:
        {
            // Compile this literal as a character literal if it would
            // fit in 8 bits, otherwise compile a normal cell.
            if ((tos.u & ~0xff) == 0)
            {
                *this->dp++ = CHARLIT;
                *this->dp++ = tos.u & 0xff;
            }
            else
            {
                *this->dp++ = LIT;
                *((Cell*)this->dp) = tos;
                this->dp += CellSize;
            }

            tos = *restDataStack++;
        }
        continue;

        // -------------------------------------------------------------
        // NUMBER? [MFORTH] "number-question" ( c-addr u -- c-addr u 0 | n -1 )
        //
        // Attempt to convert a string at c-addr of length u into
        // digits, using the radix in BASE.  The number and -1 is
        // returned if the conversion was successful, otherwise 0 is
        // returned.
        NUMBERQ:
        {
            CHECK_STACK(2, 3);

            Unsigned u = tos.u;
            uint8_t * caddr = (uint8_t*)restDataStack->pRAM;

            Int n;
            if (this->parenNumberQ(caddr, u, n))
            {
                // Stack still contains c-addr u; rewrite to n -1.
                restDataStack->i = n;
                tos.i = -1;
            }
            else
            {
                // Stack still contains c-addr u, so just push 0.
                *--restDataStack = tos;
                tos.i = 0;
            }
        }
        continue;

        OR:
        {
            CHECK_STACK(2, 1);
            tos.i |= restDataStack++->i;
        }
        continue;

        // -------------------------------------------------------------
        // PARSE-WORD [MFORTH] ( "<spaces>name<space>" -- c-addr u )
        //
        // Skip leading spaces and parse name delimited by a space.
        // c-addr is the address within the input buffer and u is the
        // length of the selected string. If the parse area is empty,
        // the resulting string has a zero length.
        PARSEWORD:
        {
            CHECK_STACK(0, 2);

            *--restDataStack = tos;

            uint8_t * caddr;
            Unsigned u;
            parenParseWord(' ', caddr, u);
            (--restDataStack)->pRAM = caddr;
            tos.u = u;
        }
        continue;

        // -------------------------------------------------------------
        // FIND-WORD [MFORTH] "paren-find-paren" ( c-addr u -- c-addr u 0 | xt 1 | xt -1 )
        //
        // Find the definition named in the string at c-addr with length
        // u in the word list whose latest definition is pointed to by
        // nfa.  If the definition is not found, return the string and
        // zero.  If the definition is found, return its execution token
        // xt.  If the definition is immediate, also return one (1),
        // otherwise also return minus-one (-1).  For a given string,
        // the values returned by FIND-WORD while compiling may differ
        // from those returned while not compiling.
        FINDWORD:
        {
            CHECK_STACK(2, 3);

            uint8_t * caddr = (uint8_t *)restDataStack->pRAM;
            Unsigned u = tos.u;

            XT xt;
            bool isImmediate;
            if (parenFindWord(caddr, u, xt, isImmediate))
            {
                // Stack still contains c-addr u; rewrite to xt 1|-1.
                restDataStack->u = xt;
                tos.i = isImmediate ? 1 : -1;
            }
            else
            {
                // Stack still contains c-addr u, so just push 0.
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

        SPACE:
        {
            CHECK_STACK(0, 0);

            if (this->emit != NULL)
            {
                this->emit(' ');
            }
        }
        continue;

        STATE:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.pRAM = &this->state;
        }
        continue;

        // -------------------------------------------------------------
        // ! [CORE] 6.1.0010 "store" ( x a-addr -- )
        //
        // Store x at a-addr.
        STORE:
        {
            CHECK_STACK(2, 0);
            *(Cell*)tos.pRAM = *restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        // -------------------------------------------------------------
        // >IN [CORE] 6.1.0560 "to-in" ( -- a-addr )
        //
        // a-addr is the address of a cell containing the offset in
        // characters from the start of the input buffer to the start of
        // the parse area.
        TOIN:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.pRAM = &this->toIn;
        }
        continue;

        TWODROP:
        {
            CHECK_STACK(2, 0);
            restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        // NOTE: Only works for strings stored in data space!
        TYPE:
        {
            CHECK_STACK(2, 0);

            if (this->emit != NULL)
            {
                for (int i = 0; i < tos.i; i++)
                {
                    this->emit(*((uint8_t*)restDataStack->pRAM + i));
                }
            }

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

        TIB:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.pRAM = &this->tib;
        }
        continue;

        TIBSIZE:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.u = sizeof(this->tib);
        }
        continue;

        // -------------------------------------------------------------
        // ACCEPT [CORE] 6.1.0695 ( c-addr +n1 -- +n2 )
        //
        // Receive a string of at most +n1 characters.  An ambiguous
        // condition exists if +n1 is zero or greater than 32,767.
        // Display graphic characters as they are received.  A program
        // that depends on the presence or absence of non-graphic
        // characters in the string has an environmental dependency.
        // The editing functions, if any, that the system performs in
        // order to construct the string are implementation-defined.
        //
        // Input terminates when an implementation-defined line
        // terminator is received.  When input terminates, nothing is
        // appended to the string, and the display is maintained in an
        // implementation-defined way.
        //
        // +n2 is the length of the string stored at c-addr.
        ACCEPT:
        {
            CHECK_STACK(2, 1);
            Cell n1 = tos;
            Cell caddr = *restDataStack++;
            tos.u = parenAccept((uint8_t *)caddr.pRAM, n1.u);
        }
        continue;

        // -------------------------------------------------------------
        // (s") [MFORTH] "paren-s-quote-paren" ( -- c-addr u )
        //
        // Runtime behavior of S": return c-addr and u.
        // NOTE: Cannot be used in program space!
        PSQUOTE:
        {
            CHECK_STACK(0, 2);

            // Push existing TOS onto the stack.
            *--restDataStack = tos;

            // Instruction stream contains the length of the string as a
            // byte and then the string itself.  Start out by pushing
            // the address of the string (ip+1) onto the stack.
            (--restDataStack)->pRAM = ip + 1;

            // Now get the string length into TOS.
            tos.i = *ip++;

            // Advance IP over the string.
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
            tos.u = *(uint8_t*)tos.pRAM;
        }
        continue;

        COUNT:
        {
            CHECK_STACK(1, 2);
            (--restDataStack)->pRAM = (uint8_t*)tos.pRAM + 1;
            tos.u = *(uint8_t*)tos.pRAM;
        }
        continue;

        // -------------------------------------------------------------
        // >NUMBER [CORE] 6.1.0567 "to-number" ( ud1 c-addr1 u1 -- ud2 c-addr2 u2 )
        //
        // ud2 is the unsigned result of converting the characters
        // within the string specified by c-addr1 u1 into digits, using
        // the number in BASE, and adding each into ud1 after
        // multiplying ud1 by the number in BASE.  Conversion continues
        // left-to-right until a character that is not convertible,
        // including any "+" or "-", is encountered or the string is
        // entirely converted.  c-addr2 is the location of the first
        // unconverted character or the first character past the end of
        // the string if the string was entirely converted.  u2 is the
        // number of unconverted characters in the string.  An ambiguous
        // condition exists if ud2 overflows during the conversion.
        TONUMBER:
        {
            CHECK_STACK(3, 3);

            Unsigned u = tos.u;
            uint8_t * caddr = (uint8_t*)restDataStack[0].pRAM;
            Unsigned ud = restDataStack[-1].u;

            parenToNumber(ud, caddr, u);

            restDataStack[-1].u = ud;
            restDataStack[0].pRAM = caddr;
            tos.u = u;
        }
        continue;

        // -------------------------------------------------------------
        // DEPTH [CORE] 6.1.1200 ( -- +n )
        //
        // +n is the number of single-cell values contained in the data
        // stack before +n was placed on the stack.
        DEPTH:
        {
            CHECK_STACK(0, 1);

            // Save TOS, then calculate the stack depth.  The return
            // value should be the number of items on the stack *before*
            // DEPTH was called and so we have to subtract one from the
            // count given that we calculate the depth *after* pushing
            // the old TOS onto the stack.
            *--restDataStack = tos;
            tos.i = &this->dataStack[32] - restDataStack - 1;
        }
        continue;

        // -------------------------------------------------------------
        // (.") [MFORTH] "paren-dot-quote-paren" ( -- )
        //
        // Prints the string that was compiled into the definition.
        PDOTQUOTE:
        {
            // Instruction stream contains the length of the string as a
            // byte and then the string itself.  Get and skip over both
            // values
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

            // Advance IP over the string.
            ip = ip + u;

            // Print out the string.
            if (this->emit != NULL)
            {
                for (int i = 0; i < u; i++)
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

                    this->emit(ch);

                    caddr++;
                }
            }
        }
        continue;

        // -------------------------------------------------------------
        // \ [CORE-EXT] 6.2.2535 "backslash"
        //
        // Compilation:
        //   Perform the execution semantics given below.
        //
        // Execution: ( "ccc<eol>" -- )
        //   Parse and discard the remainder of the parse area.  \ is an
        //   immediate word.
        BACKSLASH:
        {
            this->toIn = this->sourceLen.i;
        }
        continue;

        // -------------------------------------------------------------
        // HEX [CORE EXT] 6.2.1660 ( -- )
        //
        // Set contents of BASE to sixteen.
        HEX:
        {
            this->base = 16;
        }
        continue;

        HERE:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.pRAM = this->dp;
        }
        continue;

        LATEST:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.pRAM = &this->latest;
        }
        continue;

        TWODUP:
        {
            CHECK_STACK(2, 4);
            Cell second = *restDataStack;
            *--restDataStack = tos;
            *--restDataStack = second;
        }
        continue;

        COMMA:
        {
            CHECK_STACK(1, 0);
            *((Cell*)this->dp) = tos;
            this->dp += CellSize;
            tos = *restDataStack++;
        }
        continue;

        CCOMMA:
        {
            CHECK_STACK(1, 0);
            *this->dp++ = tos.u & 0xff;
            tos = *restDataStack++;
        }
        continue;

        TUCK:
        {
            Cell second = *restDataStack;
            *restDataStack = tos;
            *--restDataStack = second;
        }
        continue;

        ALIGN:
        {
            // No alignment necessary (unless we want to expand the
            // range of the dictionary-relative offsets at some point).
        }
        continue;

        MOVE:
        {
            CHECK_STACK(3, 0);
            Cell arg3 = tos;
            Cell arg2 = *restDataStack++;
            Cell arg1 = *restDataStack++;
            memcpy(arg2.pRAM, arg1.pRAM, arg3.u);
            tos = *restDataStack++;
        }
        continue;

        CPLUSSTORE:
        {
            CHECK_STACK(2, 0);
            uint8_t c = *(uint8_t*)tos.pRAM;
            c += (restDataStack++)->u & 0xff;
            *(uint8_t*)tos.pRAM = c;
            tos = *restDataStack++;
        }
        continue;

        ALLOT:
        {
            CHECK_STACK(1, 0);
            this->dp += tos.u;
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
            *((uint16_t*)this->dp) = (uint16_t)(tos.u & 0xffff);
            this->dp += 2;
            tos = *restDataStack++;
        }
        continue;

        // -------------------------------------------------------------
        // HIDE [MFORTH] ( -- )
        //
        // Prevent the most recent definition from being found in the
        // dictionary.
        HIDE:
        {
            *(this->latest + 2) |= 0x80;
        }
        continue;

        // -------------------------------------------------------------
        // ] [CORE] 6.1.2540 "right-bracket" ( -- )
        //
        // Enter compilation state.
        RTBRACKET:
        {
            this->state = -1;
        }
        continue;

        // -------------------------------------------------------------
        // C! [CORE] 6.1.0850 "c-store" ( char c-addr -- )
        //
        // Store char at c-addr.  When character size is smaller than
        // cell size, only the number of low-order bits corresponding to
        // character size are transferred.
        CSTORE:
        {
            CHECK_STACK(2, 0);
            *(uint8_t*)tos.pRAM = restDataStack++->u;
            tos = *restDataStack++;
        }
        continue;

        REVEAL:
        {
            *(this->latest + 2) &= 0x7f;
        }
        continue;

        LTBRACKET:
        {
            this->state = 0;
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
            this->hld = this->dp + (CellSize*8*3);
        }
        continue;

        // -------------------------------------------------------------
        // ROT [CORE] 6.1.2160 "rote" ( x1 x2 x3 -- x2 x3 x1 )
        //
        // Rotate the top three stack entries.
        ROT:
        {
            CHECK_STACK(3, 3);
            Cell x3 = tos;
            Cell x2 = *restDataStack++;
            Cell x1 = *restDataStack++;
            *--restDataStack = x2;
            *--restDataStack = x3;
            tos = x1;
        }
        continue;

        // -------------------------------------------------------------
        // #> [CORE] 6.1.0040 "number-sign-greater" ( xd -- c-addr u )
        //
        // Drop xd.  Make the pictured numeric output string available
        // as a character string.  c-addr and u specify the resulting
        // character string.  A program may replace characters within
        // the string.
        //
        // ---
        // : #> ( xd -- c-addr u ) DROP DROP  HLD @  HERE HLDEND +  OVER - ;
        NUMSIGNGRTR:
        {
            restDataStack->pRAM = this->hld;
            tos.u = (this->dp + (CellSize*8*3)) - this->hld;
        }
        continue;

        // -------------------------------------------------------------
        // 0< [CORE] 6.1.0250 "zero-less" ( b -- flag )
        //
        // flag is true if and only if n is less than zero.
        ZEROLESS:
        {
            CHECK_STACK(1, 0);
            tos.i = tos.i < 0 ? -1 : 0;
        }
        continue;

        // -------------------------------------------------------------
        // HOLD [CORE] 6.1.1670 ( char -- )
        //
        // Add char to the beginning of the pictured numeric output
        // string.  An ambiguous condition exists if HOLD executes
        // outside of a <# #> delimited number conversion.
        HOLD:
        {
            CHECK_STACK(1, 0);
            *--this->hld = tos.u;
            tos = *restDataStack++;
        }
        continue;

        // -------------------------------------------------------------
        // BASE [CORE] 6.1.0750 ( -- a-addr )
        //
        // a-addr is the address of a cell containing the current
        // number-conversion radix {{2...36}}.
        BASE:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.pRAM = &this->base;
        }
        continue;

        // -------------------------------------------------------------
        // UD/MOD [MFORTH] "u-d-slash-mod" ( ud1 u1 -- n ud2 )
        //
        // Divide ud1 by u1 giving the quotient ud2 and the remainder n.
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

        // -------------------------------------------------------------
        // > [CORE] 6.1.0540 "greater-than" ( n1 n2 -- flag )
        //
        // flag is true if and only if n1 is greater than n2.
        GREATERTHAN:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i > tos.i ? -1 : 0;
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
            returnTop = (Cell *)&this->returnStack[32];
        }
        continue;

        TICKSOURCE:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.pRAM = &this->source;
        }
        continue;

        TICKSOURCELEN:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.pRAM = &this->sourceLen;
        }
        continue;

        DOCOLON:
        {
            // IP currently points to the relative offset of the PFA of
            // the target word.  Read that offset and advance IP to the
            // opcode after the offset.
            w = this->dictionary + *(uint16_t*)ip;
            ip += 2;

        PDOCOLON:
            // IP now points to the next word in the PFA and that is the
            // location to which we should return once this new word has
            // executed.
#ifdef __AVR__
            if (inProgramSpace)
            {
                // TODO Needs to be relative to the ROM definition
                // block.
                ip = (uint8_t*)((unsigned int)ip | 0x8000);

                // We are no longer in program space since, by design,
                // DOCOLON is only ever used for user-defined words in
                // RAM.
                inProgramSpace = false;
            }
#endif
            (--returnTop)->pRAM = (void *)ip;

            // Now set the IP to the PFA of the word that is being
            // called and continue execution inside of that word.
            ip = w;
        }
        continue;

        DOCOLONROM:
        {
            // Push IP to the return stack, marking IP as
            // in-program-space as necessary.
#ifdef __AVR__
            if (inProgramSpace)
            {
                ip = (uint8_t*)((unsigned int)ip | 0x8000);
            }
#endif
            (--returnTop)->pRAM = (void *)ip;

            // Calculate the offset of the definition and set the IP to
            // the absolute address.
            int definitionOffset = (op & ~0x80) << 2;
            ip = (uint8_t*)primitiveDefinitions + definitionOffset;
#ifdef __AVR__
            inProgramSpace = true;
#endif
        }
        continue;

        EXIT:
        {
            ip = (uint8_t *)((returnTop++)->pRAM);

#ifdef __AVR__
            if (((unsigned int)ip & 0x8000) != 0)
            {
                // TODO Needs to be relative to the ROM definition
                // block.
                ip = (uint8_t*)((unsigned int)ip & 0x7FFF);
                inProgramSpace = true;
            }
            else
            {
                inProgramSpace = false;
            }
#endif
        }
        continue;
    }
}
