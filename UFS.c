#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "UFS.h"
#include "disque.h"



int bd_FormatDisk(){
	
    int i,j,l,k,z,d;
    //on saisie les blocs de 0 a 43 car sont utilisee pour sys de fichiers
    for(i = 0; i <= 43; i++){
		SeizeFreeBlock(i);
	}
	
	//on rend tous les autres blocks libres
    for(j = 44; j < N_BLOCK_ON_DISK; j++){
                ReleaseFreeBlock(j);
        }

	//on cree les inodes dans les blocs 4 a 43
    for (l = 4; l <=43 ; l++){
		iNodeEntry ine;
		
		//le # d'inode est tj egal au numero de bloc - 4
        ine.iNodeStat.st_ino = l - 4;
        ine.iNodeStat.st_blocks = 0;//tj un seul bloc associe par inode
        ine.iNodeStat.st_nlink = 0;
        ine.iNodeStat.st_size = 0;
		
		char BlockBuffer[BLOCK_SIZE];

		//copy le contenu en bytes de la struct ds buffer
		memcpy(BlockBuffer, &ine, sizeof(ine));
		//***NOTE*** pour caster : iNodeEntry* ine2 = (iNodeEntry*) BlockBuffer;

		WriteBlock(l, BlockBuffer);
		
		
	}

	//on rend tous les inodes libres
        for(k = 0; k < N_INODE_ON_DISK; k++){
                ReleaseFreeInode(k);
        }
        //on rend la partie non utilise du bitmap des inode "occupe" pour ne pas avoir en s'en soucier
        for(z = N_INODE_ON_DISK; z < BLOCK_SIZE; z++){
            SeizeFreeInode(z);
        }

	//on rend l'inode 0 occupe car utilisation special par OS
	SeizeFreeInode(0);
        
        

	//***********************on cree le dossier root(cas particulier)//*****************************
	int noInode = getNextFreeInode();//va retourner 1
        int noBlock = getNextFreeBlock();//va retourner 44;
	SeizeFreeInode(noInode);
        SeizeFreeBlock(noBlock);
	int blockOfInode = getBlockOfInode(noInode);//va retourner 5
	
	//on va lire le bloc de l'inode
	char BlockBuffInode[BLOCK_SIZE];
	ReadBlock(blockOfInode, BlockBuffInode);

	//on cast le suit de bytes dans notre struct d'inode
	iNodeEntry* ine2 = (iNodeEntry*) BlockBuffInode;	
	ine2->iNodeStat.st_mode = S_IFDIR;//indique que ce fichier est un dirctory
	

	char BlockDirEntry[BLOCK_SIZE];
        DirEntry* pDirEntry = (DirEntry *) BlockDirEntry;
	pDirEntry[0].iNode = noInode;//affect le no d'inode au dir entry (rep courant) meme que parent pour /
	strcpy(pDirEntry[0].Filename, ".");
	pDirEntry[1].iNode = noInode;//affect le no d'inode au dir entry (rep parent)
        strcpy(pDirEntry[1].Filename, "..");
        
        for(d = 2; d < MAX_DIRENTRY; d++){
            pDirEntry[d].iNode = 1000;//1000 sera un indicateur que le dir entry est vide...il n'y aura jamais 1000 inodes ds notre systeme
        } 
 
        ine2->iNodeStat.st_nlink = 2;

	//ajuste la taille du fichier dans la struct de l'inode
	ine2->iNodeStat.st_size = 32;
	//ajuste le nombre de bloque relie a l'inode
	ine2->iNodeStat.st_blocks = 1;
        //onindique le bloc ou on trouvera la les dirEntry
        ine2->Block[0] = noBlock;

	//on ecris les changement a la struct inode sur le disque
	//copy le contenu en bytes de la struct ds buffer
       // memcpy(BlockBuffInode, &ine2, 256);
        //on ecrase les donnees pour inode 1
        WriteBlock(blockOfInode, BlockBuffInode);
        
        //on ecris les donnees(la liste de direntries) dans le bloc 44
        WriteBlock(noBlock, BlockDirEntry);
        
        //*******************fin de la creation du root directory******************************************//
        
        ////***tests****///
       char testblock[BLOCK_SIZE];
    
       ReadBlock(blockOfInode, testblock);
       iNodeEntry* iNodeEntry2 = (iNodeEntry *) testblock;
       int b = iNodeEntry2->iNodeStat.st_nlink;
       
      
	return 1;



}

