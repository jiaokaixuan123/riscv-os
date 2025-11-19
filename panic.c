#include "printf.h"
#include "types.h"

void panic(const char *s){
  printf("\n!!! PANIC: %s !!!\n", s ? s : "(null)");
  for(int i = 0; i < 5; i++) {
    printf("PANIC!\n");
  }
  while(1){}
}
