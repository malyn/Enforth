#ifndef ARF_H_
#define ARF_H_

#include <inttypes.h>

class ARF
{
	public:
		ARF(const uint8_t * dictionary, int dictionarySize);
		void go(int (*sketchFn)(void));
	
	private:
		const uint8_t * dictionary;
		const int dictionarySize;
		uint8_t * here;

		uint16_t dataStack[32];
		uint16_t returnStack[32];
		
		void innerInterpreter(uint8_t * xt);
};

#endif /* ARF_H_ */
