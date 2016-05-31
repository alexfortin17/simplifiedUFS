/**************************************************************************
	Travail pratique No 3 : mini-UFS
	Fichier main.cpp pour le travail pratique 3.

	Systemes d'explotation IFT-2001
	Universite Laval, Quebec, Qc, Canada.
 **************************************************************************/
#include "UFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disque.h"

typedef union {
	int (*pFonc2String)(const char*, const char*);
	int (*pFonc1String)(const char*);
	int (*pFonc0String)(void);
} FonctionFichier;

/* Le parser utilise la structure suivante pour pouvoir appeler la bonne fonction */
typedef struct {
	char StrCommande[100];
	int  nArguments;
	FonctionFichier f;
} EntreeCommande;

/* Le tableau suivant est utilise par le parseur de commande et l'interpreteur */
EntreeCommande TableDesFonctions[] = {
//                  nom       nombre d'argument                   fonction
	{.StrCommande = "ln",     .nArguments = 2, .f.pFonc2String = &bd_hardlink},
	{.StrCommande = "mv",     .nArguments = 2, .f.pFonc2String = &bd_mv},
	{.StrCommande = "mkdir",  .nArguments = 1, .f.pFonc1String = &bd_mkdir},
	{.StrCommande = "ls",     .nArguments = 1, .f.pFonc1String = &bd_ls},
	{.StrCommande = "rm",     .nArguments = 1, .f.pFonc1String = &bd_rm},
	{.StrCommande = "create", .nArguments = 1, .f.pFonc1String = &bd_create},
	{.StrCommande = "format", .nArguments = 0, .f.pFonc0String = &bd_FormatDisk}
};

int main(int argc, char **argv) {
	int index;
	if ((argc < 2) || (argc > 4)) {
		printf("On doit fournir entre 2 et 4 arguments!\n");
		return -1;
	}
	int nCommands = sizeof(TableDesFonctions)/sizeof(EntreeCommande);
	for (index=0; index < nCommands; index++) {
		if (strcmp(TableDesFonctions[index].StrCommande,argv[1])==0) {
			int n = TableDesFonctions[index].nArguments;
			if (n!= (argc-2)) {
				printf("La commande %s demande %d arguments!\n",argv[1],n);
				return -1;
			}
			// On appelle la bonne fonction maintenant
			switch (n) {
				case 0 : 
					printf("\n===== Commande %s =====\n",argv[1]);
					return (*TableDesFonctions[index].f.pFonc0String)();
				case 1 : 
					printf("\n===== Commande %s %s =====\n",argv[1],argv[2]);
					return (*TableDesFonctions[index].f.pFonc1String)(argv[2]);
				case 2 :
					printf("\n===== Commande %s %s %s =====\n",argv[1],argv[2],argv[3]);
					return (*TableDesFonctions[index].f.pFonc2String)(argv[2], argv[3]);
			}

		}
	}
	printf("Impossible de trouver la commande %s\n",argv[1]);
	return -1;
}


