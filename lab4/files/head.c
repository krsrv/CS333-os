#include "types.h"
#include "stat.h"
#include "user.h"

char buf[1];

void
head(int fd, int lines)
{
  int n = 0, count = 0;

  while(count < lines && (n = read(fd, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) {
      printf(1, "head: write error\n");
      exit();
    }
	if(buf[0] == '\n' || buf[0] == '\r')	count++;
  }
  if(n < 0){
    printf(1, "head: read error\n");
    exit();
  }
}

int
main(int argc, char *argv[])
{
  int fd, i, n;

  if(argc <= 1){
	head(0, 10);
    exit();
  } else if(argc <= 2){
	n = 10;
	i = 1;
  } else {
	n = atoi(argv[1]);
	i = 2;
  }

  for(; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf(1, "head: cannot open %s\n", argv[i]);
      exit();
    }
	printf(1, "-- %s --\n", argv[i]);
    head(fd, n);
    close(fd);
  }
  exit();
}
