
/***********FAT ***************************************************************************************

  STRUTTURA FAT 
  
 +----------------------------------------------+-------------+-----------------+---------------+
 | AREA RISERVATA                               |FAT          | ROOT DIRECTORY  |  REGIONE DATI |
 +------------------+-------------+-------------+------+------+-----------------+---------------+
 | Settore d'avvio  | Info FS     | RISERVATI   | FAT1 | FAT2 | Roott directory | Dati          |
 | (BPB+altro)      | Solo Fat32  | Opzionale   |      |      | FAT12 FAT 16    |               |
 +------------------+-------------+-------------+------+------+-----------------+---------------+
 |n_sec_reserved                                | n_fat*fats  |RootDirSector    |numero_di_c    |
 +----------------------------------------------+-------------+-----------------+---------------+



*******************************************************************************************************/ 
// 
#ifndef FAT_H
#define FAT_H

#include "type.h" 
#include "volumi.h"

#define BOOT_SIG   	       0x29    // parametri di boot estesi 
#define BS_SIZE    		512
#define SECT_SIZE 		512
#define SIZE_ENTRY_FAT_32	  4
#define SECTOR_GROUP 		 32
/*chain of the fat 32*/ 

#define EOC_32             0x0FFFFFF8 
#define MASK_32  	   0x0FFFFFFF
#define BAD_CLUSTER_32 	   0x0FFFFFF7
#define FREE_32		   0x00000000	

#define EMPTY			 0x00
#define FAT12			 0x01 
#define FAT16 			 0x06
#define FAT32 			 0x0B
#define FAT32_LBA		 0x0C
#define EXTEND 			 0x05

#define MBP_SIZE		 446 
#define MBT_SIZE 		  64
#define MBR_SIZE 		 512 

#define isFree_fat(N) ((N&MASK_32)==FREE_32)
#define isLast_fat(N) ((N&MASK_32)>=EOC_32)
#define isBad_fat(N)  ((N&MASK_32)==BAD_CLUSTER_32)


/*******************STRUTTURE FAT BASE ****************************************************************/



typedef struct _BS_16_ {
  
  byte  BS_DrvNum;         // numero drive 
  byte  Reserved1;         // byte iservato per Windows NT 
  byte  BS_BootSig;        // se è uguale 0x29 (boot signature ) sono settati i valori successi
  dword BS_VolID;          // volume serial number 
  byte  BS_VolLab[11];     // Ettichetta registrata nella root directori
  byte  BS_FilSysType[8];  // tipo file system 

}__attribute__((packed))BS_16;  


/* 
 BPB usato per i filesystem FAT12 FAT 16 
*/ 

typedef struct _BS_32 { 

  dword  BPB_FATsz32;                //grandezza FAT for FAT32 in settori  
  word   BPB_ExtFlags;              // estesi flag vedi specifiche 
  word   BPB_FSVer;                 // versione di FAT32 viene attivata con i flag precedenti 
  dword  BPB_RootClus;              // imposta il numero (indirizzo) del primo cluster dellaroot directory ---- WARNIN----
  word   BPB_FSInfo;                // numero settore nella regione riservata che contiene la struttura FSINFO  
  word   BPB_BkBootSec;             // indica il settore di backup del boot sector 
  byte   BPB_Reserved[12];          // riservato per futuri uytilizzi
 
  byte  BS_DrvNum;                  // numero drive 
  byte  Reserved1;                  // byte iservato per Windows NT
  byte  BS_BootSig;                 // 0x29 boot signature 
  dword BS_VolID;                   // volume serial number 
  byte  BS_VolLab[11];              // Ettivchetta registrata nella root directori
  byte  BS_FilSysType[8];           // tipo file system 
}__attribute__((packed))BS_32;



/*
 *BPB  sara un po piu complessa ( usa le unioni per le parti non comuni 
 */


typedef struct _BOOT_SECTOR_ {

  /* Fine tratto comune fra  FAT12/16/32 */ 

  byte BS_jmpBoot[3];     // istruzione di salto ????????
  byte BS_OEMName[8];     // stringa, nome del sistema su cui è stato formatato il disco (mkmsdos) 
  
  word BPB_BytsPerSec;    // Byte per settore ( tipicamente 512) 
  byte BPB_SecPerClus;    // Numero di settori per unita di allocazione, deve essere una potenza di due  

  /* N.B  Il numero di byte per unita di allocazione  deve essere sempre minore di 32k 
   * bytePerSec * SecPerClus < 32k
   */

  word  BPB_RsvdSecCnt;  // numero di settori riservati vedi apppunti Non puo essere zero 
                         // fat12/16 non puo essere diverso da zero 
                         // fat32   tipicamnete 32 

  
  byte BPB_NumFATs;      //  numero di tabelle ridondanti presenti nel volume tipicamnete 2 
                         // ed è fortamente racomandato  MI$CROSFT è che sia 2 

  /* Questo campo contiene il numero di voci presenti nella cartella di root! 
   * vale zero per il fat 32 mentre assume senso per i vari fat12/16! vedi regole nel documento */ 

  word BPB_RootEntCnt;   

  word BPB_TotSec16;    // numero totale di settori per volume  

  byte BPB_Media;       // diferernzia volumi rimovibili e volumi fissi 

  word BPB_FATSz16;     // vale solo per fat12/16 indica la grandezza della fat in settori 

  word BPB_SecPerTrk;   // questo campo contien i settori per traccia vale per dispositivi mobili
 
  word BPB_NumHeads;    // n umero di testine come prima 
 
  dword BPB_HiddSec;    // Numero di settori che precedono la partizione che contiene questo volume

  dword  BPB_TotSec32;  // numero settori tottali nel volume per fat32 

  /* Fine tratto comune fra  FAT12/16/32 */ 

  union { 
    BS_16 bpb_16; // BPB 12/16 
    BS_32 bpb_32; 
  }; 
 
  byte body[420]; 
  byte magic_number[2]; 
}__attribute__((packed))BOOT_SECTOR; 
  
