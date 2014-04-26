#include <avr/pgmspace.h>

#include "..\ARF.h"

unsigned int inputOffset = 0;
static const uint8_t input[] PROGMEM = {
	'2', '7', ' ', '5', '4', ' ', '+', ' ', '.', '\n'
};

static arfInt arfStaticKeyQuestion(void)
{
	return inputOffset < sizeof(input);
}

static arfUnsigned arfStaticKey(void)
{
	if (inputOffset < sizeof(input))
	{
		return pgm_read_byte(&input[inputOffset++]);
	}
	else
	{
		while (true)
		{
			// Block forever.
		}
	}
}

static arfUnsigned lastCh = 0;
static void arfStaticEmit(arfUnsigned ch)
{
	lastCh = ch;
}

uint8_t arfDict[512];
ARF arf(arfDict, sizeof(arfDict), arfStaticKeyQuestion, arfStaticKey, arfStaticEmit);

int main(void)
{
	arf.go();
}
