#include <string.h>

#include <avr/io.h>

typedef void (*arf_op)(void);

// ROM
arf_op opcodes[128];

// RAM
unsigned char * ip;

unsigned char dictionary[1024];
unsigned char * head;

static void foo_op(void)
{
	static int counter = 0;
	PORTB = counter++;
}

int main(void)
{
	// Initialize and clear the dictionary.
	head = dictionary;
	memset(&dictionary, 0, sizeof(dictionary));
	
	// Initialize the opcode list.
	memset(&opcodes, 0, sizeof(opcodes));
	opcodes[0x00] = &foo_op;
	
	// Put some stuff in the dictionary.
	// : test FOO test ;
	memcpy(dictionary, "foo", 3);
	dictionary[3] = (char)-3;
	dictionary[4] = 0x00; // LFAhi
	dictionary[5] = 0x00; // LFAlo
	dictionary[6] = 0x00; // PFA start; opcode=0x00
	*(arf_op*)(&dictionary[7]) = foo_op;
	
	// Initialize our instruction pointer.
	ip = ((unsigned char *)dictionary) + 7;
	
	// Run our NEXT loop.
	while (true)
	{
		(*(arf_op*)ip)();
	}
}