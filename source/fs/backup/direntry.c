#include "direntry.h" 
#include "string.h" 
#include "errno.h"
#include "data.h"
#include "fat.h" 
#include "sistema.h"


/*FUNZIONI DI INTERFACIA*/ 

// crea un entry sulla cartella father e salva sul contenuto su new che viene creato
BOOL create_entry(const char * name , byte type,  const FCB * father, FCB ** new);  
// crea new e carica i valori della direntry cercata
BOOL open_entry(const char * name , const FCB * father, FCB ** new);  
//elimina direnetry
BOOL delete_entry( const FCB *);  







/*PRIVATE*/
BOOL create_short_entry( const char * name, SHORT_ENTRY * entry); 
BOOL create_long_entry(const char * name, LONG_ENTRY *entry);
BOOL to_msdos_time ( dword sys_time, msdos_time *time );
BOOL to_msdos_date ( dword sys_date, msdos_date *date );
BOOL search_entry( dword chain, const  char * name); 
int search_entry_short ( byte label , dword chain , const char * name );
int search_entry_long  ( byte label , dword chain , const wchar * name );
int create_shortname(byte label, dword dir, const char* uname, int ulen, char* name_res);


#define toUpperChar(c) (c > 96 && c < 123 )? (c-32):c 
// macro che data la lunghezza del nome riporta il numero di entrate 
// Il più uno è presente perché serve sempre un entrata per le short direcotory 
#define GetNumEntry(N) ((N >255) ? 0 : ((((N)<=13)?1:(N)/13)+1))


BOOL format_short_entry( const char * name, SHORT_ENTRY * entry, byte type);

BOOL format_long_entry(const char* name, LONG_ENTRY* entry, byte n, byte chkSum);

/*void* wmemset(void * dst, word s, size_t count) {
  
    register wchar * a = dst;
    count++;	/* this actually creates smaller code than using count-- *
    while (--count)
	*a++ = s;
    return dst;
}

*/



inline word get_n_entry(const char * str) { 
 
 size_t size=strlen(str); 
  word n=0; 
  
  n=size/SIZE_NAME_PART; 
  if(size%SIZE_NAME_PART) 
    n++;
  n++; // c'è sempre una short entry 
  
  return n; 
}
    
dword get_time() { 
 // devo fare l'inserimento dinamico ma mi server sprintf
	__asm__(" int $0x99 " ); 
	
}
dword get_date(){
  
  // devo fare l'inserimento dinamico ma mi server sprintf
  __asm__("int $0x98 " ); 
  
}



/*
unsigned char toUpperChar ( unsigned wchar c ) {
    if ( c >96  && c < 123 ) {
        return ( c - 32 );
    }
    return c;
}
*/

/*FUNZIONE  che formata un buffer pronto per la scruittura sul disco.
 *Alloca il buffer in memoria dinamica  questo buffer è lo riporta mediante buf*/

BOOL  format_entry (const char *long_name, const char * short_name, byte type, byte ** buf ) { 
 
  word n_entry=0; 
  LONG_ENTRY *l=NULL; 
  word i=0,n=0; 
  char *tmp=NULL;  
  
  n_entry=get_n_entry(long_name); // prelevo il numero di entrate
  *buf=mem_alloc(n_entry*SIZE_ENTRY, 1); // alloco lo spazio per le strutture
  l=(LONG_ENTRY*)*buf; 
  
  n_entry--;//elimino l'entry short  
  i=n_entry;
  n=strlen(long_name); 
  tmp=(long_name+n); // ultimo
  tmp=tmp-(n%SIZE_NAME_PART); // punto all'elemento da copiare
  
  flog(LOG_WARN, "n%d%d", i, n_entry); 
 // format_long_entry(tmp, l, n_entry|0x40, TRUE);   
  
  
  for (;n_entry>0; n_entry--, l++, tmp-=SIZE_NAME_PART) 
    format_long_entry(tmp,l, (i==n_entry)? (n_entry|MASK_LAST_NAME):n_entry, 0x78); 
  
  format_short_entry(short_name, (SHORT_ENTRY*)l, type); 
  
  
  return TRUE; 
  
}




BOOL format_short_entry( const char * name, SHORT_ENTRY * entry, byte type){
  
  entry->DIR_Attr=type; 
  entry->DIR_FileSize=0; 
  entry->DIR_FstClusHI=0;
  entry->DIR_FstClusLO=0;
  entry->DIR_NTRes=0; 
  entry->DIR_CrtTimeTenth=0; // ignoro millisecondi 
  
  to_msdos_date(get_date(), (msdos_date*) &entry->DIR_CrtDate); 
  to_msdos_time(get_time(), (msdos_time*) &entry->DIR_CrtTime);
  to_msdos_date(get_date(), (msdos_date*) &entry->DIR_WrtDate); 
  to_msdos_time(get_time(), (msdos_time*) &entry->DIR_WrtTime);
  
  strncpy(entry->DIR_Name, name, MSDOS_NAME); 
  
  return TRUE; 
}

BOOL format_long_entry(const char * name, LONG_ENTRY *entry, byte n, byte chkSum) {
  
 
  const char *name1=name; 
  const char *name2=name+SIZE_UNO; 
  const char *name3=name2+SIZE_DUE;
  
  BOOL salta=FALSE; 
  
  flog(LOG_WARN, "Name %s %d %x",name, n, chkSum); 
  
   if (n&0x40) { 
    
     dword size=strlen(name); 
    
     wmemset(entry->LDIR_Name1, 0xFFFF, SIZE_UNO); 
     wmemset(entry->LDIR_Name2, 0xFFFF, SIZE_DUE); 
     wmemset(entry->LDIR_Name3, 0xFFFF, SIZE_TRE);
     
     if ( size <= SIZE_UNO){
	char_to_str_uni(entry->LDIR_Name1, name1, size);
	entry->LDIR_Name1[size]=0x0000; 
	salta=TRUE; 
     }
    
    size-=SIZE_UNO; 
    
    if ( size <= SIZE_DUE & !salta ){
        char_to_str_uni(entry->LDIR_Name1, name1, SIZE_UNO);
	char_to_str_uni(entry->LDIR_Name2, name2, size);
	entry->LDIR_Name2[size]=0x0000; 
	salta=TRUE; 
    }
  
    size-=SIZE_DUE; 
    
    if ( size < SIZE_TRE &!salta){
	char_to_str_uni(entry->LDIR_Name1, name1, SIZE_UNO);
	char_to_str_uni(entry->LDIR_Name2, name2, SIZE_DUE);
	char_to_str_uni(entry->LDIR_Name3, name3, size);
	entry->LDIR_Name3[size]=0x0000; 
	return TRUE; 
    }
     
    
    if ( size = SIZE_TRE & !salta){
	char_to_str_uni(entry->LDIR_Name1, name1, SIZE_UNO);
	char_to_str_uni(entry->LDIR_Name2, name2, SIZE_DUE);
	char_to_str_uni(entry->LDIR_Name3, name3, SIZE_TRE); 
     }
    
   } else {
   
   char_to_str_uni(entry->LDIR_Name1, name1, SIZE_UNO);
   char_to_str_uni(entry->LDIR_Name2, name2, SIZE_DUE); 
   char_to_str_uni(entry->LDIR_Name3, name3, SIZE_TRE);

     
  }
  
  
   entry->LDIR_Attr=ATTR_LONG_NAME; 
   entry->LDIR_FstClusLO=0; 
   entry->LDIR_Chksum=chkSum; 
   entry->LDIR_Type=0; 
   entry->LDIR_Ord=n; 

   return TRUE;  
  
}


