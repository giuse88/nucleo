#ifndef FCB_H 
#define FCB_H

#include "volumi.h"

/* STRUTTURA FCB *************************************
 * Struttura che permette di manegiare i file senza  *
 * essere a conoscenza dei relativi cluster occupati *
 * nel disco.					     *
 *****************************************************/

struct _TABELLA_VOLUMI_; 
typedef struct _FCB_ FCB; 
struct _FCB_ { 
  
  struct _TABELLA_VOLUMI_ * volume; // puntatore nel quale è contenuto il file 
  
  dword cluster; 		//primo cluster nella regioni  data 
  
  // info file 
  byte type;			// flag presente nella short_entry 
  dword mode; 			// modalita di apertura 
  dword pos_corr;   		// posizione corrente   	
  dword size; 	         	// grandezza file
  

  //info directory padre
  // info neccessarie per un accesso rapido alla informaioni 
  
  dword cluster_father; 	// cluster del padre 
  lword offset_father;          // offset in byte nel padre 
  word  n_entry; 		// n_entry di cui è composto 
 
}__attribute__((packed)); 

void print_fcb ( const FCB * fcb );
int read_fcb(FCB*file, void *buf, size_t count);
int  write_fcb(FCB*file, const void *buf, size_t count); 

#endif