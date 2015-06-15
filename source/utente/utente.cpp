#line 1 "utente/prog/logger.in"
#include <sys.h>
#include <lib.h>
#include <colors.h>


#line 6 "utente/prog/logger.in"

#line 7 "utente/prog/logger.in"

#line 9 "utente/prog/logger.in"
log_msg __logger_buf;
log_msg __logger_msg;

const char *__logger_sev_names[] = { "DBG", "INF", "WRN", "ERR" };
int __logger_sev_fgcol[]  = { COL_BLUE, COL_GREEN, COL_RED, COL_BLACK };
int __logger_sev_bgcol[]  = { COL_BLACK, COL_BLACK, COL_BLACK, COL_RED };

enum __logger_cmd_types { NEW_MSG, QUIT } __logger_cmd;

extern int __logger_non_busy;
#line 19 "utente/prog/logger.in"
extern int __logger_new_cmd;
#line 20 "utente/prog/logger.in"
extern int __logger_mutex;
#line 22 "utente/prog/logger.in"
void __logger_send_cmd(__logger_cmd_types cmd)
{
	sem_wait(__logger_mutex);
	sem_wait(__logger_non_busy);
	__logger_cmd = cmd;
	__logger_msg = __logger_buf;
	sem_signal(__logger_new_cmd);
	sem_signal(__logger_mutex);
}

__logger_cmd_types __logger_recv_cmd(log_msg& msg)
{
	__logger_cmd_types work;
	sem_wait(__logger_new_cmd);
	work = __logger_cmd;
	msg = __logger_msg;
	sem_signal(__logger_non_busy);
	return work;
}

bool __logger_quit = false;

void __logger_main(int a)
#line 45 "utente/prog/logger.in"
{
	__logger_cmd_types my_cmd;
	log_msg my_msg;

	if (!vterm_setresident(a)) {
		flog(LOG_WARN, "log di sistema non residente");
	}

	while (!__logger_quit) {
		my_cmd = __logger_recv_cmd(my_msg);
		switch (my_cmd) {
		case NEW_MSG:
			vterm_setcolor(a, __logger_sev_fgcol[my_msg.sev],
					  __logger_sev_bgcol[my_msg.sev]);
			printf(a, "%s %d\t%s\n", __logger_sev_names[my_msg.sev],
						 my_msg.identifier,
						 my_msg.msg);
			break;
		default:
			break;
		}
	}
	printf(a, "*** logger (main) terminato ***\n");

	terminate_p();
}
void __logger_reader(int a)
#line 71 "utente/prog/logger.in"
{

	if (!resident(&__logger_buf, sizeof(__logger_buf)))
		goto out;

	while (!__logger_quit) {
		readlog(__logger_buf);
		if (!__logger_quit) {
			__logger_send_cmd(NEW_MSG);
		}
	}

out:
	printf(a, "*** logger (reader) terminato ***\n");

	terminate_p();
}
char __logger_ctrl_buf;

void __logger_ctrl(int a)
#line 90 "utente/prog/logger.in"
{
	
	for (;;) {
		readvterm_n(a, &__logger_ctrl_buf, 1, VTERM_NOECHO);
		switch (__logger_ctrl_buf) {
		case 'Q':
			vterm_shutdown();
			delay(5);
			__logger_quit = true;
			__logger_send_cmd(QUIT);
			*(int *)0 = 1;
			break;
		case 'I':
			dump(DUMP_IOAPIC);
			break;
		case 'P':
			dump(DUMP_PROC);
			break;
		case 'S':
			dump(DUMP_SEM);
			break;
		default:
			break;
		}
	}

	terminate_p();
}
#line 1 "utente/prog/nomi.in"
#include <sys.h>
#include <lib.h>
#include <string.h> 
#include <direntry.h>

#define TERMINALE 2
#define NOMI_LUNGHI 29
#define NOMI_SIMILI 15

#line 13 "utente/prog/nomi.in"
const char * nomi_lunghi[NOMI_LUNGHI]={
			  "a0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "b0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "c0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "d0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "e0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "f0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "g0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "h0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "i0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "l0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "m0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "n0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",  
			  "o00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",// 12
			  "p0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "q0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "r0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "s0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "t0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "u0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "v0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "z0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "x0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "y0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "k0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp",
			  "j0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "aa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "bb000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  "cc000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			  };