/*






// Funzione che dato l'indirizzo di un cluster appartenente ad una directory riporta la lista
// dei fcb contenuti in essa

void   get_dir_entry ( byte volume,  dword  first_cluster ) {

    // prelevare il cluster ( tabella volumi)
    TABELLA_VOLUMI * tab=get_volume ( volume );
    dword  size_cluster = ( tab->fat_info.sectors_for_cluster ) * ( tab->fat_info.byts_for_sector );
    void * cluster = ( void* ) mem_alloc ( size_cluster,1 );
    SHORT_ENTRY_FAT * entry=NULL;
    dword first_sector=0;
    FCB * tmp=NULL;
    int i=0;


    if ( first_cluster < 2 )
        flog ( LOG_WARN, "ERRORE" );
    if ( !tab )
        flog ( LOG_WARN," ERRORE" );


    flog ( LOG_WARN, " INserisco in memmoria %d %c", size_cluster, tab->label );

    // prima di leggere devo trasformare il cluster in primo settore
    first_sector=FirstDataSectorOfCluster ( first_cluster,
                                            tab->fat_info.sectors_for_cluster,
                                            tab->fat_info.first_set_data );

    //  flog(LOG_WARN, "Settore %d", tab->fat_info.first_set_data);
    //flog(LOG_WARN , "Cluster Root : %d", tab->fat_info.first_cluster_directory);

    read_part_n ( tab->ata, tab->disco, tab->indice_partizione, first_sector,
                  tab->fat_info.sectors_for_cluster, ( void* ) cluster );


    /***********************************************************************************************

    entry = ( ( SHORT_ENTRY_FAT* ) cluster );

    flog ( LOG_WARN, "Grandezza short %d, long %d", sizeof ( SHORT_ENTRY_FAT ), sizeof ( LONG_ENTRY_FAT ) );

    i=0;

    while ( TRUE ) {

        // LEBERA
        if ( entry->DIR_Name[0]==ALL_FREE )
            break;

        if ( entry->DIR_Name[0]==FREE ) {
            entry++;
            continue;
        }

        // verifichiamo che tipo di entrata è
        //    flog(LOG_WARN, "%d %x", i, entry );

        if ( entry->DIR_Attr == ATTR_LONG_NAME ) {
            tmp= add_long_dir ( ( ( LONG_ENTRY_FAT* * ) &entry ) );
            tmp->cluster_dir=first_cluster;
            tmp->offset=0;
        }//----------------????????????????????????????????????????????????????????????????
        else  {
            add_short_dir ( &entry );
        }

        print_fcb ( tmp );
        i++;
    }


    mem_free ( cluster );

}






#define SIZE_NAME 260

FCB *  add_long_dir ( LONG_ENTRY_FAT  ** dir ) {

    FCB * fcb= NULL;
    char name[SIZE_NAME];
    int n_entry=0;
    int i =0;
    if ( dir==NULL )
        return NULL;


    memset ( name, 0, SIZE_NAME );


    //flog(LOG_WARN , "ORD: %x", (*dir)->LDIR_Ord);

    if ( ( *dir )->LDIR_Ord > 0x40 )
        n_entry= ( ( *dir )->LDIR_Ord-0x40 );
    else
        return NULL; // errore


    for ( i=0; i < n_entry; i++, ( *dir ) ++ )  {

        if ( ( *dir )->LDIR_Name3[0]!=0xFFFF )
            //	flog(LOG_WARN, "3 %x", (*dir)->LDIR_Name3[0]);
            insert_part_name ( ( *dir )->LDIR_Name3, name, 2 );

        //flog(LOG_WARN, "2 %x", (*dir)->LDIR_Name2[0]);
        if ( ( *dir )->LDIR_Name2[0]!=0xFFFF )
            insert_part_name ( ( *dir )->LDIR_Name2, name, 6 );

        //  flog(LOG_WARN, "1 %x", (*dir)->LDIR_Name1[0]);
        if ( ( *dir )->LDIR_Name1[0]!=0xFFFF )
            insert_part_name ( ( *dir )->LDIR_Name1, name, 5 );
    }


    fcb= ( FCB* ) add_short_dir ( ( SHORT_ENTRY_FAT** ) dir );
    memset ( fcb->name, 0, SIZE_NAME );
//   strncpy(fcb->name, name, SIZE_NAME);

    return fcb;

}

FCB *  add_short_dir ( SHORT_ENTRY_FAT  * * dir ) {

    FCB * fcb= NULL;


    if ( dir==NULL )
        return NULL;

    fcb= ( FCB* ) mem_alloc ( sizeof ( fcb ), 1 );
    memset ( fcb, 0, sizeof ( fcb ) );

    fcb->mode= ( *dir )->DIR_Attr;
    strncpy ( fcb->name, ( *dir )->DIR_Name, 11 );

    // fcb->first_cluster=(dword)(dir->DIR_FstClusLO);

    fcb->first_cluster= ( ( ( dword ) ( ( *dir )->DIR_FstClusHI ) << 16 ) | ( dword ) ( ( *dir )->DIR_FstClusLO ) );   // primo cluster of chain individua il file

    // dobbiamo inserire i dati relativi all'ora e la data

    // se sono qui sono già sicuro che si tratta di un cartella piccola

    ( *dir ) ++;

    return fcb;

}

/* LIBRERIA *
//cpy
wchar * wcscpy(wchar * dest, const wchar * src) {
  wchar* orig=dest;
  for (; (*dest=*src); ++src,++dest) ;
  return orig;
}
//cmp
int wcscmp(const wchar* a,const wchar* b) {
  while (*a && *a == *b)
    a++, b++;
  return (*a - *b);
}
//strlen
size_t wcslen(const wchar* s) {
  size_t i;
  for (i=0; s[i]; ++i) ;
  return i;
}

wchar* wcsncat(wchar * dest, const wchar * src, size_t n) {
  wchar* orig=dest;
  size_t i;
  while (*dest) ++dest;
  for (i=0; i<n && src[i]; ++i) dest[i]=src[i];
  dest[i]=0;
  return orig;
}

wchar* wcsncpy(wchar *dest, const wchar *src,size_t n) {
  wchar* orig=dest;
  for (; dest<orig+n && (*dest=*src); ++src,++dest) ;
  for (; dest<orig+n; ++dest) *dest=0;
  return orig;
}

wchar* wcsrchr(const wchar *wcs, wchar wc) {
  wchar* last=0;
  for (; *wcs; ++wcs)
    if (*wcs == wc)
      last=(wchar*)wcs;
  return last;
}
wchar *wcsstr(const wchar *haystack, const wchar *needle) {
  size_t i,j;
  for (i=0; haystack[i]; ++i) {
    for (j=0; haystack[i+j]==needle[j]; ++j) ;
    if (!needle[j]) return (wchar*)haystack+i;
  }
  return 0;
}


int strncmp(const char *s1, const char *s2, size_t n) {
  register const unsigned char* a=(const unsigned char*)s1;
  register const unsigned char* b=(const unsigned char*)s2;
  register const unsigned char* fini=a+n;
  while (a<fini) {
    register int res=*a-*b;
    if (res) return res;
    if (!*a) return 0;
    ++a; ++b;
  }
  return 0;
}


wchar  upper  (const wchar c ) {

  if ( c >96  && c < 123 ) 
   {
        return ( c - 32 );
    }
    return c;
  
}

*/

void print_w (const wchar *name) {
  
 size_t size=wcslen(name); 
  byte * buf=mem_alloc(size+1,1); 
  memset(buf,0, size+1); 
  uni_to_str_char(name, buf, size);
  flog( LOG_WARN, "NAME %s Lung %d", buf , size); 
  
  mem_free(buf); 
}




/*
 *Il Nome del file Lo gestiamo come una pila , inseriamo sempre in testa
 * questo perchè la lista la scorriamo dall'ultimo elemento :
 * Questa funzione trasforma da unicode as asci (7 bit) il nome salvato
 * nel disco
 * NAME :  parte del nome del file UNICODE
 * BUF     . Buffer del FCB nel qual inseriamo il nome
 * non faccio nessun controllo sullo spazio  
*/


