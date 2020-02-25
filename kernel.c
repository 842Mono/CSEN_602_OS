struct procEntry
{
	int state;
	int stackPtr;
};

void printString(char*);
void readString(char*);
void readSector(char*,int);
void readFile(char*, char*);
void executeProgram(char*);
void terminate();
int matchName(char*, char*, int, int, int);
int modf(int, int);
int divf(int, int);
void handleInterrupt21(int, int , int , int);
//int charSwitch(int);
void writeSector(char* , int );
void deleteFile(char* );
void writeFile(char* , char* , int );
void handleTimerInterrupt(int, int);
void initProcessTable();
int findEmptySeg();
void killProcess(int, char*);



struct procEntry processTable[8];
int currentProcess;
int round = 0;	

int main()
{
	initProcessTable();
	makeInterrupt21();
	makeTimerInterrupt();
	interrupt(0x21, 4, "shell\0", 0x2000, 0);
	while(1);
}

void initProcessTable()
{
	int i;
	for(i = 0; i < 8; i++)
	{
		processTable[i].state = 0;
		processTable[i].stackPtr = 0xFF00;
	}
	currentProcess = -1;
}
void printString(char* chars)
{
	while(*chars != '\0')
	{
  		interrupt(0x10, 0xE*256+(*chars), 0, 0, 0);
		chars++;
	}
	//interrupt(0x10, 0xE*256+0xD, 0, 0, 0); //crr return
	//interrupt(0x10, 0xE*256+0xA, 0, 0, 0); //new line
}
void readString(char* string)
{
	int count = 0;
	while(1)
	{
		char in = interrupt(0x16, 0, 0, 0, 0);
		
		if(in == 0xD) //enter
		{
			*string = 0xA; //adds it to the last element
			*(string + 1) = 0x0;
			interrupt(0x10, 0xE*256+0xD, 0, 0, 0); //crr return
			interrupt(0x10, 0xE*256+0xA, 0, 0, 0); //new line
			break;
		}
		if(in == 0x8) //backspace
		{
			if(count > 0)
			{
				interrupt(0x10, 0xE*256+in, 0, 0, 0);
				interrupt(0x10, 0xE*256+' ', 0, 0, 0);
				interrupt(0x10, 0xE*256+in, 0, 0, 0);
				count--;
				string--;
			}
		}
		else if(count < 200)
		{
			interrupt(0x10, 0xE*256+in, 0, 0, 0);
			*string = in;
			string++;
			count++;
		}
	}
}

/**
interrupt(no,AX,BX,CX,DX)
interrupt(no,AH,AL,BX,CH,CL,DH,DL)
AX=AH*256+AL
CX=CH*256+CL
DX=DH*256+DL
• AH = 2 (this number tells the BIOS to read a sector as opposed to write)
• AL = number of sectors to read (use 1)
• BX = address where the data should be stored to (pass your char* array here)
••CH = track number
••CL = relative sector number
••DH = head number
• DL = device number (for the floppy disk, use 0)
relative sector = ( sector MOD 18 ) + 1
head = ( sector / 18 ) MOD 2
this is integer division, so the result should be rounded down
track = ( sector / 36 )
*/
void readSector(char* buffer, int sector)
{
	int relsec = modf(sector,18) + 1;
	int tmp = divf(sector,18);
	int head = modf(tmp,2);
	int track = divf(sector,36);

	interrupt(0x13,513,buffer,track*256+relsec,head*256+0);
}
int modf(int x, int y) //x mod y
{
	while(x >= y)
	{
		x = x-y;
	}
	return x;
}
int divf(int x, int y) // x/y
{
	int z = 0;
	while(x >= y)
	{
		z++;
		x = x-y;
	}
	return z;
}
void handleInterrupt21(int ax, int bx, int cx, int dx)
{
	switch(ax)
	{
		case 0: printString(bx); break;
		case 1: readString(bx); break;
		case 2: readSector(bx, cx); break;
		case 3: readFile(bx, cx); break;
		case 4: executeProgram(bx); break;
		case 5: terminate(); break;
		case 6: writeSector(bx, cx); break;
		case 7: deleteFile(bx); break;
		case 8: writeFile(bx, cx, dx); break;
		case 9: killProcess(bx, cx); break;
		default: printString("Undefined Op"); break;
	}
}
void readFile(char* fileName, char* buffer)
{
	char dirSector[512];
	int offset;
	char debug[2];
	readSector(dirSector, 2);
	offset = matchName(dirSector, fileName, 32, 512, 6);
	if(offset == -1)
	{
		*buffer = '\0';
		return;
	}
	offset += 6;
	while(dirSector[offset] != 0x00)
	{	
		readSector(buffer, dirSector[offset]);
		buffer += 512;
		offset += 1;
	}
}
//needs support for input less than 6 chars
int matchName(char* fromBuff, char* cBuff, int jmp, int size, int cmp)
{
	int i;
	for(i = 0; i < size; i += jmp)
	{
		int bOut = 0;
		int ret = i;
		int x; 
		for(x = 0; x < cmp; x++)
		{
			if(*(fromBuff+i+x) != *(cBuff+x))
			{
				bOut = 1;
				break;
			}
			bOut = 0;
		}
		if(bOut == 0) return ret;
	}
	return -1;
}
void executeProgram(char* name)
{	
	char buffer[13312];
	int i;
	readFile(name, buffer);
	if(*buffer == '\0')
	{
		char msg[18];
		msg[0]='f';
		msg[1]='i';
		msg[2]='l';
		msg[3]='e';
		msg[4]=' ';
		msg[5]='n';
		msg[6]='o';
		msg[7]='t';
		msg[8]=' ';
		msg[9]='f';
		msg[10]='o';
		msg[11]='u';
		msg[12]='n';
		msg[13]='d';
		msg[14]='!';
		msg[15]='\n';
		msg[16]='\r';
		msg[17]='\0';
		printString(msg);
		return;
	}
	else
	{
		int segNum;
		setKernelDataSegment();
		segNum = findEmptySeg();
		restoreDataSegment();
		if(segNum == -1)
		{
			char msg[18];
			msg[0]='N';
			msg[1]='o';
			msg[2]=' ';
			msg[3]='s';
			msg[4]='p';
			msg[5]='a';
			msg[6]='c';
			msg[7]='e';
			msg[8]=' ';
			msg[9]='i';
			msg[10]='n';
			msg[11]=' ';
			msg[12]='m';
			msg[13]='e';
			msg[14]='m';
			msg[15]='\n';
			msg[16]='\r';
			msg[17]='\0';
			printString(msg);
		}
		else
		{
			for(i = 0; i < 13312; i++)
			{	
				putInMemory(segNum, i, *(buffer+i));
			}
			initializeProgram(segNum);
		}
	}
	
}
int findEmptySeg()
{
	int i;
	for(i = 0; i < 8; i++)
	{	
		if(processTable[i].state == 0)
		{
			processTable[i].state = 1;
			processTable[i].stackPtr = 0xFF00;
			return 	(i+2)*0x1000;
		}	
	}
	return -1;
}
void terminate()
{
	setKernelDataSegment();
	processTable[currentProcess].state = 0;
	processTable[currentProcess].stackPtr = 0xFF00;
	restoreDataSegment();
	while(1);
	/*char shell[6];
	shell[0] = 's';
	shell[1] = 'h';
	shell[2] = 'e';
	shell[3] = 'l';
	shell[4] = 'l';
	shell[5] = '\0';
	interrupt(0x21,4,shell,0x2000,0);*/
}


