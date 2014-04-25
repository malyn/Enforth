#ifndef ARF_H_
#define ARF_H_

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

class ARF
{
	public:
		ARF(const uint8_t * dictionary, int dictionarySize);
		void go(int (*sketchFn)(void));
	
	private:
		const uint8_t * dictionary;
		const int dictionarySize;
		uint8_t * here;

		arfCell dataStack[32];
		arfCell returnStack[32];
		
		void innerInterpreter(uint8_t * xt);
};

#endif /* ARF_H_ */