void append_name (const  LONG_ENTRY * entry, wchar * n ) {

  int i=0, j=0, s=0;  
  wchar name[SIZE_NAME_PART]; 
  wchar buf[SIZE_NAME]; 
  BOOL end=FALSE; 
  
  memset(name, 0, SIZE_NAME_PART*2); 
  
  // name 1 ; 
  for (i=0; i< SIZE_UNO; i++,j++) 
      if(entry->LDIR_Name1[i]!= PADDING)
	  name[j]=entry->LDIR_Name1[i]; 
      else {
	end=TRUE; 
	break; 
      }
      
  // name 2 
	
  for (i=0; i< SIZE_DUE && !end; i++,j++) 
      if(entry->LDIR_Name2[i]!= PADDING)
	  name[j]=entry->LDIR_Name2[i]; 
       else {
	end=TRUE; 
	break; 
      }
      
  // name 3 
    for (i=0; i< SIZE_TRE & !end; i++,j++) 
      if(entry->LDIR_Name3[i]!= PADDING)
	  name[j]=entry->LDIR_Name3[i]; 
      else 
	break; 
      
   // ora su j ho quanto è lungo lo spazio 
  // traslo di j
  
  s=wcslen(n);

  if(!(entry->LDIR_Attr & MASK_LAST_NAME)) {
    memset(buf, 0, SIZE_NAME*2); 
    wcsncpy(buf,n, s);	//salvo il nome 
    memset(n, 0,(s+1)*2); 	// reset name 
    wcsncpy(n+j,buf,s); 
  }
  
  wcsncpy(n,name,j); 
  
  //flog(LOG_WARN, "Lunghezza %d", wcslen(n));
  
  return;
}




/* FUNZIONE *******************************************************************
 * Funzione che crea un entry nella tabella specificata dal cluster           *
 * Deve essere gestita in mutua esclusione su questo volume                   *
 ******************************************************************************

c

FCB * create_dir_entry ( byte label, dword first_cluster, const char*name ) {

    // dobbiamo verificare di quanti cluster è composta la mia cartella

    TABELLA_VOLUMI * tab=get_volume ( label );
    dword  size_cluster =0;
    void* cluster = NULL;
    dword first_sector=0, indice_free=0,set_for_cluster;
    dword_ptr fat = NULL;
    int i=0;
    dword n_entry_free=0,temp=0;
    FCB *fcb=NULL;
    SHORT_ENTRY_FAT short_entry;
    char  short_name[12];

    flog ( LOG_WARN , "FUNZIONE CREAZIONE DIR_ENTRY" );
    flog ( LOG_WARN , "Creazione nella directory %d, volume %c , nome %s", first_cluster, label, name );

    if ( first_cluster < 2 ) { 	// manca controllo superiore
        set_errno ( "ERRORE CLuster" );
        return NULL;
    }

    // verificare la consistenza del cluster passato

    if ( !tab ) {
        set_errno ( "Volume non trovato" );
        return NULL;
    }

    // verifico se first_cluster appartiene ad una cartella
    // come ?
    // dovrei leggere l'entry attinente a questa cartella nella cartella madre

    // quante entry mi servono ?

    // Il numero di entratte cotigue che mi serve ( il piu uno è dovuto al
    // carattere di fine stringa
    n_entry_free=GetNumEntry ( strlen ( name ) +1 );
    flog ( LOG_DEBUG, "Numero entrate %d", n_entry_free );

    if ( n_entry_free <2 ) {
        set_errno ( "Nome non corretto" );
        return NULL;
    }


    size_cluster = ( tab->fat_info.sectors_for_cluster ) * ( tab->fat_info.byts_for_sector );
    cluster = ( void* ) mem_alloc ( size_cluster,1 );
    set_for_cluster=tab->fat_info.sectors_for_cluster;
    fat=tab->fat;


//	while(TRUE) {

    //flog(LOG_WARN, " INserisco in memmoria %d %c", size_cluster, tab->label);

    // prima di leggere devo trasformare il cluster in primo settore
    first_sector=FirstDataSectorOfCluster ( first_cluster,
                                            tab->fat_info.sectors_for_cluster,
                                            tab->fat_info.first_set_data );

    flog ( LOG_DEBUG, "Devo leggere il settore : %d", first_sector );

    // carico in memoria il primo cluster della cartella
    read_part_n ( tab->ata, tab->disco, tab->indice_partizione, first_sector,
                  tab->fat_info.sectors_for_cluster, ( void* ) cluster );

    if ( ! ( indice_free=get_free_entry ( n_entry_free,cluster,set_for_cluster*SECT_SIZE ) ) )
        perror ( "ERRORE get_free_entry" );


    flog ( LOG_DEBUG, "Indice settore %d", indice_free );

    /*if ( indice_free=get_free_entry(n_entry_free,cluster,set_for_cluster*SECT_SIZE ) != 0) {}
    		//break;
    else {
    		// significa che qst cluster è pieno o non ha entry contigue
    		// sufficienti a contenere il nome che mi serve

    	if((temp=getNext(fat, first_cluster)) != 0) {
    		//significa che non è l'ultimo, quindi aggiorno il
    		//cluster è lo facciio puntare al prossimo
    		first_cluster=temp;   // uso temp come variabile di appoggio
    		//continue;
    	} else
    		//significa che no ci sono piu cluster, allora dobbiamo aggiungerlo
    		first_cluster=get_free_cluster(label, first_cluster);
    //}
    		*

//}


    print_cluster ( cluster, 10 );
    memset ( &short_entry, 0, sizeof ( short_entry ) );
    //flog(LOG_WARN, "name :%s",  createShortNameMask(name));
    createShortName ( name, ( char* ) short_name );
    create_short_entry ( &short_entry, name );

    //fcb=entry_on_cluster(indice_free, cluster, &short_entry);
//	print_cluster(cluster, 10);
//	memset(cluster, 0, 4096);

//	write_part_n(tab->ata, tab->disco, tab->indice_partizione, first_sector,
//		                tab->fat_info.sectors_for_cluster, (void*)cluster);

    mem_free ( cluster );

}


FCB * entry_on_cluster ( dword indice_free, dword_ptr cluster, const SHORT_ENTRY_FAT * short_entry ) {

    FCB *fcb=mem_alloc ( sizeof ( FCB ), 1 );
    int i=0;

    /*memset(&short_entry,0,sizeof(SHORT_ENTRY_FAT));
    memset(&long_entry,0,sizeof(LONG_ENTRY_FAT));
    memset(fcb,0,sizeof(FCB));
    */

    /*	for(i=0; i< indice_free*4 +10; i++)
    		flog(LOG_DEBUG, "%x", *cluster++);
    */


    /*long_entry.LDIR_Name1[0]=(word)'a';
    long_entry.LDIR_Name1[1]=(word)'.';
    long_entry.LDIR_Name1[2]=(word)'t';
    long_entry.LDIR_Name1[3]=(word)'x';
    long_entry.LDIR_Name1[4]=(word)'t';

    long_entry.LDIR_Chksum=0x56;
    long_entry.LDIR_Ord=0x41;
    /
    memcpy ( cluster+indice_free, ( void* ) &short_entry, 32 );
    //memcpy(++cluster+indice_free,(void*)&short_entry, 32);

    return NULL;
}


/** FUNZIONE ******************************************************************
 * Cerca n entrate libere, assumo che quando è presente FREE_ALL , tutti i    *
 * cluster successivi sono liberi, e non esistono posizioni adiacenti a questa*
 * con l'eticheta free ( il software che si preocupa di eliminare le entry 	  *
 * si incarica di fondere le varie zone. 									  *
 *     N 	   : numero di entry da cercare									  *
 *     CLUSTER : puntatore alla fat 										  *
 *	   SIZE    : grandezza di un cluster 									  *
 * Ritorna offset								  *
 *****************************************************************************/


