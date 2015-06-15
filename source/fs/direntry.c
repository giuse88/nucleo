#include "direntry.h" 
#include "string.h" 
#include "errno.h"
#include "data.h"
#include "fat.h" 
#include "sistema.h"
#include "wchar.h"
#include "fs.h"
#include "system_call.h" 
#include "fcb.h" 

/*PRIVATE*/
void print_long_entry (LONG_ENTRY * l);
void merge_dword(dword * value, const word low , const word high);
void split_dword (const dword value, word * low, word * high);
void print_short_entry(SHORT_ENTRY * s);
BOOL create_short_entry( const char * name, SHORT_ENTRY * entry); 
BOOL create_long_entry(const char * name, LONG_ENTRY *entry);
BOOL to_msdos_time ( dword sys_time, msdos_time *time );
BOOL to_msdos_date ( dword sys_date, msdos_date *date );
BOOL search_entry( dword chain, const  char * name); 
BOOL search_name ( VOL_PTR volume, dword cluster, const char * name);
BOOL format_short_entry( const char * name, SHORT_ENTRY * entry, byte type, dword c,dword size);
BOOL format_long_entry(const char* name, LONG_ENTRY* entry, byte n, byte chkSum);
BOOL test_checksum ( const SHORT_ENTRY * s, const LONG_ENTRY * l, int n_entry_long);
BOOL cmp_msdos_name ( const char * s1, const char * s2 ); 
int search_entry_short ( VOL_PTR , dword chain , const char * name , SHORT_ENTRY * short_entry );
int search_entry_long  ( VOL_PTR , dword chain , const char * name_search, char **name_find,  SHORT_ENTRY * short_entry, LONG_ENTRY **long_entry, word *n_entry_lon );
int create_shortname( VOL_PTR, dword dir, const char* uname, int ulen, char* name_res);
BOOL  format_entry (const char *long_name, const char * short_name, byte type, dword chain, dword size, byte * buf) ;

/*GLOBAL*/




extern TABELLA_VOLUMI * tabella; 




/******************IMPLEMENTAZIONE************************************************************/

void split_dword (const dword value, word * low, word * high) {

        *low=(word)(value&0x0000FFFF);
        *high=(word)((value&0xFFFF0000)>>16);
};

void merge_dword(dword * value, const word low , const word high) {

        *value=((dword)(high)<<16) | (dword)(low);

};


BOOL cmp_msdos_name ( const char * s1, const char * s2 ) { 
  
   int i=0;
   
   for ( i=0; i < MSDOS_NAME; i++) 
      if ( UPPER(s1[i])!=UPPER(s2[i])) 
	  return FALSE; 
      
  return TRUE; 
}


void insert_dot ( byte *buf, dword chain) { 

  SHORT_ENTRY s; 
  char dot[MSDOS_NAME]   =".          "; 
  
  if ( buf == NULL ||  chain <=2 ) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return;
  }
  
  memset(&s, 0, sizeof(SHORT_ENTRY)); 

  format_short_entry(dot, &s, ATTR_DIRECTORY, chain, SIZE_DIRECTORY); 
  
#ifndef DEBUG_FS
  print_short_entry((SHORT_ENTRY*)&s);
#endif
  //inserisco i campi nel buffer 
   memcpy(buf, &s, sizeof(SHORT_ENTRY));
  
  return ;  
  
}



void insert_dotdot ( byte *buf, dword chain) { 

  SHORT_ENTRY s; 
 char dotdot[MSDOS_NAME]="..         ";
  
  if ( buf == NULL) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return;
  }
  
  memset(&s, 0, sizeof(SHORT_ENTRY)); 

  format_short_entry(dotdot, &s, ATTR_DIRECTORY, chain, SIZE_DIRECTORY); 
  
#ifndef DEBUG_FS
  print_short_entry((SHORT_ENTRY*)&s);
#endif
  //inserisco i campi nel buffer 
   memcpy(buf, &s, sizeof(SHORT_ENTRY));
  return ;  
  
}





// funzione che rende il numero di entry piu quella corta /

inline word get_n_entry(const char * str) { 
 
 size_t size=strlen(str); 
  word n=0; 
  
  n=size/SIZE_NAME_PART; 
  if(size%SIZE_NAME_PART) 
    n++;
  n++; // c'è sempre una short entry 
  
  return n; 
}
    
extern dword c_get_time(); 

dword get_time() { 
	return c_get_time(); 	
}
extern dword c_get_date(); 

dword get_date(){
  
  return c_get_date(); 
}

byte check_sum ( const  byte * name) { 
  
 short i=0; 
 byte sum=0; 
 
 for ( i=MSDOS_NAME; i !=0; i--) 
      sum= (( sum & 1) ? 0x80 : 0) + ( sum >> 1) + *name++; 
 
 return sum; 
}



 natl random () { 
 
    static lword x=0;
     x= ((get_time() << 8 ) | get_date()) ; 
     
     lword a =1103515245; 
     natl c =12345; 
     
     x = (a*x +c); 
  
    return (natl)x; 
}




/*FUNZIONE***************************************************************
 * Funzione che ha il compito di formattare un buffer rendendolo idoneo  *
 * alla scrittura sul disco. Il buffer deve essere precedentemente      *
 * allocato. 								*
 * LONG_NAME  : nome lungo 						*
 * SHORT_NAME : nome corto già formatto 				*
 * TYPE       : identifica la entry 				 	*
 * CHAIN      : first cluster ( puo essere anche zero) 			*
 * Se tutto è andato bene ritorna true, altrimenti falso e setta l'er-  *
 * rore.								*
 ************************************************************************/

BOOL  format_entry (const char *long_name, const char * short_name, byte type, dword chain, dword size, byte * buf) { 
 
  word n_entry=0; 
  LONG_ENTRY *l=NULL; 
  word i=0,n=0; 
  char *tmp=NULL;  
  byte chkSum=0; 
 
  
  if ( !long_name || !short_name || !buf ) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE;
  }
  
  n_entry=get_n_entry(long_name); // prelevo il numero di entry
  l=(LONG_ENTRY*)buf; 
  
  n_entry--;//elimino l'entry short  
  i=n_entry;
  n=strlen(long_name); 
  tmp= (char*)(long_name+n); // ultimo
  tmp=tmp-(n%SIZE_NAME_PART); // punto all'elemento da copiare
  
  chkSum=check_sum((byte*)short_name); 
  
  for (;n_entry>0; n_entry--, l++, tmp-=SIZE_NAME_PART) {
    format_long_entry(tmp,l, (i==n_entry)? (n_entry|MASK_LAST_NAME):n_entry, chkSum); 
  }
  
  return  format_short_entry(short_name, (SHORT_ENTRY*)l, type, chain, size); 
  
}


// Formata un entry corta. Entry deve essere precedentemente allocata

