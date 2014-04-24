#include <avr/pgmspace.h>

#include "ARF.h"

typedef int arfInt;
typedef int arfUnsigned;

typedef union arfCell
{
	arfInt i;
	arfUnsigned u;
	void *p;
} arfCell;

typedef enum
{
	arfOpZeroArgFFI = 0x00,
	arfOpOneArgFFI,
	arfOpTwoArgFFI,
	arfOpThreeArgFFI,
	arfOpFourArgFFI,
	arfOpFiveArgFFI,
	arfOpSixArgFFI,
	arfOpSevenArgFFI,
	arfOpEightArgFFI,
	arfOpDUP,
	arfOpDROP,
	arfOpPLUS,
	arfOpMINUS,
	arfOpONEPLUS,
	arfOpONEMINUS,
	arfOpSWAP,
	//...
	arfOpEXIT,
} arfOpcode;

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

static int add(int a, int b)
{
	return a + b;
}

static int sum(int a, int b, int c)
{
	return a + b + c;
}

ARF::ARF(const uint8_t * dictionary, int dictionarySize)
 : dictionary(dictionary), dictionarySize(dictionarySize)
{
	this->here = const_cast<uint8_t *>(this->dictionary);
}

void ARF::go(int (*sketchFn)(void))
{
	// TODO Remove this test code.
	*this->here++ = 'f';
	*this->here++ = 'o';
	*this->here++ = 'o';

	*this->here++ = (char)-3;
	*this->here++ = 0x00; // LFAhi
	*this->here++ = 0x00; // LFAlo

#if true
	*this->here++ = arfOpDUP; // PFA start

	*this->here++ = arfOpZeroArgFFI;
	*this->here++ = ((unsigned int)sketchFn) & 0xff;
	*this->here++ = (((unsigned int)sketchFn) >> 8) & 0xff;
	*this->here++ = arfOpZeroArgFFI;
	*this->here++ = ((unsigned int)sketchFn) & 0xff;
	*this->here++ = (((unsigned int)sketchFn) >> 8) & 0xff;
	*this->here++ = arfOpTwoArgFFI;
	*this->here++ = ((unsigned int)add) & 0xff;
	*this->here++ = (((unsigned int)add) >> 8) & 0xff;

	*this->here++ = arfOpZeroArgFFI;
	*this->here++ = ((unsigned int)sketchFn) & 0xff;
	*this->here++ = (((unsigned int)sketchFn) >> 8) & 0xff;
	*this->here++ = arfOpZeroArgFFI;
	*this->here++ = ((unsigned int)sketchFn) & 0xff;
	*this->here++ = (((unsigned int)sketchFn) >> 8) & 0xff;
	*this->here++ = arfOpZeroArgFFI;
	*this->here++ = ((unsigned int)sketchFn) & 0xff;
	*this->here++ = (((unsigned int)sketchFn) >> 8) & 0xff;
	*this->here++ = arfOpThreeArgFFI;
	*this->here++ = ((unsigned int)sum) & 0xff;
	*this->here++ = (((unsigned int)sum) >> 8) & 0xff;

	*this->here++ = arfOpEXIT;
#else
	*this->here++ = arfOpZeroArgFFI;
	*this->here++ = ((unsigned int)sketchFn) & 0xff;
	*this->here++ = (((unsigned int)sketchFn) >> 8) & 0xff;
	*this->here++ = arfOpDROP;

	*this->here++ = arfOpZeroArgFFI;
	*this->here++ = ((unsigned int)sketchFn) & 0xff;
	*this->here++ = (((unsigned int)sketchFn) >> 8) & 0xff;
	*this->here++ = arfOpDROP;

	*this->here++ = arfOpZeroArgFFI;
	*this->here++ = ((unsigned int)sketchFn) & 0xff;
	*this->here++ = (((unsigned int)sketchFn) >> 8) & 0xff;
	*this->here++ = arfOpDROP;
#endif

	this->dataStack[0] = 0x27;
	this->dataStack[1] = 0x00;

	this->innerInterpreter(const_cast<uint8_t *>(this->dictionary) + 6);
}

void ARF::innerInterpreter(uint8_t * xt)
{
	register uint8_t *ip;
	register arfCell tos;
	register arfCell *restDataStack;
	register arfCell *returnTop;
	
	arfOpcode op;
	
	arfInt i;
	
	// Initialize our local variables.
	ip = (uint8_t *)xt;
	tos = ((arfCell*)this->dataStack)[0];
	restDataStack = (arfCell*)this->dataStack[-1];
	returnTop = (arfCell *)this->returnStack;
	
	static const void * const jtb[] PROGMEM = {
		&&arfOpZeroArgFFI, &&arfOpOneArgFFI, &&arfOpTwoArgFFI,
		&&arfOpThreeArgFFI, &&arfOpFourArgFFI, &&arfOpFiveArgFFI,
		&&arfOpSixArgFFI, &&arfOpSevenArgFFI, &&arfOpEightArgFFI,
		&&arfOpDUP, &&arfOpDROP, &&arfOpPLUS, &&arfOpMINUS,
		&&arfOpONEPLUS, &&arfOpONEMINUS, &&arfOpSWAP, &&arfOpEXIT,
	};

	while (true)
	{
		// Get the next opcode and dispatch to the label.
		op = (arfOpcode)*ip++;
		goto *(void *)pgm_read_word(&jtb[op]);

		arfOpZeroArgFFI:
		{
			arfZeroArgFFI fn = (arfZeroArgFFI)(((arfCell*)ip)->p);
			ip += 2;
			tos = (*fn)();
		}
		continue;

		arfOpOneArgFFI:
		{
			arfOneArgFFI fn = (arfOneArgFFI)(((arfCell*)ip)->p);
			ip += 2;
			tos = (*fn)(tos);
		}
		continue;

		arfOpTwoArgFFI:
		{
			arfTwoArgFFI fn = (arfTwoArgFFI)(((arfCell*)ip)->p);
			ip += 2;
			arfCell arg2 = tos;
			arfCell arg1 = *restDataStack--;
			tos = (*fn)(arg1, arg2);
		}
		continue;

		arfOpThreeArgFFI:
		{
			arfThreeArgFFI fn = (arfThreeArgFFI)(((arfCell*)ip)->p);
			ip += 2;
			arfCell arg3 = tos;
			arfCell arg2 = *restDataStack--;
			arfCell arg1 = *restDataStack--;
			tos = (*fn)(arg1, arg2, arg3);
		}
		continue;

		arfOpFourArgFFI:
		continue;

		arfOpFiveArgFFI:
		continue;

		arfOpSixArgFFI:
		continue;

		arfOpSevenArgFFI:
		continue;

		arfOpEightArgFFI:
		continue;

		arfOpDUP:
		{
			*++restDataStack = tos;
		}
		continue;

		arfOpDROP:
		{
			tos = *--restDataStack;
		}
		continue;

		arfOpPLUS:
		{
			i = (restDataStack--)->i;
			tos.i += i;
		}
		continue;

		arfOpMINUS:
		{
			i = (restDataStack--)->i;
			tos.i -= i;
		}
		continue;

		arfOpONEPLUS:
		{
			tos.i++;
		}
		continue;

		arfOpONEMINUS:
		{
			tos.i--;
		}
		continue;

		arfOpSWAP:
		{
			arfCell swap = *restDataStack;
			*restDataStack = tos;
			tos = swap;
		}
		continue;

		arfOpEXIT:
			// Not really; this is just here for testing.
			return;
		continue;
	}
}
