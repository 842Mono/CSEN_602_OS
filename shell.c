
void handleCMD(char*);
int getCMD(char*, char*);
void getNextParam(char**, char*);
void intToString(int , char*);
int modf(int, int);
int divf(int, int);
void charPadder(char* , int , char );
void copyf(char*);
void dirf();
void delf(char*);
void createf(char*);
void killProcess(char*);

int main()
{
	char cmd[512];
	while(1)
	{
		interrupt(0x21, 0, "SHELL>", 0, 0);
		interrupt(0x21, 1, cmd, 0, 0);
		handleCMD(cmd);
	}
}
void handleCMD(char* cmd)
{
	if(getCMD(cmd, "view") == 1)
	{
		char loadedFile[13312];
		cmd += 5;
		charPadder(cmd, 6, '\0');
		interrupt(0x21, 3, cmd, loadedFile, 0);
		if(*loadedFile != '\0')
		{
			interrupt(0x21, 0, loadedFile, 0, 0);
			interrupt(0x21, 0, "\n\r", 0, 0);
		}
		else
			interrupt(0x21, 0, "file doesn't exist\n\r", 0, 0);
	}
	else if(getCMD(cmd, "execute") == 1)
	{
		char arg[7];
		cmd += 8;
		getNextParam(&cmd, arg);
		charPadder(arg, 6, 0x00);
		interrupt(0x21, 4, arg, 0x0, 0);
	}
	else if(getCMD(cmd, "copy") == 1)
	{
		cmd += 5;
		copyf(cmd);
	}
	else if(getCMD(cmd, "dir") == 1)
	{
		dirf();
	}
	else if(getCMD(cmd, "delete") == 1)
	{
		cmd += 7;
		delf(cmd);
	}
	else if(getCMD(cmd, "create") == 1)
	{
		cmd += 7;
		createf(cmd);
	}
	else if(getCMD(cmd, "kill") == 1)
	{
		cmd += 5;
		killProcess(cmd);
	}
	else
		interrupt(0x21, 0, "BAD COMMAND\n\r", 0, 0);
}
void copyf(char* cmd)
{
	char buffer[13312];
	char arg1[7];
	char arg2[7];
	int sectors = 0;
	int i = 0;
	getNextParam(&cmd, arg1);
	getNextParam(&cmd, arg2);
	charPadder(arg1, 6, 0x00);
	charPadder(arg2, 6, 0x00);
	if(*arg1 == '\0' || *arg2 == '\0')
	{
		interrupt(0x21, 0, "missing arguments\n\r", 0, 0);
		return;
	}
	interrupt(0x21, 3, arg1, buffer, 0);
	while(buffer[i] != 0x00)
	{
		i++;
	}
	sectors = divf(i,512) + 1;
	if(*buffer != '\0')
		interrupt(0x21, 8, arg2, buffer, sectors);
	else
		interrupt(0x21, 0, "file doesn't exist\n\r", 0, 0);
}
void killProcess(char* cmd)
{
	int number;
	char arg[1];
	char resp[1];
	getNextParam(&cmd, arg);
	number = (arg[0] - 48);
	interrupt(0x21, 9, number, resp, 0);
	if(*resp == '0')
		interrupt(0x21, 0, "Invalid Process Number\n\r", 0, 0);
	else if(*resp == '1')
		interrupt(0x21, 0, " Process Killed\n\r", 0, 0);
	else if(*resp == '2')
		interrupt(0x21, 0, "Could not find active process\n\r", 0, 0);
	else if(*resp == '3')
		interrupt(0x21, 0, "Can not kill shell\n\r", 0, 0);
	
}
void dirf()
{
	char dirSector[512];
	int i;
	int j;
	interrupt(0x21, 2, dirSector, 2, 0);
	for(i = 0; i < 512; i+=32)
	{
		char name[7];
		char sizetxt[10];
		int size = 0;
		if(*(dirSector + i) == 0x00)
			continue;
		for(j = 0; j < 6; ++j)
		{
			name[j] = *(dirSector + i + j);
			if(*(dirSector + i + j) == '\0')
				break;
		}
		for(; j < 6; ++j)
		{
			name[j] = ' ';
		}
		name[6]='\0';
		interrupt(0x21, 0, name, 0, 0);
		for(; j < 32; ++j)
		{
			if(*(dirSector + i + j) == 0x00)
				break;
			size++;
		}
		size *= 512;
		intToString(size,sizetxt);
		interrupt(0x21, 0, " size in bytes:", 0, 0);
		interrupt(0x21, 0, sizetxt, 0, 0);
		interrupt(0x21, 0, "\n\r", 0, 0);
	}
}
void delf(char* cmd)
{
		char arg[7];
		getNextParam(&cmd, arg);
		charPadder(arg, 6, 0x00);
		interrupt(0x21, 7, arg, 0, 0);
		if(*arg == '\0')
			interrupt(0x21, 0, "file not found\n\r", 0, 0);
		else
			interrupt(0x21, 0, "deleted!\n\r", 0, 0);
}
void createf(char* cmd)
{
	char lineIn[200];
	char* lineInPointer = lineIn;
	char buffer[13312];
	char* bufferPointer = buffer;
	int i = 0;
	char arg[7];
	getNextParam(&cmd, arg);
	charPadder(arg, 6, 0x00);
	while(i < 13112) //less by 200 bytes
	{
		interrupt(0x21, 1, lineIn, 0, 0);
		if(*lineInPointer == '\n')
			break;
		while(1)
		{
			*bufferPointer++ = *lineInPointer++;
			++i;
			if(*lineInPointer == '\n')
			{
				*bufferPointer++ = '\n';
				++i;
				*bufferPointer++ = '\r';
				++i;
				break;
			}
		}
		lineInPointer = lineIn;
	}
	if(i == 0)
		return;
	*bufferPointer++ = '\0';
	i = divf(i, 512) + 1;
	interrupt(0x21, 8, arg, buffer, i);
	interrupt(0x21, 0, "file written!\n\r", 0, 0);
}


