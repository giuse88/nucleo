#ifndef _DATA_H
#define _DATA_H


#include "fat.h" 
#include "volumi.h" 
#include "sistema.h"
#include "type.h"
#include "errno.h" 


/*Interfaccia per la gestione della zona dati del disco */ 

// funzione che scrive buf  catena chain  all'offset specificato
int write_data( VOL_PTR volume  , dword chain, size_t offset, const byte * buf, size_t size); 
// funzione che legge  buf  catena chain  all'offset specificato
int read_data(VOL_PTR volume , dword chain, lword offset,  byte * buf, dword size); 

#endif