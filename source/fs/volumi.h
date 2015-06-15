#ifndef  _VOLUMI_H 
#define  _VOLUMI_H 


#include "type.h"
#include "string.h"
#include "direntry.h"
#include "fs.h" 
#include "sistema.h"
#include "errno.h" 
#include "fcb.h" 


typedef struct _TABELLA_VOLUMI_ TABELLA_VOLUMI; 
typedef struct _INFO_FAT_ INFO_FAT; 
typedef TABELLA_VOLUMI * VOL_PTR; 
struct  _FCB_; 


/*Struttura *****************************************************
 * Struttura che raggruppa tutte le informazioni neccessarie    *
 * per gestire un volume fat.					*
 ****************************************************************/ 

struct _INFO_FAT_ { 
 
  dword first_cluster_directory; 

  dword first_set_data; 
  dword size_data; 
  
  dword first_set_fat; 
  dword size_fat;

  byte sectors_for_cluster; 
  word  byts_for_sector; 

}__attribute__((packed));




struct _TABELLA_VOLUMI_ { 

  /* INFORMAZIONI GENERALI VOLUME */
  byte type_fs;
  byte ata;
  byte disco; 
  byte indice_partizione; 
  byte label; 

    /* INFORMAZIONI GENARALI FS */
  INFO_FAT fat_info;   		 // struttura che contiene info di basso livello del fs
  dword_ptr  fat;      		 // indiirizzo fat caricata in memoria
  dword      size_fat; 		 // grandezza fat in byte

  dword_ptr root;    		 // puntatore al primo cluster della directiri di root 
  struct _FCB_  *fcb_root; 		 // fcb della directopry di root 
  dword sem_fat;   		 // mutua esclusione sulla fat
  dword sem_data; 		// mutua esclusione sulla regione data 
  TABELLA_VOLUMI * next; 

}__attribute__((packed));




/* INTERFACCIA Tabella volumi ****************************************************
 * Funzioni che ci vengono messe a disposizione dall'oggetto tabella dei volumi  *
 * per la sua corretta gestione.                                                 *
 *********************************************************************************/ 

// funzione che inizializza la tabella dei volumi 
TABELLA_VOLUMI * crea_tabella_volumi(); 
// funzione che riporta un entrata della tabella dei volumi 
TABELLA_VOLUMI * get_volume ( byte label) ; 
// funzione che elimina la tabella dei volumi 
void delete_tabella();
// funzione di debug che stampa il contenuto della tabella dei volumi 
#ifdef DEBUG_FS 
  void stampa_tabella_volumi();
#endif
  



#endif