lword  get_free_entry ( byte label, dword chain, word n) {


    SHORT_ENTRY *dir;
    byte *buf=mem_alloc(SIZE_BUF,1);
    BOOL end=FALSE; 
    dword n_read=0,i=0, dir_free=0; 
    lword offset=0, offset_backup=0; 
    dword max=SIZE_BUF/SIZE_ENTRY; 

  while((n_read=read_data(label, chain,offset,buf, SIZE_BUF))&&!end) {
    
    dword number_dir_entry=n_read/SIZE_ENTRY;
    dir=(SHORT_ENTRY*)buf;
    
    for ( i=0; i<=number_dir_entry; i++, dir++) {

        if ( dir->DIR_Name[0]== FREE || dir->DIR_Name[0] == ALL_FREE )
            dir_free++;
	else 
	  dir_free=0; 
	
        if ( dir_free==n ){
	    break; 
	    end=TRUE; 
      }
    }
    
    if (dir_free > i) { 
	 
	return offset_backup + ( max - dir_free- ++i)*SIZE_ENTRY ; 
    } else 
    { 
      return offset+(++i-dir_free)*SIZE_ENTRY; 
    }
      
  }
}

/*FUNZIONE*******************************************************
 * Trova e alloca un cluster libero, appendendolo nella catena  *
 * individuata da ultimo_cluster. 							    *
 * Aggiorna la fat in memoria e nel disco, questi sono dati     *
 * sensibili e una non loro corretta gestione provocherebbe	    *
 * perdite di dati, perciò vengono preteti da semafori di mutua *
 * esclusione, presenti per ogni volume.		                *
 *		LABEL : Label del volume nel quale vogliamo inserire    *
 *				l'entry 										*
 *	    ULTIMO CLUSTER :della lista, (deve contenere EOC)		*
 * In caso di successo riporta il numero del cluster allocato   *
 * altrimenti rende 0 e setta la variabile ERRNO;				*
 ****************************************************************

dword get_free_cluster ( byte label, dword ultimo ) {


    TABELLA_VOLUMI * tab=get_volume ( label );
    dword free=0;
    dword_ptr fat_ptr=tab->fat;
    dword size_fat=tab->fat_info.size_fat;
    dword first_sect_fat =tab->fat_info.first_set_fat;
    int i=0;
    dword settore[SECT_SIZE/4];	// buffer di appogio
    dword n_set_ultimo=0, n_set_free=0; 		// numero settore da prelevare
    dword offset_ultimo=0, offset_free=0;		//Offset dei file


    // verifico se effetivamente è l'ultimo di una catena
    if ( * ( fat_ptr+ultimo ) != EOC_32 ) {
        set_errno ( "Cluster Errato" );
        return 0;
    }

    sem_wait ( tab->semaphore );

    /**Gestione FAT in memoria  *****************************

    // trovo un entry libera
    for ( i=2;i<size_fat; i++ )
        if ( * ( fat_ptr+i ) == 0x00000000 ) { // entry vuota
            free=i;
            break;
        }

    // verifico che non sia terminato lo spazio
    if ( i== size_fat ) {
        set_errno ( "Spazio Insuficiente" );
        return;
    }

    //aggiorno la fat in memoria
    * ( fat_ptr+ultimo ) =free;
    * ( fat_ptr+free ) =EOC_32;

    /*** Fine FAT memoria   **********************************


    /*** GESTIONE FAT DISCO **********************************

    //aggiorno cluster ultimo
    n_set_ultimo=FATSectorNum_32 ( ultimo, first_sect_fat );
    n_set_free=FATSectorNum_32 ( free, first_sect_fat );

    read_part_n ( tab->ata, tab->disco, tab->indice_partizione, n_set_ultimo, 1, ( void * ) settore );
    offset_ultimo=FATOffeset_32 ( ultimo );
    settore[offset_ultimo]=free;

    // se stanno su due settori diversi devo fare due letture
    if ( n_set_ultimo != n_set_free ) {
        // aggiorno ultimo
        write_part_n ( tab->ata, tab->disco, tab->indice_partizione, n_set_ultimo, 1, ( void * ) settore );

        //leggo il settore di free
        read_part_n ( tab->ata, tab->disco, tab->indice_partizione, n_set_free, 1, ( void * ) settore );
        offset_free=FATOffeset_32 ( free );
        settore[offset_free]=EOC_32;
        write_part_n ( tab->ata, tab->disco, tab->indice_partizione, n_set_free, 1, ( void * ) settore );

    }	else {
        // stanno sullo stesso settorecluster
        settore[offset_free]=EOC_32;
        write_part_n ( tab->ata, tab->disco, tab->indice_partizione, n_set_ultimo, 1, ( void * ) settore );
    }

    /****FINE**************************************************


    sem_signal ( tab->semaphore );

    return free;	// return cluster
}

byte CHkSum ( char * name ) {
    short name_len;
    byte sum=0;

    for ( name_len=11; name_len !=0; name_len-- )
        sum= ( ( sum&1 ) ?0x80:0 ) + ( sum>>1 ) +*name++;

    return sum;

}

void print_cluster ( dword_ptr cluster , int n ) {

    int i=0;
    SHORT_ENTRY_FAT * dir= ( SHORT_ENTRY_FAT* ) cluster;

    for ( i = 0; i<n; i++,dir++ )
        flog ( LOG_DEBUG, "%x", * ( char * ) dir );

}

// crea un entry corta
BOOL create_short_entry ( SHORT_ENTRY_FAT * short_entry, const char *name ) {

    if ( !short_entry ) {
        set_errno ( "Entry errata" );
        return FALSE;
    }

    if ( !name ) {
        set_errno ( "Nome non corretto" );
        return FALSE;
    }

    // test sul nome quali caratteri sono amessi e quali no
    strncpy ( short_entry->DIR_Name,name, 11 );
    short_entry->DIR_Attr=ATTR_ARCHIVE;      

}

BOOL create_long_entry ( LONG_ENTRY_FAT * short_entry, const char *name ) {

}

/*
 Create short name by squeezing long name into the 8.3 name boundaries
 lname - existing long name
 sname - buffer where to store short name

 returns: 0  if longname completely fits into the 8.3 boundaries
          1  if long name have to be truncated (ie. INFORM~1.TXT)
	 <0  if invalid long name detected
*
int createShortNameMask ( unsigned char* lname, unsigned char* sname ) {
    int i;
    int size;
    int j;
    int fit;

    if ( lname[0] == '.' ) {
        return -1;
    }

    fit = 0;
    //clean short name by putting space
    for ( i = 0; i < 11; i++ )  sname[i] = 32;
    //XPRINTF("Clear short name ='%s'\n", sname);

    //detect number of dots and space characters in the long name
    j = 0;
    for ( i = 0; lname[i] != 0; i++ ) {
        if ( lname[i] == '.' ) j++; else
            if ( lname[i] == 32 ) j+=2;
    }
    //long name contains no dot or one dot and no space char
    if ( j <= 1 ) fit++;
    //XPRINTF("fit1=%d j=%d\n", fit, j);

    //store name
    for ( i = 0; lname[i] !=0 && lname[i] != '.' && i < 8; i++ ) {
        sname[i] = toUpperChar ( lname[i] );
        //short name must not contain spaces - replace space by underscore
        if ( sname[i] == 32 ) sname[i]='_';
    }
    //check wether last char is '.' and the name is shorter than 8
    if ( lname[i] == '.' || lname[i] == 0 ) {
        fit++;
    }
    //XPRINTF("fit2=%d\n", fit);

    //find the last dot "." - filename extension
    size = strlen ( ( const char* ) lname );
    size--;

    for ( i = size; i > 0 && lname[i] !='.'; i-- );
    if ( lname[i] == '.' ) {
        i++;
        for ( j=0; lname[i] != 0 && j < 3; i++, j++ ) {
            sname[j+8] = toUpperChar ( lname[i] );
        }
        //no more than 3 characters of the extension
        if ( lname[i] == 0 ) fit++;
    } else {
        //no dot detected in the long filename
        fit++;
    }
//	XPRINTF("fit3=%d\n", fit);
//	XPRINTF("Long name=%s  Short name=%s \n", lname, sname);

    //all 3 checks passed  - the long name fits in the short name without restrictions
    if ( fit == 3 ) {
        //	XPRINTF("Short name is loseles!\n");
        return 0;
    }

    //one of the check failed - the short name have to be 'sequenced'
    //do not allow spaces in the short name
    for ( i = 0; i < 8;i++ ) {
        if ( sname[i] == 32 ) sname[i] = '_';
    }
    return 1;
}

/*
 simple conversion of the char from lower case to upper case
*
inline unsigned char toUpperChar ( unsigned char c ) ;

/* funzionec he crea il nome corto a partire dal nome lungo, inserendolo su short_name

BOOL createShortName ( const char * long_name,  char *short_name ) {

    BOOL lossy_conversion=FALSE;   // FLAG che stabilisce se devo inserire la tilde
    char *last_period =NULL; 		// puntatore all'ultimo periodo ( estensione)
    int extra=0,i,size=0;
    char *long_name_OEM=NULL, *temp=NULL;

    if ( !long_name || !short_name ) {
        set_errno ( "Errore puntatori" );
        return FALSE;
    }

    // verifica caratteri nome corto

    // conversione upper case tralasciata

    /* conversione to unicode to OEM*
    // riempo il buffer di spazi bianchi
    memset ( ( void* ) short_name, 0x20, 11 );
    size=strlen ( long_name );
    long_name_OEM=mem_alloc ( size,1 );

    //puntatore all'ultimo periodo
    for ( i=size; i>0; i-- )
        if ( * ( long_name+i ) == 0x2e ) {	//cerco il punto
            last_period=long_name+i;
            if ( * ( last_period+1 ) ==0x00 ) {// fine stringa
                *last_period=0x00; 		// elimino il punto
                last_period=NULL;
                continue;
            } else
                break;
        }

    if ( last_period )
        last_period++;

    flog ( LOG_WARN, "periodo %s", last_period ? last_period : "NULL" );

    // verifichiamo la presenza di piu spazi e punti
    for ( i =0;i < size; i++ )
        if ( * ( long_name+i ) ==0x2e )
            extra++;
        else if ( * ( long_name+i ) ==0x20 )
            extra+=2;

    if ( extra >1 )
        lossy_conversion=TRUE;



    //short file puo contenere un solo periodo

    temp=short_name;

    for ( i=0;  * ( long_name+i ) !=0x00 && * ( long_name+i ) !=0x2E && i < 8; i++ )
        if ( * ( long_name+i ) ==' ' ) {
            lossy_conversion=TRUE;
            continue;
        } else {
            * ( temp ) =toUpperChar ( * ( long_name+i ) );
            temp++;
        }


    temp=short_name+8;

    if ( strlen ( last_period ) > 3 )
        lossy_conversion=TRUE;


    if ( last_period )
        for ( i=0; i< 3 && * ( last_period +i ) !=0x00; i++ )
            if ( * ( long_name+i ) ==' ' ) {
                lossy_conversion=TRUE;
                continue;
            } else {
                * ( temp ) =toUpperChar ( * ( last_period+i ) );
                temp++;
            }

    //	flog(LOG_WARN, "name %s", short_name);

    temp=short_name;

    if ( lossy_conversion ) {
        for ( i=0; i<6 && *temp!=0x20; i++,temp++ );

        *temp='~'; temp++;
        *temp='1';
    }


    flog ( LOG_WARN, "name %s", short_name );
}
unsigned char toUpperChar ( unsigned char c ) {
    if ( c >96  && c < 123 ) {
        return ( c - 32 );
    }
    return c;
}

*

 inline unsigned char
 vfat_tolower( unsigned char c)
{
	unsigned char nc = charset2lower[c];

	return nc ? nc : c;
}

 inline unsigned char
vfat_toupper(unsigned char c)
{
	unsigned char nc = charset2upper[c];

	return nc ? nc : c;
}
/*

 int
vfat_strnicmp( const unsigned char *s1,
					const unsigned char *s2, int len)
{
	while(len--)
		if (vfat_tolower(*s1++) != vfat_tolower(*s2++))
			return 1;

	return 0;
}


*/
    
