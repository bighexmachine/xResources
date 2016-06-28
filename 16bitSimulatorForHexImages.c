#include "stdint.h"
#include "stdio.h"

#define true     -1
#define false    0

FILE *simio[8];

char connected[] = { false, false, false, false, false, false, false, false };

#define i_ldam 0x0
#define i_ldbm 0x1
#define i_stam 0x2

#define i_ldac 0x3
#define i_ldbc 0x4
#define i_ldap 0x5

#define i_ldai 0x6
#define i_ldbi 0x7
#define i_stai 0x8

#define i_br 0x9
#define i_brz 0xA
#define i_brn 0xB
#define i_brb 0xC

#define i_opr 0xD
#define i_pfix 0xE
#define i_nfix 0xF

#define o_add 0x0
#define o_sub 0x1
#define o_in 0x2
#define o_out 0x3

uint16_t mem[32768];
uint8_t* pmem = (uint8_t*)mem;

uint16_t pc;
uint16_t sp;

uint16_t areg;
uint16_t breg;
uint16_t oreg;

uint16_t inst;



void loadRamImages();
void loadMem(FILE* ramImageFile, int ramNumber);
int hexVal(char ch);
void simout(uint16_t word, uint16_t port);
uint16_t simin(uint16_t port);
int32_t isHexCharacter(char ch);

int main()
{
	printf("\n");
	loadRamImages();
	oreg = 0;

	while (true)
	{
		inst = pmem[pc];
		//printf("pc = %d; op = %d; opd = %d; areg = %d; breg = %d; oreg = %d\n", pc, (inst>>4) & 0xF, inst & 0xF, areg, breg, oreg);
		pc = pc + 1;
		oreg = oreg | (inst & 0xf);

		switch ((inst >> 4) & 0xf)
		{
		case i_ldam: areg = mem[oreg]; oreg = 0; break;
		case i_ldbm: breg = mem[oreg]; oreg = 0; break;
		case i_stam: mem[oreg] = areg; oreg = 0; break;

		case i_ldac: areg = oreg; oreg = 0; break;
		case i_ldbc: breg = oreg; oreg = 0; break;
		case i_ldap: areg = pc + oreg; oreg = 0; break;

		case i_ldai: areg = mem[areg + oreg]; oreg = 0; break;
		case i_ldbi: breg = mem[breg + oreg]; oreg = 0; break;
		case i_stai: mem[breg + oreg] = areg; oreg = 0; break;

		case i_br: pc = pc + oreg; oreg = 0; break;
		case i_brz: if (areg == 0) pc = pc + oreg; oreg = 0; break;
		case i_brn: if ((int16_t)areg < 0) pc = pc + oreg; oreg = 0; break;
		case i_brb: pc = breg + oreg; oreg = 0; break;

		case i_opr:
			switch (oreg)
			{
			case o_add: areg = areg + breg; oreg = 0; break;
			case o_sub: areg = areg - breg; oreg = 0; break;
			case o_in: areg = simin(breg); oreg = 0; break;
			case o_out: simout(areg, breg); oreg = 0; break;
			};
			break;

		case i_pfix: oreg = oreg << 4; break;
		case i_nfix: oreg = 0xFFFFFF00 | (oreg << 4); break;
		};	
	}
}

void loadRamImages()
{
	FILE* ramImageFile = fopen("sim2", "rb");
	loadMem(ramImageFile, 0);
	ramImageFile = fopen("sim3", "rb");
	loadMem(ramImageFile, 1);
}

void loadMem(FILE* ramImageFile, int ramNumber)
{
	int16_t addr = ramNumber;
	/*
	* An instruction is an 8 bit number represented in Hex delimitted by a space
	*/
	char ch = fgetc(ramImageFile);
	while (isHexCharacter(ch))
	{
		pmem[addr] = (hexVal(ch) << 4 | hexVal(fgetc(ramImageFile)));
		fgetc(ramImageFile); //throw away the space
		ch = fgetc(ramImageFile);
		addr = addr + 2; //ram 0 is even indexes and 1 is odds
	}
}

int32_t isHexCharacter(char ch)
{
	return (('0' <= ch) && (ch <= '9')) || (('A' <= ch) && (ch <= 'F'));
}

int32_t hexVal(char ch)
{
	int32_t v;
	if (('0' <= ch) && (ch <= '9')) v = ch - '0';
	else v = (ch - 'A') + 10;
	return v;
}

void simout(uint16_t word, uint16_t port)
{
	if (port < 256)
	{
		putchar(word);
	}
	else
	{
		char filename[] = { 's', 'i', 'm', ' ', '\0' };
		uint16_t fileId = (port >> 8) & 7;
		if (!connected[fileId])
		{
			filename[3] = fileId + '0';
			simio[fileId] = fopen(filename, "wb");
			connected[fileId] = true;
		}
		fputc(word, simio[fileId]);
	}
}

uint16_t simin(uint16_t port)
{
	if (port < 256)
	{
		return getchar();
	}
	else
	{
		char filename[] = { 's', 'i', 'm', ' ', '\0' };
		uint16_t fileId = (port >> 8) & 7;
		if (!connected[fileId])
		{
			filename[3] = fileId + '0';
			simio[fileId] = fopen(filename, "rb");
			connected[fileId] = true;
		}
		return fgetc(simio[fileId]);
	}
}
