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

#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM
#include <string.h>
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
    "\x04" "QUIT"
    "\x00" // TIB
    "\x00" // TIBSIZE
    "\x06" "ACCEPT"

    "\x00" // INTERPRET
    "\x00" // (S")
    "\x02" "BL"
    "\x02" "C@"

    // $28 - $2F
    "\x05" "COUNT"
    "\x07" ">NUMBER"
    "\x05" "DEPTH"
    "\x01" "."

    "\x00" // (.")
    "\x01" "\\"
    "\x03" "HEX"
    "\x06" "CREATE"

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
   source(NULL), sourceLen(0), toIn(0), prevLeave(NULL)
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
                    nameMatch = true;
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
        uint8_t primitiveLength = (uint8_t)pgm_read_byte(pPrimitives++);
        if (primitiveLength == 0xff)
        {
            break;
        }

        // Is this a match?  If so, return the XT.
        if ((primitiveLength == searchLen)
            && (strncasecmp_P(searchName, pPrimitives, searchLen) == 0))
        {
            // TODO isImmediate needs to come from the primitive table
            xt = opcode;
            isImmediate = false;
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
    while ((*(this->source + this->toIn) == delim)
            && (this->toIn < this->sourceLen))
    {
        this->toIn++;
    }

    // We're either pointing to a non-delimiter character or we're at
    // the end of the parse area; point caddr at the current location
    // and then scan forwards until we hit another delimiter or we
    // exhaust the parse area.
    caddr = this->source + this->toIn;
    char * pParse = (char *)caddr;
    while ((*pParse != delim) && (this->toIn < this->sourceLen))
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
    // FIXME The overflow logic seems slightly too aggressive -- it
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

    static const void * const jtb[128] PROGMEM = {
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
        &&QUIT,
        &&TIB,
        &&TIBSIZE,
        &&ACCEPT,

        &&INTERPRET,
        &&PSQUOTE,
        &&BL,
        &&CFETCH,

        // $28 - $2F
        &&COUNT,
        &&TONUMBER,
        &&DEPTH,
        &&DOT,

        &&PDOTQUOTE,
        &&BACKSLASH,
        &&HEX,
        &&CREATE,

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

        0, 0, 0, 0,

        // $40 - $47
        0, 0, 0, 0,
        0, 0, 0, 0,

        // $48 - $4F
        0, 0, 0, 0,
        0, 0, 0, 0,

        // $50 - $57
        0, 0, 0, 0,
        0, 0, 0, 0,

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
    };

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

            w = *(uint8_t**)ip;
            ip += FFIProcPtrSize;

        PDOFFI0:
            *--restDataStack = tos;
            tos = (*(ZeroArgFFI)w)();
        }
        continue;

        DOFFI1:
        {
            CHECK_STACK(1, 1);

            w = *(uint8_t**)ip;
            ip += FFIProcPtrSize;

        PDOFFI1:
            tos = (*(OneArgFFI)w)(tos);
        }
        continue;

        DOFFI2:
        {
            CHECK_STACK(2, 1);

            w = *(uint8_t**)ip;
            ip += FFIProcPtrSize;

        PDOFFI2:
            Cell arg2 = tos;
            Cell arg1 = *restDataStack++;
            tos = (*(TwoArgFFI)w)(arg1, arg2);
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
            goto QUIT;
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
            // TODO Needs to differentiate between XTs for primitive
            // words and user-defined words.  The latter will be encoded
            // as a 16-bit value with the high bit set and the remaining
            // 15 bits specifying the relative offset to the target word
            // from the start of the dictionary (so that XTs relocate
            // with the dictionary).  XTs for user-defined words will
            // then be converted into a DO* opcode and a relative offset
            // from the current call site (IP).  Initially these offsets
            // will be 16-bits, but we can create DO*8 opcodes that are
            // used if the range fits into 8 bits.  COMPILECOMMA will
            // need to look up the type of word that is being targed and
            // compile in the appropriate DO* opcode.  This creates
            // additional work for COMPILECOMMA, but the benefit is that
            // XTs can use the entire 15-bit range and do not need any
            // flag bits.
            //
            // XTs that reference primitive words will just be compiled
            // as-is.
            //
            // Note that the compiled offset is a native endian relative
            // offset to the word's PFA (no CFAs in MFORTH) and that the
            // offset is as of the address *after* the opcode and
            // *before* the offset since that's where the IP will be in
            // the DO* opcode when we calculate the address.
            //
            // TODO Implement this
            tos = *restDataStack++;
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

                // Find W.  For user-defined words this is the address
                // of the PFA (which is after the variable-length NFA).
                // For FFI trampolines it is the FFI function pointer
                // itself (which we get by dereferencing the FFI linked
                // list entry that is stored after the dictionary
                // flags).
                if (definitionType < CFADOFFI0)
                {
                    while ((*pTarget++ & 0x80) == 0)
                    {
                        // Loop
                    }

                    w = pTarget;
                }
                else
                {
                    // Dereference the linked list entry in order to get
                    // the FFI function pointer.
                    const FFIDef * ffi = *(FFIDef **)pTarget;
                    const void * ffiFn = (void *)pgm_read_word(&ffi->fn);
                    w = (uint8_t*)ffiFn;
                }

                // Drop the XT and get a new TOS.
                tos = *restDataStack++;

                // Dispatch to the opcode that handles this type of
                // definition.  By design, the opcodes for each
                // definition are 0x60 greater than the definition
                // type value, so we can just OR 0x60 with the
                // definition type and then use that as the opcode.
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
            // TODO Implement this.
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
        QUIT:
        {
            CHECK_STACK(0, 0);
            returnTop = (Cell *)&this->returnStack[32];
            this->state = 0;

            // : (QUIT)  ( --)
            //   BEGIN
            //       TIB  DUP TIBSIZE ACCEPT  SPACE
            //       INTERPRET
            //       CR  STATE @ 0= IF ." ok " THEN
            //   AGAIN ;
            static const int8_t parenQuit[] PROGMEM = {
                TIB, DUP, TIBSIZE, ACCEPT, SPACE,
                INTERPRET,
                CR, STATE, FETCH, ZEROEQUALS, ZBRANCH, 6,
                PDOTQUOTE, 3, 'o', 'k', ' ',
                BRANCH, -18
            };

            ip = (uint8_t*)&parenQuit;
#ifdef __AVR__
            inProgramSpace = true;
#endif
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

        // -----------------------------------------------------------------
        // INTERPRET [MFORTH] ( i*x c-addr u -- j*x )
        //
        // Interpret the given string.
        INTERPRET:
        {
            CHECK_STACK(2, 0);
            this->source = (uint8_t *)restDataStack++->pRAM;
            this->sourceLen = tos.u;
            tos = *restDataStack++;

            // : (INTERPRET) ( i*x -- j*x )
            //   0 >IN !
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
            static const int8_t parenInterpret[] PROGMEM = {
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
                EXIT,
            };

#ifdef __AVR__
            if (inProgramSpace)
            {
                ip = (uint8_t*)((unsigned int)ip | 0x8000);
            }
#endif
            (--returnTop)->pRAM = (void *)ip;

            ip = (uint8_t*)&parenInterpret;
#ifdef __AVR__
            inProgramSpace = true;
#endif
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
        // . [CORE] 6.1.0180 "dot" ( n -- )
        //
        // Display n in free field format.
        DOT:
        {
            CHECK_STACK(1, 0);

            if (this->emit != NULL)
            {
                // TODO These numbers are printing backwards; we need to
                // implement pictured numeric output.
                // TODO Use BASE.
                do
                {
                    Int i = tos.i - ((tos.i / 10) * 10);
                    this->emit('0' + i);
                    tos.i = tos.i / 10;
                } while (tos.i != 0);

                this->emit(' ');
            }

            tos = *restDataStack++;
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
            this->toIn = this->sourceLen;
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
        CREATE:
        {
            static const int8_t parenCreate[] PROGMEM = {
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
                EXIT
            };

#ifdef __AVR__
            if (inProgramSpace)
            {
                ip = (uint8_t*)((unsigned int)ip | 0x8000);
            }
#endif
            (--returnTop)->pRAM = (void *)ip;

            ip = (uint8_t*)&parenCreate;
#ifdef __AVR__
            inProgramSpace = true;
#endif
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
            // TODO Implement this as part of DOCOLON8/DOCOLON16
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

        DOCOLON:
        {
            // IP currently points to the relative offset of the PFA of
            // the target word.  Read that offset and advance IP to the
            // opcode after the offset.
            w = ip - *(uint16_t*)ip;
            ip += 2;

        PDOCOLON:
            // IP now points to the next word in the PFA and that is the
            // location to which we should return once this new word has
            // executed.
#ifdef __AVR__
            if (inProgramSpace)
            {
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

        EXIT:
        {
            ip = (uint8_t *)((returnTop++)->pRAM);

#ifdef __AVR__
            if (((unsigned int)ip & 0x8000) != 0)
            {
                // TODO Needs to be relative to something (such as the
                // block of ROM definitions, should we create such a
                // thing).
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