static const char *reserved3_names[] = {
	"con     ", "prn     ", "nul     ", "aux     ", NULL
};

static const char *reserved4_names[] = {
	"com1    ", "com2    ", "com3    ", "com4    ", "com5    ",
	"com6    ", "com7    ", "com8    ", "com9    ",
	"lpt1    ", "lpt2    ", "lpt3    ", "lpt4    ", "lpt5    ",
	"lpt6    ", "lpt7    ", "lpt8    ", "lpt9    ",
	NULL };


/* Characters that are undesirable in an MS-DOS file name */

static wchar bad_chars[] = {
	/*  `*'     `?'     `<'    `>'      `|'     `"'     `:'     `/' */
	0x002A, 0x003F, 0x003C, 0x003E, 0x007C, 0x0022, 0x003A, 0x002F,
	/*  `\' */
	0x005C, 0,
};
#define IS_BADCHAR(uni)	(vfat_unistrchr(bad_chars, (uni)) != NULL)

static wchar replace_chars[] = {
	/*  `['     `]'    `;'     `,'     `+'      `=' */
	0x005B, 0x005D, 0x003B, 0x002C, 0x002B, 0x003D, 0,
};
#define IS_REPLACECHAR(uni)	(vfat_unistrchr(replace_chars, (uni)) != NULL)

static wchar skip_chars[] = {
	/*  `.'     ` ' */
	0x002E, 0x0020, 0,
};
#define IS_SKIPCHAR(uni) \
	((wchar)(uni) == skip_chars[0] || (wchar)(uni) == skip_chars[1])

static inline wchar *vfat_unistrchr(const wchar *s, const wchar c)
{
	for(; *s != c; ++s)
		if (*s == 0)
			return NULL;
	return (wchar *) s;
}

static inline int vfat_is_used_badchars(const wchar *s, int len)
{
	int i;
	
	for (i = 0; i < len; i++)
		if (s[i] < 0x0020 || IS_BADCHAR(s[i]))
			return -EINVAL;
	return 0;
}



/*NAMEI*/ 
inline int to_shortname_char( char *buf, char *src,
				    BOOL *info)
{
	int len;

	if (IS_SKIPCHAR(*src)) {
               *info = FALSE;
//		printf("%C\n", *src); 
		return 0;
	}
	if (IS_REPLACECHAR(*src)) {
		*info = FALSE;
		buf[0] = '_';
		return 1;
	}
	
	len = uni2char(*src, buf);
	if (len <= 0) {
		*info = FALSE;
		buf[0] = '_';
		len = 1;
	} else if (len == 1) {
		unsigned char prev = buf[0];

		// asci esteso 
		if (buf[0] >= 0x7F) {
			*info = FALSE;
		}
	       

		buf[0] = UPPER(buf[0]);
		/*	if (isalpha(buf[0])) {
		  // ho avuto una conversione 
		  
		
		  }*/

	} else 
	  *info=FALSE; 
	
	return len;
}

/*
buf[0] = vfat_toupper(nls, buf[0]);
		if (isalpha(buf[0])) {
			if (buf[0] == prev)
				info->lower = 0;
			else
				info->upper = 0;
		}
	} else {
		info->lower = 0;
		info->upper = 0;
	}
	
	return len;
*


static inline unsigned char
shortname_info_to_lcase(struct shortname_info *base,
			struct shortname_info *ext)
{
	unsigned char lcase = 0;

	if (base->valid && ext->valid) {
		if (!base->upper && base->lower && (ext->lower || ext->upper))
			lcase |= CASE_LOWER_BASE;
		if (!ext->upper && ext->lower && (base->lower || base->upper))
			lcase |= CASE_LOWER_EXT;
	}

	return lcase;
}
*/
#define S 6

