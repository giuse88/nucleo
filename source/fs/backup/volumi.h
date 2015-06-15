

#ifndef  VOLUMI_H 
#define  VOLUMI_H 


#include "type.h"
#include "fat.h" 
#include "string.h" 


#define N_ATA    2 
#define N_DISK  2
#define SECT_SIZE 512 

typedef struct _TABELLA_VOLUMI_ TABELLA_VOLUMI; 
typedef struct _INFO_FAT_ INFO_FAT; 

/*SOLO FAT 32 */

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
  INFO_FAT fat_info;    // struttura che contiene info di basso livello del fs
  dword_ptr  fat;                       // indiirizzo fat caricata in memoria 
  /*************************************/

  dword_ptr root;     // puntatore al primo cluster della directiri di root 

  dword semaphore;  
	
  TABELLA_VOLUMI * next; 

}__attribute__((packed));



/*INTERFACCIA Tabella volumi
 * Funzioni che ci vengono messe a disposizione dall'oggetto tabella dei volumi 
 * per la sua corretta gestione.
 */ 

// funzione che inizializza la tabella dei volumi 
TABELLA_VOLUMI * crea_tabella_volumi(); 
// funzione che riporta un entrata della tabella dei volumi 
TABELLA_VOLUMI * get_volume ( byte label) ; 

// funzione di debug che stampa il contenuto della tabella dei volumi 
#ifdef DEBUG_FS 
  void stampa_tabella_volumi( const TABELLA_VOLUMI * list);
#endif

  
#endif
