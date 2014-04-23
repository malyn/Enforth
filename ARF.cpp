#include <string.h>

#include <avr/pgmspace.h>
#include <avr/io.h>

#define ARF_STACK_SIZE 32

typedef int arfInt;
typedef int arfUnsigned;

typedef union arfCell
{
	arfInt i;
	arfUnsigned u;
	void *p;
} arfCell;

typedef struct arfStack
{
	arfCell *top;
	arfCell base[ARF_STACK_SIZE];
} arfStack;

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

static int favnum(void)
{
	return 27;
}

static int add(int a, int b)
{
	return a + b;
}

static int sum(int a, int b, int c)
{
	return a + b + c;
}

// RAM
unsigned char dictionary[1024];
unsigned char * here;

arfStack dataStack;
arfStack returnStack;

static void arfInnerInterpreter(arfOpcode *xt)
{
	arfOpcode *ip;
	arfCell *dataTop;
	arfCell *returnTop;
	
	arfOpcode op;
	
	arfInt i;
	
	// Initialize our local variables.
	ip = xt;
	dataTop = dataStack.top;
	returnTop = returnStack.top;
	
	while (true)
	{
		// Get the next opcode.
		op = *ip++;
		
		// Is the high bit set?  If so, then this is the first byte of a
		// two-byte word that points to a RAM-based indirect-threaded
		// reference.  Otherwise it's an opcode.
		if ((unsigned char)op & 0x80)
		{
			while (true)
			{
				// I'm stuck!
				PORTB = 0x01;
			}
		}
		else
		{
			static const void * const jtb[] PROGMEM = {
				&&arfOpZeroArgFFI, &&arfOpOneArgFFI, &&arfOpTwoArgFFI,
				&&arfOpThreeArgFFI, &&arfOpFourArgFFI,
				&&arfOpFiveArgFFI, &&arfOpSixArgFFI, &&arfOpSevenArgFFI,
				&&arfOpEightArgFFI, &&arfOpDUP, &&arfOpDROP,
				&&arfOpPLUS, &&arfOpMINUS, &&arfOpONEPLUS,
				&&arfOpONEMINUS, &&arfOpSWAP, &&arfOpEXIT,
			};
			void * oplbl = (void *)pgm_read_word(&jtb[op]);
			goto *oplbl;
			continue;

			while (0)
			{
				arfOpZeroArgFFI:
				{
					arfZeroArgFFI fn = (arfZeroArgFFI)(((arfCell*)ip)->p);
					ip += 2;
					*(++dataTop) = (*fn)();
				}
				break;

				arfOpOneArgFFI:
				{
					arfOneArgFFI fn = (arfOneArgFFI)(((arfCell*)ip)->p);
					ip += 2;
					*dataTop = (*fn)(*dataTop);
				}
				break;

				arfOpTwoArgFFI:
				{
					arfTwoArgFFI fn = (arfTwoArgFFI)(((arfCell*)ip)->p);
					ip += 2;
					*dataTop = (*fn)(*dataTop--, *dataTop);
				}
				break;

				arfOpThreeArgFFI:
				{
					arfThreeArgFFI fn = (arfThreeArgFFI)(((arfCell*)ip)->p);
					ip += 2;
					*dataTop = (*fn)(*dataTop--, *dataTop--, *dataTop);
				}
				break;

				arfOpFourArgFFI:
				break;

				arfOpFiveArgFFI:
				break;

				arfOpSixArgFFI:
				break;

				arfOpSevenArgFFI:
				break;

				arfOpEightArgFFI:
				break;

				arfOpDUP:
				{
					i = dataTop->i;
					(++dataTop)->i = i;
				}
				break;

				arfOpDROP:
				{
					dataTop--;
				}
				break;

				arfOpPLUS:
				{
					i = (dataTop--)->i;
					dataTop->i += i;
				}
				break;

				arfOpMINUS:
				{
					i = (dataTop--)->i;
					dataTop->i -= i;
				}
				break;

				arfOpONEPLUS:
				{
					dataTop->i++;
				}
				break;

				arfOpONEMINUS:
				{
					dataTop->i--;
				}
				break;

				arfOpSWAP:
				{
					arfCell swap;
					swap = dataTop[0];
					dataTop[0] = dataTop[-1];
					dataTop[-1] = swap;
				}
				break;

				arfOpEXIT:
					// Not really; this is just here for testing.
					return;
				break;
			}
		}
	}
}

int main(void)
{
	// Initialize and clear the dictionary.
	here = dictionary;
	memset(&dictionary, 0, sizeof(dictionary));
	
	// Initialize and clear the stacks.
	memset(&dataStack.base, 0, sizeof(dataStack.base));
	dataStack.top = dataStack.base;
	dataStack.top->i = 0x27;

	memset(&returnStack.base, 0, sizeof(returnStack.base));
	returnStack.top = returnStack.base;
	
	// Put some stuff in the dictionary.
	// : test FOO test ;
	memcpy(dictionary, "foo", 3);
	here += 3;

	*here++ = (char)-3;
	*here++ = 0x00; // LFAhi
	*here++ = 0x00; // LFAlo

	*here++ = arfOpDUP; // PFA start

	*here++ = arfOpZeroArgFFI;
	*here++ = ((unsigned int)favnum) & 0xff;
	*here++ = (((unsigned int)favnum) >> 8) & 0xff;
	*here++ = arfOpZeroArgFFI;
	*here++ = ((unsigned int)favnum) & 0xff;
	*here++ = (((unsigned int)favnum) >> 8) & 0xff;
	*here++ = arfOpTwoArgFFI;
	*here++ = ((unsigned int)add) & 0xff;
	*here++ = (((unsigned int)add) >> 8) & 0xff;

	*here++ = arfOpZeroArgFFI;
	*here++ = ((unsigned int)favnum) & 0xff;
	*here++ = (((unsigned int)favnum) >> 8) & 0xff;
	*here++ = arfOpZeroArgFFI;
	*here++ = ((unsigned int)favnum) & 0xff;
	*here++ = (((unsigned int)favnum) >> 8) & 0xff;
	*here++ = arfOpZeroArgFFI;
	*here++ = ((unsigned int)favnum) & 0xff;
	*here++ = (((unsigned int)favnum) >> 8) & 0xff;
	*here++ = arfOpThreeArgFFI;
	*here++ = ((unsigned int)sum) & 0xff;
	*here++ = (((unsigned int)sum) >> 8) & 0xff;

	*here++ = arfOpEXIT;
	
	// Run our inner loop;
	arfInnerInterpreter((arfOpcode*)(dictionary + 6));
}
