#include "compactador.h"
#include "mapa.h"
#include "listaTree.h"
#include "bitmap.h"
#include "bitmapPLUS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//funcao auxiliar da sua semelhante ; a unica diferença é que preserva o mapa pai na chamada das funcoes e realmente faz o trabalho acontecer
unsigned long pegar_indice_maior_numero(unsigned long* v, size_t tam){
    unsigned long maior = 0, index=0;
    if(v){
        for(unsigned long i=0;i<tam;i++){
            if(v[i]){
                maior=maior?(v[i]>maior?v[i]:maior):v[i];
                index = maior==v[i]?i:index;
            }
        }
    }
    return index;
}

mapa* algoritmo_Huffman(listaArvores* lc){
    mapa* mapa_completo = 0;
    if(lc){
        mapa *aux1 = 0, *aux2 = 0;
        while(pegar_numero_elementos_listaArvores(lc)>1){   //se a lista tiver mais que um elemento, entao o algoritmo de huffman não terminou
            aux1 = pegar_mapa_listaArvores(lc, 0);          //pegar o primeiro elemento
            lc = remover_listaArvores(lc, 0);               //remove-lo da lista (será somado e depois adicionado novamente)
            aux2 = pegar_mapa_listaArvores(lc, 0);          //pegar o segundo elemento
            lc = remover_listaArvores(lc, 0);               //remove-lo da lista (será somado e depois adicionado novamente)
            lc = adicionar_listaArvores(lc, criar_mapa(0, pegar_peso_mapa(aux1)+pegar_peso_mapa(aux2), aux1, aux2), -1); //soma e readicao do resultado no final da lista (index -1))
        }
        mapa_completo = pegar_mapa_listaArvores(lc, 0);     //nesse ponto, o loop de cima terminou sua execução, o que indica que há apenas um elemento na lista circular. Esse elemento é a arvore completamente construida
        lc = remover_listaArvores(lc, 0);                   //remover a arvore da lista de arvores
        lc = liberar_listaArvores(lc);                      //opcional: liberar a lista passada
    }
    return mapa_completo;
}

void gravar_codigos_mapa(mapa* map){
    if(map) gerar_codigos_mapa_auxiliar(map, map);
}

static void gerar_codigos_mapa_auxiliar(mapa* filho, mapa* pai){
    if(filho && pai){
        if(testar_folha_mapa(filho)){   //se a arvore filho for um no-folha, proceder com verificacao
            char* rota = encontrar_rota_node_mapa(pai, filho);  //se a arvore filho for no-folha e pertencer a arvore pai, entao a variavel rota é nao nula e recebe o codigo contendo o pathing correto que leva da arvore pai a arvore filha
            if(rota){
                size_t tam = strlen(rota);
                bitmap* bm = bitmapInit((unsigned)tam);         //o tamanho (tam) da string rota corresponde exatamente ao numero de bits que serao utilizados para construcao do bitmap de um caracter
                for(size_t i=0;i<tam;i++){
                    bitmapAppendLeastSignificantBit(bm, (unsigned char) (rota[i]-'0')); //necessário fazer a subtração para converter o conteudo da variavel rota[i] do codigo ASCII para 0 ou 1 (unsigned char)
                }
                filho = preencher_bitmap_mapa(filho, bm);
            }
            free(rota); //é necessario liberar a memoria da string previamente alocada pois a funcao bitmapInit armazena uma string nova internamente que tem tamanho e conteudo semelhante ao fornecido
            rota = 0;
        }
        gerar_codigos_mapa_auxiliar(pegar_sae_mapa(filho), pai);
        gerar_codigos_mapa_auxiliar(pegar_sad_mapa(filho), pai);
    }
}