const char * nome_test2[]={"10000000000000",
			  "200000000000000000000",
			  "3000000000000000000000000000"
			  "40000000000000000000000000000000000",
			  "50000000000000000000000000000000000000000",
			  "600000000000000000000000000000000000000000000000",
			  "700000000000000000000000000000000000000000000000000000",
			  "80000000000000000000000000000000000000000000000000000000000000",
			  "110000000000",
		      "5500000000000000000000000000000000000000GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG00000000000000",
			  
			  };


const char * sim [NOMI_SIMILI]={ 
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa1",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa2",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa3",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa4",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa5",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa6",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa7",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa8",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa9",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa10",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa11",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa12",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa13",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa14",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaa15",
};

void nomi(int a)
#line 77 "utente/prog/nomi.in"
{

  char buf[5]; 
  int fd=-1; 
  
   
  printf(TERMINALE, "Programma test nomi\n" ); 
  printf(TERMINALE, "Premi ENTER per iniziare....\n");
  
  memset(buf, 0, 5); 
  readvterm_ln(a, buf, 5);
  
  if ( mkdir ("Nomi_Lunghi") < 0 ) {
      perror(a,"Mkdir"); 
      return; 
  }

  if ( chdir("Nomi_Lunghi") < 0 ) { 
    perror(a,"chdir"); 
    return; 
  }

  // creazione nomi 

  for ( int i=0; i< NOMI_LUNGHI ; i++)     
	if ( (fd=open(nomi_lunghi[i], O_CREAT )) < 0 ) {
	    perror(a, "CREATE"); 
	}else {
	   printf(a, "Creato nome %s\n", nomi_lunghi[i]);
	   close(fd); 
	}

  printf(a, "Nomi creati\n");
  printf(a, "Premere Enter Per iniziare la cancellazione...\n"); 
  memset(buf, 0, 5); 
  readvterm_ln(a, buf, 5);

   for ( int i=0; i< NOMI_LUNGHI ; i++)
	if ( remove(nomi_lunghi[i]) < 0 ) 
	     perror(a,"Remove"); 
	else 
	    printf(a, "Eliminato file %s\n", nomi_lunghi[i]); 
      
    if ( chdir("..") < 0 ) { 
	perror(a,"chdir"); 
	return; 
    }
	
    if (rmdir("Nomi_Lunghi") < 0) {
        perror(a,"Rmdir"); 
	return; 
    }
  
  printf(a, "\nNomi Eliminati\n");
  printf(a, "Premere Enter Per iniziare la creazione dei nomi Simili...\n"); 
  memset(buf, 0, 5); 
  readvterm_ln(a, buf, 5);
  
  
  if ( mkdir ("Nomi_simili") < 0 ) {
      perror(a,"Mkdir"); 
      return; 
  }

  if ( chdir("Nomi_simili") < 0 ) { 
    perror(a,"chdir"); 
    return; 
  }

  // creazione nomi 

  for ( int i=0; i< NOMI_SIMILI ; i++)     
	if ( (fd=open(sim[i], O_CREAT )) < 0 ) {
	    perror(a, "CREATE"); 
	}else {
	   printf(a, "Creato nome %s\n", sim[i]);
	   close(fd); 
	}

  printf(a, "Nomi creati\n");
  printf(a, "Premere Enter Per iniziare la cancellazione...\n"); 
  memset(buf, 0, 5); 
  readvterm_ln(a, buf, 5);

   for ( int i=0; i< NOMI_SIMILI ; i++)
	if ( remove(sim[i]) < 0 ) 
	     perror(a,"Remove"); 
	else 
	    printf(a, "Eliminato file %s\n", sim[i]); 
      
    if ( chdir("..") < 0 ) { 
	perror(a,"chdir"); 
	return; 
    }
	
    if (rmdir("Nomi_simili") < 0) {
        perror(a,"Rmdir"); 
	return; 
    }
  
    
	

	terminate_p();
}
#line 1 "utente/prog/path.in"
#include <sys.h>
#include <lib.h>
#include <string.h> 
#include <direntry.h>



