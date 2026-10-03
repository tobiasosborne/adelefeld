#include <stdlib.h>
int main(void){volatile char*p=malloc(77);p[0]=1;p=0;return 0;}
