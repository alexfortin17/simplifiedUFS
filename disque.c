#include "UFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Le disque magnetique est represente par un fichier de 64 ko, avec le nom suivant
static const char DiskName[]="DisqueVirtuel.dat"; 

// Fonctions d'acces au disque
int ReadBlock(UINT16 BlockNum, char *pBuffer) {
	//printf("READING Block %d\n",BlockNum);
	if (BlockNum > N_BLOCK_ON_DISK) {
		printf("ReadBlock ne peut lire le bloc %d\n",BlockNum);
		return 0;
	}
	FILE *pDisque = fopen(DiskName,"r");
	if (pDisque == NULL) {
		printf("Le disque %s n'a pas pu etre ouvert! Code d'erreur %s\n",DiskName, strerror(errno) );
		return -1;
	}
	fseek(pDisque, BlockNum*BLOCK_SIZE, SEEK_SET);
	size_t ret = fread(pBuffer, 1, BLOCK_SIZE, pDisque);
	if (ret != BLOCK_SIZE) {
		printf("Lecture sur le disque %s : %d et erreur %s\n",DiskName, (int) ret, strerror(errno) );
	}
	fclose(pDisque);
	return ret;
}

int WriteBlock(UINT16 BlockNum, const char *pBuffer) {
	//printf("writing Block %d\n",BlockNum);
	if (BlockNum > N_BLOCK_ON_DISK) {
		printf("ReadBlock ne peut ecrire le bloc %d\n",BlockNum);
		return 0;
	}
	FILE *pDisque = fopen(DiskName,"r+");
	if (pDisque == NULL) {
		printf("Le disque %s n'a pas pu etre ouvert! Code d'erreur %s\n",DiskName, strerror(errno) );
		return -1;
	}
	fseek(pDisque, BlockNum*BLOCK_SIZE, SEEK_SET);
	size_t ret = fwrite(pBuffer, 1, BLOCK_SIZE, pDisque);
	if (ret != BLOCK_SIZE) {
		printf("Erreur d'ecriture sur le disque %s! %d et erreur %s\n", DiskName, (int)ret, strerror(errno));
	}
	fclose(pDisque);
	return ret;
}