BOOL format_short_entry( const char * name, SHORT_ENTRY * entry, byte type, dword chain, dword size ){
  
  word low=0, high=0; 
  
  if ( !name || !entry ) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE;
  }
  
  memset(entry,0, SIZE_ENTRY); 
  
  entry->DIR_Attr=type; 
  entry->DIR_FileSize=size; 
  entry->DIR_NTRes=0; 
  entry->DIR_CrtTimeTenth=0; // ignoro millisecondi 
  
  to_msdos_date(get_date(), (msdos_date*) &entry->DIR_CrtDate); 
  to_msdos_time(get_time(), (msdos_time*) &entry->DIR_CrtTime);
  to_msdos_date(get_date(), (msdos_date*) &entry->DIR_WrtDate); 
  to_msdos_time(get_time(), (msdos_time*) &entry->DIR_WrtTime);
  to_msdos_date(get_date(), (msdos_date*) &entry->DIR_LstAccDate); 
  
  split_dword(chain,&low,&high);
  entry->DIR_FstClusHI=high;
  entry->DIR_FstClusLO=low;
  
  memset((void*)entry->DIR_Name,0,MSDOS_NAME); 
  strncpy((char*)entry->DIR_Name, name, MSDOS_NAME); 
  
  
  return TRUE; 
}

//
// Formata un entry lunga. Entry deve essere precedentemente allocata

BOOL format_long_entry(const char * name, LONG_ENTRY *entry, byte n, byte chkSum) {
  
 
  const char *name1=name; 
  const char *name2=name+SIZE_UNO; 
  const char *name3=name2+SIZE_DUE;
  
  BOOL salta=FALSE; 
  
  
   if (n&0x40) { 
    
     dword size=strlen(name); 
    
     wmemset(entry->LDIR_Name1, 0xFFFF, SIZE_UNO); 
     wmemset(entry->LDIR_Name2, 0xFFFF, SIZE_DUE); 
     wmemset(entry->LDIR_Name3, 0xFFFF, SIZE_TRE);
     
     if ( size < SIZE_UNO){
	char2uni(entry->LDIR_Name1, name1, size);
	entry->LDIR_Name1[size]=0x0000; 
	salta=TRUE; 
     }
    
    size-=SIZE_UNO; 
    
    if ( (size < SIZE_DUE) & !salta ){
        char2uni(entry->LDIR_Name1, name1, SIZE_UNO);
	char2uni(entry->LDIR_Name2, name2, size);
	entry->LDIR_Name2[size]=0x0000; 
	salta=TRUE; 
    }
  
    size-=SIZE_DUE; 
    
    if ( (size < SIZE_TRE ) & !salta){
	char2uni(entry->LDIR_Name1, name1, SIZE_UNO);
	char2uni(entry->LDIR_Name2, name2, SIZE_DUE);
	char2uni(entry->LDIR_Name3, name3, size);
	entry->LDIR_Name3[size]=0x0000; 
    }
     
    
    if ( (size == SIZE_TRE)  & !salta){
	char2uni(entry->LDIR_Name1, name1, SIZE_UNO);
	char2uni(entry->LDIR_Name2, name2, SIZE_DUE);
	char2uni(entry->LDIR_Name3, name3, SIZE_TRE); 
     }
    
   } else {
   
   char2uni(entry->LDIR_Name1, name1, SIZE_UNO);
   char2uni(entry->LDIR_Name2, name2, SIZE_DUE); 
   char2uni(entry->LDIR_Name3, name3, SIZE_TRE);

     
  }
  
  
   entry->LDIR_Attr=ATTR_LONG_NAME; 
   entry->LDIR_FstClusLO=0; 
   entry->LDIR_Chksum=chkSum; 
   entry->LDIR_Type=0; 
   entry->LDIR_Ord=n; 

   return TRUE;  
  
}










/*
 *Il Nome del file Lo gestiamo come una pila , inseriamo sempre in testa
 * questo perchè la lista la scorriamo dall'ultimo elemento :
 * Questa funzione trasforma da unicode a asci (7 bit) il nome salvato
 * nel disco
 * NAME :  parte del nome del file UNICODE
 * BUF     . Buffer del FCB nel qual inseriamo il nome
 * non faccio nessun controllo sullo spazio  
*/


void append_name (const  LONG_ENTRY * entry, wchar * n ) {

  int i=0, j=0, s=0;  
  wchar name[SIZE_NAME_PART+1]; 
  wchar buf[SIZE_NAME]; 
  BOOL end=FALSE; 

  
  memset(name, 0, (SIZE_NAME_PART+1)*2); 
  wmemset(buf, 0, SIZE_NAME); 
  // name 1 ; 
  for (i=0; (i< SIZE_UNO); i++,j++) 
      if(entry->LDIR_Name1[i]!= PADDING)
	  name[j]=entry->LDIR_Name1[i]; 
      else {
	end=TRUE; 
	break; 
      }
      
  // name 2 
	
  for (i=0; (i< SIZE_DUE) && !end; i++,j++) 
      if(entry->LDIR_Name2[i]!= PADDING)
	  name[j]=entry->LDIR_Name2[i]; 
       else {
	end=TRUE; 
	break; 
      }
      
  // name 3 
    for (i=0; (i< SIZE_TRE ) & !end; i++,j++) 
      if(entry->LDIR_Name3[i]!= PADDING)
	  name[j]=entry->LDIR_Name3[i]; 
      else 
	break; 
      
   // ora su j ho quanto è lungo lo spazio 
  // traslo di j
  
  s=wcslen(n);
  

  // se non è il primo 
  if(!(entry->LDIR_Attr & MASK_LAST_NAME)) {
    memset(buf, 0, SIZE_NAME*2); 
    wcsncpy(buf,n, s);	//salvo il nome 
    wmemset(n, 0,MAX_NAME); 	// reset name 
    wcsncpy(n+j,buf,s); 
  }
  
 wcsncpy(n,name,j);


  return;
}



/* FUNZIONE ***********************************************************************
 * Cerca n entrate libere, assumo che quando è presente FREE_ALL , tutti i       *
 * cluster successivi sono liberi, e non esistono posizioni adiacenti a questa    *
 * con l'etichetta free ( il software che si preoccupa di eliminare le entry 	  *
 * si incarica di fondere le varie zone. 					  *
 *     N 	   : numero di entry da cercare					  *
 *     CLUSTER : puntatore alla fat 						  *
 *     SIZE    : grandezza di un cluster 					  *
 * Ritorna offset								  *
 **********************************************************************************/


int  get_free_entry ( VOL_PTR volume, dword chain, word n) {


    SHORT_ENTRY *dir;
    byte *buf=0;
    BOOL end=FALSE; 
    dword n_read=0,i=0, dir_free=0; 
    dword offset=0, offset_backup=0; 
    dword max=0;  
    dword size_cluster=0; 
    
    
    
    if ( !volume || chain <2 ) {
      set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
      return -EINVAL;
    }
       
    size_cluster=(volume->fat_info.sectors_for_cluster * volume->fat_info.byts_for_sector); 
    max=size_cluster/SIZE_ENTRY; 
    
    if(!(buf=mem_alloc(size_cluster,1))){
       set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
       return -ENOMEM; 
     }
    
    memset(buf,0, size_cluster); 
    
   while( !end && (n_read=read_data(volume, chain,offset,buf, size_cluster))) {
 
     dword number_dir_entry=n_read/SIZE_ENTRY;
     dir=(SHORT_ENTRY*)buf;
     i=0; 
     
     if ( n_read <= 0 ) { 
	perror("Read"); 
	mem_free(buf); 
	return -EIO; 
      }
     
     number_dir_entry=n_read/SIZE_ENTRY;
     
     for ( i=0; i<number_dir_entry; i++, dir++) {
        
        if ( dir->DIR_Name[0]== FREE || dir->DIR_Name[0] == ALL_FREE )
            dir_free++;
  	else 
	    dir_free=0; 
  	
        if ( dir_free==n ){
  	    end=TRUE; 
  	    break; 
  	}
    }
     
     
       if(end) { 
 	
 	  if ( (dir_free) > ++i)  {
 	      flog(LOG_INFO, "Offset CAVALLO %d " , offset_backup + ( max - (dir_free- i))*SIZE_ENTRY); 
	      mem_free(buf); 
 	      return offset_backup + ( max - (dir_free- i))*SIZE_ENTRY ; 
 	  }else {
		mem_free(buf); 
		return offset+(i-dir_free)*SIZE_ENTRY;  
 	  }
       }
       
       offset_backup=offset; 
       offset+=n_read;
   
       }   // while
       
       
       
     // SE NON C'E' DEVO INSERIRE UN CLUSTER VUOTO ( mi serve la grandezza del cluster )
	if (!n_read ) { // devo appendere un cluster
  	memset(buf, 0, size_cluster); 
  	if ( write_data(volume, chain,offset, buf, size_cluster) < size_cluster) { 
	    perror("read"); 
	    mem_free(buf); 
	}
	mem_free(buf); 
  	return offset-dir_free*SIZE_ENTRY;  
     }
     
 
    mem_free(buf); 
    return -1; 
    
 
}



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
/*FUNZIONE CHE ANLISZZA I CARATTERI */ 

