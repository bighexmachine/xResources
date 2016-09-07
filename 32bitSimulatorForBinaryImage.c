
#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include <libgen.h>

#define true     -1
#define false    0

FILE* simio[8];

char connected[] = { false, false, false, false, false, false, false, false };

#define i_ldam   0x0
#define i_ldbm   0x1
#define i_stam   0x2

#define i_ldac   0x3
#define i_ldbc   0x4
#define i_ldap   0x5

#define i_ldai   0x6 
#define i_ldbi   0x7 
#define i_stai   0x8

#define i_br     0x9
#define i_brz    0xA 
#define i_brn    0xB

#define i_opr    0xD
#define i_pfix   0xE
#define i_nfix   0xF

#define o_brb    0x0
#define o_add    0x1
#define o_sub    0x2
#define o_svc    0x3

void loadRamImage();
uint32_t readBinaryHalfword(FILE* ramImageFile);
void svc();
void simout(uint32_t word, uint32_t port);
uint32_t simin(uint32_t port);
uint32_t fileOpen(uint32_t* strptr, uint32_t toWrite);

/*
* Memory is made from 32 bits words but instructions are still 8 bits
*/

uint32_t mem[200000];
uint8_t* pmem = (uint8_t*)mem;

uint32_t pc;
uint32_t sp;

uint32_t areg;
uint32_t breg;
uint32_t oreg;

char* argumentString;
int argumentStringPos;
char dirPath[100];

/*
* Instructions are 8 bits, upper nibble is opcode and lower nibble is operand
*/
uint8_t inst;

int32_t running;

int main(int argc, char *argv[])
{
     argumentString = (char*) argv[1];

     strcpy(dirPath, dirname((char*) argv[0]) );
     strcat(dirPath, "/");

     argumentStringPos = 0;
	printf("\n");

	loadRamImage();

	running = true;

	oreg = 0;

	while (running)

	{
		inst = pmem[pc];

		//printf("pc = %d; op = %d; opd = %d; areg = %d; breg = %d; oreg = %d\n", pc, (inst>>4) & 0xF, inst & 0xF, areg, breg, oreg);	 	

		pc = pc + 1;

		oreg = oreg | (inst & 0xf);

		switch ((inst >> 4) & 0xf)
		{
		case i_ldam:   areg = mem[oreg]; oreg = 0; break;
		case i_ldbm:   breg = mem[oreg]; oreg = 0; break;
		case i_stam:   mem[oreg] = areg; oreg = 0; break;

		case i_ldac:   areg = oreg; oreg = 0; break;
		case i_ldbc:   breg = oreg; oreg = 0; break;
		case i_ldap:   areg = pc + oreg; oreg = 0; break;

		case i_ldai:   areg = mem[areg + oreg]; oreg = 0; break;
		case i_ldbi:   breg = mem[breg + oreg]; oreg = 0; break;
		case i_stai:   mem[breg + oreg] = areg; oreg = 0; break;

		case i_br:     pc = pc + oreg; oreg = 0; break;
		case i_brz:    if (areg == 0) pc = pc + oreg; oreg = 0; break;
		case i_brn:    if ((int32_t)areg < 0) pc = pc + oreg; oreg = 0; break;

		case i_pfix:   oreg = oreg << 4; break;
		case i_nfix:   oreg = 0xFFFFFF00 | (oreg << 4); break;

		case i_opr:
			switch (oreg)
			{
			case o_brb:    pc = breg; oreg = 0; break;

			case o_add:    areg = areg + breg; oreg = 0; break;
			case o_sub:    areg = areg - breg; oreg = 0; break;

			case o_svc:  svc(); break;
			}
			oreg = 0; break;

		}
	}
	return 0;
}

void loadRamImage()
{
     char filePath[105];
     strcpy(filePath, dirPath);
     strcat(filePath, "a.bin");
	FILE* ramImg = fopen(filePath, "rb");
	//first word states number of subsequent words
	uint32_t numWords = fgetc(ramImg) | (fgetc(ramImg)<<8) | (fgetc(ramImg)<<16) | (fgetc(ramImg)<<24);
	uint32_t lengthInBytes = numWords << 2;

	pc = 0;
	uint32_t n;
	for (n = 0; n < lengthInBytes; n++)
     {
		pmem[n] = (uint8_t) fgetc(ramImg);
     }	
     printf("Ram Image loaded into simulator. Num bytes: %d\n", lengthInBytes);
}



void svc()
{
	sp = mem[1];
	switch (areg)
	{
	case 0: running = false; break;
	case 1: simout(mem[sp + 2], mem[sp + 3]); break;
	case 2: mem[sp + 1] = simin(mem[sp + 2]) & 0xFF; break;
	case 3: mem[sp + 1] = fileOpen(mem + mem[sp+2], mem[sp + 3 ]);break;
	}
}

uint32_t fileOpen(uint32_t* strptr, uint32_t toWrite)
{
     const char* filename = (char *) strptr;
     char filePath[150];
     strcpy(filePath, dirPath);
     strcat(filePath, filename);
     int i;
     FILE* tmp;
     for (i=4; i<8; i++)
     {
          if (!connected[i])
          {    
               if (toWrite) {
                    tmp = fopen(filePath, "wb");
               } else {
                    tmp = fopen(filePath, "rb");
               }
               if (tmp == NULL)
                    return 0xffffffff;
               simio[i] = tmp;
			connected[i] = true;
               return i<<8;
          }
     }
}

void simout(uint32_t word, uint32_t port)
{
	if (port < 256)
	{
		putchar(word);
	}
	else
	{
		char filename[] = { 's', 'i', 'm', ' ', '\0' };
		uint32_t fileId = (port >> 8) & 7;
		if (!connected[fileId])
		{
			filename[3] = fileId + '0';
			simio[fileId] = fopen(filename, "wb");
			connected[fileId] = true;
		}
		fputc(word, simio[fileId]);
	}
}

uint32_t simin(uint32_t port)
{
     //args
     if (port == 1)
     {
          if (argumentStringPos >= strlen(argumentString)) { return 255 /*EOF*/; }
          return argumentString[argumentStringPos++];
     }
	else if (port < 256)
	{
		return getchar();
	}
	else
	{
		char filename[] = { 's', 'i', 'm', ' ', '\0' };
		uint32_t fileId = (port >> 8) & 7;
		if (!connected[fileId])
		{
			filename[3] = fileId + '0';
			simio[fileId] = fopen(filename, "rb");
			connected[fileId] = true;
		}
		return fgetc(simio[fileId]);
	}
}


