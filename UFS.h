/**************************************************************************
	Travail pratique No 3 : mini-UFS
	Fichier .h contenant les structures necessaire au travail pratique 3.

	Systemes d'explotation IFT-2001
	Universite Laval, Quebec, Qc, Canada.
 **************************************************************************/
#ifndef UFS_H
#define UFS_H

#ifdef _MSC_VER
	#define UINT16 unsigned __int16 // pour definir un entier 16 bit sur WINDOWS.
#else
	#include <stdint.h>
	#define UINT16 int16_t // pour definir un entier 16 bit.
#endif

#pragma pack(1)   // pour forcer a packer les structures au maximum.

#define N_BLOCK_PER_INODE 1     // nombre maximal de bloc de donnees associe a un i-node
#define N_INODE_ON_DISK   40	// nombre maximal d'i-node (donc de fichier) sur votre disque
#define BLOCK_SIZE 256		// taille d'un bloc de donnee
#define N_BLOCK_ON_DISK 256	// nombre de bloc sur le disque au complet
#define FREE_BLOCK_BITMAP 2	// numero du bloc contenant le bitmap des block libres
#define FREE_INODE_BITMAP 3	// numero du bloc contenant le bitmap des i-nodes libres
#define BASE_BLOCK_INODE  4     // bloc de depart ou les i-nodes sont stockes sur disque
#define DISKSIZE N_BLOCK_ON_DISK*BLOCK_SIZE // taille du disque
#define ROOT_INODE        1     // numero du i-node correspondant au repertoire racine
#define FILENAME_SIZE 14        // taille en caractere d'un nom de fichier, incluant le NULL
#define MAX_DIRENTRY 16         //nombre max de dir entry par directory
#define FREE_DIRENTRY   1000    //nombre representant le numero d'inode quand une direntry est libre

typedef UINT16 ino; // type associe aux i-nodes

// Les flags suivants sont pour st_mode
#define S_IFREG  0010   // Indique que l'i-node est un fichier ordinaire
#define S_IFDIR  0020   // Indique que l'i-node est un repertoire

typedef struct {
	ino iNode;
	char Filename[FILENAME_SIZE];
} DirEntry;

typedef struct {
	ino    st_ino;		// numero de l'i-node
	UINT16 st_mode;		// S_IFREG ou S_IFDIR. Normalement contient aussi RWX
	UINT16 st_nlink;	// nombre de lien pointant vers l'i-node
	UINT16 st_size;		// taille du fichier, en octets
	UINT16 st_blocks;	// nombre de bloc de donnees avec l'i-node. Sera 0 ou 1 dans le TP
} stat;

typedef struct {
	stat iNodeStat;
	UINT16 Block[N_BLOCK_PER_INODE]; // numero des blocs de donnees du fichier
} iNodeEntry;

// Fonctions a implementer
int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien);
int bd_mv(const char *pFilename, const char *pFilenameDest);
int bd_mkdir(const char *pDirName);
int bd_create(const char *pFilename);
int bd_ls(const char *pDirLocation);
int bd_rm(const char *pFilename);
int bd_FormatDisk();

int ReleaseFreeBlock(UINT16 BlockNum);
int SeizeFreeBlock(UINT16 BlockNum);
int ReleaseFreeInode(UINT16 InodeNum);
int SeizeFreeInode(UINT16 InodeNum);
int getNextFreeInode();
int getNextFreeBlock();
int getBlockOfInode(int iNodeNumber);
int verifParentExiste(int compteurSlash, char** path, iNodeEntry** ine, DirEntry** pDirEntry);
int getInodeParentDeFichier(const char* pathFichier);
int getInodeFichier(const char* pathFichier);
char* getNomFich(const char* pathFichier);
int createEmptyDir(int inodeCourant, int blocdeinocourant, int inodeParent, const char* nomNewDir);


#endif
