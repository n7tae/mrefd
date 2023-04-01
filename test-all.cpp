#include <cstdio>
#include <cstring>

#include "crc.h"
#include "callsign.h"

int main()
{
	if (sizeof(uint64_t) != sizeof(size_t))
	{
		printf("ERROR! Type size_t is not 64 bits.\n");
		return 1;
	}

	std::string callsign("M17-USA A");
	CCallsign cs(callsign);
	auto coded = cs.Hash();
	if (coded == 0x05f74c74caedu)
	{
		uint8_t code[6] = { 0 };
		auto c = coded;
		for (int i=5; c; i--)
		{
			code[i] = c % 0x100u;
			c /= 0x100u;
		}
		CCallsign cs1(code);
		printf("Callsign %s is 0x%lx and decodes back to %s\n", callsign.c_str(), coded, cs1.GetCS().c_str());
	}
	else
	{
		printf("ERROR with callsign encoding!\n");
		return 1;
	}

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