int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien){

    if((pPathExistant[0] == '/') != 1){
        return 0;
    }
    
    int inoFichALinker = getInodeFichier(pPathExistant);
    int inoDirOuOnLink = getInodeParentDeFichier(pPathNouveauLien);
    char* nomFichALinker = getNomFich(pPathNouveauLien);
    
    if(inoDirOuOnLink == -1 || inoFichALinker == -1){
        return 0;
    }

    ///-------------------------------------------------------------------------------------------------


    //inoFichALinker est egale a l'inode du fichier qu'on veut linker
    //inoDirOuOnLink est egale a l'inode du directory ds lequel on link
    //nomFichALinker est le nom du fichier a linker....on donnera le meme nom puisque on a pas l'option de le changer
    
    char BuffInoDir[BLOCK_SIZE];
    ReadBlock(getBlockOfInode(inoDirOuOnLink), BuffInoDir);//lit l'inode du dir et cast
    iNodeEntry* inoDir = (iNodeEntry*) BuffInoDir;
    char BuffDirEntryDir[BLOCK_SIZE];
    ReadBlock(inoDir->Block[0], BuffDirEntryDir);//lit direntry du dir et cast
    DirEntry* DirEntryDir = (DirEntry*) BuffDirEntryDir;
    
    //on verifie si un fichier avec ce nom existe deja
    int h;
    for(h = 0; h < MAX_DIRENTRY; h++){
        if(DirEntryDir[h].iNode != FREE_DIRENTRY){
            if((strcmp(DirEntryDir[h].Filename, nomFichALinker)) == 0){
                return 0;
            }
        }
    }

    
    //rendu ici toute les verif sont fait et on fait l'operation ie on ++stlink du fichier et on ajoute un dirEntry dans le directory qui pointe vers l'ino du fichier
    
    //on ajoute le dirEntry dans le dir qui pointe vers notre fichier a linker
    int o;
    for(o = 0; o < MAX_DIRENTRY; o++){
        if(DirEntryDir[o].iNode == FREE_DIRENTRY){
            DirEntryDir[o].iNode = inoFichALinker;
            strcpy(DirEntryDir[o].Filename, nomFichALinker);
            break;
        }
    }
    WriteBlock(inoDir->Block[0], BuffDirEntryDir);//ecrit update de direntry du dir
    
    //on modifie les donnees du dir (dans ce cas la taille du dir augmentera de 16 a cause du nouveau dirEntry)
    inoDir->iNodeStat.st_size =  inoDir->iNodeStat.st_size + 16;
    WriteBlock(getBlockOfInode(inoDirOuOnLink), BuffInoDir);//ecrit les update fait a inode du dir
    
    //on va ensuite incremente le stLink du fichier
    char BuffInoFich[BLOCK_SIZE];
    ReadBlock(getBlockOfInode(inoFichALinker), BuffInoFich);//lit l'inode du dir et cast
    iNodeEntry* inoFich = (iNodeEntry*) BuffInoFich;
    inoFich->iNodeStat.st_nlink = inoFich->iNodeStat.st_nlink + 1;
    WriteBlock(getBlockOfInode(inoFichALinker), BuffInoFich);//ecrit le data de l'inode du fichier (modifie)


    return 1;
}

int bd_mv(const char *pFilename, const char *pFilenameDest){
    int inoFichierOriginal =getInodeFichier(pFilename);
    int inoParentFichierOriginal = getInodeParentDeFichier(pFilename);
    int inoDirectoryCible = getInodeParentDeFichier(pFilenameDest);
    
    if(inoDirectoryCible == -1 || inoFichierOriginal ==-1 || inoParentFichierOriginal == -1){
        return 0;
    }
    
    
    char* nomFichOriginal = getNomFich(pFilename);
    char* nomNouvFich = getNomFich(pFilenameDest);
    
    char BuffinoFichierOriginal[BLOCK_SIZE];
    ReadBlock(getBlockOfInode(inoFichierOriginal), BuffinoFichierOriginal);//lit l'inode du dir et cast
    iNodeEntry* inoFichOri = (iNodeEntry*) BuffinoFichierOriginal;
    
    //si fichier original est un dir, on retourne 0;
    if(inoFichOri->iNodeStat.st_mode == S_IFDIR){
        return 0;
    }
    
    
    //on va ensuite verifier si le directory cible n'est pas plein
    char BuffinoDirectoryCible[BLOCK_SIZE];
    ReadBlock(getBlockOfInode(inoDirectoryCible), BuffinoDirectoryCible);//lit l'inode du dir et cast
    iNodeEntry* inoEntryDirectoryCible = (iNodeEntry*) BuffinoDirectoryCible;
    if(inoEntryDirectoryCible->iNodeStat.st_size == BLOCK_SIZE){
        return 0;
    }
    //si pas plein on va rajouter un direntry dans dir cible avec le nouveau nom
    char BuffDirEntryDirCible[BLOCK_SIZE];
    ReadBlock(inoEntryDirectoryCible->Block[0], BuffDirEntryDirCible);//lit direntry du dir et cast
    DirEntry* DEDirCible = (DirEntry*) BuffDirEntryDirCible;
    int n;
    for(n = 0; n < MAX_DIRENTRY; n++){
        if(DEDirCible[n].iNode == FREE_DIRENTRY){
            DEDirCible[n].iNode = inoFichierOriginal;
            strcpy(DEDirCible[n].Filename, nomNouvFich);
            inoEntryDirectoryCible->iNodeStat.st_size = inoEntryDirectoryCible->iNodeStat.st_size + 16;
            WriteBlock(getBlockOfInode(inoDirectoryCible), BuffinoDirectoryCible);
            WriteBlock(inoEntryDirectoryCible->Block[0], BuffDirEntryDirCible);
            
            break;          
        }
    }
    
    
    //on va ensuite retirer le dirEntry du dir parent de l'original
    char BuffinoParentFichierOriginal[BLOCK_SIZE];
    ReadBlock(getBlockOfInode(inoParentFichierOriginal), BuffinoParentFichierOriginal);//lit l'inode du dir et cast
    iNodeEntry* inoFichParentOri = (iNodeEntry*) BuffinoParentFichierOriginal;
    char BuffDirEntryParentFichierOriginal[BLOCK_SIZE];
    ReadBlock(inoFichParentOri->Block[0], BuffDirEntryParentFichierOriginal);//lit direntry du dir et cast
    DirEntry* DEFichParentOri = (DirEntry*) BuffDirEntryParentFichierOriginal;
    int m;
    int fichTrouve = 0;
    for(m = 0; m < MAX_DIRENTRY; m++){
        if(DEFichParentOri[m].iNode == inoFichierOriginal){
            DEFichParentOri[m].iNode = FREE_DIRENTRY;
            fichTrouve = 1;
            inoFichParentOri->iNodeStat.st_size = inoFichParentOri->iNodeStat.st_size - 16;
            WriteBlock(getBlockOfInode(inoParentFichierOriginal), BuffinoParentFichierOriginal);
            WriteBlock(inoFichParentOri->Block[0], BuffDirEntryParentFichierOriginal);
            
            break;
        }
    }
    
    if(fichTrouve == 0){
        return 0;
    }
    

    return 1;
}