char *nomi[S]= { "A       TXT", 
		 "AA      TXT", 
		 "A          ", 
		 "A~1     TXT",
		 "A~2     TXT",
		 "THEQUI~1FOX",
};


int find (char * name ) { 

  int i=0; 

  for (i=0; i <S; i++) 
    if(!strncmp(name, nomi[i], 11))
      return 0; 
  return -1; 
  
}


/*
 * Givecn a valid longname, create a unique shortname.  Make sure the
 * shortname does not exist
 * Returns negative number on error, 0 for a normal
 * return, and 1 for valid shortname
 */

#define NLS_MAX_CHARSET_SIZE 260

/* uname unico de name 
 * ulen lunghezza unicode 
 * name res 
 */

int create_shortname(byte label, dword dir,  const  char *uname, int ulen,   char *name_res)
{
	char *ip, *ext_start, *end, *name_start;
	unsigned char base[9], ext[4], buf[8], *p;
	unsigned char charbuf[NLS_MAX_CHARSET_SIZE];
	int chl, chi;
	int sz = 0, extlen, baselen, i, numtail_baselen, numtail2_baselen;
	int is_shortname;
	BOOL  base_info=1, ext_info=1; // se attive devo inserire la tilde 
	
	is_shortname = 1; // è un nome corto

       /* Now, we need to create a shortname from the long name */
	ext_start = end = &uname[ulen];
	while (--ext_start >= uname) {
     
		if (*ext_start == 0x2E) { /* is `.' */
			if (ext_start == end - 1) {
				sz = ulen;
				ext_start = NULL;
			}
			break;
		}
	}



	if (ext_start == uname - 1) {
//	        printf("Caso senza estensione"); 
		sz = ulen;
		ext_start = NULL;
	} else if (ext_start) {
		/*
		 * Names which start with a dot could be just
		 * an extension eg. "...test".  In this case Win95
		 * uses the extension as the name and sets no extension.
		 */
		name_start = &uname[0];
		while (name_start < ext_start) {
		  if (!IS_SKIPCHAR(*name_start)){
		     
				break;
		  }
			name_start++;
		}
		if (name_start != ext_start) {
			sz = ext_start - uname;
			ext_start++;
		} else {
			sz = ulen;
			ext_start=NULL;
		}
	}


	numtail_baselen = 6;
	numtail2_baselen = 2;
	for (baselen = i = 0, p = base, ip = uname; i < sz; i++, ip++) {
	  // printf("%d",i); 
	        chl = to_shortname_char(charbuf,ip, &base_info);
	//	printf("%d : %c %d %d info :%d\n",i,  *charbuf, baselen, chl, base_info); 
		if (chl == 0)
			continue;

		if (baselen < 2 && (baselen + chl) > 2)
			numtail2_baselen = baselen;
		if (baselen < 6 && (baselen + chl) > 6)
			numtail_baselen = baselen;
		for (chi = 0; chi < chl; chi++){
			*p++ = charbuf[chi];
			baselen++;
			if (baselen >= 8)
				break;
		}
		if (baselen >= 8) {
			if ((chi < chl - 1) || (ip + 1) - uname < sz)
				is_shortname = 0;
			break;
		}
	}
	if (baselen == 0) {
		return -EINVAL;
	}

	extlen = 0;
	if (ext_start) {
		for (p = ext, ip = ext_start; extlen < 3 && ip < end; ip++) {
			chl = to_shortname_char(charbuf,
						ip, &ext_info);
			if (chl == 0)
				continue;

			if ((extlen + chl) > 3) {
				is_shortname = 0;
				break;
			}
			for (chi = 0; chi < chl; chi++) {
				*p++ = charbuf[chi];
				extlen++;
			}
			if (extlen >= 3) {
				if (ip + 1 != end)
					is_shortname = 0;
				break;
			}
		}
	}
	ext[extlen] = '\0';
	base[baselen] = '\0';

	/* Yes, it can happen. ".\xe5" would do it. */
	if (base[0] == DELETED_FLAG)
		base[0] = 0x05;

	/* OK, at this point we know that base is not longer than 8 symbols,
	 * ext is not longer than 3, base is nonempty, both don't contain
	 * any bad symbols (lowercase transformed to uppercase).
	 */

	memset(name_res, ' ', MSDOS_NAME);
	memcpy(name_res, base, baselen);
	memcpy(name_res + 8, ext, extlen);

//	printf("NAME_RES %s\n", name_res); 

//	printf("Short %d base %d ext %d\n", is_shortname, base_info, ext_info);
	if (is_shortname && base_info && ext_info) {
	  	if (search_entry_short(label, dir,(const char*) name_res))
	 		return -EEXIST;
		return 0; 
	       
	} 
	/*

	if (numtail == 0)
	  if (vfat_find_form(dir, name_res) < 0)
		return 0;
	*/
	/*
	 * Try to find a unique extension.  This used to
	 * iterate through all possibilities sequentially,
	 * but that gave extremely bad performance.  Windows
	 * only tries a few cases before using random
	 * values for part of the base.
	 */

	if (baselen>6) {
		baselen = numtail_baselen;
		name_res[7] = ' ';
	}
	name_res[baselen] = '~';
	for (i = 1; i < 10; i++) {
		name_res[baselen+1] = i + '0';
		
		if (!search_entry_short(label, dir,(const char*) name_res))
			return 0;
	}
	
	// random 

	/*	i = jiffies & 0xffff;
	sz = (jiffies >> 16) & 0x7;
	if (baselen>2) {
		baselen = numtail2_baselen;
		name_res[7] = ' ';
	}
	name_res[baselen+4] = '~';
	name_res[baselen+5] = '1' + sz;
	while (1) {
		sprintf(buf, "%04X", i);
		memcpy(&name_res[baselen], buf, 4);
		if (find(name_res) < 0)
			break;
		i -= 11;
	}

	*/ 

	return 0;


}







BOOL valid_longname(const char *name, int len, int xlate)
{
	const char **reserved, *walk;
	int baselen;

	if (len && name[len-1] == ' ') return -EINVAL;
	if (len >= 256) return -EINVAL;
 	if (len < 3) return 0;

	for (walk = name; *walk != 0 && *walk != '.'; walk++);
	baselen = walk - name;

	if (baselen == 3) {
		for (reserved = reserved3_names; *reserved; reserved++) {
			if (!strnicmp(name,*reserved,baselen))
				return -EINVAL;
		}
	} else if (baselen == 4) {
		for (reserved = reserved4_names; *reserved; reserved++) {
			if (!strnicmp(name,*reserved,baselen))
				return -EINVAL;
		}
	}
	return 0;
}


  

BOOL to_msdos_time ( dword sys_time, msdos_time *time ) { 
  
    
    time->Second=((sys_time & 0x000000FF)&0x1F) / 2;  // coppie di secondi)
    time->Minutes=((sys_time & 0x0000FF00) >> 8)&0x1F; 
    time->Hours=((sys_time & 0x00FF0000) >> 16)&0x2F; 
  
    return TRUE; 
}

BOOL to_msdos_date ( dword sys_date, msdos_date * date) {

	date->Day=(sys_date & 0x000000FF)&0x1F; // giorno prelevo 5 bit
	date->Month=((sys_date & 0x0000FF00) >> 8)&0x0F; //mese 4 bit
	date->Years=(((sys_date & 0x00FF0000) >> 16)&0xEF)+20; //anno 2000 (msdos 1980)
  
      return TRUE; 
}


/********************************************************************************/
//funzione ceh cerca un entry nell spazio dei nomi corto 

// 0 niente 
// 1 presente 

