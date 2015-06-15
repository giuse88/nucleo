
#include "stdarg.h"
#include "string.h" 


#define DEC_BUFSIZE 12
// funzione che inserisce la stringa formatata su str, str deve essere abbastanza grande 

int vsnprintf(char *str, natl size, const char *fmt, va_list ap)
{
  natl in = 0, out = 0, tmp;
  char *aux, buf[DEC_BUFSIZE];
  natl cifre;

  while(out < size - 1 && fmt[in]) {
      switch(fmt[in]) {
    case '%':
      cifre = 8;
    again:
      switch(fmt[++in]) {
      case '1':
      case '2':
      case '4':
      case '8':
	cifre = fmt[in] - '0';
      goto again;
      case 'd':
	tmp = va_arg(ap, int);
	itostr(buf, DEC_BUFSIZE, tmp);
	if(strlen(buf) >
	   size - out - 1)
	  goto end;
	for(aux = buf; *aux; ++aux)
	  str[out++] = *aux;
	break;
      case 'x':
	tmp = va_arg(ap, int);
	if(out > size - (cifre + 1)) 
	  goto end;
	htostr(&str[out], tmp, cifre);
	out += cifre;
	break;
      case 'X':
         tmp = va_arg(ap, int);
	if(out > size - (cifre + 1)) 
	  goto end;
	htostr_upper(&str[out], tmp, cifre);
	out += cifre;
	break;
      case 's':
	aux = va_arg(ap, char *);
	while(out < size - 1 && *aux)
	  str[out++] = *aux++;
	break;
      case 'c':
	tmp = va_arg(ap, int);
	if (out < size - 1)
	  str[out++] = tmp;
	break;
      }
      ++in;
      break;
    default:
      str[out++] = fmt[in++];
    }
  }
 end:
  str[out++] = 0;

  
  return out;
}

int snprintf(char *buf, unsigned long n, const char *fmt, ...)
{
  va_list ap;
  int l;

  va_start(ap, fmt);
  l = vsnprintf(buf, n, fmt, ap);
  va_end(ap);

  return l;
}
