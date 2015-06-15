
/*HEADER
 *Interfaccia per la gestione dell'MBR per poter gestire i volumi presenti su un disco
 */ 

#ifndef MBR_MY
#define MBR_MY

#include "type.h" 


/*[DEFINE]-----------------------------------------------------------*/
#define EMPTY 0x00
#define FAT12 0x01 
#define FAT16 0x06
#define FAT32 0x0B
#define EXTEND 0x05

#define MBP_SIZE 446 
#define MBT_SIZE 64
#define MBR_SIZE 512 

/*-----------------------------------------------------------------*/


/*[STRUTTURE DATI]-------------------------------------------------*/



/* Layout dei recodr della BMT 
  +--------------------------------------------------------+
  |OFFSET| SIZE | DESCRIPTION                              |
  +--------------------------------------------------------+
  |0x00  |  1   | FLAG boot 0x00 (non attivo ) 0x80 attivo |
  +--------------------------------------------------------+
  |0x01  |  3   | CHS ADDRESS FIRST                        |
  +--------------------------------------------------------+
  |0x04  |  1   | TYPE                                     |
  +--------------------------------------------------------+
  |0x05  |  3   | CHS ADDRESS LAST                         |
  +--------------------------------------------------------+
  |0x08  |  4   | LBA                                      | 
  +--------------------------------------------------------+
  |0x0C  |  4   | NUMBER SECTOR                            | 
  +--------------------------------------------------------+

 Formally, status values other than 0x00 and 0x80 are undefined.

 Starting Sector fields are limited to 1024 cylinders, 255 heads,
 and 63 sectors;[citation needed]. Ending Sector fields have the 
 same limitations.

 The range for sector is 1 through 63; the range for cylinder is 
 0 through 1023; the range for head is 0through 254 inclusive.

 The 10-bit cylinder value is split into two bytes. The value
 can be reassembled using this formula: 
  cylinder = ((byte[2] & 0xc0) << 2) | byte[3]
*/
 
typedef struct _RECORD_MBT_ { 
 
  byte boot_flag;
  byte chs_first_sector[3];
  byte type;
  byte chs_last_sector[3]; 
  dword lba; 
  dword number_sectors; //Number of sectors in partition

}RECORD_MBT; 

/*MBT */
typedef struct _MBT_ { 
        RECORD_MBT record_mbt[4]; 
}MBT; 

/*MBP*/
typedef struct _MBP_ { 
        byte body[446]; 
}MBP; 

/*Struttura del Master Boot Record , è presente nel primo settore del disco 
 * , ha una grandezza di 512 byte ( grandezza settore ).
 * E' composto da due parti: 
 *      - MBP : codice che viene messo in esecuzione dal BIOS ( generlmente
 *              carica il bootmanager. SIZE 446 byte
 *      - MBT : tabella delle partizioni , 4 record da 16 byte.
 * 
         +------------------------------------------------------+
         | HEX | DEC |       CONT                 |       SIZE  |
         +------------------------------------------------------+
         |0000 |0    | code area                  |440(max. 446)|
         +------------------------------------------------------+
         |01B8 |440  |disk signature (optional)   |4            |
         +------------------------------------------------------+
         |01BC |444  |Usually nulls;0x0000        | 2           |
         +------------------------------------------------------+
         |01BE |446  |Table of primary partitions |64           |
         -------------------------------------------------------+
         |01FE |510  |  0xAA55  MBR signature;    | 2           |
         +------------------------------------------------------+

*/

//#pragma pack(1) 	// alineo la struttura al byte altrimenti non funziona 


typedef struct _VOLUME_LIST_ VOLUME_LIST; 
typedef RECORD_MBT RECORD_VOLUME; 

// Struttura per la gestione dei volumi 
 
struct _VOLUME_LIST_ { 
  RECORD_VOLUME record_volume; 
  VOLUME_LIST * next;  
};  

// struttura di un indirizzo CHS

typedef struct _CHS_ { 
  byte head; 
  word cylinder;
  byte sector;  

}CHS; 

typedef struct _MBR_ { 
        MBP mbp;    
        MBT mbt ; 
        byte magic_number[2]; 
}__attribute__((packed)) MBR; 

/*INTERFACIA*********************************************************/


// vrea la lista dei volumi sul disco 
//( Analizza anche le partizioni estese e ci porta una lista trasparente) 
VOLUME_LIST * get_list_volume ( int); 
//STAMPA la lista dei volumi 
void stampa_list_volume(VOLUME_LIST *); 
// elimina la lista dei volumi 
BOOL delete_list_volume (VOLUME_LIST **);
// ritorna il primo settore CHS del volume
CHS get_first_sector ( RECORD_VOLUME * record);
// ritorna l'ultimo settore CHS del volume 
CHS get_last_sector ( RECORD_VOLUME * record);
// verifica che ci sia un magic number
BOOL check_boot_sector ( MBR  * j); 
/********************************************************************/

#endif 
