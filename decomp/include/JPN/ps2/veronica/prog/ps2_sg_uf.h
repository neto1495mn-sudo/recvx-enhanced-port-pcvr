#ifndef _PS2_SG_UF_H_
#define _PS2_SG_UF_H_

#include "types.h"

Bool ufAutoSynch(Uint32 count); 
Bool ufChangeWorkBuffer(void* work1, void* work2, void* work3, void* work4); 
void ufExitSystem(void); 
Bool ufInitSystem(void* work1, void* work2, void* work3, void* work4); 
Sint32 ufPlayTime(Uint8 mode, Uint8 est, Uint8 carry, Uint8 param[32]); 
Bool ufReadAllHead(Uint8* ProdNum, UFS_AllPlayHistory* buff);
Bool ufSynch(void);

#endif
