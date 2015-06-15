#ifndef _DATA_H
#define _DATA_H


#include "type.h"

/*Interfaccia per la gestione della zona dati del disco */ 

// funzione che scrive buf sul volume nella catena chain 
BOOL write_data(byte label , dword chain, lword offset, const byte * buf, dword size); 
// funzione che legge buf dal volume label sulla catena chain 
dword read_data(byte label , dword chain, lword offset,  byte * buf, dword size); 

BOOL test_read ( byte label);


#endif