#line 9 "utente/prog/path.in"
#define COMAND_SIZE 300
#define COMAND_COUNT 9
#define MAX_PATH 260
#define TERM 3 
#define CLUSTER 4096 
#define PRINT_NAME 10
#define SIZE_BUF 1024

const char *comand[COMAND_COUNT]={ "ls", "cd", "cp", "rm", "mv", "mkdir", "rmdir", "create", "cat" };

void elabora_comando(char *buf); 
int searchComand(char *buf); 

void changeDir(char *path); 
void showDir(); 
void cp (char *, char *); 
void mv (char *, char *); 
void rm (char *); 
void createDirecotory(char *); 
void removeDirectory(char *); 
void create(char *); 
void cat(char*); 



void path(int a)
#line 35 "utente/prog/path.in"
{


      char * buf=(char*)mem_alloc(COMAND_SIZE); 
      
      if (!buf) 
	printf(TERM, "errore memorai"); 

      while (1)  {
		vterm_setcolor(a, COL_GREEN, COL_BLACK);
		    
		if (!getcwd(buf, MAX_PATH)) {
		      vterm_setcolor(a, COL_RED, COL_BLACK);
		      perror(TERM, "Errore GetCwd"); 
		      break; 
		}
		
		printf(a, "%s : ", buf);
		vterm_setcolor(a, COL_WHITE, COL_BLACK);
		readvterm_ln(a, buf, COMAND_SIZE);
		elabora_comando(buf); 
      }
  
     vterm_setcolor(a, COL_WHITE, COL_BLACK);
     mem_free(buf); 
     printf(TERM, "Programma Terminato\n");   

	terminate_p();
}
int searchComand( char *cmd) { 

    char * buf=NULL; 
    char * tok=NULL;
    int i=0; 

    if (!(buf=(char*)mem_alloc(COMAND_SIZE)))
	return -23; 
    
    memset(buf, 0, COMAND_SIZE); 
    strncpy(buf, cmd, strlen(cmd));

    tok=strtok(buf, " "); 

    if (!tok) 
      return -23; 

    for ( i=0; i<COMAND_COUNT; i++) 
      if(!strncmp(tok, comand[i], strlen(tok)))
	  return i; 
    
     mem_free(buf); 
     return -1; 
}

void elabora_comando(char *buf) {


  char *path=NULL; 


  if (!strcmp(buf, "")) 
      return; 


    switch (searchComand(buf)) { 
	    case -1: 
	    break; 
	    case 0: 
		  showDir(); 
	    return; 
	    case 1: 
		  if (!(path=strchr(buf,' ')))
		      printf(TERM, "Inserire path\n"); 
		  else {
		      path++; 
		      changeDir(path);
		  }
	    return;  
	    case 2: 
		  if (!(path=strchr(buf,' ')))
		      printf(TERM, "Inserire path\n"); 
		  else {
		      char * tok1=NULL, *tok2=NULL; 
		      tok1=strtok(path, " "); 
		      tok2=strtok(NULL, " "); 
		      if ( !tok1 || !tok2) {
			    printf(TERM, "Inserire path destinatario\n"); 
			    return; 
			    }
		    cp(tok1,tok2); 
		    }
	    return ; 
	    case 3: 
		    if (!(path=strchr(buf,' ')))
		      printf(TERM, "Inserire path\n"); 
		  else {
		      path++; 
		      rm (path);
		    }
	    return; 
	    case 4:
		  if (!(path=strchr(buf,' ')))
		      printf(TERM, "Inserire path\n"); 
		  else {
		      char * tok1=NULL, *tok2=NULL; 
		      tok1=strtok(path, " "); 
		      tok2=strtok(NULL, " "); 
		      if ( !tok1 || !tok2) {
			    printf(TERM, "Inserire path destinatario\n"); 
			    return; 
			    }
		    mv(tok1,tok2); 
		    }
	    return;
	    case 5: 
		  if (!(path=strchr(buf,' ')))
		      printf(TERM, "Inserire path\n"); 
		  else {
		      path++; 
		      createDirecotory(path); 
		    }
		  
	    return; 
	    case 6: 
		   if (!(path=strchr(buf,' ')))
		      printf(TERM, "Inserire path\n"); 
		  else {
		      path++; 
		      removeDirectory(path); 
		    }
		   
	    return; 
	    case 7: 
		  if (!(path=strchr(buf,' ')))
		      printf(TERM, "Inserire path\n"); 
		  else {
		      path++; 
		      create(path);
		  } 
	    return; 
	    case 8: 
		  if (!(path=strchr(buf,' ')))
		      printf(TERM, "Inserire path\n"); 
		  else {
		      path++; 
		      cat(path); 
		  } 
	    return; 
	    default: 	
		printf(TERM, "Errore"); 
	    return; 
	  
    }

  
  printf(3, "Comando not found\n"); 


}