int bd_mkdir(const char *pDirName){
    int i,k,h,d;

    if((pDirName[0] == '/') != 1){
        return 0;
    }
    char* path = getNomFich(pDirName);
    int inoDirParent = getInodeParentDeFichier(pDirName);
    
    if(inoDirParent == -1){
        return 0;
    }
    
    char BlockBuff[BLOCK_SIZE];
    ReadBlock(getBlockOfInode(inoDirParent), BlockBuff);
    iNodeEntry* ine = (iNodeEntry*) BlockBuff;
    
    
    
     //renudu ici path devrait etre egale au nom du directory a creer
    if(ine->iNodeStat.st_size == BLOCK_SIZE){//si le directory est deja plein, erreur
        return 0;
    }
    
    //on verifie si un fichier/directory avec ce nom existe deja
    char BuffVerifDup[BLOCK_SIZE];
    int blockNum = ine->Block[0];
    ReadBlock(blockNum, BuffVerifDup);
    DirEntry* pDirEntry3 = (DirEntry *) BuffVerifDup;
    for(h = 0; h < MAX_DIRENTRY; h++){
        if(pDirEntry3[h].iNode != FREE_DIRENTRY){
            if((strcmp(pDirEntry3[h].Filename, path)) == 0){
                return 0;
            }
        }
    }

    int noNewInode = getNextFreeInode();
    SeizeFreeInode(noNewInode);
    int noNewBlock = getNextFreeBlock();
    SeizeFreeBlock(noNewBlock);
    
    char* nomNewDir = getNomFich(pDirName);
    
    int x = 0;
    x = createEmptyDir(noNewInode, noNewBlock,  ine->iNodeStat.st_ino, nomNewDir);
    
    char BlockFreeBitmap[BLOCK_SIZE];
    ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
    
    if(x == 0){
        return 0;
    }
 
    return 1;
    
    
    
    
}

int bd_create(const char *pFilename){
    if((pFilename[0] == '/') != 1){
        return 0;
    }
    
    char* path = getNomFich(pFilename);
    int noInoParent = getInodeParentDeFichier(pFilename);
    
    if(noInoParent == -1){
       return 0;
    }
    
    char BlockBuff[BLOCK_SIZE];
    ReadBlock(getBlockOfInode(noInoParent), BlockBuff);
    iNodeEntry* ine = (iNodeEntry*) BlockBuff;
    
    
    if(ine->iNodeStat.st_size == BLOCK_SIZE){//si le directory est deja plein, erreur
        return 0;
    }
    
    //on verifie si un fichier avec ce nom existe deja
    char BuffVerifDup[BLOCK_SIZE];
    ReadBlock(ine->Block[0], BuffVerifDup);
    DirEntry* pDirEntry3 = (DirEntry *) BuffVerifDup;
    int h;
    for(h = 0; h < MAX_DIRENTRY; h++){
        if(pDirEntry3[h].iNode != FREE_DIRENTRY){
            if((strcmp(pDirEntry3[h].Filename, path)) == 0){
                return 0;
            }
        }
    }
    
    
  //on cree l'inode de notre fichier et on l'ecris sur le disque------------------------------------------------------
    int noNewInode = getNextFreeInode();
    SeizeFreeInode(noNewInode);
    int noNewBlock = getNextFreeBlock();
    SeizeFreeBlock(noNewBlock);
    char BuffNewInode[BLOCK_SIZE];
    iNodeEntry* newInode = (iNodeEntry*) BuffNewInode;	
    newInode->Block[0] = noNewBlock;//affecte le numero du nouveau bloc
    newInode->iNodeStat.st_blocks = 0;
    newInode->iNodeStat.st_ino = noNewInode;
    newInode->iNodeStat.st_mode = S_IFREG;
    newInode->iNodeStat.st_nlink = 1;
    newInode->iNodeStat.st_size = 0;
    int blockDeNewInode = getBlockOfInode(noNewInode);
    WriteBlock(blockDeNewInode, BuffNewInode);


//------------------------------------------------------------------------------------------------------------------------------------------------------------
    
    //on mais a jour le fichier special du repertoire parent au fichier
    char BuffDirEntryParent[BLOCK_SIZE];
    ReadBlock(ine->Block[0], BuffDirEntryParent);
    DirEntry* pDirEntry2 = (DirEntry *) BuffDirEntryParent;
    
    int iter = 0;
    while(pDirEntry2[iter].iNode != 1000){
        iter++;
    }

    pDirEntry2[iter].iNode = noNewInode;
    strcpy(pDirEntry2[iter].Filename, path);
    //test int test = strcmp(pDirEntry2[iter].Filename, path);
    
    WriteBlock(ine->Block[0], BuffDirEntryParent);

    
//----------------------------------------------------------------------------------------------------------------------------------------------------------------
    
//----------------------------on met a jour l'inode du rep parent------------------------------------------------------------------------------------
    int blocReelDernInode = getBlockOfInode(ine->iNodeStat.st_ino);
    char BlocStatInode[BLOCK_SIZE];
    ReadBlock(blocReelDernInode, BlocStatInode);
    iNodeEntry* ino = (iNodeEntry*) BlocStatInode;
    ino->iNodeStat.st_size = (ino->iNodeStat.st_size) + 16;
    WriteBlock(blocReelDernInode, BlocStatInode);
    
    
    return 1;
}

