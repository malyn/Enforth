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

#ifndef ARF_H_
#define ARF_H_

#include <stdlib.h>
#include <inttypes.h>

#ifdef __AVR__
typedef int16_t arfInt;
typedef uint16_t arfUnsigned;
#else
typedef int32_t arfInt;
typedef uint32_t arfUnsigned;
#endif

typedef union arfCell
{
    arfInt i;
    arfUnsigned u;
    void *p;
} arfCell;

typedef arfInt (*arfKeyQuestion)(void);
typedef arfUnsigned (*arfKey)(void);
typedef void (*arfEmit)(arfUnsigned);

class ARF
{
    public:
        ARF(const uint8_t * dictionary, int dictionarySize,
                int latestOffset, int hereOffset,
                arfKeyQuestion keyQ, arfKey key, arfEmit emit);
        void go();

    private:
        const arfKeyQuestion keyQ;
        const arfKey key;
        const arfEmit emit;

        const uint8_t * dictionary;
        const int dictionarySize;
        uint8_t * latest; // NULL means empty dictionary
        uint8_t * here;

        arfInt state;
        arfUnsigned base;

        arfCell dataStack[32];
        arfCell returnStack[32];

        uint8_t tib[64];

        uint8_t * source;
        arfInt sourceLen;
        arfInt toIn;

        arfUnsigned parenAccept(uint8_t * caddr, arfUnsigned n1);
        bool parenFindWord(uint8_t * caddr, arfUnsigned u, uint16_t &xt, bool &isImmediate);
        bool parenNumberQ(uint8_t * caddr, arfUnsigned u, arfInt &n);
        void parenToNumber(arfUnsigned &ud, uint8_t * &caddr, arfUnsigned &u);
        void parenParseWord(uint8_t delim, uint8_t * &caddr, arfUnsigned &u);
};

#endif /* ARF_H_ */