/******************* END STRUTTURE FAT BASE ****************************************************************/



/*********************************************MACRO**********************************************************/

//ROOT_DIR_SECTOR : 
//Calcola la grandezza della root directory nei file system FAT16 FAT 12 
#define RootDirSector(n_ent_root, byts_for_sect)  (((n_ent_root*32)+(byts_for_sect-1))/byts_for_sect) 
//FIRST_DATA_SECTOR
//Calcola il primo settore della regione dati relativo all'inizio del volume 
#define FirstDataSector(n_sect_reserved, n_FATs, FAT_size, root_dir_sector)  n_sect_reserved + (n_FATs * FAT_size) + root_dir_sector 
//SIZE_DATA
//Calcolo grandezza area  dati in settori 
#define SizeData( n_cluster, setor_for_cluster) n_cluster*sector_for_cluster
//FIRST_DATA_SECTOR
//Primo settore di un cluster N puo' essere relativo al disco o al volume in base al valore di first_data_sector 
#define FirstDataSectorOfCluster(N, set_for_cluster, first_data_sector) (((N-2)*set_for_cluster)+first_data_sector)
//FAT_SIZE 
//Grandezza regione FAT in settori 
#define FATsize( FAT16_size, FAT32_size) (FAT16_size != 0) ? FAT16_size : FAT32_size 
//FIRST_sect_FAT 
// Primo settore della fat 
#define FirstSectFAT(n_sect_reserved) n_sect_reserved
//LAST SECTOR FAT 
//Calcola ultimo settore fat
#define LastSectFAT(First, size) First+size 
//TOT_SECTOR
//TOttale settori del disco  
#define TotSector(TotSect16, TotSect32) (TotSect16 != 0) ? TotSect16 : TotSect32
//Data SIZE 
//Grandezza della regione dati in settori 
#define DataSize( tot_sec, n_sect_reserved, n_FAT, FAT_size, root_dir_sector) tot_sec-( n_sect_reserved + (n_FAT * FAT_size) + root_dir_sector)
// Data SIZE CLUSTER
// Grandezza della regione dati in cluster 
#define DataSizeCluster( DataSize, set_for_cluster) DataSize / set_for_cluster 
//getSectRoot
//Calcola il primo settore di root
#define FirstRootDirSecNum(n_sect_reserved, n_FAT, FAT_size) n_sect_reserved + ( n_FAT*FAT_size)
// FAT SECTOR NUMBER 
// Dato il cluster calcola il settore del disco in cui è posizionato
#define FATSectorNum( N, FirstSectFat, type, byte_for_sect)  FirstSectFat + ( N* ((type == FAT16) ? 2 : 4))/byte_for_sect
//FAT OFFSET 
// Dato il cluster calcola l'oofest all'interno del settore
#define FATOffset( N, type, byte_for_sect)   ( N* ((type == FAT16) ? 2 : 4))%byte_for_sect
/*Macro specifiche per fat 32 con settori da 512 */
#define FATSectorNum_32(N, First) FATSectorNum(N,First, FAT32, 512) 
#define FATOffeset_32(N) FATOffset(N, FAT32, 512) 




/************************************************************END MACRO*************************************************************/



/*FUNZIONI**************************************************************************/

/*GESTIONE FAT *********************************************************************
 * INTERFACCIA  che mi permette di gestire la tabella FAT.			   *
 * Funzioni elementari che lavorano sulla tabella FAT per convenzione tutte le	   *
 * funzioni che riguardano a fat terminano con fat*				   *
 ***********************************************************************************/

// inizializza la fat in memoria 
BOOL init_fat();
// elimina  tutte le fat presenti in memoria
void delete_memory_fat();
// crea una catena 
BOOL create_fat   (const VOL_PTR v, dword* chain); 
// aggiunge un cluster alla lista chain
BOOL append_fat (const VOL_PTR v , const dword chain, dword *addr); 
// elimina l'ultimo cluster dalla lista chain  ( non utilizzata ) 
BOOL delete_fat (VOL_PTR v, const dword chain); 
// elimina tutta una lista chain 
BOOL delete_all_fat (const VOL_PTR volume, const dword chain);
//scorre una catena 
dword get_next_fat(const fat_ptr fat, const dword cluster); 
// funzione di debug che stampa informazioni attinenti alla tabella fat
void info_fat(); 
// stampa la catena individuata da chain 
void print_chain( VOL_PTR, dword chain );


/***********************************************************************************/




#endif 