void changeDir(char *path) {

   if ( chdir(path) < 0 ) 
	perror(TERM,"CHDIR");  

}



void create ( char * path) { 
  
   int fd=-1; 

   if ((fd=open(path, 0x40 )) < 0 ) 
      perror(TERM, "Error open");
   else 
      printf(TERM, "Creato File %s\n", path); 

    if (close(fd) <0)
      perror(TERM, "Close"); 

}

void cp(char * source , char * dest ) {
   
  int fd_s=-1, fd_d=-1; 
  char *buf=NULL; 
  int n_read=0; 

   if ( !source || !dest) 
      return; 

   if (!(buf=(char*)mem_alloc(SIZE_BUF)))
	return; 

  if ((fd_s=open(source,O_RDONLY))<0) {
      perror(TERM, "Open sorgente"); 
      return; 
  }

  if((fd_d=open(dest,O_CREAT|O_RDWR)) < 0 ) {
      perror(TERM, "Open destinatario"); 
      return; 
  }

  printf(TERM,"Inizio copia %s...",source); 
 
    while( (n_read=read(fd_s,buf,SIZE_BUF)) > 0 ) 
	  if ( write(fd_d,buf,n_read) < 0 ) {
		perror(TERM, "write error"); 
		return; 
	  }

  printf(TERM,"Fine copia\n");


  mem_free(buf); 

  if (close(fd_s) <0)
      perror(TERM, "Close"); 

 if (close(fd_d) <0)
      perror(TERM, "Close"); 


}

void mv (char *s, char *d) {
  printf(TERM, "move\n");

  if ( !s || !d)
      return; 

  if (rename(s,d) < 0 )
    perror(TERM,"Rename"); 

}

void rm(char * path) {

  if ( !path) 
      return; 
 
  if ( remove(path) <0 )
      perror(TERM, "Remove"); 

}
void createDirecotory(char * path) { 
  printf(TERM, "CreateDirectory\n");

  if (!path)
      return; 

  if ( mkdir(path) <0  ) {
    perror(TERM, "Mkdir"); 
    return; 
  }
  

}
void removeDirectory(char *path){

  if (!path)
      return; 

  if (rmdir(path) <0  ) {
    perror(TERM, "rmdir"); 
    return; 
  }

}

//////////////////////////
// Gestione entry ////////
//////////////////////////

typedef struct { 
    char name[MAX_NAME]; 

    natb   type; 
    natl cluster; 

    msdos_time CrtTime ;  		  
    msdos_date CrtDate; 		 
    msdos_date LstAccDate;	

    natl size; 
}info_entry; 
   
byte check_sum ( const  byte * name) { 
  
 short i=0; 
 byte sum=0; 
 
 for ( i=MSDOS_NAME; i !=0; i--) 
      sum= (( sum & 1) ? 0x80 : 0) + ( sum >> 1) + *name++; 
 
 
 return sum; 
}


