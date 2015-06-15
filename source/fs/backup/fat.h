
/***********FAT ***************************************************************************************

Utiliziamo il file system fat introdotto con windows 95 o meglio la struttura BPB fondamentale 
per il corretto funzionamento. 
Usiamo lo stesso modello Micro$oft per analizare il filesystem vedi dopo

-Assuminamo che i settori siano da 512 byte 
 
 Il filse system fat è diviso in tre parti separate : 
     0-REserved Region 
     1-FatRegion 
     2-Root Direcoty Region ( non esiste su fat32) 
     3-File e Directory 
   
 +----------------------------------------------+-------------+-----------------+---------------------------------+
 | AREA RISERVATA                               |FAT          | ROOT DIRECTORY  |  REGIONE DATI |
 +------------------+-------------+-------------+------- ------+-----------------+---------------+
 | Settore d'avvio  | Info FS        | RISERVATI   | FAT1 | FAT2 | Roott directory | Dati          |
 | (BPB+altro)       | Solo Fat32  | Opzionale   |      |      | FAT12 FAT 16    |               |
 +------------------+-------------+-------------+------+------+-----------------+---------------+
 |n_sec_reserved                                | n_fat*fats  |RootDirSector    |numero_di_c    |
 +----------------------------------------------+-------------+-----------------+---------------+



*******************************************************************************************************/ 
// 
#ifndef FAT_H
#define FAT_H

#include <type.h> 
#include <mbr.h> 
#include <fs.h>

#define BOOT_SIG    0x29    // parametri di boot estesi 
#define BPB_SIZE    512
#define SECT_SIZE   512
#define SIZE_ENTRY_FAT_32 4

/*chain of the fat 32*/ 

#define EOC_32             0x0FFFFFF8 
#define MASK_32  	   0x0FFFFFFF
#define BAD_CLUSTER_32 	   0x0FFFFFF7
#define FREE_32		   0x00000000	



/*******************STRUTTURE FAT BASE ****************************************************************/
/* 
 BPB usato per i filesystem FAT12 FAT 16 
*/ 


typedef struct _BPB_16_ {
  
  byte  BS_DrvNum;         // numero drive 
  byte  Reserved1;         // byte iservato per Windows NT 
  byte  BS_BootSig;        // se è uguale 0x29 (boot signature ) sono settati i valori successi
  dword BS_VolID;          // volume serial number 
  byte  BS_VolLab[11];     // Ettichetta registrata nella root directori
  byte  BS_FilSysType[8];  // tipo file system 

}__attribute__((packed))BPB_16;  


/* 
 BPB usato per i filesystem FAT12 FAT 16 
*/ 

typedef struct _BPB_32 { 

  dword  BPB_FATsz32;                //grandezza FAT for FAT32 in settori  
  word   BPB_ExtFlags;              // estesi flag vedi specifiche 
  word   BPB_FSVer;                 // versione di FAT32 viene attivata con i flag precedenti 
  dword  BPB_RootClus;              // imposta il numero (indirizzo) del primo cluster dellaroot directory ---- WARNIN----
  word   BPB_FSInfo;                // numero settore nella regione riservata che contiene la struttura FSINFO  
  word   BPB_BkBootSec;             // indica il settore di backup del boot sector 
  byte   BPB_Reserved[12];          // riservato per futuri uytilizzi
 
  byte  BS_DrvNum;                    // numero drive 
  byte  Reserved1;                    // byte iservato per Windows NT
  byte  BS_BootSig;                   // 0x29 boot signature 
  dword BS_VolID;                     // volume serial number 
  byte  BS_VolLab[11];                // Ettivchetta registrata nella root directori
  byte  BS_FilSysType[8];             // tipo file system 
}__attribute__((packed))BPB_32;



/*
 *BPB  sara un po piu complessa ( usa le unioni per le parti non comuni 
 */


typedef struct _BPB_ {

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
    BPB_16 bpb_16; // BPB 12/16 
    BPB_32 bpb_32; 
  }; 
 
  byte body[420]; 
  byte magic_number[2]; 
}__attribute__((packed))BPB; 
  

typedef struct _CLUSTER_LIST_  CLUSTER_LIST; 

// SOLO FAT 32

struct   _CLUSTER_LIST { 
  dword  cluster; 
  CLUSTER_LIST  *next; 
};

