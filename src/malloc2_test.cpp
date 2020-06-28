#include "malloc2.h"

int main(){
    int *a, *b;
    printf("========Malloc 3*sizeof(int) to a========\n");
    a=(int*)my_malloc(3*sizeof(int));
    
    for(int i=0;i<3;i++){
       a[i]=2*i;
       printf("a[%d]=%d\n",i,a[i]);
   }

   printf("========Realloc 5*sizeof(int) to a========\n");
   a = (int*)my_realloc(a,5*sizeof(int));
    for(int i=0;i<5;i++){
      a[i]=2*i;
      printf("a[%d]=%d\n",i,a[i]);
    }
    
    printf("========Free a and then Malloc b========\n");
    printf("Addr of a before free a: %p\n",a);
    my_free(a);

    b=(int*)my_malloc(5*sizeof(int));
    printf("Addr of b after malloc b: %p\n",b);
    
    return 0;
}