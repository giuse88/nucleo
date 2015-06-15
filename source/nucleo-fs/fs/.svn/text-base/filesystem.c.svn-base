

/**************************************************************FILE SYSTEM ***********************************/

#include "fat.h"
#include "fs.h" 
#include "type.h"
#include "sistema.h" 
#include "volumi.h"

#include "data.h"
#include "direntry.h" 
#include "fs.h" 

BOOL fs_init(); 


/*STRUTTURE DATI GBOLBALI */
TABELLA_VOLUMI * tabella=NULL; 
FCB * root=NULL; 



void test_fat( byte label); 

/* Pubblica FUNZIONE 
 * Questa funzione ha lo scopo di inizializzare il FS mediante delle chiamate 
 * hai vari moduli di cui è composto il fs. Se attiva la macro DEBUG_FS esegue
 * dei test sui vari oggetti che lo compongono.
 * Può fallire quando fallisce la chiamata di un sotto modulo. 
 */ 
BOOL  fs_init(void){ 

  
  flog(LOG_INFO, "Inizialiazzazione FILE SYSTEM "); 

  // chiamata per la creazione della tabella dei volumi 
  if(!(tabella=crea_tabella_volumi())) {
      perror("Creazione tabella volumi fallita"); 
      return FALSE; 
  }

#ifdef DEBUG_FS 
  stampa_tabella_volumi('C');
#endif 
  
  // carico fat in memoria 
  if(!( init_fat())){
      perror("Caricamento FAT fallito"); 
      return FALSE; 
  }
  
#ifdef DEBUG_FS 
  test_fat('C'); 
#endif
  
  //stampa_tabella_volumi(tabella);   //DEBUG
  //flog(LOG_INFO, " aaa : %x", getNext(tabella->fat, 2)); 
  //get_dir_entry('C', 2); 
  //create_dir_entry('C', 2, "a.txtllll"); 
 
  //test_read('C'); 

  
  test_dir('C');
  
  
  flog(LOG_INFO,"Fine inizializzazione FILE SYSTEM"); 


  return TRUE; 
}