int bd_ls(const char *pDirLocation){
    
    int t,i,k,o;
    
    //Poutine de verif habituelle------------------------------------------------------------------------------------
    if((pDirLocation[0] == '/') != 1){
        return 0;
    }
    
    int noInode = getInodeFichier(pDirLocation);
    
    if (noInode == -1){
        return 0;
    }
    
    
    //--------------------------------------------on va lire l'inode du fichier en question----------------------------------------------------------------------------------
    char BuffInodRech[BLOCK_SIZE];
    int noBlockIno = getBlockOfInode(noInode);
    ReadBlock(noBlockIno, BuffInodRech);
    iNodeEntry* inoUtil = (iNodeEntry*) BuffInodRech;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //-----------------------------on va lire le dirEntry de notre dir pour l'afficher----------------------------------------------------------------------------------
    char BuffDirEntRech[BLOCK_SIZE];
    int blockToRead = inoUtil->Block[0];
    ReadBlock(blockToRead, BuffDirEntRech);
    DirEntry* DirEntryCourant = (DirEntry *) BuffDirEntRech;
    //--------------------------------------------------------------------------------------------------------------------------------------------------------------------

    //print de ls et de /
    printf("%s\n",pDirLocation);

    //---------------------------Pour ch entree du direntry, on va checher les stats de l'inod et affiche----------------------------------------------------------------------------------
    for(t = 0; t < MAX_DIRENTRY; t++){
        if(DirEntryCourant[t].iNode != FREE_DIRENTRY){//si ce n'est pas une dirEntry libre
            char BuffInoALire[BLOCK_SIZE];
            int blockToRead = getBlockOfInode(DirEntryCourant[t].iNode);
            ReadBlock(blockToRead, BuffInoALire);
            iNodeEntry* inoUtil2 = (iNodeEntry*) BuffInoALire;



            //--------------print du d ou - ---------------------------
            if(inoUtil2->iNodeStat.st_mode == S_IFDIR){
                printf("%c\t ",'d');
            }
            else{
                 printf("%c\t ",'-');
            }

            //on affiche nom du fichier
            printf("%s\t ", DirEntryCourant[t].Filename);

            //on affiche le mot size
            printf("%s\t "," size: ");

            //on affiche la size du fichier
            printf("%d\t ", inoUtil2->iNodeStat.st_size);

            //on affiche le mot inode
            printf("%s\t "," inode: ");

            //on affiche le # d'inode
            printf("%d\t ", inoUtil2->iNodeStat.st_ino);

            //on affiche le mot nlink
            printf("%s\t "," nlink: ");

            //on affiche le # de ling
            printf("%d\n", inoUtil2->iNodeStat.st_nlink);


        }
    }


    return 1;
       
}
   