int search_entry_short ( byte label , dword chain , const char * name ) {
  
  int n_read=0; 
  byte *buf=mem_alloc(SIZE_BUF,1);   // SIZE BUF multiplo di 32
  lword offset=0; 
  BOOL end=FALSE, r=FALSE;
  
  
  // leggo finché non finisce la cartella 
  while((n_read=read_data(label, chain,offset,buf, SIZE_BUF))&&!end) {
    
    int i=0, n=n_read/SIZE_ENTRY; 
    SHORT_ENTRY *entry=(SHORT_ENTRY *)buf; 
    BOOL long_entry=FALSE; 
    
    if (n%SIZE_ENTRY) { 
	mem_free(buf); 
	return -1; 
    }
    
    // analizziamo le varie entry saltando quelle lunghe
    for ( i=0; i<n; i++, entry++) { 
  
      
      	if (entry->DIR_Attr == ATTR_LONG_NAME)  
	    continue; 
	if (entry->DIR_Name[0] ==  FREE)	// entry libera
		continue;
	if ( entry->DIR_Name[0] ==  ALL_FREE){ // non ci sono piu entry 
		end=TRUE; 
		break; 
	}
	// ho trovato il nome 
	if(!strncmp(entry->DIR_Name, name, SIZE_NAME_SHORT)) { 
	    end=TRUE; 
	    r=TRUE; 
	    break;
	}   
      
    }  
  }
  
  mem_free(buf); 
  return r;
  
}


 
int search_entry_long  ( byte label , dword chain , const wchar * name ) {
 
  int n_read=0; 
  byte *buf=mem_alloc(SIZE_BUF,1);   // SIZE BUF multiplo di 32
  lword offset=0; 
  BOOL end=FALSE, r=FALSE;
  wchar name_read[MAX_NAME]; 
  
  memset(name_read, 0, MAX_NAME*2); 
  
  if ( name == NULL) 
    return 0; 
  
  
  // leggo finché non finisce la cartella 
  while((n_read=read_data(label, chain,offset,buf, SIZE_BUF))&&!end) {
    
    int i=0, n=n_read/SIZE_ENTRY; 
    LONG_ENTRY *entry=(LONG_ENTRY *)buf; 
    BOOL long_entry=FALSE; 
    byte n_char[260];
    
    if (n%SIZE_ENTRY) { 
	mem_free(buf); 
	return FALSE; 
    }
    
    // analizziamo le varie entry saltando quelle corte
   for ( i=0; i<n; i++, entry++) { 
  
    // flog(LOG_WARN, "%d", entry->LDIR_Attr); 
     
	if (entry->LDIR_Ord == FREE) 
	    continue; 
	else if (entry->LDIR_Ord==ALL_FREE) {
	  end=TRUE; 
	  break; 
	} else  if (entry->LDIR_Attr != ATTR_LONG_NAME)  
	    continue; 
	// significa che ho trovato l'ultimo elemento di nome lungo 
	if (entry->LDIR_Attr == ATTR_LONG_NAME && (entry->LDIR_Ord & MASK_LAST_NAME)) {  
	    int j=entry->LDIR_Ord & 0x3F; // prelevo il numero di entry            
	 
	      if (  j< 1 || j > 20 ) // errore 
		return -1; 
	 
	      i+=(j-1); 
	 
	      for (; j>0; j--, entry++) 
		  append_name(entry,name_read);  
	
	      entry--;
	  }
	
	  print_w(name_read); 
	  if(!wcsicmp(name, name_read)) { 
	    r=TRUE; 
	    end=TRUE; 
	    break; 
	  }
	  
	}
  }
  
  mem_free(buf); 
  return r;
  
}



void print_short_entry(SHORT_ENTRY * s) {
  
  flog(LOG_DEBUG,"Name       %s",s->DIR_Name);  
  flog(LOG_DEBUG,"Attr       %x",s->DIR_Attr);  
  flog(LOG_DEBUG,"Nt         %x",s->DIR_NTRes);  
  flog(LOG_DEBUG,"Time tenth %d",s->DIR_CrtTimeTenth);  
  flog(LOG_DEBUG,"Crt Time   %d",s->DIR_CrtTime);  
  flog(LOG_DEBUG,"Crt Date   %d",s->DIR_CrtDate);  
  flog(LOG_DEBUG,"Lst Date   %d",s->DIR_LstAccDate);  
  flog(LOG_DEBUG,"HI         %d",s->DIR_FstClusHI);  
  flog(LOG_DEBUG,"Wrt Date   %d",s->DIR_WrtDate);  
  flog(LOG_DEBUG,"Wrt Time   %d",s->DIR_WrtTime);  
  flog(LOG_DEBUG,"Lo         %d",s->DIR_FstClusLO); 
  flog(LOG_DEBUG,"Size       %d\n",s->DIR_FileSize);  
  
}



void print_long_entry (LONG_ENTRY * l) {

   char Name1[SIZE_UNO+1]; 
   char Name2[SIZE_DUE+1]; 
   char Name3[SIZE_TRE+1];
  
   memset(Name1,0,SIZE_UNO+1); 
   memset(Name2,0,SIZE_DUE+1); 
   memset(Name3,0,SIZE_TRE+1); 
   
   uni_to_str_char(l->LDIR_Name1, Name1, SIZE_UNO); 
   uni_to_str_char(l->LDIR_Name2, Name2, SIZE_DUE); 
   uni_to_str_char(l->LDIR_Name3, Name3, SIZE_TRE);
   
   
   flog(LOG_DEBUG, "ID    %x", l->LDIR_Ord);
   flog(LOG_DEBUG, "Name  %s", Name1); 
   flog(LOG_DEBUG, "Attr  %x", l->LDIR_Attr);
   flog(LOG_DEBUG, "Type  %x", l->LDIR_Type); 
   flog(LOG_DEBUG, "CHCK  %x", l->LDIR_Chksum); 
   flog(LOG_DEBUG, "Name2 %s", Name2); 
   flog(LOG_DEBUG, "FstLO %x", l->LDIR_FstClusLO); 
   flog(LOG_DEBUG, "Name3 %s\n", Name3); 
}




void print_directory ( byte label, dword chain ) {
  
  flog(LOG_DEBUG, "DIRECTORY");
  int n_read=0; 
  byte *buf=mem_alloc(SIZE_BUF,1);   // SIZE BUF multiplo di 32
  lword offset =0; 
  BOOL end=FALSE; 
  
  while((n_read=read_data(label, chain,offset,buf, SIZE_BUF))&&!end) { 
    
    int i=0, n=n_read/SIZE_ENTRY; 
    SHORT_ENTRY *entry=(SHORT_ENTRY *)buf; 
    BOOL long_entry=FALSE; 
    offset+=n_read; 
    
    if (n%SIZE_ENTRY) { 
	mem_free(buf); 
	return ; 
    }
    
    flog(LOG_DEBUG, "LONG %x ARCHIVE %x", ATTR_LONG_NAME, ATTR_ARCHIVE); 
    
    for ( i=0; i<n; i++, entry++) {
     
	
	if (entry->DIR_Name[0] == FREE) 
	    continue; 
	else if (entry->DIR_Name[0]==ALL_FREE) {
	  end=TRUE; 
	  break; 
	}else if (entry->DIR_Attr==ATTR_LONG_NAME)  
	    print_long_entry((LONG_ENTRY*)entry);
	 else 
	    print_short_entry((SHORT_ENTRY*)entry);
    }
   
    
  }
  
  return ; 
    
}

extern FCB*root; 



// funzione che inizializza la directory di root 

void init_root_fcb ( byte label) { 
  
  root=mem_alloc(sizeof(FCB),1); 
  
  memset(root, 0, sizeof(FCB)); 
  memset(root->name, 0, SIZE_NAME); 
  root->cluster=2;
  root->offset_father=0; 
  root->type=ATTR_DIRECTORY; 
  root->father=NULL; 
  root->semaphore=sem_ini(1); 
  root->volume=label; 
}















/* funzionc ehce crea un entry su father * 
int create_shortname(byte label, dword dir,   char *uname, int ulen,   char *name_res);
*/


