#include "types.h"
#include "stat.h"
#include "user.h"

#define MAX_SZ 1000

// The second process should resume before the pause duration of 500 ticks

void do_some_job(int starts)
{
	int pid;
	int ends;

	pid = getpid();

	printf(1, "\nchild %d: starts executiion at %d \n", pid, starts );

	sleep(10);
	
	ends = uptime();
	printf(1, "child %d: now at %d \n", pid, ends);
	printf(1, "child %d: time taken = %d ticks \n", pid, (ends - starts) );

	sleep(10);
	
	ends = uptime();
	printf(1, "child %d: ends at %d \n", pid, ends);
	printf(1, "child %d: time taken = %d ticks \n", pid, (ends - starts) );
}

int main(int argc, const char **argv) 
{
	int pid1, pid2;
	int starts;

	printf(1, "\nparent %d: Creating first child \n", getpid());
	starts = uptime();
	pid1 = fork();
	if(pid1 == 0) 
	{ 
		do_some_job(starts);
		exit();
	}

	wait();

	printf(1, "\nparent %d: Creating second child \n", getpid());
	starts = uptime();
	pid2 = fork();
	if(pid2 == 0) 
	{ 
		do_some_job(starts);
		exit();
	}

	printf(1, "\nparent %d: Pausing second child \n", getpid());
	pause(pid2, 500);
	sleep(150);
	signal_process(pid2, SIGNAL_CONTINUE);
	sleep(100);

	wait();

	exit();

}