int bd_rm(const char *pFilename){

    if((pFilename[0] == '/') != 1){
        return 0;
    }
    
    int noInoParent = getInodeParentDeFichier(pFilename);
    if(noInoParent == -1){
        return 0;
    }
    
    char* path = getNomFich(pFilename);
    
    char BlockBuff[BLOCK_SIZE];
    ReadBlock(getBlockOfInode(noInoParent), BlockBuff);
    iNodeEntry* ine = (iNodeEntry*) BlockBuff;




    //on verifie si le fichier existe
    char BuffVerifDup[BLOCK_SIZE];
    ReadBlock(ine->Block[0], BuffVerifDup);
    DirEntry* pDirEntry3 = (DirEntry *) BuffVerifDup;
    int h;
    for(h = 0; h < MAX_DIRENTRY; h++){
        if(pDirEntry3[h].iNode != FREE_DIRENTRY){
            if((strcmp(pDirEntry3[h].Filename, path)) == 0){

                //On vérifie si c'est un répertoire
                int blockOfInodeSource = getBlockOfInode(pDirEntry3[h].iNode);
                //on va lire le bloc de l'inode
                char BlockBuffInodeSource[BLOCK_SIZE];
                ReadBlock(blockOfInodeSource, BlockBuffInodeSource);
                //on cast le suit de bytes dans notre struct d'inode
                iNodeEntry* ineSource = (iNodeEntry*) BlockBuffInodeSource;
                
                //Cas d'un repertoire
                if(ineSource->iNodeStat.st_mode == S_IFDIR){
                    //On supprime un repertoire
                    char BuffVerifDup[BLOCK_SIZE];
                    ReadBlock(ineSource->Block[0], BuffVerifDup);
                    DirEntry* pDirEntry3 = (DirEntry *) BuffVerifDup;
                    int NbreEntry = 0;
                    for(h = 0; h < MAX_DIRENTRY; h++){
                        if(pDirEntry3[h].iNode != FREE_DIRENTRY){
                            NbreEntry++;
                        }
                    }
                    if(NbreEntry > 2){  //Le repertoire n'est pas vide
                        printf("Le repertoire n'est pas vide\n");
                        return 0;
                    }

                    //On met a jour l'inode du parent
                    int blockOfInodeParent = getBlockOfInode(pDirEntry3[1].iNode);
                    char BlockBuffInodeParent[BLOCK_SIZE];
                    ReadBlock(blockOfInodeParent, BlockBuffInodeParent);
                    //on cast le suit de bytes dans notre struct d'inode
                    iNodeEntry* ineParent = (iNodeEntry*) BlockBuffInodeParent;
                    ineParent->iNodeStat.st_nlink =  ineParent->iNodeStat.st_nlink - 1;
                    ineParent->iNodeStat.st_size = ineParent->iNodeStat.st_size - 16;
                    WriteBlock(blockOfInodeParent, BlockBuffInodeParent);
                    //on va ensuite enlever ce repertoire du direntry de son papa
                    char BlockBuffDirParent[BLOCK_SIZE];
                    ReadBlock(ineParent->Block[0],BlockBuffDirParent);
                    DirEntry* DirParent = (DirEntry*) BlockBuffDirParent;
                    for(h = 0; h < MAX_DIRENTRY; h++){
                        if((strcmp(DirParent[h].Filename, path)) == 0){
                            DirParent[h].iNode = FREE_DIRENTRY;
                        }
                    }
                    
                    
                    //On compacte le DirEntry parent*******************************************************


                    WriteBlock(ineParent->Block[0],BlockBuffDirParent);

                    //On supprime le block
                    int d;
                    for(d = 0; d < MAX_DIRENTRY; d++){
                        pDirEntry3[d].iNode = FREE_DIRENTRY;
                    }

                    WriteBlock(ineSource->Block[0], BuffVerifDup);
                    
                    ReleaseFreeBlock(ineSource->Block[0]);
                    ReleaseFreeInode(ineSource->iNodeStat.st_ino);

                }
                else{
                    //On supprime un fichier. 
                    //on s'occupe d'abord d'aller enlever l'entree ds dir parent
                    
                    int inodeDuParent = getInodeParentDeFichier(pFilename);
                    int NumBlocInodeParent = getBlockOfInode(inodeDuParent);
                    char BlocInodeParent[BLOCK_SIZE];
                    ReadBlock(NumBlocInodeParent, BlocInodeParent);
                    iNodeEntry* inoPapa = (iNodeEntry*) BlocInodeParent;
                    
                    char BlocDEParent[BLOCK_SIZE];
                    ReadBlock(inoPapa->Block[0], BlocDEParent);
                    DirEntry* DEpapa = (DirEntry*) BlocDEParent;
                    
                    int j;
                    for(j = 0; j < MAX_DIRENTRY; j++){
                        if(DEpapa[j].iNode == ineSource->iNodeStat.st_ino){
                            DEpapa[j].iNode = FREE_DIRENTRY;
                            inoPapa->iNodeStat.st_size = inoPapa->iNodeStat.st_size - 16;
                            break;
                        }
                    }
                    WriteBlock(inoPapa->Block[0], BlocDEParent);
                    WriteBlock(NumBlocInodeParent, BlocInodeParent);
                    //on decrement ensuite le nlink du fichier
                    ineSource->iNodeStat.st_nlink =  ineSource->iNodeStat.st_nlink - 1;
                    
                    if(ineSource->iNodeStat.st_nlink == 0){
                        ReleaseFreeBlock(ineSource->Block[0]);
                        ReleaseFreeInode(ineSource->iNodeStat.st_ino);
                    }
                    else{//si l'inode du fichier est conserve on va l'updater
                        WriteBlock(blockOfInodeSource, BlockBuffInodeSource);
                    }
                    
                }
                return 1;
            }
        }
    }
    return 0;
}


int ReleaseFreeBlock(UINT16 BlockNum) {
        char BlockFreeBitmap[BLOCK_SIZE];
        ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
        BlockFreeBitmap[BlockNum] = '1';
        printf("IFTFS: Relache block %d\n",BlockNum);
        WriteBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
}

int SeizeFreeBlock(UINT16 BlockNum) {
	char BlockFreeBitmap[BLOCK_SIZE];
        ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
        BlockFreeBitmap[BlockNum] = '0';
        printf("IFTFS: Saisie block %d\n",BlockNum);
        WriteBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
}

int ReleaseFreeInode(UINT16 InodeNum){
	char InodeFreeBitmap[BLOCK_SIZE];
        ReadBlock(FREE_INODE_BITMAP, InodeFreeBitmap);
        InodeFreeBitmap[InodeNum] = '1';
        printf("IFTFS: Relache inode %d\n",InodeNum);
        WriteBlock(FREE_INODE_BITMAP, InodeFreeBitmap);
}
int SeizeFreeInode(UINT16 InodeNum){
	char InodeFreeBitmap[BLOCK_SIZE];
        ReadBlock(FREE_INODE_BITMAP, InodeFreeBitmap);
        InodeFreeBitmap[InodeNum] = '0';
        printf("IFTFS: Saisie inode %d\n",InodeNum);
        WriteBlock(FREE_INODE_BITMAP, InodeFreeBitmap);
}

//Retourne l'index de la premiere occurence libre dans le bitmap des inodes
int getNextFreeInode(){
	char InodeFreeBitmap[BLOCK_SIZE];
        ReadBlock(FREE_INODE_BITMAP, InodeFreeBitmap);
                
        char* ptr = strchr(InodeFreeBitmap, '1');
	
	return ptr - InodeFreeBitmap;
}

int getNextFreeBlock(){
	char BlockFreeBitmap[BLOCK_SIZE];
        ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);

        char* ptr;
        ptr = strchr(BlockFreeBitmap, '1');

        return ptr - BlockFreeBitmap;
}