BOOL create_entry(const char * name , byte type,  const FCB * father, FCB ** new){ 
  
  char name_short[MSDOS_NAME]; 
  byte *buf=NULL ; 
  lword off=0; 
  word free_entry=0; 
  FCB * tmp=0; 
  
  if ( new == NULL || father==NULL) { 
    set_errno(1,"Errore paramentri"); 
    return FALSE; 
  }

  if ( father->type!=ATTR_DIRECTORY) { 
     set_errno(1,"Bad directory"); 
     return FALSE; 
  }


  if (validate_name(father->volume, father->cluster, name)) {
    return FALSE; 
  }

  memset((void*)name_short,0, MSDOS_NAME); 

  //nome corto come va usato 
  create_shortname(father->volume, father->cluster, name, strlen(name), name_short);
  format_entry(name, name_short, type, &buf); 
  free_entry= get_n_entry(name); 
  
  sem_wait(father->semaphore); 
  
    off=get_free_entry(father->volume, father->cluster,free_entry); 
//    write_data(father->volume, father->cluster, off, buf, free_entry*SIZE_ENTRY); 
  
  sem_signal(father->semaphore); 
  
  
  *new=mem_alloc(sizeof(FCB), 1);
  
  
  tmp=*new; 
  
  memset(tmp,0, sizeof(FCB)); 
  memset(tmp->name, 0, MAX_NAME); 
  flog(LOG_WARN, "nome %s", name); 
  strncpy(tmp->name, name, strlen(name)); 
  flog(LOG_WARN, "nome %s", tmp->name);  
  if (type == ATTR_DIRECTORY) 
    tmp->semaphore=sem_ini(1); 
  
  tmp->father=father; 
  tmp->offset_father=off; 
  
  tmp->volume=father->volume; 
  tmp->cluster=0; // vuoto 
  
  return TRUE; 
}

BOOL open_entry(const char * name , const FCB * father, FCB ** new) {
  
  
  
}

BOOL delete_direntry(FCB * f) {
  
  
  
}






void print_fcb ( const FCB * fcb ) {

    flog ( LOG_DEBUG, "Name       :%s" ,fcb->name );
    flog ( LOG_DEBUG, "Cluster    :%d", fcb->cluster );
    flog ( LOG_DEBUG, "Father     :%x", fcb->father );
    flog ( LOG_DEBUG, "Father off :%x", fcb->offset_father); 
}



void test_dir (byte label){
  
  init_root_fcb(label);
  FCB *speranza=NULL; 
  
 if(!create_entry("prova.txt", ATTR_ARCHIVE, root, &speranza))
   perror("Creazione entry"); 
 
  print_fcb(speranza); 
//  delete_entry(speranza); 
  
}













/*

void test_dir ( byte label) {
 
 wchar *name_u=mem_alloc(MAX_NAME,2);  
 char *name_c=mem_alloc(MAX_NAME,1); 
 char *a=mem_alloc(1024,1); 
 char *b=mem_alloc(1024,1); 
 
 char short_name[MSDOS_NAME];
 
  int i=0,r; 
  
/*
 memset(name_u, 0, MAX_NAME*2); 
 strcpy(name_c, "The quick brown.fox"); 
 char_to_str_uni(name_u, name_c, strlen(name_c)); 
 print_w(name_u); 
 r=search_entry_long(label,2,name_u);
 
  if(r<0)
   flog(LOG_WARN, "Errore"); 
 else if(r)
   flog(LOG_WARN, "Nome presente"); 
 else 
   flog(LOG_WARN, "Nome assente"); 
 
 memset(name_u, 0, MAX_NAME*2); 
 strcpy(name_c, "giuseppe"); 
 char_to_str_uni(name_u, name_c, strlen(name_c)); 
 print_w(name_u); 
 r=search_entry_long(label,2,name_u);
 
  if(r<0)
   flog(LOG_WARN, "Errore"); 
 else if(r)
   flog(LOG_WARN, "Nome presente"); 
 else 
   flog(LOG_WARN, "Nome assente"); 
 
 memset(name_u, 0, MAX_NAME*2); 
 strcpy(name_c, "the quick brown.fox"); 
 char_to_str_uni(name_u, name_c, strlen(name_c)); 
 print_w(name_u); 
 r=search_entry_long(label,2,name_u);
 if(create_shortname(label, 2, name_u, wcslen(name_u), a) <0)
    flog(LOG_WARN, " create name Errore"); 
  
   flog(LOG_DEBUG, "SHORT :%s",a);  
  
  if(r<0)
   flog(LOG_WARN, "Errore"); 
 else if(r)
   flog(LOG_WARN, "Nome presente"); 
 else 
   flog(LOG_WARN, "Nome assente"); 
 
 memset(name_u, 0, MAX_NAME*2); 
 strcpy(name_c, "a.txt"); 
 char_to_str_uni(name_u, name_c, strlen(name_c)); 
 print_w(name_u); 
 r=search_entry_long(label,2,name_u);
 
  if(r<0)
   flog(LOG_WARN, "Errore"); 
 else if(r)
   flog(LOG_WARN, "Nome presente"); 
 else 
   flog(LOG_WARN, "Nome assente"); 
 

  memset(a,0,1024);
  memset(b,0, 1024);



  if(create_shortname(label, 2, name_u, wcslen(name_u), a) <0)
    flog(LOG_WARN, " create name Errore"); 
  
  flog(LOG_DEBUG, "SHORT :%s",a);  
  
  r=search_entry_short(label,2,a);
 
 if(r<0)
   flog(LOG_INFO, " r Errore"); 
 else if(r)
   flog(LOG_INFO, "Nome presente"); 
 else 
   flog(LOG_INFO, "Nome assente"); 
 
 mem_free(a); 
 mem_free(b); 
 mem_free(name_c); 
 mem_free(name_u); 
 
 
 { 
   dword date=get_date(); 
   dword time=get_time(); 
   msdos_date d; 
   msdos_time b; 
   
   
   memset(&d,0, sizeof(d)); 
   
   flog(LOG_WARN,"DATE BIOS %x size %d", date, sizeof(d)); 
   to_msdos_date(date,&d);
   flog(LOG_WARN,"DATE MSODS %x %x %x", d.Day, d.Month, d.Years); 
   
   time=get_time(); 
  
   memset(&b,0, sizeof(b)); 
   
   flog(LOG_WARN,"TIME BIOS %x size %d", time, sizeof(b)); 
   to_msdos_time(time,&b);
   flog(LOG_WARN,"TIME MSODS %x %x %x",b.Hours, b.Minutes, b.Second); 
   
   
 }
 *

{ 
//  char s[MSDOS_NAME]; 
  char * name="The quick brown giuseppe pes.fox"; 
  byte *buf=NULL; 
  SHORT_ENTRY *s;
  lword i=0, r=0; 
  
   /*
  create_shortname('C', 2,  name, strlen(name), s);
  format_entry(name, s,ATTR_ARCHIVE, &buf); 
  
  print_long_entry((LONG_ENTRY*)buf);
  buf+=32;
  print_long_entry((LONG_ENTRY*)buf);
  buf+=32;
  print_long_entry((LONG_ENTRY*)buf);
  buf+=32;
  print_short_entry((SHORT_ENTRY*)buf);
  *
  buf=mem_alloc(1024,1); 
  
  read_data('C', 2, 0, buf,1024);
  s=(SHORT_ENTRY*)buf; 
  
  for ( i=0; i<1024/32; i++, s++) 
    flog(LOG_WARN, "%d : %x",i,  s->DIR_Name[0]); 
   
  r=get_free_entry('C', 2, 5);
  
  flog(LOG_WARN, "r :%d", r); 
  
  read_data('C', 2, r ,buf,SIZE_ENTRY*5);
  
  s=(SHORT_ENTRY*)buf; 
  
  for ( i=0; i<5; i++, s++) 
    flog(LOG_WARN, "%d : %x",i,  s->DIR_Name[0]); 
   
  
  
}
 
}

*/