char * load_dir ( int fd , char ** t, size_t * size)  { 

  char *buf=NULL,*temp=NULL, *buf_tmp=NULL, *buf_read=NULL;
  int n_read=0; 
  int i=2; 

  if (!(buf_read=(char*)mem_alloc(CLUSTER))) 
	return NULL; 

  temp=buf=NULL; 
  *size=0; 

  while ( (n_read=read(fd, buf_read, CLUSTER )) > 0 ) { 
	  if ( n_read < CLUSTER) {
	      memcpy(buf+CLUSTER*(i-1),buf_read,CLUSTER );
	      *t=buf; 
	      *size=+n_read; 
	       return buf; 
	  } else 
	      { 
		buf_tmp=buf; 
		if (!(buf=(char*)mem_alloc(CLUSTER*i))) {
		  mem_free(temp); 
		  return NULL; 
		}
		memset(buf, 0, CLUSTER*i);
	        memcpy(buf,buf_tmp, CLUSTER*(i-2)); 
		memcpy(buf+CLUSTER*(i-2),buf_read,CLUSTER ); 
		mem_free(buf_tmp); 
		*size+=n_read; 
		i++; 
	      }
    }

  return buf; 
}

//funzione che trasforma uan stringa da unicode a char 

int  uni2char(const short * uni, char * n, size_t size) { 

  unsigned int i=0; 
  
  memset(n,0,size); 
   
   for ( i=0; i < size; i++) 
      if (uni[i] > 0x00FF)
	n[i]=0; 
      else 
      n[i]=(char)(uni[i]&0x00FF);
  
 return TRUE; 
}


void print_info ( info_entry * info) { 

  char name[PRINT_NAME+2]; 
  size_t size=0; 

  memset(name, 0, PRINT_NAME+2);
  size=(strlen(info->name)); 

  if (size < PRINT_NAME)  {
    strncpy(name, info->name, size); 
    memset(name+size, ' ', PRINT_NAME-size); 
  }else {
    strncpy(name, info->name, PRINT_NAME); 
    name[PRINT_NAME]='*'; 
  }

  printf(TERM, "%c : ", (info->type==ATTR_DIRECTORY )? 'D' : 'F'); 
  printf(TERM, "%s\t", name); 
  printf(TERM, "%d\t%d\t", (info->size), info->cluster);  
  printf(TERM, "%d:%d:%d\t", (info->CrtDate.Day),(info->CrtDate.Month),(info->CrtDate.Years) );
  printf(TERM, "%d:%d:%d  \t", (info->CrtTime.Hours), (info->CrtTime.Minutes),(info->CrtTime.Second)); 
  printf(TERM, "%d:%d:%d\n", (info->LstAccDate.Day),(info->LstAccDate.Month),(info->LstAccDate.Years) );

}




bool isLong( SHORT_ENTRY * s, char * find_name) { 

    natb check=0; 
    int e=0, i=0, j=0, end=0, no=0; 
    LONG_ENTRY *l=NULL; 
    short name[SIZE_NAME_PART]; 
    char * tmp=NULL; 

    if ( !s || !find_name) 
	return false; 

    tmp=find_name;
    check=check_sum(s->DIR_Name); 
    l=(LONG_ENTRY*)s;
    memset(name,0, SIZE_NAME_PART*2); 

    do{      
	e++; 
	l--; 
	no=0; 
	j=0; 

	if ( (l->LDIR_Ord & 0X40) && l->LDIR_Attr == ATTR_LONG_NAME ) 
	      end=1;
	if ( !end && ( l->LDIR_Ord  != e || l->LDIR_Attr != ATTR_LONG_NAME || check!=l->LDIR_Chksum))
	      return false; 
	
//	printf(TERM, "END %d", end); 
	
	 for (i=0; (i< SIZE_UNO); i++,j++) 
	    if(l->LDIR_Name1[i]!= PADDING)
		  name[j]=l->LDIR_Name1[i]; 
	      else {
	  	    no=1; 
		    break; 
	        }

	

	 for (i=0; (i< SIZE_DUE) && !no; i++,j++) 
		if(l->LDIR_Name2[i]!= PADDING)
		    name[j]=l->LDIR_Name2[i]; 
		else {
		    no=1; 
		    break; 
	      }
	

	for (i=0; (i< SIZE_TRE ) & !no; i++,j++) 
	      if(l->LDIR_Name3[i]!= PADDING)
		  name[j]=l->LDIR_Name3[i]; 
	      else{
		  end=1; 
		  break; 
	      }
	
	uni2char(name, tmp,j); 
//	printf(TERM,"%s %d", tmp, j);
	tmp+=j; 

    }while(end==0); 

  // printf(TERM, "Nome letto %s %d\n", find_name, strlen(find_name)); 

    return true; 
}