int getBlockOfInode(int iNodeNumber){
	return iNodeNumber + 4;
}

int getInodeParentDeFichier(const char* pathFichier){
    if((pathFichier[0] == '/') != 1){
        return -1;
    }
    
    if(strcmp("/", pathFichier) == 0){
        return ROOT_INODE;
    }//cas particulier si on demande de faire ls sur ROOT
     
    int compteurSlash, i, k;
    char * ptrCptSlash = pathFichier;
    for (compteurSlash=0; ptrCptSlash[compteurSlash]; ptrCptSlash[compteurSlash]=='/' ? compteurSlash++ : *ptrCptSlash++);
    
    //on va d'abord aller charcher l'inode de root
    int blockOfInode = getBlockOfInode(ROOT_INODE);//va retourner 5
    //on va lire le bloc de l'inode
    char BlockBuffInode[BLOCK_SIZE];
    ReadBlock(blockOfInode, BlockBuffInode);
    //on cast le suit de bytes dans notre struct d'inode
    iNodeEntry* ine = (iNodeEntry*) BlockBuffInode;
    //on va chercher le num de bloc relie a l'inode
    int numBlocDeInode = ine[0].Block[0];
    //on va lire le bloc de data liees l'inode
    char BlockBuffBlock[BLOCK_SIZE];
    ReadBlock(numBlocDeInode, BlockBuffBlock);
    //on cast le suit de bytes dans notre struct de directory
    DirEntry* pDirEntry = (DirEntry *) BlockBuffBlock;
    
    
    char* path = pathFichier;//on discard le premier slash
    path++;//on discard le premier slash
    
    for(i = 0; i < compteurSlash - 1; i++){//boucle ou on s'occupe des directory sur le path

        char* ptrPrchainSlash;
        ptrPrchainSlash = strchr(path, '/');
        int y = ptrPrchainSlash - path;//index du prochain /

        char nomfichier[y+1];
        strncpy(nomfichier, path, y);
        nomfichier[y] = '\0';//marqueur fin de string
        char* nomFich2 = nomfichier;

        path = path + y + 1;//on incremente le ptr de path pour depasser le nom du fichier et le slash qui le suit

        int DirectoryOK = 0;

        for(k = 0; k < MAX_DIRENTRY; k++){

            if(pDirEntry[k].iNode != 1000){
                if((strcmp(pDirEntry[k].Filename, nomFich2)) == 0 ){//si les nom de fichiers concordent
                    int blockInode = getBlockOfInode(pDirEntry[k].iNode);//on va chercher l'inode du dossier en question
                    //on doit verifier que c'est bien un dossier
                    char BlockBuff[BLOCK_SIZE];
                    ReadBlock(blockInode, BlockBuff);
                    iNodeEntry* ine2 = (iNodeEntry*) BlockBuff;
                    if(ine2->iNodeStat.st_mode == S_IFDIR){
                        DirectoryOK = 1;//indique que le dossier a ete trouve
                        ine = ine2;

                        //on reaffecte tous les valeurs pour le nivau inferieur
                        //blockOfInode = getBlockOfInode(pDirEntry->iNode);
                        //ReadBlock(blockOfInode, BlockBuffInode);
                        //ine = (iNodeEntry*) BlockBuffInode;
                        numBlocDeInode = ine[0].Block[0];
                        ReadBlock(numBlocDeInode, BlockBuffBlock);
                        pDirEntry = (DirEntry *) BlockBuffBlock;
                        break;
                    }

                }
            }
        }
        if(DirectoryOK == 0){//si on a pas trouve retourne -1
            return -1;
        }

    }
    
    return ine->iNodeStat.st_ino;
}

int getInodeFichier(const char* pathFichier){
    
    if((pathFichier[0] == '/') != 1){
        return -1;
    }
    
    if(strcmp("/", pathFichier) == 0){
        return ROOT_INODE;
    }//cas particulier si on demande de faire ls sur ROOT
     
    int compteurSlash, i, k;
    char * ptrCptSlash = pathFichier;
    for (compteurSlash=0; ptrCptSlash[compteurSlash]; ptrCptSlash[compteurSlash]=='/' ? compteurSlash++ : *ptrCptSlash++);
    
    //on va d'abord aller charcher l'inode de root
    int blockOfInode = getBlockOfInode(ROOT_INODE);//va retourner 5
    //on va lire le bloc de l'inode
    char BlockBuffInode[BLOCK_SIZE];
    ReadBlock(blockOfInode, BlockBuffInode);
    //on cast le suit de bytes dans notre struct d'inode
    iNodeEntry* ine = (iNodeEntry*) BlockBuffInode;
    //on va chercher le num de bloc relie a l'inode
    int numBlocDeInode = ine[0].Block[0];
    //on va lire le bloc de data liees l'inode
    char BlockBuffBlock[BLOCK_SIZE];
    ReadBlock(numBlocDeInode, BlockBuffBlock);
    //on cast le suit de bytes dans notre struct de directory
    DirEntry* pDirEntry = (DirEntry *) BlockBuffBlock;
    
    
    char* path = pathFichier;//on discard le premier slash
    path++;//on discard le premier slash
    
    for(i = 0; i < compteurSlash - 1; i++){//boucle ou on s'occupe des directory sur le path

        char* ptrPrchainSlash;
        ptrPrchainSlash = strchr(path, '/');
        int y = ptrPrchainSlash - path;//index du prochain /

        char nomfichier[y+1];
        strncpy(nomfichier, path, y);
        nomfichier[y] = '\0';//marqueur fin de string
        char* nomFich2 = nomfichier;

        path = path + y + 1;//on incremente le ptr de path pour depasser le nom du fichier et le slash qui le suit

        int DirectoryOK = 0;

        for(k = 0; k < MAX_DIRENTRY; k++){

            if(pDirEntry[k].iNode != 1000){
                if((strcmp(pDirEntry[k].Filename, nomFich2)) == 0 ){//si les nom de fichiers concordent
                    int blockInode = getBlockOfInode(pDirEntry[k].iNode);//on va chercher l'inode du dossier en question
                    //on doit verifier que c'est bien un dossier
                    char BlockBuff[BLOCK_SIZE];
                    ReadBlock(blockInode, BlockBuff);
                    iNodeEntry* ine2 = (iNodeEntry*) BlockBuff;
                    if(ine2->iNodeStat.st_mode == S_IFDIR){
                        DirectoryOK = 1;//indique que le dossier a ete trouve
                        ine = ine2;

                        //on reaffecte tous les valeurs pour le nivau inferieur
                        //blockOfInode = getBlockOfInode(pDirEntry->iNode);
                        //ReadBlock(blockOfInode, BlockBuffInode);
                        //ine = (iNodeEntry*) BlockBuffInode;
                        numBlocDeInode = ine[0].Block[0];
                        ReadBlock(numBlocDeInode, BlockBuffBlock);
                        pDirEntry = (DirEntry *) BlockBuffBlock;
                        break;
                    }

                }
            }
        }
        if(DirectoryOK == 0){//si on a pas trouve retourne -1
            return -1;
        }

    }
    //ici path est egale au nom du fichier
    int oo;
    for(oo = 0; oo < MAX_DIRENTRY; oo++){
        if((strcmp(pDirEntry[oo].Filename, path)) == 0 ){
            return pDirEntry[oo].iNode;
        }
    }
    //si erreur return -1
    return -1;
    
}