void writeSector(char* buffer, int sector) //similar to readSector
{
	int relsec = modf(sector,18) + 1;
	int tmp = divf(sector,18);
	int head = modf(tmp,2);
	int track = divf(sector,36);

	interrupt(0x13,769,buffer,track*256+relsec,head*256+0);
}
void deleteFile(char* name)
{
	char mapSector[512];
	char dirSector[512];
	int offset;
	readSector(mapSector, 1);
	readSector(dirSector, 2);

	offset = matchName(dirSector, name, 32, 512, 6);

	if(offset != -1)
	{
		dirSector[offset] = 0x00;
		writeSector(dirSector, 2);

		offset += 6;
		while(dirSector[offset] != 0x00)
		{
			int tmp2 = dirSector[offset];
			mapSector[tmp2] = 0x00;
			offset++;
		}
		writeSector(mapSector, 1);
	}
	else
		*name = '\0';
}
void writeFile(char* name, char* buffer, int secNum)
{
	char mapSector[512];
	char dirSector[512];
	int i;
	int k;
	int c = 0;
	int anchor;
	readSector(mapSector, 1);
	readSector(dirSector, 2);

	if(secNum > 26)
	{
		printString("Sector Number Out Of Bounds.");
		return;
	}
	
	for(k = 0; k < 512; k += 32) //checks if dir has a free entry
	{
		if(dirSector[k] == 0x00)
		{
			anchor = k + 32;
			for(i = 3; i < 512; ++i) //checks map if there's free space
			{
				if(mapSector[i] == 0x00)
					++c;
				if(c == secNum)
				{
					//start writing file
					for(i = 0; i < 6; ++i) //write name
					{
						if(*(name + i) != '\0')
							dirSector[k++] = *(name + i);
						else
							for(; i < 6; ++i)
							{
								dirSector[k++] = 0x00;
							}
					}
					c = 0;
					for(i = 3; i < 512; ++i)
					{
						//if(mapSector[i] == 0xFF)
						//	continue;
						if(mapSector[i] == 0x00)
						{
							mapSector[i] = 0xFF; //allocate in map
							dirSector[k++] = i; //add sector number
							//printString(*(buffer));
							writeSector(buffer,i);
							buffer += 512;
							if(++c == secNum)
							{
								while(k < anchor)
								{
									dirSector[k++] = 0x00; //fill zeros
								}
								writeSector(mapSector, 1);
								writeSector(dirSector, 2);
								return;
							}
						}
					}
				}
			}
			printString("map sector full! (no free space)");
			return;
		}
	}
	printString("dir sector full");
}
void handleTimerInterrupt(int segment, int sp)
{
	setKernelDataSegment();
	round++;
	if(round == 100)
	{
		int i;
		int procPtr;
		int bs;
		round = 0;
		if(segment != 0x1000)
			processTable[currentProcess].stackPtr = sp;
		procPtr = modf(currentProcess + 1, 8);
		for(i = 0; i < 8; i++)
		{
			if(processTable[procPtr].state == 1)
			{	
				currentProcess = procPtr;
				sp = processTable[procPtr].stackPtr;
				segment = (procPtr + 2)*0x1000;				
				break;
			}
			procPtr = modf(procPtr + 1, 8);
		}
	}
	restoreDataSegment();
	returnFromTimer(segment, sp);
}
void killProcess(int procNumber, char* resp)
{
	char res;
	if(procNumber == 0)
	{
		res = '3';
	}
	else if(procNumber < 1 || procNumber > 7)
	{
		res = '0';
	}
	else
	{
		setKernelDataSegment();
		if(processTable[procNumber].state == 1)
		{
			processTable[procNumber].state = 0;
			processTable[procNumber].stackPtr = 0xFF00;
			res = '1';
		}
		else
		{
			res = '2';
		}
		restoreDataSegment();
	}
	*resp = res;
}
