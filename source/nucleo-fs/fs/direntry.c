#include "direntry.h" 
#include "string.h" 
#include "errno.h"
#include "data.h"
#include "fat.h" 
#include "sistema.h"
#include "wchar.h"
#include "fs.h"


/*FUNZIONI DI INTERFACIA*/ 

// crea un entry sulla cartella father e salva sul contenuto su new che viene creato
BOOL create_entry(const char * name , byte type,  const FCB * father, FCB ** new);  
// crea new e carica i valori della direntry cercata
BOOL open_entry(const char * name , const FCB * father, FCB ** new);  
//elimina direnetry
BOOL delete_entry( const FCB *);  




int _ii_;

#define print(buf,size) for(_ii_=0; _ii_ < size; _ii_++) flog(LOG_DEBUG,"%d %x", _ii_, buf[_ii_]);

//macro che unisce due word
#define merge(hi,lo) ((dword)hi<<16 | (dword)lo)

/*PRIVATE*/
BOOL create_short_entry( const char * name, SHORT_ENTRY * entry); 
BOOL create_long_entry(const char * name, LONG_ENTRY *entry);
BOOL to_msdos_time ( dword sys_time, msdos_time *time );
BOOL to_msdos_date ( dword sys_date, msdos_date *date );
BOOL search_entry( dword chain, const  char * name); 
long int search_entry_short ( byte label , dword chain , const char * name );
long int search_entry_long  ( byte label, dword chain, const wchar* name, char** );
int create_shortname(byte label, dword dir, const char* uname, int ulen, char* name_res);
BOOL search_name ( byte volume, dword cluster, const char * name);


BOOL format_short_entry( const char * name, SHORT_ENTRY * entry, byte type);

BOOL format_long_entry(const char* name, LONG_ENTRY* entry, byte n, byte chkSum);




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

byte check_sum ( const  byte * name) { 
  
 short i=0; 
 byte sum=0; 
 
 for ( i=MSDOS_NAME; i !=0; i--) 
      sum= (( sum & 1) ? 0x80 : 0) + ( sum >> 1) + *name++; 
 
 flog(LOG_DEBUG, "CHECKSUM : %x", sum); 
 
 return sum; 
}






/*FUNZIONE  che formata un buffer pronto per la scruittura sul disco.
 *Alloca il buffer in memoria dinamica  questo buffer è lo riporta mediante buf*/

