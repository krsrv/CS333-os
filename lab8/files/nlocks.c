#include "types.h"
#include "stat.h"
#include "user.h"

#define NCHILDREN 30

void mutex_lock(int index)
{
  acquire_mutex_spinlock(index);
  while(get_mutex_value(index) != 0)
	cond_wait(index, index);
  set_mutex_value(index, 1);
  release_mutex_spinlock(index);
}

void mutex_unlock(int index)
{
  acquire_mutex_spinlock(index);
  set_mutex_value(index, 0);
  cond_signal(index);
  release_mutex_spinlock(index);
}

int main()
{
  int i=0, j, ret;

  init_counters();

  for(i = 0; i < 10; i++){
  	ret = fork();
  	if(ret == 0)	break;
  }

  if(ret != 0){
  	// Parent process
  	for(i = 0; i < 1000; i++)
  		for(j = 0; j < 10; j++){
	  		mutex_lock(j);
			set_var(j, get_var(j)+1);
			mutex_unlock(j);
	  	}
  }
  else {
  	for(j = 0; j < 1000; j++){
	  	mutex_lock(i);
		set_var(i, get_var(i)+1);
	  	mutex_unlock(i);
  	}
  	exit();
  }

  for(i=0; i<10; i++)
  {
	wait();
  }
  
  for(i=0; i<10; i++)
  {
	printf(1, "data[%d] = %d\n", i, get_var(i));
  }
  exit();
}

