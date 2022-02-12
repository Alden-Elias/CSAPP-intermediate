#include<stdio.h>

int main(){
    char s[] = "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?";
    int i;
    for(i = 0; i < 0xf; i++){
        printf("%c -->  %c\n",'a'+i,s[('a'+i)&0xf]);
    }
    return 0;
}