char* getNomFich(const char* pathFichier){
    
     
    int compteurSlash, i, k;
    char * ptrCptSlash = pathFichier;
    for (compteurSlash=0; ptrCptSlash[compteurSlash]; ptrCptSlash[compteurSlash]=='/' ? compteurSlash++ : *ptrCptSlash++);
    
    //on va d'abord aller charcher l'inode de root
    int blockOfInode = getBlockOfInode(ROOT_INODE);//va retourner 5
    //on va lire le bloc de l'inode
    char BlockBuffInode[BLOCK_SIZE];
    ReadBlock(blockOfInode, BlockBuffInode);
    //on cast le suit de bytes dans notre struct d'inode
    iNodeEntry* ine = (iNodeEntry*) BlockBuffInode;
    //on va chercher le num de bloc relie a l'inode
    int numBlocDeInode = ine[0].Block[0];
    //on va lire le bloc de data liees l'inode
    char BlockBuffBlock[BLOCK_SIZE];
    ReadBlock(numBlocDeInode, BlockBuffBlock);
    //on cast le suit de bytes dans notre struct de directory
    DirEntry* pDirEntry = (DirEntry *) BlockBuffBlock;
    
    
    char* path = pathFichier;//on discard le premier slash
    path++;//on discard le premier slash
    
    for(i = 0; i < compteurSlash - 1; i++){//boucle ou on s'occupe des directory sur le path

        char* ptrPrchainSlash;
        ptrPrchainSlash = strchr(path, '/');
        int y = ptrPrchainSlash - path;//index du prochain /

        char nomfichier[y+1];
        strncpy(nomfichier, path, y);
        nomfichier[y] = '\0';//marqueur fin de string
        char* nomFich2 = nomfichier;

        path = path + y + 1;//on incremente le ptr de path pour depasser le nom du fichier et le slash qui le suit

        int DirectoryOK = 0;

        for(k = 0; k < MAX_DIRENTRY; k++){

            if(pDirEntry[k].iNode != 1000){
                if((strcmp(pDirEntry[k].Filename, nomFich2)) == 0 ){//si les nom de fichiers concordent
                    int blockInode = getBlockOfInode(pDirEntry[k].iNode);//on va chercher l'inode du dossier en question
                    //on doit verifier que c'est bien un dossier
                    char BlockBuff[BLOCK_SIZE];
                    ReadBlock(blockInode, BlockBuff);
                    iNodeEntry* ine2 = (iNodeEntry*) BlockBuff;
                    if(ine2->iNodeStat.st_mode == S_IFDIR){
                        DirectoryOK = 1;//indique que le dossier a ete trouve
                        ine = ine2;

                        //on reaffecte tous les valeurs pour le nivau inferieur
                        //blockOfInode = getBlockOfInode(pDirEntry->iNode);
                        //ReadBlock(blockOfInode, BlockBuffInode);
                        //ine = (iNodeEntry*) BlockBuffInode;
                        numBlocDeInode = ine[0].Block[0];
                        ReadBlock(numBlocDeInode, BlockBuffBlock);
                        pDirEntry = (DirEntry *) BlockBuffBlock;
                        break;
                    }

                }
            }
        }
        

    }
    //ici path est egale au nom du fichier
    return path;
}