inline int to_shortname_char( char *buf, unsigned char *src,
				    BOOL *info)
{
	int len=1;

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

	buf[0]=(char)*src; 
	
	//se è un carattere non esprimibile in ASCII BASE 
	// lo sostituisco
	if (buf[0] >= 0x7F) {
		*info = FALSE;
 		buf[0]='_';   //<-------------------- MIA AGGIUNTA 
	}
	 buf[0] = UPPER(buf[0]);
		
	return len;
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

int create_shortname(VOL_PTR volume, dword dir,  const  char *uname, int ulen,   char *name_res)
{
	char *ip, *ext_start, *end, *name_start;
	unsigned char base[9], ext[4], buf[8], *p;
	unsigned char charbuf[NLS_MAX_CHARSET_SIZE];
	int chl, chi;
	int sz = 0, extlen, baselen, i, numtail_baselen, numtail2_baselen;
	int is_shortname;
	BOOL  base_info=1, ext_info=1; // se attive devo inserire la tilde 
	dword jiffies; 
	
	is_shortname = 1; // e' un nome corto

       /* Now, we need to create a shortname from the long name */
	ext_start = end = (char *)&uname[ulen];
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
		name_start = (char*)&uname[0];
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
	for (baselen = i = 0, p = base, ip =(char*) uname; i < sz; i++, ip++) {
 
	        chl = to_shortname_char((char*)charbuf,(byte*)ip, &base_info);

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
		return FALSE;
	}

	extlen = 0;
	if (ext_start) {
		for (p = ext, ip = ext_start; extlen < 3 && ip < end; ip++) {
			chl = to_shortname_char((char*)charbuf,
						(byte*)ip, &ext_info);
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

	/*
	 *Se shortaname e la base e l'estensione  del nome sono true controllo che 
	  non esista gia in il nome se non esiste ritorno true 
	  BASE INFO : true se non ci sono caratteri non  validi 
	  EXT INFO  :  true se non ci sono caratteri non  validi
	 */
	if (is_shortname && base_info && ext_info) {
	  	if (search_name(volume, dir,(const char*) name_res)) {
		    set_errno(EEXIST, "FILE gia' esistente"); 
		    return FALSE;
		}
		return TRUE; 
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
		int aw;
		name_res[baselen+1] = i + '0';
		//flog(LOG_WARN, " Calcolo nome corto  %s %d ", name_res, i); 
		//flog(LOG_WARN, " Calcolo risultato %d", aw=search_entry_short(volume, dir,(const char*) name_res, NULL)); 
		if ((aw=search_entry_short(volume, dir,(const char*) name_res, NULL) < 0 )) {	//significa che non è presente
		    return TRUE; 
		} 
	
		  
	}

	// se il numero è maggiore di 9 bisogna generare il numero casualmente 
	
	//flog(LOG_WARN, "Gestione nome casuale"); 

	jiffies=random(); 
	i =  jiffies & 0xffff;
	sz = (jiffies >> 16) & 0x7;
	//flog(LOG_WARN, " i RANDOME :%d %x",i,i);  
	//flog(LOG_WARN, " sz RANDOME :%d %x",sz,sz);  
	if (baselen>2) {
		baselen = numtail2_baselen;
		name_res[7] = ' ';
	}
	name_res[baselen+4] = '~';
	name_res[baselen+5] = '1' + sz;
	i=i<<16; 
 	while (1) {
		sprintf((char*)buf, "%X", i);
		//flog(LOG_DEBUG, "BUF :%s NAME %s", buf, name_res); 
 		memcpy(&name_res[baselen], buf, 4);
 		if (search_entry_short(volume, dir,(const char*) name_res, NULL) < 0 )
		  break;
		i -= 11;
 	}



	return TRUE;


}




/*Funzione che valuta la correttezza di un nome */ 


BOOL valid_longname(const char *name, int len) {

	if ((len && name[len-1] == ' ') || len >= 256 ||  len < 1 )  {
	  set_errno(EINVAL, "Parametro non corretto"); 
	  return FALSE;
	}

	// Bisogna verificare con quale carattere inizia il nome
	if ( *name == '.' )  {
	        set_errno(EINVAL, "Nome non permesso");
		return FALSE;
	}
	
	  
	reset_errno(); 
	return TRUE;
}


  

BOOL to_msdos_time ( dword sys_time, msdos_time *time ) { 
  
    
    time->Second=((sys_time & 0x000000FF)&0x1F) / 2;  // coppie di secondi)
    time->Minutes=((sys_time & 0x0000FF00) >> 8)&0x1F; 
    time->Hours=((sys_time & 0x00FF0000) >> 16)&0x1F; 
    
    return TRUE; 
}

BOOL to_msdos_date ( dword sys_date, msdos_date * date) {

	date->Day=(sys_date & 0x000000FF)&0x1F; // giorno prelevo 5 bit
	date->Month=((sys_date & 0x0000FF00) >> 8)&0x0F; //mese 4 bit
	date->Years=(((sys_date & 0x00FF0000) >> 16)&0xEF)+20; //anno 2000 (msdos 1980)
  
      return TRUE; 
}


/************************************************************************
 * Funzione che cerca un entry all'interno di una cartella individuata  *
 * da chain , analizzando solamente lo spazio dei nomi corto. 		*
 * Il nome da cercare deve essere un nome corto ( 11 byte ). 		*
 *  LABEL : individua il volume nel quale effettuare la ricerca 		*
 *  CHAIN : individua il primo cluster della catena nel quale 		*
 *	    effettuare la ricerca 					*
 *  Name  : nome corto da cercare 					*
 * Short_entry :puntatore ad una struttura short_entry in cui inserisce *
 * 		i dati se vengono trovati.				*
 * Se trova il nome ritorna l'offset della entry, altrimenti riporta 	*
 * -1. Short Entry deve essere precedentemente allocata!							* 
 ************************************************************************/  

int search_entry_short ( VOL_PTR volume, dword chain , const char * name , SHORT_ENTRY * short_entry ) {
  
  int n_read=0; 
  byte *buf=NULL;   // SIZE BUF multiplo di 32
  int offset=0; 
  BOOL end=FALSE;
  int r=-1, i=0, lun=0; 
  char name_msdos[MSDOS_NAME]; 
  int size_name=0; 
  dword size_cluster=0; 

  
  if ( !volume || !name  || chain < 2 ) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return -EINVAL;
  }
  
   size_cluster=(volume->fat_info.sectors_for_cluster * volume->fat_info.byts_for_sector); 
  
  if(!(buf=mem_alloc(size_cluster,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return -ENOMEM; 
  }

  
  memset(name_msdos,' ', MSDOS_NAME); 
  strncpy(name_msdos, name, size_name); 

  size_name=strlen(name); 
  
  if ( size_name > MSDOS_NAME ) 
      lun=MSDOS_NAME; 
  else 
     lun=size_name; 
   
   // formato il nome 
   for(i=0; i< lun; i++) 
      name_msdos[i]=name[i]; 
  
  
  
  // leggo finchè  non finisce la cartella 
  while(!end && (n_read=read_data(volume, chain,offset,buf, size_cluster))) {
    
    int i=0, n=n_read/SIZE_ENTRY; 
    SHORT_ENTRY *entry=(SHORT_ENTRY *)buf; 
 
    
    if (n%SIZE_ENTRY) { 
	mem_free(buf); 
	set_errno(EIO, "Errore lettura/scrittura (%s-line%d)", __FILE__, __LINE__); 
	return -EIO; 
    }
    
    // analizziamo le varie entry saltando quelle lunghe
    for ( i=0; i<n; i++, entry++) { 
	
	// verifichiamo che l'entrata sia libera o non c'è ne siano piu 
	if (entry->DIR_Name[0] ==  FREE)	// entry libera
		continue;
	if ( entry->DIR_Name[0] ==  ALL_FREE){ // non ci sono piu entry 
		end=TRUE; 
		r=-1; 		// non è presente 
		break; 
	}
	     
	if (entry->DIR_Attr == ATTR_LONG_NAME)  
	    continue;
	

	if(cmp_msdos_name((const char *)entry->DIR_Name, (const char *)name_msdos)) { 
	    end=TRUE; 
	    if (short_entry)
		  memcpy(short_entry, entry, SIZE_ENTRY);    
	    r=offset+i*SIZE_ENTRY; 
	    break;
	}    
    } 
    offset+=n_read; 
    
  }
  
  
  mem_free(buf); 
  return r;
  
}

/* FUNZIONE*****************************************************************
 * Analizza lo spazio dei nomi lungo, di una cartella cercando un entry    *
 * che abbia un nome uguale a quello passato per argomento. 		   *
 *	VOLUME 	  : volume del disco 					   *
 *	CHAIN     : primo cluster della directory  			   *
 * 	NAME 	  : nome in formato UNICODE da cercare 			   *
 * 	NAME_FIND : indirizzo nel quale dobbiamo inserire il nome trovato  * 
 * Riporta,l'offset all'interno della cartella se è presente, altrimenti  *
 * riporta -1.   	
 short_entry deve essere alloca, mentre long_entry viene allocata dalla funzione*
 ***************************************************************************/

 
int  search_entry_long  ( VOL_PTR 		volume ,		// puntatore al volume 
			  dword 		chain ,			// cluster cartella 
			  const char * 		name_search,		// nome da cercare (ASCII)
			  char **		name_find,		// nome trovato    (ASCII)
			  SHORT_ENTRY *		s_entry, 		// short entry
			  LONG_ENTRY **		l_entry, 		// l_entry (alloca questa funzione) 
			  word 	*		n_entry_long )		// lunghezza
  {
 
  int n_read=0; 
  int offset_old=0; // mi serve per tenere traccia dell'offset nel caso di nomi lunghi  
  byte *buf=NULL;   // SIZE BUF multiplo di 32
  long long int offset=0; 
  BOOL end=FALSE;
  int size_name_search=0,r =-1;
  wchar *name_read=NULL; 
  dword size_cluster=0; 
  
  LONG_ENTRY *buffer_long_entry=NULL; 	// variabile che contiene tutte le entry lunghe lette
  SHORT_ENTRY *buffer_short_entry=NULL;	// variabile che contiene tutte le entry corte lette 
  LONG_ENTRY * buffer_long_entry_tmp=NULL; 
  int total_entry_long=0; 
  
  wchar *name= NULL; 
  
  reset_errno(); 
    
  if ( !name_search )  { 
    set_errno(EINVAL, "Errore parametri (%s-line%d)", __FILE__, __LINE__); 
    return -EINVAL; 
   } 
  
  if(name_find)
    *name_find=NULL; 
  
  size_name_search=strlen(name_search); 
  
  if ( size_name_search <= 0 )  { 
    set_errno(EINVAL, "Errore parametri (%s-line%d)", __FILE__, __LINE__); 
    return -EINVAL; 
  }
    
  if(!(name=mem_alloc((size_name_search+1)*2,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  wmemset(name, 0, size_name_search+1);
  char2uni(name, name_search, size_name_search); 
  
  if(!(buffer_long_entry=mem_alloc(MAX_LONG_ENTRY*SIZE_ENTRY, 1))) { 
    set_errno(ENOMEM, "Errore mem_alloc (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  if(!(buffer_short_entry=mem_alloc(SIZE_ENTRY,1))) { 
    set_errno(ENOMEM, "Errore mem_alloc (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  if(!(name_read=mem_alloc(MAX_NAME*2, 1))) { 
    set_errno(ENOMEM, "Errore mem_alloc (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
 size_cluster=(volume->fat_info.sectors_for_cluster * volume->fat_info.byts_for_sector); 
  
  
  if(!(buf=mem_alloc(size_cluster,1))) { 
    set_errno(ENOMEM, "Errore mem_alloc (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  // buffer temporaneo che uso per scorrere il buffer delle entry lunghe 
  buffer_long_entry_tmp=buffer_long_entry; 

  memset(name_read, 0, MAX_NAME*2); 
  memset(buf, 0, SIZE_BUF); 
  memset(buffer_long_entry,0,MAX_LONG_ENTRY*SIZE_ENTRY); 
  memset(buffer_short_entry,0, SIZE_ENTRY); 


  while( !end && (n_read=read_data(volume, chain,offset,buf,size_cluster))) {
    
    int i=0, n=n_read/SIZE_ENTRY; 
    LONG_ENTRY *entry=(LONG_ENTRY *)buf; 
    byte *buf_tmp=NULL; 
    
    if (n%SIZE_ENTRY || n_read<=0 ) { 
 	mem_free(buf); 
	mem_free(name_read); 
	flog(LOG_WARN,"Errore Lettura n %d n_read %d", n, n_read ); 
	set_errno(EIO, "Errore lettura/scrittura (%s-line%d)", __FILE__, __LINE__); 
 	return -EIO; 
     }
    
   // analizziamo le varie entry saltando quelle corte
   for (i=0; i<n;) { 
begin_for:
      total_entry_long=0; 
      buffer_long_entry_tmp=buffer_long_entry; 
 
      if (entry->LDIR_Ord==ALL_FREE) {   
	  end=TRUE; 
	  r=-1;
	  break; 
      } else if ( entry->LDIR_Ord== DELETED_FLAG || entry->LDIR_Attr != ATTR_LONG_NAME) { 
	i++; entry++; 
	continue; 
      } else if (entry->LDIR_Attr == ATTR_LONG_NAME && (entry->LDIR_Ord & MASK_LAST_NAME)) {  

	  int j=entry->LDIR_Ord & 0x3F; 	// prelevo il numero di entry     
	  int ap=0;				// variabile che contiene le restanti entry da analizzare 
	  total_entry_long=j; 			// totale numero entry lunghe
	
	  offset_old=offset+i*SIZE_ENTRY; 	// salvo l'offset della prima entry

	  if (  j< 1 || j > 20 ){// errore 
		mem_free(name_read);
		mem_free(buf); 			// ci sono piu valori da liberare 
		set_errno(EIO, "Errore lettura/scrittura (%s-line%d)", __FILE__, __LINE__); 
		return -1; 
	    }
	      
	  memset(name_read, 0, MAX_NAME*2); 
	     
	      if ( (i+j) < n) {
		int count=j; 
		  for (; j>0; i++, j--, entry++,buffer_long_entry_tmp++, count=entry->LDIR_Ord)  {
		    if (entry->LDIR_Attr == ATTR_LONG_NAME && count == j) {
			  append_name(entry,name_read);
			  memcpy(buffer_long_entry_tmp, entry, SIZE_ENTRY); // coppio il buffer nel buffer temporaneo 
		      } else {
			  flog(LOG_WARN, "Entry corrotta %x", offset+(i-1)*SIZE_ENTRY);
			  break; 
		      }
		 }

	      } else { 
		int count=j; 
		ap=n-i; // positivo 
		for (; ap>0; ap--, j--, entry++, buffer_long_entry_tmp++,count=entry->LDIR_Ord) {
 		  if (entry->LDIR_Attr == ATTR_LONG_NAME && count == j) {
			  append_name(entry,name_read);
			  memcpy(buffer_long_entry_tmp, entry, SIZE_ENTRY); // coppio il buffer nel buffer temporaneo 
		      } else {
			 flog(LOG_WARN, "Entry corrotta %x", offset+(i-1)*SIZE_ENTRY);
			goto begin_for;  
		      }
		}

		offset+=n_read;
		memset(buf, 0, SIZE_BUF); 
		
		n_read=read_data(volume, chain,offset,buf, size_cluster); 
		 
		if ( n_read <= 0 ) {
		    end=TRUE; 
		    r=-1; 
		    break; 
		}
		 
		buf_tmp=buf; 
		entry=(LONG_ENTRY*)buf; 
		ap=j; // parti da leggere 
		count=j; // perche' viene inizializzato a un valore errato
		i=0; 
		for (; ap>0; ap--, entry++, i++, buffer_long_entry_tmp++,count=entry->LDIR_Ord) {
		     if (entry->LDIR_Attr == ATTR_LONG_NAME && count == ap) {
			  append_name(entry,name_read);
			  memcpy(buffer_long_entry_tmp, entry, SIZE_ENTRY); // coppio il buffer nel buffer temporaneo 
		      } else {
			  flog(LOG_WARN, "Entry corrotta  j= %d count=%d", ap, count); 
			  break; 
		      }
		 } 
	      } 
	      
	   }else {
		  entry++; i++;
		  continue;
	   }

	  if(!wcsicmp(name, name_read)) { 
	     int size_entry_long=0;
	//    flog(LOG_DEBUG, "Coincide "); 
	    if(name_find) {
	      *name_find=mem_alloc(SIZE_NAME,1); 
	      memset(*name_find,0, SIZE_NAME); 
	      uni2char(name_read,*name_find, wcslen(name_read)); 
	    }
	    
	    if(l_entry) {
	      size_entry_long=(SIZE_ENTRY*total_entry_long);
	      *l_entry=mem_alloc(size_entry_long,1); 
 	      memset((LONG_ENTRY*)(*l_entry),0, size_entry_long); 
 	      memcpy((LONG_ENTRY*)(*l_entry),buffer_long_entry,size_entry_long); 
	      *n_entry_long=total_entry_long;
	    }
	    
	    if (s_entry) {
		memset((s_entry),0, SIZE_ENTRY); 
		  if (entry->LDIR_Attr == ATTR_LONG_NAME){
		      flog(LOG_WARN, "ERRORE SHORT ENTRY");
		      set_errno(EINVAL, "Non presente entry corta"); 
		      r=-1; 
		      goto end;
		  }else 
		      memcpy((s_entry),entry,SIZE_ENTRY); 
	    }
	    
	    r=offset_old; 
	    end=TRUE; 
	    break; 
	  }
	  
	}
	 offset+=n_read; // aggiorno l'offset
  }
  




end:  
  mem_free(buffer_long_entry); 
  mem_free(buffer_short_entry); 
  mem_free(name_read);
  mem_free(buf); 
  return r;
  
}



void print_short_entry(SHORT_ENTRY * s) {
  
  char name[12]; 
  
  memset(name,0, 12); 
  strncpy(name, (const char *)s->DIR_Name, MSDOS_NAME); 
  
  flog(LOG_DEBUG,"Name       %s",name);  
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
   
   uni2char(l->LDIR_Name1, Name1, SIZE_UNO); 
   uni2char(l->LDIR_Name2, Name2, SIZE_DUE); 
   uni2char(l->LDIR_Name3, Name3, SIZE_TRE);
   
   
   flog(LOG_DEBUG, "ID    %x", l->LDIR_Ord);
   flog(LOG_DEBUG, "Name  %s", Name1); 
   flog(LOG_DEBUG, "Attr  %x", l->LDIR_Attr);
   flog(LOG_DEBUG, "Type  %x", l->LDIR_Type); 
   flog(LOG_DEBUG, "CHCK  %x", l->LDIR_Chksum); 
   flog(LOG_DEBUG, "Name2 %s", Name2); 
   flog(LOG_DEBUG, "FstLO %x", l->LDIR_FstClusLO); 
   flog(LOG_DEBUG, "Name3 %s\n", Name3); 
}




void print_directory ( VOL_PTR label, dword chain ) {
  
  flog(LOG_DEBUG, "DIRECTORY %d\n", chain);
  int n_read=0; 
  byte *buf=mem_alloc(SIZE_BUF,1);   // SIZE BUF multiplo di 32
  lword offset =0; 
  BOOL end=FALSE; 
  
  while(!end && (n_read=read_data(label, chain,offset,buf, SIZE_BUF))) { 
    
    int i=0, n=n_read/SIZE_ENTRY; 
    SHORT_ENTRY *entry=(SHORT_ENTRY *)buf; 
    offset+=n_read; 
    
    if (n%SIZE_ENTRY) { 
	mem_free(buf); 
	return ; 
    }
    
    
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

extern FCB *root; 


/* Se presente riporta vero */ 


BOOL search_name ( VOL_PTR volume, dword cluster, const char * name) {

    BOOL r=FALSE;
    SHORT_ENTRY short_entry; 
    LONG_ENTRY *long_entry=NULL;
    word n_entry=0; 

    
    if ( !volume || !name || cluster < 2  ) {
      set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
      return FALSE;
    }
 
    
    
  
   if (search_entry_long(volume, cluster, name, NULL,&short_entry,&long_entry,&n_entry) >=0) 
	r=TRUE; 
  
    r= r && test_checksum(&short_entry, long_entry, (int)n_entry); 

    if ( !r &&  search_entry_short(volume, cluster, name, NULL) >=0) 
	r=TRUE; 

	return r; 
}


BOOL is_dir_empty(FCB* dir ) { 

  byte* buf=NULL; 
  VOL_PTR vol=NULL; 
  dword size_cluster=0; 
  SHORT_ENTRY * entry=NULL; 
  int r=0, i=0,n=0; 
  int offset=0; 
  
  if ( !dir) { 
    set_errno(EINVAL,"Errore paramenti"); 
    return FALSE; 
  }

  if ( dir->type!=ATTR_DIRECTORY || dir->cluster < 2 ) { 
     set_errno(ENOTDIR,"Bad directory"); 
     return FALSE; 
  }

  vol=dir->volume;
  size_cluster=vol->fat_info.byts_for_sector*vol->fat_info.sectors_for_cluster; 

  
  if(!(buf=mem_alloc(size_cluster,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }

 offset=64; 

  while ( (r=read_data(vol,dir->cluster, offset,buf, size_cluster)) > 0 ) { 
	n=r/SIZE_ENTRY; 
	offset+=r; 
	entry=(SHORT_ENTRY*)buf; 
	for ( i=0; i<n; i++, entry++) 
		if ( entry->DIR_Name[0] != ALL_FREE && entry->DIR_Name[0] !=FREE) {
		    mem_free(buf); 
		    return FALSE; 
		}
    }
  
  return TRUE; 
  
}

/****************************************************************
 * create entry crea un entry ed inizializza il FCB della entry.*
 * Effettua una ricerca del nome prima nello spazio dei nomi    *
 * lungo e poi in quello corto. 				*
 * Se l'entry non è presente effettua la creazione.		*
 * NAME   : nome del file da aprire				*
 * FATHER : direnty nella quale creare l'entry.			*
 * NEW    : FCb nel quale inserisce le informazione della 	*
 *	    nuova entry. 					*
 * Return TRUE, se è stato possibile creare l'entry, altrimenti *
 * ritorna false e setta opportunamente errno.			*
 ****************************************************************/



BOOL create_entry(const char * name , byte type, dword first_cluster, dword size_file, const FCB * father, FCB * new){ 
  
  char name_short[MSDOS_NAME]; 
  byte *buf=NULL ; 
  int off=0;
  dword free_entry=0, size=0; 
  TABELLA_VOLUMI * current_vol=NULL; 
  
  reset_errno();
  
  if ( new == NULL || father==NULL || !name) { 
    set_errno(EINVAL,"Errore paramenti"); 
    return FALSE; 
  }

  if ( father->type!=ATTR_DIRECTORY || father->cluster < 2 ) { 
     set_errno(ENOTDIR,"Bad directory"); 
     return FALSE; 
  }

  if ( first_cluster ==2 || first_cluster==1 || first_cluster>=BAD_CLUSTER_32) {
     set_errno(EINVAL,"Errore cluster"); 
     return FALSE; 
  }

  if (!valid_longname(name, strlen(name))) {
     perror("Nome non valido"); 
     set_errno(EINVAL, "Errore nome non valido"); 
     return FALSE; 
  }
  
  
  if (!(current_vol=(father->volume))) { 
    set_errno(EINVAL, "Volume non valido"); 
    return FALSE; 
  }
    
  // verifico che non sia un nome gia in uso 
  if(search_name(father->volume, father->cluster, name)) { 
    set_errno(EEXIST, "File exist %s", name);
    return FALSE; 
  }
  
  free_entry= get_n_entry(name); 
  size=free_entry*SIZE_ENTRY; 
  
  if(!(buf=mem_alloc(size,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }


/********************************* BUFFER ***********************************/ 

  memset((void*)name_short,0, MSDOS_NAME); 
 
  if(!create_shortname(father->volume, father->cluster, name, strlen(name), name_short)) { 
    mem_free(buf); 
    return FALSE; 
  } /*else */
//     flog(LOG_WARN, "NOME CORTO : %s", name_short); 
    
  format_entry(name, name_short, type, first_cluster, size_file,  buf); 

#ifdef DEBUG_FS 
  {
      SHORT_ENTRY*s=(SHORT_ENTRY*)buf; 
      int i=0; 
      
      flog(LOG_DEBUG, "*************************** INFO %s ******************* ", name);

      for ( i=0; i<free_entry; i++, s++)
	if (s->DIR_Attr==ATTR_LONG_NAME)
	      print_long_entry((LONG_ENTRY*)s);
        else
	      print_short_entry((SHORT_ENTRY*)s);

      flog(LOG_DEBUG, "******************************************************* ");

    
  }
#endif
//******************************** SCRITTURA ********************************/ 
  
  sem_wait(current_vol->sem_data); 
  
  off=get_free_entry(father->volume, father->cluster,free_entry); 
    
    if((int)off < 0) { 
      perror("get_free_entry"); 
      set_errno(EIO, "Impossibile trovare entry libera"); 
      mem_free(buf); 
      sem_signal(current_vol->sem_data); 
      return FALSE; 
    }
    
    
 
  
          if(!write_data(father->volume, father->cluster, off, buf, size))  {
            perror("ERRORE write"); 
	    set_errno(EIO, "Impossibile scrivere entry libera");
	    sem_signal(current_vol->sem_data); 
	    return FALSE; 
          }
     
  sem_signal(current_vol->sem_data); 
  

  
  // CREAZIONE FCB 
//********************************** FCB ***********************************/

  
  memset(new,0, sizeof(FCB)); 
  

  new->offset_father=off; 		// offset all'interno del padre
  new->cluster_father=father->cluster; 	// cluster del padre 
  new->volume=father->volume; 		// volume 
  new->cluster=first_cluster; 			// primo cluster  
  new->size=size_file; 				// grandezza 
  new->type=type; 			// ATTR di tipo 
  new->n_entry=free_entry; 		// n_entry


  mem_free(buf); 
  reset_errno();
  return TRUE; 
}

// funzione che testa se la entry lunga e associata alla giusta entry corta */


BOOL test_checksum ( const SHORT_ENTRY * s, const LONG_ENTRY * l, int n_entry_long) { 
  
  byte checkSum=0; 
  int i=0; 
  
  if ( !s ||  !l || n_entry_long < 0 || n_entry_long > 20)
    return FALSE; 
  
  // calcolo il checksum 
  checkSum=check_sum(s->DIR_Name);
  
  for ( i=0; i<n_entry_long;i++) 
      if(l->LDIR_Chksum!=checkSum) 
	  return FALSE; 
  
  return TRUE; 
}



/****************************************************************
 * Open entry apre un entry ed inizializza il FCB della entry.  *
 * Effettua una ricerca del nome prima nello spazio dei nomi    *
 * lungo e poi in quello corto. 				*
 * Una volta individuato il nome formata il fcb.		*
 * NAME   : nome del file da aprire				*
 * FATHER : fcb nel quale iniziare la ricerca. 			*
 * NEW    : FCb nel quale inserisce le informazione della 	*
 *	    nuova entry. 					*
 * Return TRUE, se è stato possibile aprire l'entry, altrimenti *
 * ritorna false e setta opportunamente errno.			*
 ****************************************************************/



BOOL open_entry(const char * name , const FCB * father, FCB * new) {

   dword size=0;
   lword offset=0; 
   int  n_entry=0, i=0, size_cluster=0; 
   char *name_read=NULL;
   int offset_short_entry=0, offset_long_entry=0; 
   wchar 	*name_unicode=NULL; 			// nome da cercare
   SHORT_ENTRY   *short_entry; 	
   LONG_ENTRY   *long_entry=NULL; 
   word		 n_entry_long=0; 
   BOOL          isLong=FALSE; 
   
   if ( !name || !father || !new  ) {
     set_errno(EINVAL,"Parametro invalido (%s-line%d)", __FILE__, __LINE__); 
     return FALSE;
   }
   
   // controlli sul FCB del padre 
   
   if ( father->type!=ATTR_DIRECTORY || father->cluster < 2  ) { 
      set_errno(EINVAL, "FCB father incoretto (%s-line%d)", __FILE__, __LINE__); 
      return FALSE; 
   }
     
   // lunghezza del nome della entry da cercare 
   size=strlen(name);
   
   if(!(name_unicode=mem_alloc((size+1)*2,1))) { 
     set_errno(ENOMEM, "Errore mem_alloc (%s-line%d)", __FILE__, __LINE__); 
     return FALSE; 
   }
   
   if(!(short_entry=mem_alloc(SIZE_ENTRY,1))){
     set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
     return FALSE; 
   }
   


   
   /******************************* CERCO LE ENTRY NEL DISCO **********************/


   //cerco l'offset della direntry lunga  ( l'offset e' espresso mediante un int )
   offset_long_entry=search_entry_long(father->volume, father->cluster, name, &name_read, short_entry, &long_entry, &n_entry_long); 
   

   
   if ( offset_long_entry < 0)   {
	      // cerco il nome nello spazio dei nomi corto  
 	      if (!errno) 
 		offset_short_entry=search_entry_short(father->volume, father->cluster, name, short_entry); 
 	      else 
 	      { 
		   set_errno(EINVAL, "File corrotto %s", name);
 		   mem_free(name_unicode); 
 		   mem_free(long_entry); 
 		   mem_free(short_entry); 
 		   return FALSE; 
 	      }
   } else 
     isLong=TRUE; 
  
   

   
   if( (offset_short_entry < 0) && (offset_long_entry < 0 )){ 
     set_errno(ENOENT, "Non presente %s", name); 
     mem_free(name_unicode); 
     mem_free(long_entry); 
     mem_free(short_entry); 
     return FALSE; 
   }
 //     print_short_entry(short_entry); 
   
   /***************************** FORMATO I FCB **********************************/
   
   // ora bisogna identificare in quale caso siamo 
   // SHORT_ENTRY o SHORT_ENTRY + LONG_ENTRY
   
   
   if (isLong) {
      offset=offset_long_entry; 
      n_entry=n_entry_long +1;  
    } else {
      offset=offset_short_entry;
      n_entry=1;	
  }

  memset(new, 0, sizeof(FCB)); 

  // parte comune tra entry short e entry long 
  
  //info file 
  new->type=short_entry->DIR_Attr; 
  new->size=short_entry->DIR_FileSize;
  merge_dword(&new->cluster, short_entry->DIR_FstClusLO, short_entry->DIR_FstClusHI); 
  
  // father position 
  new->offset_father=offset; 		// offset all'interno dell cartella padre
  new->cluster_father=father->cluster;  // first cluster father 
  new->volume=father->volume; 		// volume 
  new->n_entry=n_entry;  		// numero entry presenti nel disco, compresa la short 
 
  // gestione nome e path 

    
    if ( isLong && !test_checksum(short_entry, long_entry, n_entry-1 )){ 
	    flog(LOG_WARN, "CHECKSUM FALITO"); 
	    set_errno(ENOENT, "Non presente %s", name); 
	    mem_free(name_unicode);
	    mem_free(long_entry); 
	    mem_free(short_entry); 
	    return FALSE; 
    }
    



      // se e' zero ed e' una directory significa che è la directory di root
      if (new->cluster == 0  && new->type == ATTR_DIRECTORY) 
	memcpy(new, (new->volume->fcb_root), sizeof(FCB)); 
      
      i=1; 
      size_cluster=new->volume->fat_info.byts_for_sector*new->volume->fat_info.sectors_for_cluster; 
      
      
      if ( new->type == ATTR_DIRECTORY) {
	  dword temp=get_next_fat(new->volume->fat, new->cluster); 
	  while ( temp < BAD_CLUSTER_32) {
		 i++; 
		 temp=get_next_fat(new->volume->fat, temp);
		 if(!temp)
		    break; 
	  }
	  new->size=i*size_cluster; 
       }
      
   mem_free(name_unicode);
   mem_free(long_entry); 
   mem_free(short_entry); 
  
  reset_errno(); 
  return TRUE; 
}




/********************************************************
 * Funzione che elimina il file individuato da file.    *
 * file ; FCB del file da eliminare.		        * 
 * Ritorna TRUE se l'eliminazione è avvenuta, altrimenti* 
 * ritorna FALSE e setta l'errore!			*
 ********************************************************/

BOOL delete_entry(const FCB * file) {
  
  word size=0;
  SHORT_ENTRY *entry=NULL;
  byte * buf=NULL;  
  int i=0, r=0; 
  
  if ( file == NULL ) {
    set_errno(EINVAL,"Parametro errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE;
  }
  
  if ( !file->volume || !file->cluster_father || !file->n_entry || file->type ==ATTR_DIRECTORY) { 
    set_errno(EINVAL,"FCb errato (%s-line%d)", __FILE__, __LINE__); 
    return FALSE;
  }
  
  size=file->n_entry*SIZE_ENTRY; 
 
  if(!(buf=mem_alloc(size,1))){
      set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
      return FALSE; 
    }
  
  memset(buf,0, size);
  
  //devo eliminare la catena se  presente  
  if (file->cluster != 0 ) {
      if ( file->cluster <=2) { 
	   set_errno(EINVAL,"Cluster Errato (%s-line%d)", __FILE__, __LINE__); 
	   mem_free(buf); 
	   return FALSE;
      }
      if (!delete_all_fat(file->volume, file->cluster)) {
	   perror("DELETE ALL FAT "); 
	   mem_free(buf); 
	  return FALSE; 
      }
  } 
  
 
   
  if ((r=read_data(file->volume, file->cluster_father,file->offset_father , buf, size)) < 0  ) { 
      perror("Write data"); 
      mem_free(buf); 
      return FALSE; 
    }
  
    entry=(SHORT_ENTRY*)buf;
	for (i=0; i< file->n_entry;i++, entry++)
	    entry->DIR_Name[0]=DELETED_FLAG;  
  

    if ((r=write_data(file->volume, file->cluster_father,file->offset_father , buf, size)) < 0 ) { 
      perror("Write data"); 
      mem_free(buf); 
      return FALSE; 
    }
  
  mem_free(buf); 
  return TRUE; 
}

/********************************************************************
 * Funzione che crea una cartella, del tutto simile ad create_entry.* 
 * L'unica differenza è che crea anche i le entry dot e dotdot.     *
 ********************************************************************/

BOOL create_directory(const char * name,  const FCB * father, FCB * new){ 
  
  char name_short[MSDOS_NAME]; 
  byte *buffer_new_entry=NULL, 
       *cluster=NULL; 
  lword off=0; 
  word free_entry=0;  
  TABELLA_VOLUMI * current_vol=NULL; 
  dword first_cluster=0, size_new_entry=0,size_cluster=0, dotdot=0; 
  
  reset_errno();
  
  if ( new == NULL || father==NULL || !name) { 
    set_errno(EINVAL,"Errore paramenti"); 
    return FALSE; 
  }

  if ( father->type!=ATTR_DIRECTORY ||  father->cluster < 2 ) { 
     set_errno(ENOTDIR,"Bad directory"); 
     return FALSE; 
  }

  flog(LOG_DEBUG, "%s n %d", name, strlen(name)); 
  if (!valid_longname(name, strlen(name))) {
     perror("Nome non valido"); 
     set_errno(EINVAL, "Errore nome non valido"); 
     return FALSE; 
  }
  
  if (!(current_vol=(father->volume))) { 
    set_errno(EINVAL, "Volume non valido"); 
    return FALSE; 
  }
    
  // verifico che non sia un nome gia in uso 
  if(search_name(father->volume, father->cluster, name)) { 
    set_errno(EEXIST, "File exist %s", name);
    return FALSE; 
  }
  
  free_entry= get_n_entry(name); 
  size_new_entry=free_entry*SIZE_ENTRY; 
  
  // alloco il buffer per la nuova entry  
  if(!(buffer_new_entry=mem_alloc(size_new_entry,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  size_cluster=current_vol->fat_info.sectors_for_cluster *current_vol->fat_info.byts_for_sector; 
  // allocco il buffer per il nuovo cluster 
  if(!(cluster=mem_alloc(size_cluster,1))){
    set_errno(ENOMEM,"Errore memoria (%s-line%d)", __FILE__, __LINE__); 
    mem_free(buffer_new_entry);
    return FALSE; 
  }

 /****************** CREAZIONE ENTRY *******************************/ 

  memset((void*)name_short,0, MSDOS_NAME); 
  memset(buffer_new_entry,0, size_new_entry); 
  memset(cluster, 0, size_cluster); 
 
  // creo il nome corto 
  if(!create_shortname(father->volume, father->cluster, name, strlen(name), name_short)) 
      goto end_err_directory; 
//   else 
//     flog(LOG_WARN, "NOME CORTO : %s", name_short); 
  
  // creo la catena di fat 
  if (!create_fat(father->volume, &first_cluster)) { 
     perror("CREATE_FAT"); 
     goto end_err_directory; 
   }else {
      flog(LOG_WARN,"Ho creato %d ",first_cluster); 
   }
  
  // inserisco tutto in un buffer
  format_entry(name, name_short, ATTR_DIRECTORY, first_cluster, SIZE_DIRECTORY, buffer_new_entry);
  
  
  if (father->cluster == 2 ) 	// le cartelle che sono presenti nella directory di root 
      dotdot=0; 		// hanno la carateristica di dotdot=0
  else 
      dotdot=father->cluster; 

  insert_dot(cluster, first_cluster ); // gli offset sono fissi 0 
  insert_dotdot(cluster+SIZE_ENTRY, dotdot); // offset 32 

#ifdef DEBUG_FS 
  {
      SHORT_ENTRY*s=(SHORT_ENTRY*) buffer_new_entry; 
      int i=0; 
      
      flog(LOG_DEBUG, "********************* INFO %s ******************* ", name);

      for ( i=0; i<free_entry; i++, s++)
	if (s->DIR_Attr==ATTR_LONG_NAME)
	      print_long_entry((LONG_ENTRY*)s);
        else
	      print_short_entry((SHORT_ENTRY*)s);

      flog(LOG_DEBUG, "DOT :"); 
      print_short_entry((SHORT_ENTRY*)cluster); 
      flog(LOG_DEBUG, "DOTDOT: ");
      print_short_entry((SHORT_ENTRY*)(cluster+SIZE_ENTRY)); 
	
    flog(LOG_DEBUG, "*************************************************** ");

    
  }
#endif 
  

  
  /****************** SCRITTURA ENTRY *******************************/ 
  sem_wait(current_vol->sem_data); 
  
  
    off=get_free_entry(father->volume, father->cluster,free_entry); 
    
    if((int)off < 0)  {
      flog(LOG_WARN, "NEGATIVO"); 
      goto end_err_directory_sem; 
    }
    
    if(write_data(father->volume, father->cluster, off, buffer_new_entry, size_new_entry) <0)  {
      perror("ERRORE write"); 
      goto end_err_directory_sem; 
    }
    

    if (write_data(father->volume,first_cluster, 0 , cluster, size_cluster) < 0 ) {
      perror("Write Dot & DotDot"); 
      goto end_err_directory_sem; 
    }
  
  
  
  sem_signal(current_vol->sem_data); 
  

/**************************************** FCB ************************************/ 
  
  flog(LOG_DEBUG, "OFFSET RIPORTATO : %d", off); 
  
  // CREAZIONE FCB 

 
  
  memset(new,0, sizeof(FCB)); 



  new->offset_father=off; 		// offset all'interno del padre
  new->cluster_father=father->cluster; 	// cluster del padre 
  new->volume=father->volume; 		// volume 
  new->cluster=first_cluster; 			// primo cluster  
  new->size=0; 				// grandezza 
  new->type=ATTR_DIRECTORY; 			// ATTR di tipo 
  new->n_entry=free_entry; 		// n_entry

  
  mem_free(cluster); 
  mem_free(buffer_new_entry); 	
  reset_errno();
  return TRUE;

end_err_directory_sem: 
    sem_signal(current_vol->sem_data); 
end_err_directory: 
    mem_free(cluster); 
    mem_free(buffer_new_entry); 
    
  return FALSE; 
}
 
 /**************************************************************************************
 * Funzione che elimina cartella individuata da fcb. Controlla se la cartella è libera *
 * prima di procedere all'eliminazione.						       *
 ***************************************************************************************/
 
 
BOOL delete_directory(FCB*fcb) {
  
  if (!fcb) { 
    set_errno(EINVAL,"Errore paramenti"); 
    return FALSE; 
  }

  if ( fcb->type!=ATTR_DIRECTORY ||  fcb->cluster <= 2 ) { 
     set_errno(ENOTDIR,"Bad directory"); 
     return FALSE; 
  }
  
  if (!is_dir_empty(fcb)) {
    set_errno(EIO, "Directory is not empty"); 
    return FALSE; 
  }
  
  fcb->type=ATTR_ARCHIVE; 
  
  return delete_entry(fcb); 
  
}
  

/**********************************************************
 *Rename, cambia il nome della entry individuata da old   *
 * con il nome specificato da name.			  *
 * Father individua l'entry nella quale è posizionata old.*
 *********************************************************/
BOOL rename_entry(const char*name, FCB*father,  FCB* old) {
  
  dword size=0; 
  dword cluster =0; 
  byte type=0; 
  FCB new; 
  
  if ( !name || !old || !father) { 
    set_errno(EINVAL,"Errore paramenti (%s-line%d)", __FILE__, __LINE__); 
    return FALSE; 
  }
  
  
  
  memset(&new, 0, sizeof(FCB)); 
  if ( old->type != ATTR_DIRECTORY ) 
      size=old->size; 
  else 
      size=0; 
  
  cluster=old->cluster; 
  type=old->type; 
  
  
  
  if (!create_entry(name, type, cluster, size, father, &new)) { 
      return FALSE; 
  }
  
  old->cluster=0; 
  old->type=ATTR_ARCHIVE; 
  
  if (!delete_entry(old)) {
	perror("Delete entry"); 
	set_errno(EIO, "Impossibile cancellare il file"); 
	return FALSE; 
   }
   
   memset(old, 0, sizeof(FCB)); 
   memcpy(old, &new, sizeof(FCB)); 
   
   return TRUE ; 
}
 