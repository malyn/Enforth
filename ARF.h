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
				arfKeyQuestion keyQ, arfKey key, arfEmit emit);
		void go();
	
	private:
		const arfKeyQuestion keyQ;
		const arfKey key;
		const arfEmit emit;

		const uint8_t * dictionary;
		const int dictionarySize;
		uint8_t * here;

		arfInt state;
		arfUnsigned base;

		arfCell dataStack[32];
		arfCell returnStack[32];

		uint8_t tib[64];
		uint8_t word[32];

		uint8_t * source;
		arfInt sourceLen;
		arfInt toIn;

		void * docolonCFA;

		void compileParenInterpret();
		void compileParenQuit();
		void compileParenEvaluate();

		arfUnsigned parenAccept(uint8_t * caddr, arfUnsigned n1);
		bool parenFind(uint8_t * caddr, arfUnsigned &xt, bool &isImmediate);
		bool parenNumberQ(uint8_t * caddr, arfUnsigned u, arfInt &n);
		void parenToNumber(arfUnsigned &ud, uint8_t * &caddr, arfUnsigned &u);
		void parenWord(uint8_t delim);
};

#endif /* ARF_H_ */