int createEmptyDir(int InodeCourant, int blocInodeCourant, int inodeParent, const char* nomNewDir){
    
    char* nomDir = nomNewDir;
    
    char BuffNewInode[BLOCK_SIZE];
    iNodeEntry* newInode = (iNodeEntry*) BuffNewInode;	
    newInode->Block[0] = blocInodeCourant;//affecte le numero du nouveau bloc
    newInode->iNodeStat.st_blocks = 1;
    newInode->iNodeStat.st_ino = InodeCourant;
    newInode->iNodeStat.st_mode = S_IFDIR;
    newInode->iNodeStat.st_nlink = 2;//on va ajouter un link ds dir parent et va se referer luimeme ds "."
    newInode->iNodeStat.st_size = 32;
    
    int blockDeNewInode = getBlockOfInode(InodeCourant);
    
    
    
    WriteBlock(blockDeNewInode, BuffNewInode);
    
    
    
    //on met a jour le direntry du repertoire parent au fichier----------------------------------------------------
    char BuffInoRepParent[BLOCK_SIZE];
    int blockofino = getBlockOfInode(inodeParent);
    ReadBlock(getBlockOfInode(inodeParent), BuffInoRepParent);
    iNodeEntry* inorepParent = (iNodeEntry*) BuffInoRepParent;	
    
    char BuffDirEntryParent[BLOCK_SIZE];
    ReadBlock(inorepParent->Block[0], BuffDirEntryParent);
    DirEntry* pDirEntry2 = (DirEntry *) BuffDirEntryParent;
    
    int iter = 0;
    while(pDirEntry2[iter].iNode != 1000){
        iter++;
    }
    
    pDirEntry2[iter].iNode = InodeCourant;
    strcpy(pDirEntry2[iter].Filename, nomDir);
    
    WriteBlock(inorepParent->Block[0], BuffDirEntryParent);
    
    //----------------------------on met a jour l'inode du rep parent------------------------------------------------------------------------------------
   
    inorepParent->iNodeStat.st_size = inorepParent->iNodeStat.st_size + 16;
    inorepParent->iNodeStat.st_nlink++;
    WriteBlock(getBlockOfInode(inodeParent), BuffInoRepParent);
    
     
     //-------------------------------------------------------------------------------------------------------------------
    
    //----------------------------On ecris le fichier special du repertoire courant------------------------------------------------------------------------------------
    
    char BuffnewDirEntry[BLOCK_SIZE];
    DirEntry* pDirEntryNewDir = (DirEntry *) BuffnewDirEntry;
    pDirEntryNewDir[0].iNode = InodeCourant;//affect le no d'inode au dir entry (rep courant) meme que parent pour /
    strcpy(pDirEntryNewDir[0].Filename, ".");
    pDirEntryNewDir[1].iNode = inorepParent->iNodeStat.st_ino;//affect le no d'inode au dir entry (rep parent)
    strcpy(pDirEntryNewDir[1].Filename, "..");

    int d;
    for(d = 2; d < MAX_DIRENTRY; d++){
        pDirEntryNewDir[d].iNode = 1000;//1000 sera un indicateur que le dir entry est vide...il n'y aura jamais 1000 inodes ds notre systeme
    } 
    
    WriteBlock(newInode->Block[0], BuffnewDirEntry);
    
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------
    
    return 1;
}




//verifie que le parent existe et met a jour path pour qu'il soit egale au nom du fichier
int verifParentExiste(int compteurSlash, char** pathArg, iNodeEntry** ineArg, DirEntry** pDirEntryArg){
    
    DirEntry* pDirEntry = *pDirEntryArg;
    iNodeEntry* ine = *ineArg;
    char* path = *pathArg;
    char BlockBuffBlock[BLOCK_SIZE];
    int numBlocDeInode, i, k;
    
    for(i = 0; i < compteurSlash - 1; i++){//boucle ou on s'occupe des directory sur le path
        
        char* ptrPrchainSlash;
        ptrPrchainSlash = strchr(path, '/');
        int y = ptrPrchainSlash - path;//index du prochain /
        
        char nomfichier[y+1];
        strncpy(nomfichier, path, y);
        nomfichier[y] = '\0';//marqueur fin de string
        char* nomFich2 = nomfichier;
        
        path = path + y + 1;//on incremente le ptr de path pour depasser le nom du fichier et le slash qui le suit
        
        int DirectoryOK = 0;
        
        for(k = 0; k < MAX_DIRENTRY; k++){
            
            if((strcmp(pDirEntry[k].Filename, nomFich2)) == 0 ){//si les nom de fichiers concordent
                int blockInode = getBlockOfInode(pDirEntry[k].iNode);//on va chercher l'inode du dossier en question
                //on doit verifier que c'est bien un dossier
                char BlockBuff[BLOCK_SIZE];
                ReadBlock(blockInode, BlockBuff);
                iNodeEntry* ine2 = (iNodeEntry*) BlockBuff;
                if(ine2->iNodeStat.st_mode == S_IFDIR){
                    DirectoryOK = 1;//indique que le dossier a ete trouve
                    ine = ine2;
                    
                    //on reaffecte tous les valeurs pour le nivau inferieur
                    //blockOfInode = getBlockOfInode(pDirEntry->iNode);
                    //ReadBlock(blockOfInode, BlockBuffInode);
                    //ine = (iNodeEntry*) BlockBuffInode;
                    numBlocDeInode = ine[0].Block[0];
                    ReadBlock(numBlocDeInode, BlockBuffBlock);
                    pDirEntry = (DirEntry *) BlockBuffBlock;
                    break;
                }
                
            }
        }
        if(DirectoryOK == 0){//si on a pas trouve tous les dossiers passe en parametre: erreur on retourne 0
            return 0;
        }
        
    }
    
    //on reaffecte les valeurs dans la fonction appelante by ref
    *pathArg = path;
    *ineArg = ine;
    
    return 1;
    
}
