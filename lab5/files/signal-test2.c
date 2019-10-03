#include "types.h"
#include "stat.h"
#include "user.h"

#define MAX_SZ 1000000

// The second statement should not be printed
//
int main(int argc, const char **argv) 
{
	printf(1, "sending kill signal to itself\n");
	signal_process(getpid(), SIGNAL_KILL);
	sleep(5e1);
	
	printf(1, "not killed\n");
	exit();
}
