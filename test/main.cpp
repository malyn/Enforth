#include "..\ARF.h"

unsigned char arfDict[512];
ARF arf(arfDict, sizeof(arfDict));

int favnum(void)
{
	return 27;
}

int main(void)
{
	arf.go(favnum);
}
