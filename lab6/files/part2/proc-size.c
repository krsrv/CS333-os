#include "types.h"
#include "stat.h"
#include "user.h"
char b[1024];


int getpusz()
{
	uint count = 0, pa;
	int flag;
	for(uint i = 0X0; i < (1 << 31); i += (1 << 12))
		if(get_va_to_pa(i, &pa, &flag))	count++;
	return count << 12;
}
int getpksz()
{
	uint count = 0, pa;
	int flag;
	for(uint i = 0X0; i < (1 << 31); i += (1 << 12))
		if(get_va_to_pa(i + (1 << 31), &pa, &flag))	count++;
	return count << 12;
}

int
main(int argc, char *argv[])
{
	char *buf;

	printf(1, "\ngetpsz: %d bytes \n", getpsz());
	printf(1,"getpusz: %d bytes \n",getpusz());
	printf(1,"getpksz: %d bytes\n",getpksz());


	buf=sbrk(4096);
	buf[0]='\0';
	printf(1, "\ngetpsz: %d bytes \n", getpsz());
	printf(1,"getpusz: %d bytes \n",getpusz());
	printf(1,"getpksz: %d bytes\n",getpksz());

	
	buf=sbrk(4096*7);
	printf(1, "\ngetpsz: %d bytes \n", getpsz());
	printf(1,"getpusz: %d bytes \n",getpusz());
	printf(1,"getpksz: %d bytes\n",getpksz());

	buf=sbrk(1);
	printf(1, "\ngetpsz: %d bytes \n", getpsz());
	printf(1,"getpusz: %d bytes \n",getpusz());
	printf(1,"getpksz: %d bytes\n",getpksz());

	buf=sbrk(2);
	printf(1, "\ngetpsz: %d bytes \n", getpsz());
	printf(1,"getpusz: %d bytes \n",getpusz());
	printf(1,"getpksz: %d bytes\n",getpksz());

	exit();
}
