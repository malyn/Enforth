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

#ifndef MFORTH_H_
#define MFORTH_H_

#include <stdlib.h>
#include <inttypes.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM
#endif

// FFI Macros
#define LAST_FFI NULL

#define MFORTH_EXTERN(name, fn, arity) \
    static const char FFIDEF_ ## name ## _NAME[] PROGMEM = #name; \
    static const MFORTH::FFIDef FFIDEF_##name PROGMEM = { LAST_FFI, FFIDEF_ ## name ## _NAME, arity, (void*)fn };

#define GET_LAST_FFI(name) &FFIDEF_ ## name

class MFORTH
{
    // Public Typedefs
    public:
        typedef bool (*KeyQuestion)(void);
        typedef char (*Key)(void);
        typedef void (*Emit)(char);

        typedef struct FFIDef
        {
            const FFIDef * const prev;
            const char * const name;
            uint8_t arity;
            void * fn;
        } FFIDef;

        static const int FFIProcPtrSize = sizeof(void*);


    // Private Typedefs
    private:
#ifdef __AVR__
        typedef int16_t Int;
        typedef uint16_t Unsigned;
#else
        typedef int32_t Int;
        typedef uint32_t Unsigned;
#endif

        // Execution Tokens (XTs) are always 16-bits, even on 32-bit
        // processors or processors with more than 16 bits of program
        // space.  MFORTH handles this constraint by ensuring that all
        // XTs are relative to the start of the dictionary.
        typedef uint16_t XT;

        typedef union
        {
            Int i;
            Unsigned u;

            // Pointer to RAM.  Why not just a generic pointer?  Because
            // MFORTH works on processors that have more program space
            // than can be referenced by a cell-sized pointer.  The name
            // of this field is designed to make it clear that cells can
            // only reference addresses in RAM.
            void * pRAM;
        } Cell;

        static const int CellSize = sizeof(Cell);

        typedef Cell (*ZeroArgFFI)(void);
        typedef Cell (*OneArgFFI)(Cell a);
        typedef Cell (*TwoArgFFI)(Cell a, Cell b);
        typedef Cell (*ThreeArgFFI)(Cell a, Cell b, Cell c);
        typedef Cell (*FourArgFFI)(Cell a, Cell b, Cell c, Cell d);
        typedef Cell (*FiveArgFFI)(Cell a, Cell b, Cell c, Cell d,
                                   Cell e);
        typedef Cell (*SixArgFFI)(Cell a, Cell b, Cell c, Cell d,
                                  Cell e, Cell f);
        typedef Cell (*SevenArgFFI)(Cell a, Cell b, Cell c, Cell d,
                                    Cell e, Cell f, Cell g);
        typedef Cell (*EightArgFFI)(Cell a, Cell b, Cell c, Cell d,
                                    Cell e, Cell f, Cell g, Cell h);


    // Private enums.
    private:
        typedef enum
        {
            CFADOCOLON = 0,
            CFADOIMMEDIATE,
            CFADOCONSTANT,
            CFADOCREATE,
            CFADODOES,
            CFADOVARIABLE,
            CFADOFFI0,
            CFADOFFI1,
            CFADOFFI2,
            CFADOFFI3,
            CFADOFFI4,
            CFADOFFI5,
            CFADOFFI6,
            CFADOFFI7,
            CFADOFFI8
        } CFA;