//tools

void getNextParam(char** in, char* arg)
{
	int i = 0;
	while(**in != ' ' && **in != '\0' && **in != '\n')
	{
		*(arg + i++) = **in;
		(*in)++;
	}
	*(arg + i) = '\0';
	(*in)++;
	return;
}
void charPadder(char* in, int totalLength, char character)
{
	int count = 0;
	while(*in != '\0' && *in != '\n')
	{
		in++;
		count++;
	}
	for(; count < totalLength; ++count)
	{
		*in++ = character;
	}
}
int getCMD(char* in, char* cmp)
{
	int i = 0;
	while(*(cmp + i) != '\0')
	{
		if(*(in+i) != *(cmp+i))
			return -1;
		i++;
	}
	return 1;
}
void intToString(int num, char* out)
{
	char tt[10];
	char* t;
	char* inv = t;
	while(num != 0)
	{
		int unit = modf(num,10);
		num = divf(num,10);
		switch(unit)
		{
			case 0: *t = '0'; break;
			case 1: *t = '1'; break;
			case 2: *t = '2'; break;
			case 3: *t = '3'; break;
			case 4: *t = '4'; break;
			case 5: *t = '5'; break;
			case 6: *t = '6'; break;
			case 7: *t = '7'; break;
			case 8: *t = '8'; break;
			case 9: *t = '9'; break;
			default: *t = ' '; break;
		}
		t++;
	}
	*t-- = '\0';
	while(*inv != '\0')
	{
		*out++ = *t--;
		inv++;
	}
	*out = '\0';
}

int modf(int x, int y) //from kernel
{
	while(x >= y)
	{
		x = x-y;
	}
	return x;
}
int divf(int x, int y)
{
	int z = 0;
	while(x >= y)
	{
		z++;
		x = x-y;
	}
	return z;
}
