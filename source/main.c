#include "cliente.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//O arquivo compactado sempre será gerado na pasta compactados ; lembre-se disso após compactar um arquivo

int main(int argc, char *argv[]){
    //Caso o primeiro argumento seja do programa compactador, efetuar rotina de compactação ; se for o programaa Descompactador, eetuar descompactação 
    if(!strcmp(argv[0], "./Compactador")) compactar(argv[1]);
    else if(!strcmp(argv[0], "./Descompactador")) descompactar(argv[1]);
    return 0;
}
