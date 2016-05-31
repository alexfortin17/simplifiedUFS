// Les fonctions d'acces au disque dur. Les fonctions bd_*
// utilisent ces fonctions pour lire et ecrire sur le disque.
int ReadBlock(UINT16 BlockNum, char *pBuffer);
int WriteBlock(UINT16 BlockNum, const char *pBuffer);