BOOL  format_entry (const char *long_name, const char * short_name, byte type, byte ** buf ) { 
 
  word n_entry=0; 
  LONG_ENTRY *l=NULL; 
  word i=0,n=0; 
  char *tmp=NULL;  
  byte chkSum=0; 
  n_entry=get_n_entry(long_name); // prelevo il numero di entrate
  *buf=mem_alloc(n_entry*SIZE_ENTRY, 1); // alloco lo spazio per le strutture
  l=(LONG_ENTRY*)*buf; 
  
  n_entry--;//elimino l'entry short  
  i=n_entry;
  n=strlen(long_name); 
  tmp=(long_name+n); // ultimo
  tmp=tmp-(n%SIZE_NAME_PART); // punto all'elemento da copiare
  
  chkSum=check_sum((byte*)short_name); 
  
  
  for (;n_entry>0; n_entry--, l++, tmp-=SIZE_NAME_PART) 
    format_long_entry(tmp,l, (i==n_entry)? (n_entry|MASK_LAST_NAME):n_entry, chkSum); 
  
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
  
//  flog(LOG_WARN, "Name %s %d %x",name, n, chkSum); 
  
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
    
    if ( size < SIZE_DUE & !salta ){
        char2uni(entry->LDIR_Name1, name1, SIZE_UNO);
	char2uni(entry->LDIR_Name2, name2, size);
	entry->LDIR_Name2[size]=0x0000; 
	salta=TRUE; 
    }
  
    size-=SIZE_DUE; 
    
    if ( size < SIZE_TRE &!salta){
	char2uni(entry->LDIR_Name1, name1, SIZE_UNO);
	char2uni(entry->LDIR_Name2, name2, SIZE_DUE);
	char2uni(entry->LDIR_Name3, name3, size);
	entry->LDIR_Name3[size]=0x0000; 
	return TRUE; 
    }
     
    
    if ( size = SIZE_TRE & !salta){
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
 * Questa funzione trasforma da unicode as asci (7 bit) il nome salvato
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
  

  // se non è il primo 
  if(!(entry->LDIR_Attr & MASK_LAST_NAME)) {
    memset(buf, 0, SIZE_NAME*2); 
    wcsncpy(buf,n, s);	//salvo il nome 
    wmemset(n, 0,MAX_NAME); 	// reset name 
    wcsncpy(n+j,buf,s); 
  }
  
 wcsncpy(n,name,j);
 //print_w(buf); 
 //flog(LOG_WARN, "Lunghezza %d", wcslen(n));

  return;
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
/*FUNZIONE CHE ANLISZZA I CARATTERI */ 

inline int to_shortname_char( char *buf, char *src,
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
/*	
 * Se è un carattere unicode non esprimibile in asci
 * Elimino questa condizione tanto non si puo verificare 
 * 
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
		if (isalpha(buf[0])) {
		  // ho avuto una conversione 
		  
		
		  }*

	} else 
	  *info=FALSE; 
	*/

	buf[0]=(char)*src; 
	
	//se è un carattere non esprimibile in ASCII BASE 
	// lo sostituisco???
	if (buf[0] >= 0x7F) {
		*info = FALSE;
 		buf[0]='_';   //<-------------------- MIA AGGIUNTA 
	}
	 buf[0] = UPPER(buf[0]);
		
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
 
	        chl = to_shortname_char(charbuf,ip, &base_info);

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

	/*
	 *Se shortaname e la base e l'estensione  del nome sono true controllo che 
	  non esista gia in il nome se non esiste ritorno true 
	  BASE INFO : true se non ci sono caratteri non  validi 
	  EXT INFO  :  true se non ci sono caratteri non  validi
	 */
	if (is_shortname && base_info && ext_info) {
	  	if (search_name(label, dir,(const char*) name_res)) {
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
		name_res[baselen+1] = i + '0';
		flog(LOG_WARN, " %s %d ", name_res, i); 
		if (search_entry_short(label, dir,(const char*) name_res) <0) 	//significa che non è presente
		  return TRUE;
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

	return TRUE;


}




/*Funzione che valuta la correttezza di un nome */ 


BOOL valid_longname(const char *name, int len)
{
	const char **reserved, *walk;
	int baselen;

	if ((len && name[len-1] == ' ') || len >= 256 ||  len < 3 )  {
	  set_errno(EINVAL, "Parametro non corretto"); 
	  return FALSE;
	}

	// Bisogna verificare con quale carattere inizia il nome
	if ( *name == '.' )  {
	    set_errno(EINVAL, "Nome non permesso");
		return FALSE;
	}
	
	  
	// nomi speciali 
	for (walk = name; *walk != 0 && *walk != '.'; walk++);
	baselen = walk - name;

	if (baselen == 3) {
		for (reserved = reserved3_names; *reserved; reserved++) {
			if (!strnicmp(name,*reserved,baselen))
				set_errno(EINVAL, "Nome non permesso");
				return FALSE;
		}
	} else if (baselen == 4) {
		for (reserved = reserved4_names; *reserved; reserved++) {
			if (!strnicmp(name,*reserved,baselen))
				set_errno(EINVAL, "Nome non permesso");
				return FALSE;
		}
	}
	
	
	return TRUE;
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

long int search_entry_short ( byte label , dword chain , const char * name ) {
  
  int n_read=0; 
  byte *buf=mem_alloc(SIZE_BUF,1);   // SIZE BUF multiplo di 32
  lword offset=0; 
  BOOL end=FALSE;
  long int r=0; 
  
  
  // leggo finché non finisce la cartella 
  while(!end && (n_read=read_data(label, chain,offset,buf, SIZE_BUF))) {
    
    int i=0, n=n_read/SIZE_ENTRY; 
    SHORT_ENTRY *entry=(SHORT_ENTRY *)buf; 
    BOOL long_entry=FALSE; 
 
    
    if (n%SIZE_ENTRY) { 
	mem_free(buf); 
	return -1; 
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
	      
// 	flog(LOG_WARN, "I %d E: %x", i, entry->DIR_Name[0]);  
	     
	if (entry->DIR_Attr == ATTR_LONG_NAME)  
	    continue;
	
	// ho trovato il nome 
	if(!strncmp(entry->DIR_Name, name, SIZE_NAME_SHORT)) { 
	    end=TRUE; 
	    r=offset+i*SIZE_ENTRY; 
	    break;
	}    
    } 
    offset+=n_read; 
    
  }
  
  
  mem_free(buf); 
  return r;
  
}

/*FUNZIONE analizza soltanto lo spazio dei nomi lungo
 deve riportarmi l'offset della cartella fondamentale per un corretto funzionamento 
 */ 

 
long int  search_entry_long  ( byte label , dword chain , const wchar * name, char **name_find ) {
 
  int n_read=0; 
  lword offset_old=0; // mi serve per tenere traccia dell'offsete nel caso di nomi lunghi  
  byte *buf=NULL;   // SIZE BUF multiplo di 32
  long long int offset=0; 
  BOOL end=FALSE;
  lword r =0;
  wchar *name_read=NULL; 
  
  if ( name == NULL) 
    return -1; 
  
  if(name_find)
    *name_find=NULL; 
  
  
  // alloco lo spazio in memoria  
  name_read=mem_alloc(MAX_NAME*2, 1); 
  memset(name_read, 0, MAX_NAME*2); 
  buf=mem_alloc(SIZE_BUF,1); 
  memset(buf, 0, SIZE_BUF); 
  print_w(name); 

  // leggo finché non finisce la cartella 
  while( !end && (n_read=read_data(label, chain,offset,buf, SIZE_BUF))) {
    
    int i=0, n=n_read/SIZE_ENTRY; 
    LONG_ENTRY *entry=(LONG_ENTRY *)buf; 
    BOOL long_entry=FALSE; 
    byte n_char[260];
   
      flog(LOG_WARN, "off : %d n :%d read %d", offset, n , n_read); 

      // fondamentale per una corretta gestione dei nomi 
      // perché il nome si può trovare a cavallo di due letture o più 
      // per far fronte a questa evenienza uso buffer che mi assicurino 
      // che qualsiasi nome sia contenuto in due letture quindi mi servono
      // buffer di minimo 1024 byte 
      
     if (n%SIZE_ENTRY) { 
 	mem_free(buf); 
	mem_free(name_read); 
 	return FALSE; 
     }
    
   // analizziamo le varie entry saltando quelle corte
   for ( i=0; i<n; i++, entry++) { 
  
//     flog(LOG_WARN, " I %d ORD %x ATTR %d" ,i, entry->LDIR_Ord, entry->LDIR_Attr); 
     
	if (entry->LDIR_Ord == FREE) 
	    continue; 
	else if (entry->LDIR_Ord==ALL_FREE) {
	  flog(LOG_WARN, "FINE              FINE ------------------------------FINE "); 
	  end=TRUE; 
	  r=-1;
	  break; 
	} else  if (entry->LDIR_Attr != ATTR_LONG_NAME)  
	    continue; 
	
	
	// codice che anlizza il nome lungo 
	// questo si puo trovare a cavallo tra piu buffer ed cluster 
	// significa che ho trovato l'ultimo elemento di nome lungo 
	if (entry->LDIR_Attr == ATTR_LONG_NAME && (entry->LDIR_Ord & MASK_LAST_NAME)) {  
	    
	  int j=entry->LDIR_Ord & 0x3F; // prelevo il numero di entry     
	  int ap=0;	// variabile che contiene le restanti entry da analizzare 
	    
	    
	      offset_old=offset+i*SIZE_ENTRY; // salvo l'offset della prima entry
  
	//      flog(LOG_WARN, "I %d N %d J %d OLD OFFSET %d BUFFER %d", i, n, j, offset_old,entry); 
	    
	      if (  j< 1 || j > 20 ){// errore 
		mem_free(name_read);
		mem_free(buf); 
		return -1; 
	      }
	      
	      
	      // nel caso in cui il nome si trovi a cavallo di più buffer 
	      // devo fare un ulteriore operazione di lettura , la con-
	      // dizione di sopra ci assicura che il buffer si può trovare 
	      // a cavallo solo due buffer
	      
	      if ( (i+j) < n) { // il long non è a cavallo di due buffer 
		  i+=(j-1); 
		  for (; j>0; j--, entry++)  {
// 		    flog(LOG_WARN, "J %d ", j);
		    append_name(entry,name_read);
// 		    print_w(name_read); 
		  }
	         entry--; // devo controllare tt le entry corte perchè una di queste potrebbe contenere un =x00
// 		  flog(LOG_WARN, "I %d N %d J %d OLD OFFSET %d BUFFER %d", i, n, j, offset_old,entry);
	      }else { 
		
		flog(LOG_WARN,"NOME A CAVALLO DI PIU BUFFER "); 
		
		// long name a cavallo di più buffer 
		// assumo che il disco sia in uno stato consistente 
// 		carico la parte bassa del nome*/
		ap=n-i; // positivo 
		flog(LOG_INFO, "I %d N %d J %d AP %d OLD OFFSET %d", i, n, j, ap,offset_old);
		for (; ap>0; ap--, j--, entry++) //<--------------------- NOTTA che decremento j--
		    append_name(entry,name_read); 
		print_w(name_read); 
		// a questo punto ho solamente una metta del nome 
		    
		 offset+=n_read;
		 n_read=read_data(label, chain,offset,buf, SIZE_BUF);
		  
		 entry=(LONG_ENTRY*)buf; 
		 ap=j; // parti da leggere 
		 for (; ap>0; ap--, entry++) 
		    append_name(entry,name_read);
		 print_w(name_read); 
		 i=ap; 
		 flog(LOG_INFO, "I %d N %d J %d AP %d OLD OFFSET %d ", i, n, j, ap,offset_old);
	      }
	}

	 // flog(LOG_WARN, "off : %d off_name %d", offset, offset+i*SIZE_ENTRY);
// 	  print_w(name_read); 
	  if(!wcsicmp(name, name_read)) { 
	    
	    if(name_find) {
	      *name_find=mem_alloc(SIZE_NAME,1); 
	      uni2char(name_read,*name_find, wcslen(name_read)); 
	    }
	    
	    r=offset_old; 
	    end=TRUE; 
	    break; 
	  }
	  
	}
	 offset+=n_read; // aggiorno l'offset
  }
  
  mem_free(name_read);
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




void print_directory ( byte label, dword chain ) {
  
  flog(LOG_DEBUG, "DIRECTORY %d\n", chain);
  int n_read=0; 
  byte *buf=mem_alloc(SIZE_BUF,1);   // SIZE BUF multiplo di 32
  lword offset =0; 
  BOOL end=FALSE; 
  
  while(!end && (n_read=read_data(label, chain,offset,buf, SIZE_BUF))) { 
    
    int i=0, n=n_read/SIZE_ENTRY; 
    SHORT_ENTRY *entry=(SHORT_ENTRY *)buf; 
    BOOL long_entry=FALSE; 
    offset+=n_read; 
    
    if (n%SIZE_ENTRY) { 
	mem_free(buf); 
	return ; 
    }
    
    
    for ( i=0; i<n; i++, entry++) {
   
   // flog(LOG_WARN, " I %d VALUE %x ", i, entry->DIR_Name[0]); 

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



/* Se presente riporta vero */ 


BOOL search_name ( byte volume, dword cluster, const char * name) {
    
    int s =strlen(name); 
    wchar *name_unicode=mem_alloc((s+1)*2,1);  
    wmemset(name_unicode, 0, s+1);
    char2uni(name_unicode, name, s); 
    BOOL r=FALSE; 
    
    // RICERCA NELLO SPAZIO DEI NOMI LUNGO 
    if (search_entry_long(volume, cluster, name_unicode, NULL) >=0) 
	r=TRUE; 
    flog(LOG_DEBUG, "R %d", r); 
    // RICERCA  NELLO SPAZIO DEI NOMI CORTO 
    if ( !r &&  search_entry_short(volume, cluster, name) >=0) 
	r=TRUE; 
    flog(LOG_DEBUG, "R %d", r); 
    
	if (r)
	  set_errno(EEXIST, "File exist %s", name);
	else 
	  reset_errno(); 
	
	return r; 
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
  SHORT_ENTRY * short_entry=NULL; 
  
  
  if ( new == NULL || father==NULL) { 
    set_errno(EINVAL,"Errore paramenti"); 
    return FALSE; 
  }

  if ( father->type!=ATTR_DIRECTORY) { 
     set_errno(EINVAL,"Bad directory"); 
     return FALSE; 
  }

  if (!valid_longname(name, strlen(name))) {
     set_errno(EINVAL, "Errore nome non valido"); 
    return FALSE; 
  }
  
  // verifico che non sia un nome già in uso 
  if(search_name(father->volume, father->cluster, name))
    return FALSE; 



  memset((void*)name_short,0, MSDOS_NAME); 
 
 
  if(!create_shortname(father->volume, father->cluster, name, strlen(name), name_short)) 
    return FALSE; 
    
 
  format_entry(name, name_short, type, &buf); 
  free_entry= get_n_entry(name); 
 
#ifdef DEBUG 
  {
   SHORT_ENTRY *e=NULL; 
   e=(SHORT_ENTRY*)buf; 
   int i=0, n=get_n_entry(name); 
   
    flog(LOG_DEBUG, "              DEBUG            "); 
    for ( i=0; i<n; i++, e++) {
	if (e->DIR_Name[0] == FREE) 
	    continue; 
	else if (e->DIR_Name[0]==ALL_FREE) {
	  break; 
	}else if (e->DIR_Attr==ATTR_LONG_NAME)  
	    print_long_entry((LONG_ENTRY*)e);
	 else 
	    print_short_entry((SHORT_ENTRY*)e);
    }
    
    
  }
#endif   

  // SCRITTURA NEL DISCO 

  
  sem_wait(father->semaphore); 
  
    off=get_free_entry(father->volume, father->cluster,free_entry); 
    write_data(father->volume, father->cluster, off, buf, free_entry*SIZE_ENTRY); 
  
  sem_signal(father->semaphore); 
  
  
  // FORMAT FCB 
  
  *new=mem_alloc(sizeof(FCB), 1);
  short_entry=( SHORT_ENTRY*)buf + (get_n_entry(name) -1); 
  tmp=*new; 
  
  memset(tmp,0, sizeof(FCB)); 
  memset(tmp->name, 0, MAX_NAME); 
  
  //NAME
  strncpy(tmp->name, name, strlen(name)); 

  //FATHER 
  
  tmp->father=father;			// Puntatore al padre 
  tmp->offset_father=off; 		// offset all'interno del padre
  tmp->cluster_father=father->cluster; 	// cluster del padre 
  tmp->volume=father->volume; 		// volume 
  tmp->cluster=0; 			// primo cluster  
  tmp->size=0; 				// grandezza 
  tmp->type=type; 			// ATTR di tipo 
  
  //SEMAPHORE
  if (type == ATTR_DIRECTORY) 
      tmp->semaphore=sem_ini(1); 

  
  // ACCESSI
  tmp->type=short_entry->DIR_Attr;
  tmp->CrtTime= short_entry->DIR_CrtTime ;  // ora di creazione 
  tmp->CrtDate=short_entry->DIR_CrtDate; // data di creazione 
  tmp->LstAccDate=short_entry->DIR_LstAccDate; // Ultimo accesso
  tmp->WrtTime=short_entry->DIR_WrtTime;
  tmp->WrtDate=short_entry->DIR_WrtDate; // data discrittura
  

  
  mem_free(buf); 
  reset_errno();


  return TRUE; 
}

/*Cerca un entry nell cartella corrente 
 * se non è presente riporta false
 * la cosa piu importante che dev trovae è l'offset della cartella del padre
 */ 




BOOL open_entry(const char * name , const FCB * father, FCB ** new) {

   FCB*tmp=NULL; 
   wchar *name_unicode=NULL; 
   SHORT_ENTRY short_entry; 
   dword size=strlen(name);
   lword offset=0; 
   lword off_short_entry=0; 
   char *n=NULL; 
   int i=0; 
   char *name_read=NULL;
   *new=NULL;

//    flog(LOG_WARN, "Nome %s, lun %d", name,size); 
   
   name_unicode=mem_alloc((size+1)*2,1); 
   wmemset(name_unicode, 0, size+1);
   char2uni(name_unicode, name, size); 
   
   print_w(name_unicode); 

   
   offset=search_entry_long(father->volume, father->cluster, name_unicode, &name_read); 
   
   if((int)offset <0){ 
     set_errno(ENOENT, "Non presente %s", name); 
     mem_free(name_unicode); 
     return FALSE; 
   }
#ifdef DEBUG
   {
     int n=get_n_entry(name_read); 
     byte * buf =NULL; 
     int s = n*SIZE_ENTRY;
     SHORT_ENTRY *e=0; 
     
     buf=mem_alloc(s,1); 
     flog(LOG_WARN, "Nome letto :%s  offset : %d n : %d ", name_read, offset,n);
    
     read_data(root->volume, root->cluster, offset, buf,s);
      e=(SHORT_ENTRY*)buf; 
  
    for ( i=0; i<n; i++, e++) {
	if (e->DIR_Name[0] == FREE) 
	    continue; 
	else if (e->DIR_Name[0]==ALL_FREE) {
	  break; 
	}else if (e->DIR_Attr==ATTR_LONG_NAME)  
	    print_long_entry((LONG_ENTRY*)e);
	 else 
	    print_short_entry((SHORT_ENTRY*)e);
       }
    
    
   }
 #endif

   //inserisco il nuovo FCB 
  *new=mem_alloc(sizeof(FCB), 1); 
  tmp=*new; 
  
  if (!tmp) { 
      set_errno(ENOMEM, "Memoria Finita"); 
      mem_free(name_unicode); 
      return FALSE; 
      
  }
  // riempio il nuovo fcb 
  memset(tmp, 0, sizeof(FCB)); 
  memset(tmp->name, 0, SIZE_NAME); 
  memset(&short_entry, 0, SIZE_ENTRY); 
  
  strncpy(tmp->name, name_read, strlen(name_read)); 
  
   
  //FATHER 
  tmp->father=father;			// FCB father 
  tmp->offset_father=offset; 		// offset all'interno dell cartella padre
  tmp->cluster_father=father->cluster;  // first cluster father 
  tmp->volume=father->volume; 		// volume 
  tmp->n_entry=get_n_entry(name_read);  // nmero entry presenti nel disco, compresa la short 
  
  
  off_short_entry=tmp->offset_father+(tmp->n_entry-1)*SIZE_ENTRY; 
  
  //devo leggere i dati dalla short entry 
  read_data(tmp->volume, tmp->cluster_father,off_short_entry , (byte*)&short_entry,SIZE_ENTRY);
 // print_short_entry(&short_entry); 
  
  // controollo CHECKSUM				--------------------------------------------fgffffffffff
  
  //FIRTST CLUSTER 
  tmp->cluster=merge(short_entry.DIR_FstClusHI, short_entry.DIR_FstClusLO); 
  
  // ACCESSI
  tmp->type=short_entry.DIR_Attr;
  tmp->CrtTime= short_entry.DIR_CrtTime ;  // ora di creazione 
  tmp->CrtDate=short_entry.DIR_CrtDate; // data di creazione 
  tmp->LstAccDate=short_entry.DIR_LstAccDate; // Ultimo accesso
  tmp->WrtTime=short_entry.DIR_WrtTime;
  tmp->WrtDate=short_entry.DIR_WrtDate; // data discrittura
  
  //GRANDEZZA
  tmp->size=short_entry.DIR_FileSize; 

  
  mem_free(name_unicode); 
  
  reset_errno(); 
  return TRUE; 
}


//funzione che elimina un entry dalla memorria e dal disco 
BOOL delete_direntry(FCB * f) {
  
  word size=f->n_entry*SIZE_ENTRY;
  SHORT_ENTRY *entry=NULL;
  byte * buf=mem_alloc(size,1);  
  int i=0; 
  
  // pulisco tt le entry 
  memset(&entry,0, size);
  
  entry=(SHORT_ENTRY*)buf; 
  
  for (i=0; i< f->n_entry;i++, entry++)
     entry->DIR_Name[0]=DELETED_FLAG;  
  
  write_data(f->volume, f->cluster_father, f->offset_father, buf, size);
}






void print_fcb ( const FCB * fcb ) {

    flog ( LOG_DEBUG, "Name       :%s" ,fcb->name );
    flog ( LOG_DEBUG, "Cluster    :%d", fcb->cluster );
    flog ( LOG_DEBUG, "Father     :%x", fcb->father );
    flog ( LOG_DEBUG, "Father off :%x", fcb->offset_father); 
    flog ( LOG_DEBUG, "N entry    :%d", fcb->n_entry );
    flog ( LOG_DEBUG, "Type       :%d", fcb->type);  
    flog ( LOG_DEBUG, "Size       :%d", fcb->size );
    flog ( LOG_DEBUG, "Date Crt   :%x", fcb->CrtDate); 
    flog ( LOG_DEBUG, "Time Crt   :%x", fcb->CrtTime );
    flog ( LOG_DEBUG, "Wrt  Date  :%x", fcb->WrtDate );
    flog ( LOG_DEBUG, "Wrt  Time  :%x", fcb->WrtTime); 
  
}



void test_dir (byte label){
  
  FCB *speranza=NULL; 
  char *buf; 
  SHORT_ENTRY *s; 
  int i=0; 
  buf=mem_alloc(1024,1);
  
  init_root_fcb(label); // inizializzo root
  
  
  if(!create_entry("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", ATTR_ARCHIVE, root, &speranza))
     perror("Creazione entry"); 
   else 
   flog(LOG_WARN,"OK 1"); 
  
    if(!create_entry(".ggg", ATTR_ARCHIVE, root, &speranza))
     perror("Creazione entry"); 
   else 
   flog(LOG_WARN,"OK 1"); 
  
   if(!create_entry("ksdfg.txt", ATTR_ARCHIVE, root, &speranza))
     perror("Creazione entry"); 
   else 
   flog(LOG_WARN,"OK 1"); 
   
     if (!open_entry("ksdfg.txt", root, &speranza))
    perror("apertura entry "); 
  else 
    print_fcb(speranza); 
  
//    print_fcb(speranza); 

/*

  
    if (!open_entry("ssssssssssssssssssssssssss", root, &speranza))
    perror("apertura entry "); 
  else 
    print_fcb(speranza); 
  */
  

  
  //print_directory(root->volume, root->cluster);
  
  
 
}



/*
 * 00000000000000000000000000000000000000000000000000000000.txt
11111111111111111111111111111111111111111111111111111111.txt
22222222222222222222222222222222222222222222222222222222.txt
33333333333333333333333333333333333333333333333333333333.txt
44444444444444444444444444444444444444444444444444444444.txt
55555555555555555555555555555555555555555555555555555555.txt
66666666666666666666666666666666666666666666666666666666.txt
66666666666666666666666.txt
giuseppe
*/



 // flog(LOG_WARN, "nome %s", name); 





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
 char2uni(name_u, name_c, strlen(name_c)); 
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
 char2uni(name_u, name_c, strlen(name_c)); 
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
 char2uni(name_u, name_c, strlen(name_c)); 
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
 char2uni(name_u, name_c, strlen(name_c)); 
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