/*
    o mapa é escrito da seguinte maneira:
    {numero de bits que a serem lidos para gerar o mapa (esse numero é do tipo unsigned long)}{mapa/arvore em si}

    a arvore é exibida da seguinte maneira: 0 significa um nodulo raiz, 1 significa um nodulo folha
    a precedencia é sempre: nodulo raiz -> nodulo esquerdo -> nodulo direito
    quando for indicado um nodulo folha (1), logo em seguida imprime-se 1 byte que indica o codigo na tabela ASCII do caracter armazenado nesse nodulo

    resumo: imprimir o tamanho de bits que o mapa apresenta, depois os bits do mapa; quando chegar numa folha (1) imprimir em sequencia 1 byte inidicando qual letra esta nessa folha
*/
void exportar_mapa_formato_bitmap(mapa* map, FILE* fpout){
    if(map && fpout){
        unsigned long tam_bm = contar_nodes_mapa(map)+(contar_folhas_mapa(map)*8);
        fwrite(&tam_bm, sizeof(unsigned long), 1, fpout);  //escrevendo o numero de bits que serao necessarios para ler e remontar o mapa        
        tam_bm = tam_bm%8?(tam_bm+(8-tam_bm%8)):tam_bm;        //colocando bits adicionais de preenchimento no bitmap (evitar problemas na leitura/descompactacao do mapa) //os bits adicionais completarão o numero até que seja um multiplo de 8 ; esses bits adicionais serao nulos na impressao
        bitmap* bm = bitmapInit(tam_bm);
        gerar_mapa_formato_bitmap(map, bm);  //preenchendo o bitmap que contem a arvore na forma de bits corridos
        bitmapUnloadContents(bm, fpout);    //descarregando o conteudo do bitmap no arquivo apontado por fpout
        bitmapLibera(bm);
    }
}

void gerar_mapa_formato_bitmap(mapa* map, bitmap* bm){
    if(map && bm){
        unsigned ehFolha = testar_folha_mapa(map);                     //adquirindo informação se nodulo é ou não é folha
        bitmapAppendLeastSignificantBit(bm, (unsigned char) ehFolha);  //apençar a informacao em forma de bit no mapa de bits
        if(!ehFolha){
            gerar_mapa_formato_bitmap(pegar_sae_mapa(map), bm);                 //senao for no folha, entao chamar recursivamente a funcao para os nodulos filhos
            gerar_mapa_formato_bitmap(pegar_sad_mapa(map), bm);
        }
        else{                                                          //se for um no folha, entao é necessario ainda escrever a letra que esta armazenada nesse no folha
            unsigned char letra = pegar_ASCII_mapa(map), bit = 0;
            for(size_t i=0;i<8;i++){        //iterando nas 8 casas ocupantes de um byte (da variavel unsigned char letra lida acima)
                bit = letra&0x80;           //0x80 = 128 :: armazenando o valor contido no bit de menor significância na variável letra
                bit >>= 7;                  //como o bit de menor significância está na última casa da célula, é preciso fazer um rshift de 7 casas para converter o valor para unitário (1 ou 0)
                letra = letra<<1;           //atualiza-se a letra com um lshift para trazer o bit a direita do lido anterior para a posição de leitura da iteração seguinte
                bitmapAppendLeastSignificantBit(bm, (unsigned char) bit); //adiciona-se o bit
            }
        }
    }
}

/*  
            for(unsigned i=0;i<8;i++){
                ascii = bitmapGetBit(bm, ++index)&pos;
                pos<<=1;
            }
*/

void exportar_texto_formato_bitmap(mapa* map, FILE* fpin, FILE* fpout){
    if(map && fpin && fpout){
        bitmap* bm_texto = bitmapInit(calcular_tamanho_bits_mapa(map)); //max_size = tamanho em bits do mapa, quantidade de bytes alocados pelo conteudo do bitmap = numero em BYTES !!!!!
        unsigned int ascii = 0;                         //essa variavel se faz necessaria, pois não é possível armazenar o valor de fgetc numa variavel unsigned, já que essa funcao retorna -1 para o caracter EOF e esse é o fim do loop
        for(char c=fgetc(fpin);c!=EOF;c=fgetc(fpin)){
            ascii = c;
            bitmapCatContents(bm_texto, pegar_bitmap_mapa(buscar_ASCII_mapa(map, &ascii)));
        }
        bitmapUnloadContents(bm_texto, fpout); //descarregando o conteudo do bitmap no arquivo apontado por fpout
        bitmapLibera(bm_texto);
    }
}