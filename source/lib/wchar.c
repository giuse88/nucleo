#include "string.h"
#include "wchar.h"
#include "errno.h"


int _ii_;

#define print(buf,size) for(_ii_=0; _ii_ < size; _ii_++) flog(LOG_DEBUG,"%d %x", _ii_, buf[_ii_]);



extern void flog(log_sev sev, const char* fmt, ...);
extern void* mem_alloc(natl dim, natl balign );
extern void  mem_free(void *p);

// nota bene, fa utilizzo di primitive sistema, non è portabile
void print_w (const wchar * name) {
 
  dword size=wcslen(name); 
  
  char *buf= (char *)mem_alloc(size+1, 1); 
  
  memset(buf,0,size+1); 
  uni2char(name, buf, size); 
  
  flog(LOG_WARN, "%d %s ", size, buf); 
  
  mem_free(buf); 
  
}


//funzione che trasforma uan stringa da unicode a char 

int  uni2char(const wchar * uni, char * n, size_t size) { 

  unsigned int i=0; 
  
  memset(n,0,size); 
   
   for ( i=0; i < size; i++) 
      if (uni[i] > 0x00FF)
	n[i]=0; 
      else 
      n[i]=(char)(uni[i]&0x00FF);
  
 return TRUE; 
}


//funzione che trasforma da char to unicode 
int char2uni( wchar * uni, const  char  * name, size_t size) { 
  
  unsigned int i=0; 

  wmemset(uni, 0, size);  
  
  for ( i=0; i < size; i++) 
      uni[i]=(wchar)(name[i])&0x00FF;

  return TRUE; 
  
}


/* LIBRERIA */ 
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

int wcsncmp(const wchar *a, const wchar *b, size_t s) {
  unsigned  int i=0;

   for (i=0; i<s && *a==*b; i++, a++, b++) 
    if ( !(*a && *b))
      return -1; 
  
  return *a-*b; 
}

//strlen
size_t wcslen(const wchar* s) {
  size_t i;
  for (i=0; s[i]; i++);
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


wchar * wmemset(wchar * buf, wchar c , size_t size)  {

  unsigned int i=0; 

  for(i=0; i< size; i++) 
    buf[i]=c; 
  
  return buf; 

} 



int wcsicmp(const wchar* a,const wchar* b) {
  
  int i=0;
  
 while ( *a &&UPPER((char)*a) == UPPER((char)*b)) {
    a++; b++; i++;
  }    
 
  return *a-*b;

}