/******************* END STRUTTURE FAT BASE ****************************************************************/



/*********************************************MACRO**********************************************************/

/* Individuiamo la regione dei dati mediante l'indirizzo al primo settore 
   Calcoliamo il FirstDataSector=
   FirstDataSector = Reserved Region + FatRegion + Root Direcoty Region ( non esiste su fat32)
  
   1) Ci serve sapere la grandezza della regione riservata!! ??
   2) Grandezza regione fat  ??  
   3) Root Direcory ????

   1) E' un campo del BPB : DWORD BPB_RsvdSxecCnt (offset 14 ) grandezza espressa in settori 
   2) grandezza della regione fa, sarà uguale a FatSize * numero di fat 
   3) nel caso sia presente la root directory ( filesystem 16/12 ) dobbiamo sapere quando è grande 

   -----> calcolo grandezza root directory 
   la grandezza della root directory sara uguale a :  numero entry * sizeof(entry) [ 32 byte ] 


  Primo settore dei dati = BPB_RsvdSecCnt + (BPB_NumFatsz) + RootDirSectors  ( indirizzo riferito al primo settore del volume che contien e il BPB 

   ...

// COme faccioa ad individuare la FAT???

*/ 



/*ROOT_DIR_SECTOR : 
 *Calcola la grandezza della root directori nei file system FAT16 FAT 12 *
 *************************************************************************/

#define RootDirSector(n_ent_root, byts_for_sect)  (((n_ent_root*32)+(byts_for_sect-1))/byts_for_sect) 

/*FIRST_DATA_SECTOR
 *Calcola il primo settore della regione dati relativo all'inizio del volume 
 */

#define FirstDataSector(n_sect_reserved, n_FATs, FAT_size, root_dir_sector)  n_sect_reserved + (n_FATs * FAT_size) + root_dir_sector 

/* SIZE_DATA
 * Calcolo grandezza area  dati in settori 
 */

#define SizeData( n_cluster, setor_for_cluster) n_cluster*sector_for_cluster

/*FIRST_DATA_SECTOR
 *Primo settore di un cluster N puo esere realativo al disco o al volume in base al valore di first_data_sector 
 */
#define FirstDataSectorOfCluster(N, set_for_cluster, first_data_sector) (((N-2)*set_for_cluster)+first_data_sector)

/*FAT_SIZE *
 *Grandezza regione FAT in settori
 */ 

#define FATsize( FAT16_size, FAT32_size) (FAT16_size != 0) ? FAT16_size : FAT32_size 

/*FIRST_sect_FAT 
  Primo settre della fat 
*/ 
#define FirstSectFAT(n_sect_reserved) n_sect_reserved

#define LastSectFAT(First, size) First+size 

/*TOT_SECTOR
 *TOttale settori del disco 
 */ 

#define TotSector(TotSect16, TotSect32) (TotSect16 != 0) ? TotSect16 : TotSect32

/* Data SIZE 
 * Grandezza della regione dati in settori 
 */ 
#define DataSize( tot_sec, n_sect_reserved, n_FAT, FAT_size, root_dir_sector) tot_sec-( n_sect_reserved + (n_FAT * FAT_size) + root_dir_sector)


/* Data SIZE CLUSTER
 * Grandezza della regione dati in cluster 
 */ 
#define DataSizeCluster( DataSize, set_for_cluster) DataSize / set_for_cluster 

/* getSectRoot */ 
#define FirstRootDirSecNum(n_sect_reserved, n_FAT, FAT_size) n_sect_reserved + ( n_FAT*FAT_size)


/* Dato un cluster_number N, dove nella FAT è il record per quel cluster numero?  
 */ 

#define FATSectorNum( N, FirstSectFat, type, byte_for_sect)  FirstSectFat + ( N* ((type == FAT16) ? 2 : 4))/byte_for_sect

#define FATOffset( N, type, byte_for_sect)   ( N* ((type == FAT16) ? 2 : 4))%byte_for_sect

/*Macro specifiche per fat 32 con settori da 512 */

#define FATSectorNum_32(N, First) FATSectorNum(N,First, FAT32, 512) 
#define FATOffeset_32(N) FATOffset(N, FAT32, 512) 