void print_entry(char * buf, size_t size ) {

  SHORT_ENTRY *entry=NULL; 
  int count=0, i=0; 
  info_entry info; 


  if ( buf == NULL || size%SIZE_ENTRY ) 
	return; 

  count=size/SIZE_ENTRY; 
  entry=(SHORT_ENTRY*)buf;; 

  for ( i=0; i<count; i++, entry++) { 
	if (entry->DIR_Name[0] ==  FREE)	
		continue;
	else if ( entry->DIR_Name[0] ==  ALL_FREE)
		break; 
	else if (entry->DIR_Attr == ATTR_LONG_NAME)  
	        continue;
	else {
	   memset(&info, 0, sizeof(info_entry));
	   memcpy(&info.CrtTime, &entry->DIR_CrtTime, sizeof(msdos_time));   		 // ora di creazione 
           memcpy(&info.CrtDate, &entry->DIR_CrtDate, sizeof(msdos_date));  		 // data di creazione 
	   memcpy(&info.LstAccDate,&entry->DIR_LstAccDate,sizeof(msdos_date)); 	 //data ultimo accesso	
	   info.size=entry->DIR_FileSize; 
	   info.type=entry->DIR_Attr; 
	   info.cluster=(((natw)(entry->DIR_FstClusHI)<<16) | (natw)(entry->DIR_FstClusLO));
	   if (i==0 || !isLong(entry, info.name)) 
		  strncpy((char *)info.name, (const char *) entry->DIR_Name, MSDOS_NAME); 
	   
	  print_info(&info); 
      
	}    
    } 

}

void showDir() {
    
    char path[MAX_PATH]; 
    int fd=0; 
    char * buf=NULL; 
    size_t size=0; 
    char name[3]; 
    char * tok=NULL; 

    memset(path,0,MAX_PATH); 
    memset(name,0,3); 

    if (!getcwd(path,MAX_PATH)) {
	perror(TERM,"GetCwd"); 
	return; 
    }

    tok=strrchr(path, '/'); 

    // posso farlo perchè sono sicuro che prima c'è sempre qualcosa
    if ( *(--tok)==':') 
	      name[0]='/'; 
    else 
	      name[0]='.'; 

    if ((fd=open(name, O_RDONLY|O_DIRECTORY)) < 0 ) {
	perror(TERM, "Open"); 
	return; 
    }
    
   
    if (!(buf=load_dir(fd, &buf, &size)) < 0 ) 
      printf(TERM, "Load dir error"); 

    print_entry(buf, size); 

    
    if (close(fd)<0 ) 
	perror(TERM, "Close"); 
     
}

void cat (char * path ) { 

   int fd=0; 
   char buf[100]; 
   int n=0; 

   if ((fd=open(path, O_RDONLY)) < 0 ) {
	perror(TERM, "Open"); 
	return; 
    }
   
    while ( (n=read(fd,buf,100)) > 0 ) { 
      printf(TERM, buf); 
      memset(buf,0, 100); 
    }

    printf(TERM,"\n"); 

  if (close(fd)<0 ) 
	perror(TERM, "Close");  
}

#line 903 "utente/utente.cpp"
short __logger_p0;
short __logger_p1;
short __logger_p2;
int __logger_non_busy;
int __logger_new_cmd;
int __logger_mutex;
short proc_nomi;
short proc_path;

int main()
{
	lib_init();
	__logger_p0 = activate_p(__logger_main, 1, 201, LIV_UTENTE);
	__logger_p1 = activate_p(__logger_reader, 1, 200, LIV_UTENTE);
	__logger_p2 = activate_p(__logger_ctrl, 1, 200, LIV_UTENTE);
	__logger_non_busy = sem_ini(1);
	__logger_new_cmd = sem_ini(0);
	__logger_mutex = sem_ini(1);
	proc_nomi = activate_p(nomi, 2, 12, LIV_UTENTE);
	proc_path = activate_p(path, 3, 11, LIV_UTENTE);

	terminate_p();
}
