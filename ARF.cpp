/* Copyright (c) 2014, Michael Alyn Miller <malyn@strangeGizmo.com>.
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

#include "ARF.h"

#define FFIPROCSZ (sizeof(void*))
#define CELLSZ (sizeof(arfCell))

// TODO We've gotten our names a bit wrong here.  These aren't
// "opcodes" as much as they are "primitive words".  Similarly, the
// RAM definitions are "user-defined words."
//
// The key point here is that the PFA contains a list of tokens to
// primitive words.  There is no longer a concept of "opcode" since ARF
// is a Forth machine and not a CPU.
enum arfOpcode
{
    //arfOpCOLD = 0x00,
    arfOpDOCOLON = 0x01,
    //arfOpDOIMMEDIATE,
    //arfOpDOCONSTANT
    //arfOpDOCREATE
    //arfOpDODOES
    //arfOpDOVARIABLE
    arfOpDOFFI0 = 0x07,
    arfOpDOFFI1,
    arfOpDOFFI2,
    arfOpDOFFI3,
    arfOpDOFFI4,
    arfOpDOFFI5,
    arfOpDOFFI6,
    arfOpDOFFI7,
    arfOpDOFFI8,
    arfOpLIT,
    arfOpDUP,
    arfOpDROP,
    arfOpPLUS,
    arfOpMINUS,
    arfOpONEPLUS,
    arfOpONEMINUS,
    arfOpSWAP,
    arfOpBRANCH,
    arfOpABORT,
    arfOpCHARLIT,
    arfOpCOMPILECOMMA,
    arfOpCR,
    arfOpEMIT,
    arfOpEXECUTE,
    arfOpFETCH,
    arfOpLITERAL,
    arfOpNUMBERQ,
    arfOpOR,
    arfOpPARSEWORD,
    arfOpFINDWORD,
    arfOpQDUP,
    arfOpSPACE,
    arfOpSTATE,
    arfOpSTORE,
    arfOpTOIN,
    arfOpTWODROP,
    arfOpTYPE,
    arfOpZBRANCH,
    arfOpZERO,
    arfOpZEROEQUALS,
    arfOpQUIT,
    arfOpTIB,
    arfOpTIBSIZE,
    arfOpACCEPT,
    arfOpINTERPRET,
    arfOpPSQUOTE,
    arfOpBL,
    arfOpCFETCH,
    arfOpCOUNT,
    arfOpTONUMBER,
    arfOpDEPTH,
    arfOpDOT,
    arfOpPDOTQUOTE,
    //...
    arfOpEXIT = 0x7F,
} arfOpcode;

struct arfPrimitiveWord {
    uint8_t opcode;
    uint8_t nameLen;
    const char * name;
};

static const char arfOpNameABORT[] PROGMEM = "ABORT";
static const char arfOpNameACCEPT[] PROGMEM = "ACCEPT";
static const char arfOpNameBL[] PROGMEM = "BL";
static const char arfOpNameCFETCH[] PROGMEM = "C@";
static const char arfOpNameCOMPILECOMMA[] PROGMEM = "COMPILE,";
static const char arfOpNameCOUNT[] PROGMEM = "COUNT";
static const char arfOpNameCR[] PROGMEM = "CR";
static const char arfOpNameDEPTH[] PROGMEM = "DEPTH";
static const char arfOpNameDOT[] PROGMEM = ".";
static const char arfOpNameDROP[] PROGMEM = "DROP";
static const char arfOpNameDUP[] PROGMEM = "DUP";
static const char arfOpNameEMIT[] PROGMEM = "EMIT";
static const char arfOpNameEXECUTE[] PROGMEM = "EXECUTE";
static const char arfOpNameFETCH[] PROGMEM = "@";
static const char arfOpNameLITERAL[] PROGMEM = "LITERAL";
static const char arfOpNameMINUS[] PROGMEM = "-";
static const char arfOpNameNUMBERQ[] PROGMEM = "NUMBER?";
static const char arfOpNameONEMINUS[] PROGMEM = "1-";
static const char arfOpNameONEPLUS[] PROGMEM = "1+";
static const char arfOpNameOR[] PROGMEM = "OR";
static const char arfOpNamePLUS[] PROGMEM = "+";
static const char arfOpNameQDUP[] PROGMEM = "?DUP";
static const char arfOpNameQUIT[] PROGMEM = "QUIT";
static const char arfOpNameSPACE[] PROGMEM = "SPACE";
static const char arfOpNameSTATE[] PROGMEM = "STATE";
static const char arfOpNameSTORE[] PROGMEM = "STORE";
static const char arfOpNameSWAP[] PROGMEM = "SWAP";
static const char arfOpNameTOIN[] PROGMEM = ">IN";
static const char arfOpNameTONUMBER[] PROGMEM = ">NUMBER";
static const char arfOpNameTWODROP[] PROGMEM = "2DROP";
static const char arfOpNameTYPE[] PROGMEM = "TYPE";
static const char arfOpNameZERO[] PROGMEM = "0";
static const char arfOpNameZEROEQUALS[] PROGMEM = "0=";

// TODO The size of this data structure could be reduced by at least 384
// bytes if we pack the entire thing into a single byte array and then
// just inline the strings, using the count to traverse from
// word-to-word, with a separate counter being used in code to decide
// which opcode we are looking at (instead of including the opcode in
// the struct).  128 bytes saved for the opcode, another 256 bytes saved
// for the string pointer, then some number of extra bytes saved for all
// of the NULL (internal) words.
static const arfPrimitiveWord primitives[128] PROGMEM = {
    // $00 - $07
    { 0,                    0,  NULL },
    { arfOpDOCOLON,         0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { arfOpDOFFI0,          0,  NULL },

    // $08 - $0F
    { arfOpDOFFI1,          0,  NULL },
    { arfOpDOFFI2,          0,  NULL },
    { arfOpDOFFI3,          0,  NULL },
    { arfOpDOFFI4,          0,  NULL },

    { arfOpDOFFI5,          0,  NULL },
    { arfOpDOFFI6,          0,  NULL },
    { arfOpDOFFI7,          0,  NULL },
    { arfOpDOFFI8,          0,  NULL },

    // $10 - $17
    { arfOpLIT,             0,  NULL },
    { arfOpDUP,             3,  arfOpNameDUP },
    { arfOpDROP,            4,  arfOpNameDROP },
    { arfOpPLUS,            1,  arfOpNamePLUS },

    { arfOpMINUS,           1,  arfOpNameMINUS },
    { arfOpONEPLUS,         2,  arfOpNameONEPLUS },
    { arfOpONEMINUS,        2,  arfOpNameONEMINUS },
    { arfOpSWAP,            4,  arfOpNameSWAP },

    // $18 - $1F
    { arfOpBRANCH,          0,  NULL },
    { arfOpABORT,           5,  arfOpNameABORT },
    { arfOpCHARLIT,         0,  NULL },
    { arfOpCOMPILECOMMA,    8,  arfOpNameCOMPILECOMMA },

    { arfOpCR,              2,  arfOpNameCR },
    { arfOpEMIT,            4,  arfOpNameEMIT },
    { arfOpEXECUTE,         7,  arfOpNameEXECUTE },
    { arfOpFETCH,           1,  arfOpNameFETCH },

    // $20 - $27
    { arfOpLITERAL,         7,  arfOpNameLITERAL },
    { arfOpNUMBERQ,         7,  arfOpNameNUMBERQ },
    { arfOpOR,              2,  arfOpNameOR },
    { arfOpPARSEWORD,       0,  NULL },

    { arfOpFINDWORD,        0,  NULL },
    { arfOpQDUP,            4,  arfOpNameQDUP },
    { arfOpSPACE,           5,  arfOpNameSPACE },
    { arfOpSTATE,           5,  arfOpNameSTATE },

    // $28 - $2F
    { arfOpSTORE,           5,  arfOpNameSTORE },
    { arfOpTOIN,            3,  arfOpNameTOIN },
    { arfOpTWODROP,         5,  arfOpNameTWODROP },
    { arfOpTYPE,            4,  arfOpNameTYPE },

    { arfOpZBRANCH,         0,  NULL },
    { arfOpZERO,            1,  arfOpNameZERO },
    { arfOpZEROEQUALS,      2,  arfOpNameZEROEQUALS },
    { arfOpQUIT,            4,  arfOpNameQUIT },

    // $30 - $37
    { arfOpTIB,             0,  NULL },
    { arfOpTIBSIZE,         0,  NULL },
    { arfOpACCEPT,          6,  arfOpNameACCEPT },
    { arfOpINTERPRET,       0,  NULL },

    { arfOpPSQUOTE,         0,  NULL },
    { arfOpBL,              2,  arfOpNameBL },
    { arfOpCFETCH,          2,  arfOpNameCFETCH },
    { arfOpCOUNT,           5,  arfOpNameCOUNT },

    // $38 - $3F
    { arfOpTONUMBER,        7,  arfOpNameTONUMBER },
    { arfOpDEPTH,           5,  arfOpNameDEPTH },
    { arfOpDOT,             1,  arfOpNameDOT },
    { arfOpPDOTQUOTE,       0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    // $40 - $47
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    // $48 - $4F
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    // $50 - $57
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    // $58 - $5F
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    // $60 - $67
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    // $68 - $6F
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    // $70 - $77
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    // $78 - $7F
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },

    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { 0,                    0,  NULL },
    { arfOpEXIT,            0,  NULL },
};

typedef enum
{
    arfCFADOCOLON = 1,
    arfCFADOIMMEDIATE,
    arfCFADOCONSTANT,
    arfCFADOCREATE,
    arfCFADODOES,
    arfCFADOVARIABLE,
    arfCFADOFFI0,
    arfCFADOFFI1,
    arfCFADOFFI2,
    arfCFADOFFI3,
    arfCFADOFFI4,
    arfCFADOFFI5,
    arfCFADOFFI6,
    arfCFADOFFI7,
    arfCFADOFFI8
} arfCFA;

typedef arfCell (*arfZeroArgFFI)(void);
typedef arfCell (*arfOneArgFFI)(arfCell a);
typedef arfCell (*arfTwoArgFFI)(arfCell a, arfCell b);
typedef arfCell (*arfThreeArgFFI)(arfCell a, arfCell b, arfCell c);
typedef arfCell (*arfFourArgFFI)(arfCell a, arfCell b, arfCell c, arfCell d);
typedef arfCell (*arfFiveArgFFI)(arfCell a, arfCell b, arfCell c, arfCell d,
                                 arfCell e);
typedef arfCell (*arfSixArgFFI)(arfCell a, arfCell b, arfCell c, arfCell d,
                                arfCell e, arfCell f);
typedef arfCell (*arfSevenArgFFI)(arfCell a, arfCell b, arfCell c, arfCell d,
                                  arfCell e, arfCell f, arfCell g);
typedef arfCell (*arfEightArgFFI)(arfCell a, arfCell b, arfCell c, arfCell d,
                                  arfCell e, arfCell f, arfCell g, arfCell h);

ARF::ARF(const uint8_t * dictionary, int dictionarySize,
        int latestOffset, int hereOffset,
        const arfFFI * const lastFFI,
        arfKeyQuestion keyQ, arfKey key, arfEmit emit)
 : lastFFI(lastFFI),
   keyQ(keyQ), key(key), emit(emit),
   dictionary(dictionary), dictionarySize(dictionarySize),
   state(0)
{
    if (latestOffset == -1)
    {
        this->latest = NULL;
    }
    else
    {
        this->latest = const_cast<uint8_t *>(this->dictionary) + latestOffset;
    }

    this->here = const_cast<uint8_t *>(this->dictionary) + hereOffset;
}

arfUnsigned ARF::parenAccept(uint8_t * caddr, arfUnsigned n1)
{
    uint8_t * p = caddr;
    arfUnsigned n2 = 0;
    uint8_t ch;

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

bool ARF::parenFindWord(uint8_t * caddr, arfUnsigned u, uint16_t &xt, bool &isImmediate)
{
    uint8_t searchLen = u;
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
        if (definitionType < arfCFADOFFI0)
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
            const arfFFI * ffi = *(arfFFI **)(curWord + 3);
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
            isImmediate = *(curWord + 2) == arfCFADOIMMEDIATE;
            return true;
        }
    }

    // Search through the opcodes for a match against this search name.
    for (int op = 0; op < 128; op++)
    {
        // Get a reference to this primitive.
        const arfPrimitiveWord &prim = primitives[op];

        // Ignore this opcode if its length is not the same as our
        // search length.
        if (pgm_read_byte(&prim.nameLen) != searchLen)
        {
            continue;
        }

        // Compare the characters.
        if (strncasecmp_P(searchName, (char *)pgm_read_word(&prim.name), searchLen) == 0)
        {
            // TODO isImmediate needs to come from the primitive table
            xt = pgm_read_byte(&prim.opcode);
            isImmediate = false;
            return true;
        }
    }

    // No match.
    return false;
}

// NUMBER? [ARF] "number-question" ( c-addr u -- c-addr u 0 | n -1 )
//
// Attempt to convert a string at c-addr of length u into digits, using
// the radix in BASE.  The number and -1 is returned if the conversion
// was successful, otherwise 0 is returned.
bool ARF::parenNumberQ(uint8_t * caddr, arfUnsigned u, arfInt &n)
{
    // Store the sign as a value to be multipled against the final
    // number.
    arfInt sign = 1;
    if (*caddr == '-')
    {
        sign = -1;
        caddr++;
        u--;
    }

    // Try to convert the (rest of, if we found a sign) number.
    arfUnsigned ud = 0;
    this->parenToNumber(ud, caddr, u);
    if (u == 0)
    {
        n = (arfInt)ud * sign;
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
void ARF::parenToNumber(arfUnsigned &ud, uint8_t * &caddr, arfUnsigned &u)
{
    while (u > 0)
    {
        uint8_t ch = *caddr;
        if (ch < '0')
        {
            break;
        }

        arfUnsigned digit = ch - '0';
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

void ARF::parenParseWord(uint8_t delim, uint8_t * &caddr, arfUnsigned &u)
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
    uint8_t * pParse = caddr = this->source + this->toIn;
    while ((*pParse != delim) && (this->toIn < this->sourceLen))
    {
        pParse++;
        this->toIn++;
    }

    // Set the string length.
    u = pParse - caddr;
}

void ARF::go()
{
    register uint8_t *ip;
    register arfCell tos;
    register arfCell *restDataStack; // Points at the second item on the stack.
    register uint8_t *w;
    register arfCell *returnTop;

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
        &&arfOpDOCOLON,
        0, 0,

        0, 0, 0,
        &&arfOpDOFFI0,

        // $08 - $0F
        &&arfOpDOFFI1,
        &&arfOpDOFFI2,
        &&arfOpDOFFI3,
        &&arfOpDOFFI4,

        &&arfOpDOFFI5,
        &&arfOpDOFFI6,
        &&arfOpDOFFI7,
        &&arfOpDOFFI8,

        // $10 - $17
        &&arfOpLIT,
        &&arfOpDUP,
        &&arfOpDROP,
        &&arfOpPLUS,

        &&arfOpMINUS,
        &&arfOpONEPLUS,
        &&arfOpONEMINUS,
        &&arfOpSWAP,

        // $18 - $1F
        &&arfOpBRANCH,
        &&arfOpABORT,
        &&arfOpCHARLIT,
        &&arfOpCOMPILECOMMA,

        &&arfOpCR,
        &&arfOpEMIT,
        &&arfOpEXECUTE,
        &&arfOpFETCH,

        // $20 - $27
        &&arfOpLITERAL,
        &&arfOpNUMBERQ,
        &&arfOpOR,
        &&arfOpPARSEWORD,

        &&arfOpFINDWORD,
        &&arfOpQDUP,
        &&arfOpSPACE,
        &&arfOpSTATE,

        // $28 - $2F
        &&arfOpSTORE,
        &&arfOpTOIN,
        &&arfOpTWODROP,
        &&arfOpTYPE,

        &&arfOpZBRANCH,
        &&arfOpZERO,
        &&arfOpZEROEQUALS,
        &&arfOpQUIT,

        // $30 - $37
        &&arfOpTIB,
        &&arfOpTIBSIZE,
        &&arfOpACCEPT,
        &&arfOpINTERPRET,

        &&arfOpPSQUOTE,
        &&arfOpBL,
        &&arfOpCFETCH,
        &&arfOpCOUNT,

        // $38 - $3F
        &&arfOpTONUMBER,
        &&arfOpDEPTH,
        &&arfOpDOT,
        &&arfOpPDOTQUOTE,

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
        0, 0, 0, 0,
        0, 0, 0, 0,

        // $68 - $6F
        0, 0, 0, 0,
        0, 0, 0, 0,

        // $70 - $77
        0, 0, 0, 0,
        0, 0, 0, 0,

        // $78 - $7F
        0, 0, 0, 0,
        0, 0, 0,
        &&arfOpEXIT,
    };

    // Jump to ABORT, which initializes the IP, our stacks, etc.
    goto arfOpABORT;

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

        arfOpDOFFI0:
        {
            CHECK_STACK(0, 1);

            w = (uint8_t*)((arfCell*)ip)->p;
            ip += FFIPROCSZ;

        arfOpDOFFI0_WITH_W:
            *--restDataStack = tos;
            tos = (*(arfZeroArgFFI)w)();
        }
        continue;

        arfOpDOFFI1:
        {
            CHECK_STACK(1, 1);

            w = (uint8_t*)((arfCell*)ip)->p;
            ip += FFIPROCSZ;

        arfOpDOFFI1_WITH_W:
            tos = (*(arfOneArgFFI)w)(tos);
        }
        continue;

        arfOpDOFFI2:
        {
            CHECK_STACK(2, 1);

            // FIXME Needs to use inProgramSpace; or maybe the function pointer should be on the stack..?
            arfTwoArgFFI fn = (arfTwoArgFFI)(((arfCell*)ip)->p);
            ip += FFIPROCSZ;

            arfCell arg2 = tos;
            arfCell arg1 = *restDataStack++;
            tos = (*fn)(arg1, arg2);
        }
        continue;

        arfOpDOFFI3:
        {
            CHECK_STACK(3, 1);

            // FIXME Needs to use inProgramSpace; or maybe the function pointer should be on the stack..?
            arfThreeArgFFI fn = (arfThreeArgFFI)(((arfCell*)ip)->p);
            ip += FFIPROCSZ;
            arfCell arg3 = tos;
            arfCell arg2 = *restDataStack++;
            arfCell arg1 = *restDataStack++;
            tos = (*fn)(arg1, arg2, arg3);
        }
        continue;

        arfOpDOFFI4:
            CHECK_STACK(4, 1);
        continue;

        arfOpDOFFI5:
            CHECK_STACK(5, 1);
        continue;

        arfOpDOFFI6:
            CHECK_STACK(6, 1);
        continue;

        arfOpDOFFI7:
            CHECK_STACK(7, 1);
        continue;

        arfOpDOFFI8:
            CHECK_STACK(8, 1);
        continue;

        arfOpLIT:
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
                tos = *(arfCell*)ip;
            }

            ip += CELLSZ;
        }
        continue;

        arfOpDUP:
        {
            CHECK_STACK(1, 2);
            *--restDataStack = tos;
        }
        continue;

        arfOpDROP:
        {
            CHECK_STACK(1, 0);
            tos = *restDataStack++;
        }
        continue;

        arfOpPLUS:
        {
            CHECK_STACK(2, 1);
            tos.i += restDataStack++->i;
        }
        continue;

        arfOpMINUS:
        {
            CHECK_STACK(2, 1);
            tos.i = restDataStack++->i - tos.i;
        }
        continue;

        arfOpONEPLUS:
        {
            CHECK_STACK(1, 1);
            tos.i++;
        }
        continue;

        arfOpONEMINUS:
        {
            CHECK_STACK(1, 1);
            tos.i--;
        }
        continue;

        arfOpSWAP:
        {
            CHECK_STACK(2, 2);
            arfCell swap = restDataStack[0];
            restDataStack[0] = tos;
            tos = swap;
        }
        continue;

        // TODO Add docs re: Note that (branch) and (0branch) offsets
        // are 8-bit relative offsets.  UNLIKE word addresses, the IP
        // points at the offset in arfOpBRANCH/arfOpZBRANCH.  These
        // offsets can be positive or negative because branches can go
        // both forwards and backwards.
        arfOpBRANCH:
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
            goto arfOpABORT;
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
            goto arfOpABORT;
        }
        continue;
#endif
        // -------------------------------------------------------------
        // ABORT [CORE] 6.1.0670 ( i*x -- ) ( R: j*x -- )
        //
        // Empty the data stack and perform the function of QUIT, which
        // includes emptying the return stack, without displaying a
        // message.
        arfOpABORT:
        {
            tos.i = 0;
            restDataStack = (arfCell*)&this->dataStack[32];
            this->base = 10;
            goto arfOpQUIT;
        }
        continue;

        arfOpCHARLIT:
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

        arfOpCOMPILECOMMA:
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
            // offset to the word's PFA (no CFAs in ARF) and that the
            // offset is as of the address *after* the opcode and
            // *before* the offset since that's where the IP will be in
            // the DO* opcode when we calculate the address.
            //
            // TODO Implement this
            tos = *restDataStack++;
        }
        continue;

        arfOpCR:
        {
            CHECK_STACK(0, 0);

            if (this->emit != NULL)
            {
                this->emit('\n');
            }
        }
        continue;

        arfOpEMIT:
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
        arfOpEXECUTE:
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
                if (definitionType < arfCFADOFFI0)
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
                    const arfFFI * ffi = *(arfFFI **)pTarget;
                    const void * ffiFn = (void *)pgm_read_word(&ffi->fn);
                    w = (uint8_t*)ffiFn;
                }

                // Drop the XT and get a new TOS.
                tos = *restDataStack++;

                // Dispatch to the opcode that handles this type of
                // definition.
                // TODO Move the non-*_W opcodes to 16-32 and the _W
                // opcodes to 0-16; then we can just jump to them by
                // dereferencing jtb using definitionType.
                switch (definitionType)
                {
                    case arfCFADOCOLON:
                        goto arfOpDOCOLON_WITH_W;

                    case arfCFADOFFI0:
                        goto arfOpDOFFI0_WITH_W;

                    case arfCFADOFFI1:
                        goto arfOpDOFFI1_WITH_W;

                    default:
                        if (this->emit != NULL)
                        {
                            this->emit('\n');
                            this->emit('!');
                            this->emit('D');
                            this->emit('E');
                            this->emit('F');
                            this->emit('\n');
                        }
                        goto arfOpABORT;
                }
            }
        }
        continue;

        arfOpFETCH:
        {
            CHECK_STACK(1, 1);
            tos = *(arfCell*)tos.p;
        }
        continue;

        arfOpLITERAL:
        {
            // TODO Implement this.
        }
        continue;

        // -------------------------------------------------------------
        // NUMBER? [ARF] "number-question" ( c-addr u -- c-addr u 0 | n -1 )
        //
        // Attempt to convert a string at c-addr of length u into
        // digits, using the radix in BASE.  The number and -1 is
        // returned if the conversion was successful, otherwise 0 is
        // returned.
        arfOpNUMBERQ:
        {
            CHECK_STACK(2, 3);

            arfUnsigned u = tos.u;
            uint8_t * caddr = (uint8_t*)restDataStack->p;

            arfInt n;
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

        arfOpOR:
        {
            CHECK_STACK(2, 1);
            tos.i |= restDataStack++->i;
        }
        continue;

        // -------------------------------------------------------------
        // PARSE-WORD [ARF] ( "<spaces>name<space>" -- c-addr u )
        //
        // Skip leading spaces and parse name delimited by a space.
        // c-addr is the address within the input buffer and u is the
        // length of the selected string. If the parse area is empty,
        // the resulting string has a zero length.
        arfOpPARSEWORD:
        {
            CHECK_STACK(0, 2);

            *--restDataStack = tos;

            uint8_t * caddr;
            arfUnsigned u;
            parenParseWord(' ', caddr, u);
            (--restDataStack)->p = caddr;
            tos.u = u;
        }
        continue;

        // -------------------------------------------------------------
        // FIND-WORD [ARF] "paren-find-paren" ( c-addr u -- c-addr u 0 | xt 1 | xt -1 )
        //
        // Find the definition named in the string at c-addr with length
        // u in the word list whose latest definition is pointed to by
        // nfa.  If the definition is not found, return the string and
        // zero.  If the definition is found, return its execution token
        // xt.  If the definition is immediate, also return one (1),
        // otherwise also return minus-one (-1).  For a given string,
        // the values returned by FIND-WORD while compiling may differ
        // from those returned while not compiling.
        arfOpFINDWORD:
        {
            CHECK_STACK(2, 3);

            uint8_t * caddr = (uint8_t *)restDataStack->p;
            arfUnsigned u = tos.u;

            uint16_t xt;
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

        arfOpQDUP:
        {
            CHECK_STACK(1, 2);

            if (tos.i != 0)
            {
                *--restDataStack = tos;
            }
        }
        continue;

        arfOpSPACE:
        {
            CHECK_STACK(0, 0);

            if (this->emit != NULL)
            {
                this->emit(' ');
            }
        }
        continue;

        arfOpSTATE:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.p = &this->state;
        }
        continue;

        // -------------------------------------------------------------
        // ! [CORE] 6.1.0010 "store" ( x a-addr -- )
        //
        // Store x at a-addr.
        arfOpSTORE:
        {
            CHECK_STACK(2, 0);
            *(arfCell*)tos.p = *restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        // -------------------------------------------------------------
        // >IN [CORE] 6.1.0560 "to-in" ( -- a-addr )
        //
        // a-addr is the address of a cell containing the offset in
        // characters from the start of the input buffer to the start of
        // the parse area.
        arfOpTOIN:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.p = &this->toIn;
        }
        continue;

        arfOpTWODROP:
        {
            CHECK_STACK(2, 0);
            restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        // NOTE: Only works for strings stored in data space!
        arfOpTYPE:
        {
            CHECK_STACK(2, 0);

            if (this->emit != NULL)
            {
                for (int i = 0; i < tos.i; i++)
                {
                    this->emit(*((uint8_t*)restDataStack->p + i));
                }
            }

            restDataStack++;
            tos = *restDataStack++;
        }
        continue;

        arfOpZBRANCH:
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

        arfOpZERO:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.i = 0;
        }
        continue;

        arfOpZEROEQUALS:
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
        arfOpQUIT:
        {
            CHECK_STACK(0, 0);
            returnTop = (arfCell *)&this->returnStack[32];
            this->state = 0;

            // : (QUIT)  ( --)
            //   BEGIN
            //       TIB  DUP TIBSIZE ACCEPT  SPACE
            //       INTERPRET
            //       CR  STATE @ 0= IF ." ok " THEN
            //   AGAIN ;
            static const int8_t parenQuit[] PROGMEM = {
                arfOpTIB, arfOpDUP, arfOpTIBSIZE, arfOpACCEPT, arfOpSPACE,
                arfOpINTERPRET,
                arfOpCR, arfOpSTATE, arfOpFETCH, arfOpZEROEQUALS,
                    arfOpZBRANCH, 6,
                arfOpPDOTQUOTE, 3, 'o', 'k', ' ',
                arfOpBRANCH, -18
            };

#ifdef __AVR__
            if (inProgramSpace)
            {
                ip = (uint8_t*)((unsigned int)ip | 0x8000);
            }
#endif
            (--returnTop)->p = (void *)ip;

            ip = (uint8_t*)&parenQuit;
#ifdef __AVR__
            inProgramSpace = true;
#endif
        }
        continue;

        arfOpTIB:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.p = &this->tib;
        }
        continue;

        arfOpTIBSIZE:
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
        arfOpACCEPT:
        {
            CHECK_STACK(2, 1);
            arfCell n1 = tos;
            arfCell caddr = *restDataStack++;
            tos.u = parenAccept((uint8_t *)caddr.p, n1.u);
        }
        continue;

        // -----------------------------------------------------------------
        // INTERPRET [ARF] ( i*x c-addr u -- j*x )
        //
        // Interpret the given string.
        arfOpINTERPRET:
        {
            CHECK_STACK(2, 0);
            this->source = (uint8_t *)restDataStack++->p;
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
                arfOpZERO, arfOpTOIN, arfOpSTORE,
                arfOpPARSEWORD, arfOpDUP, arfOpZBRANCH, 37,
                arfOpFINDWORD, arfOpQDUP, arfOpZBRANCH, 14,
                arfOpONEPLUS, arfOpSTATE, arfOpFETCH, arfOpZEROEQUALS, arfOpOR,
                    arfOpZBRANCH, 4,
                arfOpEXECUTE, arfOpBRANCH, 21,
                arfOpCOMPILECOMMA, arfOpBRANCH, 18,
                arfOpNUMBERQ, arfOpZBRANCH, 8,
                arfOpSTATE, arfOpFETCH, arfOpZBRANCH, 11,
                arfOpLITERAL, arfOpBRANCH, 8,
                arfOpTYPE, arfOpSPACE, arfOpCHARLIT, '?', arfOpEMIT, arfOpCR,
                    arfOpABORT,
                arfOpBRANCH, -39,
                arfOpTWODROP,
                arfOpEXIT,
            };

#ifdef __AVR__
            if (inProgramSpace)
            {
                ip = (uint8_t*)((unsigned int)ip | 0x8000);
            }
#endif
            (--returnTop)->p = (void *)ip;

            ip = (uint8_t*)&parenInterpret;
#ifdef __AVR__
            inProgramSpace = true;
#endif
        }
        continue;

        // -------------------------------------------------------------
        // (s") [ARF] "paren-s-quote-paren" ( -- c-addr u )
        //
        // Runtime behavior of S": return c-addr and u.
        // NOTE: Cannot be used in program space!
        arfOpPSQUOTE:
        {
            CHECK_STACK(0, 2);

            // Push existing TOS onto the stack.
            *--restDataStack = tos;

            // Instruction stream contains the length of the string as a
            // byte and then the string itself.  Start out by pushing
            // the address of the string (ip+1) onto the stack.
            (--restDataStack)->p = ip + 1;

            // Now get the string length into TOS.
            tos.i = *ip++;

            // Advance IP over the string.
            ip = ip + tos.i;
        }
        continue;

        arfOpBL:
        {
            CHECK_STACK(0, 1);
            *--restDataStack = tos;
            tos.u = ' ';
        }
        continue;

        arfOpCFETCH:
        {
            CHECK_STACK(1, 1);
            tos.u = *(uint8_t*)tos.p;
        }
        continue;

        arfOpCOUNT:
        {
            CHECK_STACK(1, 2);
            (--restDataStack)->p = (uint8_t*)tos.p + 1;
            tos.u = *(uint8_t*)tos.p;
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
        arfOpTONUMBER:
        {
            CHECK_STACK(3, 3);

            arfUnsigned u = tos.u;
            uint8_t * caddr = (uint8_t*)restDataStack[0].p;
            arfUnsigned ud = restDataStack[-1].u;

            parenToNumber(ud, caddr, u);

            restDataStack[-1].u = ud;
            restDataStack[0].p = caddr;
            tos.u = u;
        }
        continue;

        // -------------------------------------------------------------
        // DEPTH [CORE] 6.1.1200 ( -- +n )
        //
        // +n is the number of single-cell values contained in the data
        // stack before +n was placed on the stack.
        arfOpDEPTH:
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
        arfOpDOT:
        {
            CHECK_STACK(1, 0);

            if (this->emit != NULL)
            {
                // FIXME These numbers are printing backwards; we
                // probably need to implement pictured numeric output.
                // FIXME Use BASE.
                do
                {
                    arfInt i = tos.i - ((tos.i / 10) * 10);
                    this->emit('0' + i);
                    tos.i = tos.i / 10;
                } while (tos.i != 0);

                this->emit(' ');
            }

            tos = *restDataStack++;
        }
        continue;

        // -------------------------------------------------------------
        // (.") [ARF] "paren-dot-quote-paren" ( -- )
        //
        // Prints the string that was compiled into the definition.
        arfOpPDOTQUOTE:
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

        arfOpDOCOLON:
        {
            // IP currently points to the relative offset of the PFA of
            // the target word.  Read that offset and advance IP to the
            // opcode after the offset.
            w = ip + ((arfCell*)ip)->i;
            ip += CELLSZ;

        arfOpDOCOLON_WITH_W:
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
            (--returnTop)->p = (void *)ip;

            // Now set the IP to the PFA of the word that is being
            // called and continue execution inside of that word.
            ip = w;
        }
        continue;

        arfOpEXIT:
        {
            ip = (uint8_t *)((returnTop++)->p);

#ifdef __AVR__
            if (((unsigned int)ip & 0x8000) != 0)
            {
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