        // TODO We've gotten our names a bit wrong here.  These aren't
        // "opcodes" as much as they are "primitive words".  Similarly,
        // the RAM definitions are "user-defined words."
        //
        // The key point here is that the PFA contains a list of tokens
        // to primitive words.  There is no longer a concept of "opcode"
        // since MFORTH is a Forth machine and not a CPU.
        typedef enum
        {
            //COLD = 0x00,
            LIT = 0x01,
            DUP,
            DROP,
            PLUS,
            MINUS,
            ONEPLUS,
            ONEMINUS,
            SWAP,
            BRANCH,
            ABORT,
            CHARLIT,
            COMPILECOMMA,
            CR,
            EMIT,
            EXECUTE,
            FETCH,
            LITERAL,
            NUMBERQ,
            OR,
            PARSEWORD,
            FINDWORD,
            QDUP,
            SPACE,
            STATE,
            STORE,
            TOIN,
            TWODROP,
            TYPE,
            ZBRANCH,
            ZERO,
            ZEROEQUALS,
            unused_was_QUIT, // UNUSED
            TIB,
            TIBSIZE,
            ACCEPT,
            INTERPRET,
            PSQUOTE,
            BL,
            CFETCH,
            COUNT,
            TONUMBER,
            DEPTH,
            unused_was_DOT, // UNUSED
            PDOTQUOTE,
            BACKSLASH,
            HEX,
            unused_was_CREATE, // UNUSED
            HERE,
            LATEST,
            TWODUP,
            COMMA,
            CCOMMA,
            TUCK,
            ALIGN,
            MOVE,
            CPLUSSTORE,
            ALLOT,
            NIP,
            WCOMMA,
            unused_was_COLON, // COLON
            HIDE,
            RTBRACKET,
            CSTORE,
            unused_was_SEMICOLON, // UNUSED
            REVEAL,
            LTBRACKET,
            ABS,
            LESSNUMSIGN,
            unused_was_NUMSIGNS, // UNUSED
            ROT,
            unused_was_SIGN, // UNUSED
            NUMSIGNGRTR,
            unused_was_NUMSIGN, // UNUSED
            ZEROLESS,
            HOLD,
            BASE,
            UDSLASHMOD,
            GREATERTHAN,
            AND,
            NOTEQUALS,
            unused_was_UDOT, // UNUSED
            INITRP,

            //...

            // Opcodes 0x61-0x6e are reserved for jump labels to the "CFA"
            // opcodes *after* the point where W (the Word Pointer) has already
            // been set.  This allows words like EXECUTE to jump to a CFA
            // without having to use a switch statement to convert definition
            // types to opcodes.  The opcode names themselves do not need to be
            // defined because they are never referenced (we're just reserving
            // space in the primitive list and Address Interpreter jump table,
            // in other words), but we do list them here in order to make it
            // easier to turn raw opcodes into enum values in the debugger.
            PDOCOLON = 0x60,
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

            // Just like the above, these opcodes are never used in code and
            // this list of enum values is only used to simplify debugging.
            DOCOLON = 0x70,
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

            // This is a normal opcode and is placed at the end in order
            // to make it easier to identify in definitions.
            EXIT = 0x7F,

            // ROM definitions
            // TODO: Ultimately these will be scattered throughout the
            // token space, but for now we're just putting them here
            // until we have the build tool that will organize the
            // tokens and generate .h/.cpp files.
            SEMICOLON = 0x80,
            UDOT = 0x82,
            DOT = 0x84,
            CREATE = 0x89,
            COLON = 0x93,
            NUMSIGNS = 0x96,
            SIGN = 0x98,
            NUMSIGN = 0x9a,
            QUIT = 0x9f,
        } Opcode;


    public:
        MFORTH(uint8_t * const dictionary, int dictionarySize,
                const FFIDef * const lastFFI,
                // TODO Remove this and just have it be in the FFI
                // (which we look up in the constructor and then stash
                // into our private function pointers).
                KeyQuestion keyQ, Key key, Emit emit);
        void addDefinition(const uint8_t * const def, int defSize);
        void go();

    private:
        // VM constants
        const FFIDef * const lastFFI;

        const KeyQuestion keyQ;
        const Key key;
        const Emit emit;

        uint8_t * const dictionary;
        const int dictionarySize;

        // Dictionary vars
        uint8_t * dp;
        uint8_t * latest; // NULL means empty dictionary

        // Transient (dictionary-based) vars
        uint8_t * hld;

        // Text Interpreter vars
        Int state;

        uint8_t tib[80];
        uint8_t * source;
        Int sourceLen;
        Int toIn;

        uint8_t * prevLeave;

        // Task vars
        Unsigned base;
        Cell dataStack[32];
        Cell returnStack[32];

        Unsigned parenAccept(uint8_t * caddr, Unsigned n1);
        bool parenFindWord(uint8_t * caddr, Unsigned u, XT &xt, bool &isImmediate);
        bool parenNumberQ(uint8_t * caddr, Unsigned u, Int &n);
        void parenToNumber(Unsigned &ud, uint8_t * &caddr, Unsigned &u);
        void parenParseWord(uint8_t delim, uint8_t * &caddr, Unsigned &u);
};

#endif /* MFORTH_H_ */
