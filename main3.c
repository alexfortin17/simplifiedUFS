/* 
 * File:   main.c
 * Author: alfo
 *
 * Created on November 18, 2015, 3:31 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "UFS.h"



/*
 * 
 */
int main(int argc, char** argv) {
    

    bd_FormatDisk();
    
    int k = bd_ls("/");
    
    printf("*****Creation repertoire /doc *****\n");
    int i = bd_mkdir("/doc");
    k = bd_ls("/");

    printf("*****Creation repertoire /tmp *****\n");
    int u = bd_mkdir("/tmp");
    k = bd_ls("/");

    printf("*****Creation repertoire /tmp/lib *****\n");
    int p = bd_mkdir("/tmp/lib");
    k = bd_ls("/");
    k = bd_ls("/tmp");

    printf("*****Creation repertoire /tmp/lib/deep *****\n");
    p = bd_mkdir("/tmp/lib/deep");
    k = bd_ls("/tmp/lib/deep");
    
    printf("*****Creation fichier /doc/pire.txt *****\n");
    int a = bd_create("/doc/pire.txt");

    printf("*****Creation fichier /vomi.txt *****\n");
    int b = bd_create("/vomi.txt");

    printf("*****Creation repertoire /doc/hhh *****\n");
    int hfhf = bd_mkdir("/doc/hhh");

    printf("*****Creation fichier /doc/hhh/allo.txt *****\n");
    int qj = bd_create("/doc/hhh/allo.txt");
    
    k = bd_ls("/");
    k = bd_ls("/doc");
    k = bd_ls("/doc/hhh");
    
    printf("*****Creation repertoire /doc/hhh/deeper *****\n");
    hfhf = bd_mkdir("/doc/hhh/deeper");

    printf("*****Creation fichier /doc/hhh/deeper/loin.txt *****\n");
    qj = bd_create("/doc/hhh/deeper/loin.txt");
    
    k = bd_ls("/doc");
    k = bd_ls("/doc/hhh");
    
    
    //char* xxx = "/usr/pire/guy.txt";
    //testsrings(xxx);
    
 

    return (EXIT_SUCCESS);
}
