#include <cstdio>
#include <cstring>

#include "crc.h"

int main()
{
	CCRC crc;

	unsigned char string[256];

	string[0] = 'A';

	auto calc = crc.CalcCRC(string, 1);
	printf("\nThe calcluated crc of \"A\" is 0x%x. This result is %s!\n\n", calc, (calc==0x206eu) ? "correct" : "INCORRECT");

	memcpy(string, "123456789", 9);

	calc = crc.CalcCRC(string, 9);
	printf("The calcluated crc of \"123456789\" is 0x%x. This result is %s!\n\n", calc, (calc==0x772bu) ? "correct" : "INCORRECT");

	string[0] = 'A';
	for (int i=0; i<0x100; i++)
		string[i] = i;
	calc = crc.CalcCRC(string, 256);
	printf("The calcluated crc of {0x0..0xff} is 0x%x. This result is %s!\n\n", calc, (calc==0x1c31u) ? "correct" : "INCORRECT");

	return 0;
}
