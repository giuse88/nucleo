#ifndef _DIR_ENTRY_
#define _DIR_ENTRY_

#include "type.h"
#include "volumi.h" 
#include "fcb.h" 

/*ENTRY DIR */ 

#define FREE             	0xE5                // entry libera
#define ALL_FREE    	 	0x00                // terminate le entry
#define DELETED_FLAG 		0xE5		    // entry libera 

#define ATTR_READ_ONLY 	 	0x01 
#define ATTR_HIDDEN      	0x02 
#define ATTR_SYSTEM      	0x04 
#define ATTR_VOLUME_ID   	0x08
#define ATTR_DIRECTORY   	0x10 
#define ATTR_ARCHIVE     	0x20 
#define ATTR_LONG_NAME   	(ATTR_READ_ONLY  | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK	(ATTR_READ_ONLY  | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE)

#define MASK_LAST_NAME 		0x40	// maschera ultima entry lunga 
#define PADDING			0xFFFF	// riempimento 
#define SIZE_NAME_PART 		13	// numero di lettere inserite in un entry long 
#define SIZE_NAME 		255	// lunghezza massima nome lungo 
#define MAX_NAME 		255	// lunghezza massima nome lungo 
#define MAX_PATH 		260 	// lunghezza massima path 
#define MSDOS_NAME 		11 	// lunghezza nome corto 
#define SIZE_NAME_SHORT 	11 	// lunghezza nome corto 
#define SIZE_ENTRY 		32	// grandezza entry 
#define MAX_LONG_ENTRY		20	// numero massimo di entry lunghe 
#define SIZE_DIRECTORY 		0	// grandezza default directory 
// grandezza componenti di un nome lungo 

#define SIZE_UNO 5
#define SIZE_DUE 6
#define SIZE_TRE 2

// grandezza dei buffer con il quale avvengono scritture e letture 
#define SIZE_BUF 	1024





/*STRUTTURA ***************************************
 * Che rapresenta la struttura delle entry corte  * 
 * prese nti sul disco.				  *
 **************************************************/ 

typedef struct _SHORT_ENTRY_{ 
  byte DIR_Name[MSDOS_NAME];	 // nome corto 
  byte DIR_Attr;   		 // attributi 
  byte DIR_NTRes;   		 // riservato per windows nt
  byte DIR_CrtTimeTenth; 	 // vedi specifiche 

  word DIR_CrtTime ;  		 // ora di creazione 
  word DIR_CrtDate; 		 // data di creazione 
  word DIR_LstAccDate;		 // Ultimo accesso

  word DIR_FstClusHI; 		 // parte alta indirizzo clusterFAT32 

  word DIR_WrtTime ; 		 // ora di scrittura 
  word DIR_WrtDate; 		 // data discrittura
  word DIR_FstClusLO;		 // Parte bassa dell'indirizzo   

  dword DIR_FileSize;		 // grandezza del file in byte 

} __attribute__((packed))SHORT_ENTRY;  



/*STRUTTURA ***************************************
 * Che rapresenta la struttura delle entry lunghe * 
 * prese nti sul disco.				  *
 **************************************************/ 

typedef struct _LONG_ENTRY_FAT_ { 
  byte    LDIR_Ord;    		 // Ordine delle entrate 
  word    LDIR_Name1[5];   	// 1-5  
  byte    LDIR_Attr;    	// attributo ( directory lunga)
  byte    LDIR_Type;  		// type di entry 

  byte  LDIR_Chksum ;  		// Checksum, 
  word 	LDIR_Name2[6];     	// data di creazione 
  word	LDIR_FstClusLO;  	// Deve essere zero
  word 	LDIR_Name3[2];     	//  


}__attribute__((packed))LONG_ENTRY;  


//void print_fcb (const  FCB*); 

/* STRUTTURE **********************************
 * Strutture per gestire ora e data microsoft *
 **********************************************/

struct _msdos_date_  { 
 unsigned  Day :5; 
 unsigned Month:4; 
 unsigned Years:7; 
}__attribute__((packed));

typedef struct _msdos_date_ msdos_date; 

struct _msdos_time_ { 
  unsigned Second:5;
  unsigned Minutes:6; 
  unsigned Hours:5; 
}__attribute__((packed));
  
typedef struct  _msdos_time_ msdos_time; 

/******************* INTERFACE **************/

// gestione entry
BOOL open_entry   (const char * name , const FCB * father, FCB * new); 
BOOL delete_entry (const FCB * file); 
BOOL create_entry (const char * name , byte type, dword first_cluster, dword size, const FCB * father, FCB * new);
BOOL rename_entry(const char*name, FCB*father,  FCB* old);
//gestione directory 
BOOL create_directory(const char * name,  const FCB * father, FCB * new); 
BOOL delete_directory(FCB*); 
#endif
