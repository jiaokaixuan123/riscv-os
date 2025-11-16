#include "printf.h"
#include "types.h"

void panic(const char *s){
  printf("panic: %s\n", s ? s : "(null)");
  while(1){}
}