/* PER LA MEMORIA*/ 
// Dato l'indirizzo della fat ritorna 

//#define getNext ( fat, cluster) 

/* ENTRY*/ 


/************************************************************END MACRO*************************************************************/



// Poiche il dirver lavora su  


/*
unsigned char FAT_table[cluster_size];
unsigned int fat_offset = active_cluster * 4;
unsigned int fat_sector = first_fat_sector + (fat_offset / cluster_size);
unsigned int ent_offset = fat_offset % cluster_size;
 
//at this point you need to read from sector "fat_sector" on the disk into "FAT_table".
 
//remember to ignore the high 4 bits.
unsigned int table_value = *(unsigned int*)&FAT_table[ent_offset] & 0x0FFFFFFF;
 
//the variable "table_value" now has the information you need about the next cluster in the chain.




typedef struct _ENTRY_FAT_ { 
  
  SHORT_ENTRY_FAT short_entry; 
  LONG_ENTRY_FAT * long_entry;  

}ENTRY_FAT; 




/*FUNZIONI************************************************************************************************************************/

/*Solo FAT32 */ 

/* FUNzione che determina precisamente che tipo di file sistem è 
   Seguo loa standard micro$oft
*/

byte getFileSystemType ( dword DataSizeCluster ); 

/* Funzione che analizza la FAT e riporta la lista di cluster di un FILE 
CLUSTER_LIST  get_chain_cluster ( dword first_cluster); 
*/ 

/* Funzione dato il cluster mi rendono il settore */  


// QUESTE SONO LE FUNZIONI CHE MI ASTRAGONO IL FILE DAI SETTORI E CLUSTE E LO MOSTRANO COME INSIEME DI BYTE 
dword  get_cluster( dword offset, CLUSTER_LIST ); 
dword  get_cluster_offset ( dword  offset, CLUSTER_LIST); 



/*
// funzione che rende la lista del entry di una directory a partire dalla lista dei cluster; 
list_entry_fat * get_dir_entry( list_cluster_dir * list); // lavora nella regione dati 
lista_cluster *  get_chain_cluster ( first_cluster); 
dword write_sect(dword number_of_sector , buff, size_guff); 
dword read_sect(dword number_of_sector, buff, size_buff); 



sector get_settore ( offset, cluster); 
sector get_settore_offset ( offset, cluster); 
////////////////////////////////////////////////////////////////////////////////////////////////////////////

lista_cluster get_free_cluster(size); 

//////////////////////////////////////////////////////////////////////////////////////////////////////



*/



BOOL init_fat();

dword getNext( dword_ptr fat , dword cluster);   // lavora su fat 

/***********************************************************************************/



/* GESTIONE FAT ********************************************************************
 * Interfaccia che mi permette di gestire la tabella FAT			   *
 * Funzioni elementari che lavorano sulla tabella FAT				   *
 ***********************************************************************************/


// aggiunge un cluster alla lista chain
// se chain è zero la crea 
BOOL append_fat (const byte volume, const dword chain, dword* addr); 
// elimina l'ultimo cluster dalla lista chain 
BOOL delete_fat (const byte volume, const dword chain); 
// elimina tutta una lista chain 
BOOL delete_all_fat (byte volume,dword chain); 

dword get_next_fat(const fat_ptr fat, const dword cluster); 
void test_fat (byte label);
BOOL create_fat (const byte volume, dword* chain); 
#define isLast_fat(N) ((N&MASK_32)==EOC_32)
/************************************************************************************/


/*GESTIONE CARTELLE A BASSO LIVELLO */ 

/*FUNZIONE 
 *Estrae dal disco la lista dei FCB contenuti in una cartella, individuata da
 * label ( per individuare il volume),first_cluster per individuare il primo 
 * Cluster della catena di cui la tabella è composta
 */ 
void  get_dir_entry(byte,  dword  first_cluster); // lavora nella regione dati 

/*Funzione che crea un entry nella directory specificata da label cluster 
  con il nome passato come argomento, rende il fcb del nuova entry
 */ 
FCB * create_dir_entry(byte label, dword first_cluster, const char * name_entry);

/* FUNZIONE CHE elimina un entry */

BOOL delete_dir_entry(FCB* file); 



void print_list_entry_dir ( void); 
/***********************************************************************************/






#endif 
