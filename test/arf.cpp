#include <stdio.h>

#include "ARF.h"

unsigned char arfDict[512];
ARF arf(arfDict, sizeof(arfDict));

int favnum(void)
{
	printf("favnum\n");
	return 0;
}

int main(void)
{
	arf.go(favnum);
}
