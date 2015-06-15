#include <lib.h>
#include <sys.h>

char* copy(const char* src, char* dst) {
	while (*src)
		*dst++ = *src++;
	*dst = '\0';
	return dst;
}

char* convint(int n, char* out)
{
	char buf[12];
	int i = 11;
	bool neg = false;

	if (n == 0) 
		return copy("0", out);

	buf[i--] = '\0';

	if (n < 0) {
		n = -n;
		neg = true;
	}
	while (n > 0) {
		buf[i--] = n % 10 + '0';
		n = n / 10;
	}
	if (neg)
		buf[i--] = '-';
	return copy(buf + i + 1, out);
}
/*
natl strlen(const char *s)
{
	natl l = 0;

	while(*s++)
		++l;

	return l;
}

char *strncpy(char *dest, const char *src, size_t l)
{
	size_t i;

	for(i = 0; i < l && src[i]; ++i)
		dest[i] = src[i];

	return dest;
}*/

static const char hex_map[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

static void htostr(char *buf, unsigned long l, int cifre = 8)
{
	int i;

	for(i = cifre - 1; i >= 0; --i) {
		buf[i] = hex_map[l % 16];
		l /= 16;
	}
}

static void itostr(char *buf, natl len, long l)
{
	natl i, div = 1000000000, v, w = 0;

	if(l == (-2147483647 - 1)) {
		strncpy(buf, "-2147483648", 12);
		return;
	} else if(l < 0) {
		buf[0] = '-';
		l = -l;
		i = 1;
	} else if(l == 0) {
		buf[0] = '0';
		buf[1] = 0;
		return;
	} else
		i = 0;

	while(i < len - 1 && div != 0) {
		if((v = l / div) || w) {
			buf[i++] = '0' + (char)v;
			w = 1;
		}

		l %= div;
		div /= 10;
	}

	buf[i] = 0;
}

#define DEC_BUFSIZE 12

int vsnprintf(char *str, natl size, const char *fmt, va_list ap)
{
	natl in = 0, out = 0, tmp;
	char *aux, buf[DEC_BUFSIZE];
	int cifre;

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
					case 's':
						aux = va_arg(ap, char *);
						while(out < size - 1 && *aux) 
							str[out++] = *aux++;
						break;	
					case 'c':
						tmp = va_arg(ap, int);
						if (out < size - 1)
							str[out++] = tmp;
				}
				++in;
				break;
			default:
				str[out++] = fmt[in++];
		}
	}
end:
	str[out] = 0;

	return out;
}


int snprintf(char *buf, size_t n, const char *fmt, ...)
{
	va_list ap;
	int l;

	va_start(ap, fmt);
	l = vsnprintf(buf, n, fmt, ap);
	va_end(ap);

	return l;
}

int printf(const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	int l;

	va_start(ap, fmt);
	l = vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);

	writeconsole(buf);

	return l;
}

int printf(int term, const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	int l;

	va_start(ap, fmt);
	l = vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);

	writevterm_n(term, buf, l);

	return l;
}


// copia n byte da src a dest
extern "C" void *memcpy(void *dest, const void *src, natl n)
{
	char       *dest_ptr = static_cast<char*>(dest);
	const char *src_ptr  = static_cast<const char*>(src);

	for (natl i = 0; i < n; i++)
		dest_ptr[i] = src_ptr[i];

	return dest;
}

// scrive n byte pari a c, a partire da dest
void *memset(void *dest, int c, natl n)
{
	char *dest_ptr = static_cast<char*>(dest);

        for (natl i = 0; i < n; i++)
              dest_ptr[i] = static_cast<char>(c);

        return dest;
}


#define ERR_BUF 2048

void perror(int a, const char *str) { 

    natl errno=0; 
    char *buf=0; 
   
    if (!str) 
      return; 
    
    if (!(buf=(char*)mem_alloc(ERR_BUF))) 
      return; 
    
    memset(buf,0,ERR_BUF); 
    
    get_error( &errno, buf, ERR_BUF); 
  
    printf(a, "%s : %s\n", str, buf); 
    
    return; 
} 

////////////////////////////////////////////////////////////////////////////////
//                          TERMINALI VIRTUALI                               
////////////////////////////////////////////////////////////////////////////////

#include <keycodes.h>

struct des_vterm;
struct vterm_map {
	void (*action)(des_vterm*, int v, natw code);
	int arg;
};

#define VTERM_MODIF_SHIFT	1U
#define VTERM_MODIF_ALTGR	2U
#define VTERM_MODIF_CTRL	4U
#define VTERM_MODIF_ALT		8U

#define VTERM_SHUTDOWN		0xffff

char vterm_decode_map[4][64] = {
  // tasto senza modificatori
 { 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
      'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
      'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
      '`', '\\',
      'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
      ' ',
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       '\b', '\n', '\r', '\t'
 },
 // tasto + shift
 { 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 
      'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
      'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',
      '~', '|',
      'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
      ' ',
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       '\b', '\r', '\n', '\t'
 },
 // tasto + altgr
 { 0
 },
 // tasto + shift + altgr
 { 0
 },
};

typedef void (*vterm_ctrl_func)(des_vterm *, char c);

static void vterm_do_ctrl(des_vterm*, char);
static void vterm_edit_eof(des_vterm*, char);

vterm_ctrl_func vterm_ctrl_map[4][32] = {
 { 0,
 },
  // tasto + ctrl
 {
	vterm_do_ctrl,	// '@'
	vterm_do_ctrl,	// 'a'
	vterm_do_ctrl,	// 'b'
	vterm_do_ctrl,	// 'c'
	vterm_edit_eof,	// 'd'
	vterm_do_ctrl,	// 'e'
	vterm_do_ctrl,	// 'f'
	vterm_do_ctrl,	// 'g'
	vterm_do_ctrl,	// 'h'
	vterm_do_ctrl,	// 'i'
	vterm_do_ctrl,	// 'j'
	vterm_do_ctrl,	// 'k'
	vterm_do_ctrl,	// 'l'
	vterm_do_ctrl,	// 'm'
	vterm_do_ctrl,	// 'n'
	vterm_do_ctrl,	// 'o'
	vterm_do_ctrl,	// 'p'
	vterm_do_ctrl,	// 'q'
	vterm_do_ctrl,	// 'r'
	vterm_do_ctrl,	// 's'
	vterm_do_ctrl,	// 't'
	vterm_do_ctrl,	// 'u'
	vterm_do_ctrl,	// 'v'
	vterm_do_ctrl,	// 'w'
	vterm_do_ctrl,	// 'x'
	vterm_do_ctrl,	// 'y'
	vterm_do_ctrl,	// 'z'
	vterm_do_ctrl,	// '['
	vterm_do_ctrl,	// '\'
	vterm_do_ctrl,	// ']'
	vterm_do_ctrl,	// '^'
	vterm_do_ctrl 	// '_'
 },
  // tasto + alt
 { 0
 },
  // tasto + ctrl + alt
 { 0
 }
};

static void vterm_esc(des_vterm*, int v, natw code);
static void vterm_decode(des_vterm*, int v, natw code);
static void vterm_modif(des_vterm*, int v, natw code); 
static void vterm_motion(des_vterm*, int v, natw code);
static void vterm_fnkey(des_vterm*, int v, natw code); 
static void vterm_keypad(des_vterm*, int v, natw code);
static void vterm_edit(des_vterm*, int v, natw code);
static void vterm_special(des_vterm*, int v, natw code);
static void vterm_numlock(des_vterm*, int v, natw code);
static void vterm_capslock(des_vterm*, int v, natw code);
static void vterm_scrlock(des_vterm*, int v, natw code);
static void vterm_sysrq(des_vterm*, int v, natw code);
static void vterm_tocons(des_vterm*, int v, natw code);



#define VTERM_EDIT_HOME		'7'
#define VTERM_EDIT_END		'1'
#define VTERM_EDIT_LEFT		'4'
#define VTERM_EDIT_RIGHT	'6'
#define VTERM_EDIT_INSERT	'0'
#define VTERM_EDIT_DELETE	'.'
#define VTERM_EDIT_UP		'8'
#define VTERM_EDIT_DOWN		'2'
#define VTERM_EDIT_PAGEUP       '9'
#define VTERM_EDIT_PAGEDOWN    	'3'

#define VTERM_MOTION_HOME	VTERM_EDIT_HOME
#define VTERM_MOTION_END	VTERM_EDIT_END
#define VTERM_MOTION_UP		VTERM_EDIT_UP	   
#define VTERM_MOTION_DOWN	VTERM_EDIT_DOWN	   
#define VTERM_MOTION_PAGEUP	VTERM_EDIT_PAGEUP  
#define VTERM_MOTION_PAGEDOWN	VTERM_EDIT_PAGEDOWN

#include <colors.h>


static natl vterm_shift(natl flags)
{
	return (flags & (VTERM_MODIF_SHIFT | VTERM_MODIF_ALTGR));
}
static natl vterm_ctrl(natl flags)
{
	return (flags & (VTERM_MODIF_CTRL | VTERM_MODIF_ALT)) >> 2U;
}


vterm_map vterm_maps[128] = {
	{ 0,             0 }, // 0x00
	{ vterm_esc,     0 }, // 0x01 esc
	{ vterm_decode,  1 }, // 0x02 1
	{ vterm_decode,  2 }, // 0x03 2
	{ vterm_decode,  3 }, // 0x04 3
	{ vterm_decode,  4 }, // 0x05 4
	{ vterm_decode,  5 }, // 0x06 5
	{ vterm_decode,  6 }, // 0x07 6
	{ vterm_decode,  7 }, // 0x08 7
	{ vterm_decode,  8 }, // 0x09 8
	{ vterm_decode,  9 }, // 0x0a 9
	{ vterm_decode, 10 }, // 0x0b 0
	{ vterm_decode, 11 }, // 0x0c -
	{ vterm_decode, 12 }, // 0x0d =
	{ vterm_decode, 60 }, // 0x0e backspace
	{ vterm_decode,	63 }, // 0x0f tab
	{ vterm_decode, 13 }, // 0x10 q
	{ vterm_decode, 14 }, // 0x11 w
	{ vterm_decode, 15 }, // 0x12 e
	{ vterm_decode, 16 }, // 0x13 r
	{ vterm_decode, 17 }, // 0x14 t
	{ vterm_decode, 18 }, // 0x15 y
	{ vterm_decode, 19 }, // 0x16 u
	{ vterm_decode, 20 }, // 0x17 i
	{ vterm_decode, 21 }, // 0x18 o
	{ vterm_decode, 22 }, // 0x19 p
	{ vterm_decode, 23 }, // 0x1a [
	{ vterm_decode, 24 }, // 0x1b ]
	{ vterm_decode, 61 }, // 0x1c enter
	{ vterm_modif,  VTERM_MODIF_CTRL }, // 0x1d Lctrl
	{ vterm_decode, 25 }, // 0x1e a
	{ vterm_decode, 26 }, // 0x1f s
	{ vterm_decode, 27 } ,// 0x20 d
	{ vterm_decode, 28 }, // 0x21 f
	{ vterm_decode, 29 }, // 0x22 g
	{ vterm_decode, 30 }, // 0x23 h
	{ vterm_decode, 31 }, // 0x24 j
	{ vterm_decode, 32 }, // 0x25 k
	{ vterm_decode, 33 }, // 0x26 l
	{ vterm_decode, 34 }, // 0x27 ;
	{ vterm_decode, 35 }, // 0x28 '
	{ vterm_decode, 36 }, // 0x29 `
	{ vterm_modif,  VTERM_MODIF_SHIFT }, // 0x2a Lshift
	{ vterm_decode, 37 }, // 0x2b backslash
	{ vterm_decode, 38 }, // 0x2c z
	{ vterm_decode, 39 }, // 0x2d x
	{ vterm_decode, 40 }, // 0x2e c
	{ vterm_decode, 41 }, // 0x2f v
	{ vterm_decode, 42 }, // 0x30 b
	{ vterm_decode, 43 }, // 0x31 n
	{ vterm_decode, 44 }, // 0x32 m
	{ vterm_decode, 45 }, // 0x33 ,
	{ vterm_decode, 46 }, // 0x34 .
	{ vterm_decode, 47 }, // 0x35 /
	{ vterm_modif,  VTERM_MODIF_SHIFT }, // 0x 36 Rshift
	{ vterm_keypad, '*' }, // 0x37 keypad *
	{ vterm_modif,  VTERM_MODIF_ALT }, // 0x38 Lalt
	{ vterm_decode, 48 }, // 0x39 space
	{ vterm_capslock, 0 }, // 0x3a caps lock
	{ vterm_fnkey,   0 }, // 0x3b F1
	{ vterm_fnkey,   1 }, // 0x3c F2
	{ vterm_fnkey,   2 }, // 0x3d F3
	{ vterm_fnkey,   3 }, // 0x3e F4
	{ vterm_fnkey,   4 }, // 0x3f F5
	{ vterm_fnkey,   5 }, // 0x40 F6
	{ vterm_fnkey,   6 }, // 0x41 F7
	{ vterm_fnkey,   7 }, // 0x42 F8
	{ vterm_fnkey,   8 }, // 0x43 F9
	{ vterm_fnkey,   9 }, // 0x44 F10
	{ vterm_numlock, 0 }, // 0x45
	{ vterm_scrlock, 0 }, // 0x46
	{ vterm_keypad,  '7' }, // 0x47 keypad 7
	{ vterm_keypad,  '8' }, // 0x48 keypad 8
	{ vterm_keypad,  '9' }, // 0x49 keypad 9
	{ vterm_keypad,  '-' }, // 0x4a keypad -
	{ vterm_keypad,  '4' }, // 0x4b keypad 4
	{ vterm_keypad,  '5' }, // 0x4c keypad 5
	{ vterm_keypad,  '6' }, // 0x4d keypad 6
	{ vterm_keypad,  '+' }, // 0x4e keypad +
	{ vterm_keypad,  '1' }, // 0x4f keypad 1
	{ vterm_keypad,  '2' }, // 0x50 keypad 2
	{ vterm_keypad,  '3' }, // 0x51 keypad 3
	{ vterm_keypad,  '0' }, // 0x52 keypad 0
	{ vterm_keypad,  '.' }, // 0x53 keypad .
	{ 0,	         0 }, // 0x54
	{ 0,	         0 }, // 0x55
	{ 0,	         0 }, // 0x56
	{ vterm_fnkey,  10 }, // 0x57 F11
	{ vterm_fnkey,  11 }  // 0x58 F12
};

vterm_map vterm_emaps[128] = {
	{ 0,             0 }, // 0x00
	{ 0,             0 }, // 0x01
	{ 0,             0 }, // 0x02
	{ 0,             0 }, // 0x03
	{ 0,             0 }, // 0x04
	{ 0,             0 }, // 0x05
	{ 0,             0 }, // 0x06
	{ 0,             0 }, // 0x07
	{ 0,             0 }, // 0x08
	{ 0,             0 }, // 0x09
	{ 0,             0 }, // 0x0a
	{ 0,             0 }, // 0x0b
	{ 0,             0 }, // 0x0c
	{ 0,             0 }, // 0x0d
	{ 0,             0 }, // 0x0e
	{ 0,             0 }, // 0x0f
	{ 0,             0 }, // 0x10
	{ 0,             0 }, // 0x11
	{ 0,             0 }, // 0x12
	{ 0,             0 }, // 0x13
	{ 0,             0 }, // 0x14
	{ 0,             0 }, // 0x15
	{ 0,             0 }, // 0x16
	{ 0,             0 }, // 0x17
	{ 0,             0 }, // 0x18
	{ 0,             0 }, // 0x19
	{ 0,             0 }, // 0x1a
	{ 0,             0 }, // 0x1b
	{ vterm_keypad, '\n' }, // 0x1c keypad enter
	{ vterm_modif,  VTERM_MODIF_CTRL }, // 0x1d Rctrl
	{ 0,             0 }, // 0x1e
	{ 0,             0 }, // 0x1f
	{ 0,             0 }, // 0x20
	{ 0,             0 }, // 0x21
	{ 0,             0 }, // 0x22
	{ 0,             0 }, // 0x23
	{ 0,             0 }, // 0x24
	{ 0,             0 }, // 0x25
	{ 0,             0 }, // 0x26
	{ 0,             0 }, // 0x27
	{ 0,             0 }, // 0x28
	{ 0,             0 }, // 0x29
	{ 0,             0 }, // 0x2a
	{ 0,             0 }, // 0x2b
	{ 0,             0 }, // 0x2c
	{ 0,             0 }, // 0x2d
	{ 0,             0 }, // 0x2e
	{ 0,             0 }, // 0x2f
	{ 0,             0 }, // 0x30
	{ 0,             0 }, // 0x31
	{ 0,             0 }, // 0x32
	{ 0,             0 }, // 0x33
	{ 0,             0 }, // 0x34
	{ vterm_keypad, '/' }, // 0x35 keypad /
	{ 0,             0 }, // 0x36
	{ vterm_sysrq,   0 }, // 0x37
	{ vterm_modif,   VTERM_MODIF_ALTGR }, // 0x38 Ralt
	{ 0,             0 }, // 0x39
	{ 0,             0 }, // 0x3a
	{ 0,             0 }, // 0x3b
	{ 0,             0 }, // 0x3c
	{ 0,             0 }, // 0x3d
	{ 0,             0 }, // 0x3e
	{ 0,             0 }, // 0x3f
	{ 0,             0 }, // 0x40
	{ 0,             0 }, // 0x41
	{ 0,             0 }, // 0x42
	{ 0,             0 }, // 0x43
	{ 0,             0 }, // 0x44
	{ 0,             0 }, // 0x45
	{ 0,             0 }, // 0x46
	{ vterm_edit,    VTERM_EDIT_HOME }, // 0x47 home
	{ vterm_edit,    VTERM_EDIT_UP }, // 0x48 up
	{ vterm_edit,    VTERM_EDIT_PAGEUP }, // 0x49 page up
	{ 0,             0 }, // 0x4a
	{ vterm_edit,    VTERM_EDIT_LEFT }, // 0x4b left
	{ 0,             0 }, // 0x4c
	{ vterm_edit,    VTERM_EDIT_RIGHT }, // 0x4d right
	{ 0,             0 }, // 0x4e
	{ vterm_edit,    VTERM_EDIT_END }, // 0x4f end
	{ vterm_edit,    VTERM_EDIT_DOWN }, // 0x50 down
	{ vterm_edit,    VTERM_EDIT_PAGEDOWN }, // 0x51 page down
	{ vterm_edit,    VTERM_EDIT_INSERT }, // 0x52 insert
	{ vterm_edit,    VTERM_EDIT_DELETE }, // 0x53 delete
	{ 0,             0 }, // 0x54
	{ 0,             0 }, // 0x55
	{ 0,             0 }, // 0x56
	{ 0,             0 }, // 0x57
	{ 0,             0 }, // 0x58
	{ 0,             0 }, // 0x59
	{ 0,             0 }, // 0x5a
	{ vterm_special, 1 }, // 0x5b Lwin
	{ vterm_special, 1 }, // 0x5c Rwin
	{ vterm_special, 2 }, // 0x5d menu
};

vterm_map console_maps[128] = {
	{ 0,             0 }, // 0x00
	{ vterm_tocons,  0 }, // 0x01 esc
	{ vterm_tocons,  1 }, // 0x02 1
	{ vterm_tocons,  2 }, // 0x03 2
	{ vterm_tocons,  3 }, // 0x04 3
	{ vterm_tocons,  4 }, // 0x05 4
	{ vterm_tocons,  5 }, // 0x06 5
	{ vterm_tocons,  6 }, // 0x07 6
	{ vterm_tocons,  7 }, // 0x08 7
	{ vterm_tocons,  8 }, // 0x09 8
	{ vterm_tocons,  9 }, // 0x0a 9
	{ vterm_tocons, 10 }, // 0x0b 0
	{ vterm_tocons, 11 }, // 0x0c -
	{ vterm_tocons, 12 }, // 0x0d =
	{ vterm_tocons, 60 }, // 0x0e backspace
	{ vterm_tocons,	63 }, // 0x0f tab
	{ vterm_tocons, 13 }, // 0x10 q
	{ vterm_tocons, 14 }, // 0x11 w
	{ vterm_tocons, 15 }, // 0x12 e
	{ vterm_tocons, 16 }, // 0x13 r
	{ vterm_tocons, 17 }, // 0x14 t
	{ vterm_tocons, 18 }, // 0x15 y
	{ vterm_tocons, 19 }, // 0x16 u
	{ vterm_tocons, 20 }, // 0x17 i
	{ vterm_tocons, 21 }, // 0x18 o
	{ vterm_tocons, 22 }, // 0x19 p
	{ vterm_tocons, 23 }, // 0x1a {
	{ vterm_tocons, 24 }, // 0x1b }
	{ vterm_tocons, 61 }, // 0x1c 
	{ vterm_tocons,  VTERM_MODIF_CTRL }, // 0x1d Lctrl
	{ vterm_tocons, 25 }, // 0x1e a
	{ vterm_tocons, 26 }, // 0x1f s
	{ vterm_tocons, 27 } ,// 0x20 d
	{ vterm_tocons, 28 }, // 0x21 f
	{ vterm_tocons, 29 }, // 0x22 g
	{ vterm_tocons, 30 }, // 0x23 h
	{ vterm_tocons, 31 }, // 0x24 j
	{ vterm_tocons, 32 }, // 0x25 k
	{ vterm_tocons, 33 }, // 0x26 l
	{ vterm_tocons, 34 }, // 0x27 ;
	{ vterm_tocons, 35 }, // 0x28 '
	{ vterm_tocons, 36 }, // 0x29 `
	{ vterm_tocons,  VTERM_MODIF_SHIFT }, // 0x2a Lshift
	{ vterm_tocons, 37 }, // 0x2b backslash
	{ vterm_tocons, 38 }, // 0x2c z
	{ vterm_tocons, 39 }, // 0x2d x
	{ vterm_tocons, 40 }, // 0x2e c
	{ vterm_tocons, 41 }, // 0x2f v
	{ vterm_tocons, 42 }, // 0x30 b
	{ vterm_tocons, 43 }, // 0x31 n
	{ vterm_tocons, 44 }, // 0x32 m
	{ vterm_tocons, 45 }, // 0x33 ,
	{ vterm_tocons, 46 }, // 0x34 .
	{ vterm_tocons, 47 }, // 0x35 /
	{ vterm_tocons,  VTERM_MODIF_SHIFT }, // 0x 36 Rshift
	{ vterm_tocons, '*' }, // 0x37 keypad *
	{ vterm_modif,  VTERM_MODIF_ALT }, // 0x38 Lalt
	{ vterm_tocons, 48 }, // 0x39 space
	{ vterm_tocons, 0 }, // 0x3a caps lock
	{ vterm_fnkey,   0 }, // 0x3b F1
	{ vterm_fnkey,   1 }, // 0x3c F2
	{ vterm_fnkey,   2 }, // 0x3d F3
	{ vterm_fnkey,   3 }, // 0x3e F4
	{ vterm_fnkey,   4 }, // 0x3f F5
	{ vterm_fnkey,   5 }, // 0x40 F6
	{ vterm_fnkey,   6 }, // 0x41 F7
	{ vterm_fnkey,   7 }, // 0x42 F8
	{ vterm_fnkey,   8 }, // 0x43 F9
	{ vterm_fnkey,   9 }, // 0x44 F10
	{ vterm_tocons, 0 }, // 0x45
	{ vterm_tocons, 0 }, // 0x46
	{ vterm_tocons,  '7' }, // 0x47 keypad 7
	{ vterm_tocons,  '8' }, // 0x48 keypad 8
	{ vterm_tocons,  '9' }, // 0x49 keypad 9
	{ vterm_tocons,  '-' }, // 0x4a keypad -
	{ vterm_tocons,  '4' }, // 0x4b keypad 4
	{ vterm_tocons,  '5' }, // 0x4c keypad 5
	{ vterm_tocons,  '6' }, // 0x4d keypad 6
	{ vterm_tocons,  '+' }, // 0x4e keypad +
	{ vterm_tocons,  '1' }, // 0x4f keypad 1
	{ vterm_tocons,  '2' }, // 0x50 keypad 2
	{ vterm_tocons,  '3' }, // 0x51 keypad 3
	{ vterm_tocons,  '0' }, // 0x52 keypad 0
	{ vterm_tocons,  '.' }, // 0x53 keypad .
	{ 0,	         0 }, // 0x54
	{ 0,	         0 }, // 0x55
	{ 0,	         0 }, // 0x56
	{ vterm_fnkey,  10 }, // 0x57 F11
	{ vterm_fnkey,  11 }  // 0x58 F12
};


enum funz { none, input_ln, input_n };

struct des_vterm_global {
	bool numlock;		// stato del tasto numlock
	bool capslock;		// stato del tasto capslock
	natl flags;	// stato dei tasti modificatori
} vterm_global;

struct des_vterm {
	natl num;		// numero del terminale virtuale
	natl id;			// identificatore del processo di input

	natl mutex_r;		// mutua esclusione in lettura
	natl sincr;		// sincronizzazione in lettura
	natl vkbd;		// id della tastiera virtuale (dal modulo IO)
	natl vmon;		// id del monitor virtuale (dal modulo IO)
	natl cont;		// caratteri contenuti nel buffer di lettura
	natl orig_cont;		// dimensione del buffer di lettura
	char* punt;		// puntatore di inserimento nel buffer
	char* orig_punt;	// primo carattere nel buffer di lettura
	natl orig_off;		// offset (rispetto a "base") sul video del
				// primo carattere nel buffer
	natl letti;		// caratteri letti
	funz funzione;		// tipo di lettura (linea o fisso)
	bool echo;		// echo su video dei caratteri inseriti
	bool insert;		// modalita' inserimento o sostituzione

	natl mutex_w;		// mutua esclusione in scrittura
	natw *video;		// puntatore al buffer video
	natl video_max_x,	// numero di colonne del buffer video
	    video_max_y;	// numero di righe del buffer video
	natl base;		// offset (rispetto a "video") 
				// del primo caratter valido nel
				// buffer video (allineato alla riga)
	natl video_off;		// offset (rispetto a "base") del cursore
	natl append_off;		// offset (rispetto a "base") della posizione
				// successiva all'ultimo carattere valido
				// contenuto nel buffer video
	natl pref_off;		// offset (rispetto a "base") a cui il cursore si dovrebbe spostare
				// tramite le operazioni di movimento cursore
	natl vmon_off;		// offset (rispetto a "base") del primo
				// carattere visualizzato sul monitor virtuale
	natl uncleared;		// offset (rispetto a "base") del primo
				// carattere non pulito
	vterm_edit_status stat;
	natl vmon_size;		// dimensione del monitor virtuale
	natl video_size;	// dimensione del buffer video
	natb attr;		// byte di attributi colore
	natb clear_attr;
	natl tab;		// dimensione delle tabulazioni

	bool scroll_lock;	// blocco dello scorrimento
	natl scroll_sincr;	// sincronizzazione sul blocco dello scorrimento
	natl waiting_scroll;	// numero di processi in attesa dello sblocco
				// dello scorrimento
};

void vterm_switch(natl v);
static void vterm_update_vmoncursor(des_vterm *t);
static void vterm_update_vmon(des_vterm *t, natl first, natl end);
static natl vterm_row(const des_vterm *t, natl off);
static void vterm_move_cursor(des_vterm *t, natl off);
static void vterm_write_chars(des_vterm* t, const char vetti[], int quanti);
static bool vterm_make_visible(des_vterm* t, natl off);
static int  vterm_calc_off(const des_vterm *t, natl off, const char* start, const char* end);
static void vterm_rewrite_chars(des_vterm *t, const char vetti[], int quanti, bool visible = false);
static void vterm_edit_insert(des_vterm *t, const char* v, int quanti);
static void vterm_edit_replace(des_vterm *t, const char *v, int quanti);
static void vterm_edit_remove(des_vterm *t, int quanti);
static int  vterm_delta_off(const des_vterm *t, natl off, char c);
static void vterm_output_status(des_vterm *t);

void vterm_esc(des_vterm*, int v, natw code)
{
}

void vterm_add_char(des_vterm *t, char c)
{
	bool fine = false;

	if (t->funzione != input_n && t->funzione != input_ln)
		return;

	if (t->echo)
		sem_wait(t->mutex_w);

	if (c == '\b') {
		if (t->punt > t->orig_punt) {
			t->punt--;
			t->pref_off = vterm_calc_off(t, t->orig_off, t->orig_punt, t->punt);
			vterm_move_cursor(t, t->pref_off);
			vterm_edit_remove(t, 1);
		}
	} else if (c == '\n' && t->funzione == input_ln) {
		*(t->orig_punt + t->cont) = '\0';
		vterm_output_status(t);
		if (t->echo) {
			vterm_move_cursor(t, t->append_off);
			vterm_write_chars(t, &c, 1);
		}
		fine = true;
	} else {
		if (t->punt < t->orig_punt + t->orig_cont) {
			if (t->insert) {
				if (t->cont < t->orig_cont)
					vterm_edit_insert(t, &c, 1);
			} else {
				vterm_edit_replace(t, &c, 1);
			}
		}
		if (t->funzione == input_n && t->cont >= t->orig_cont) {
			vterm_output_status(t);
			vterm_move_cursor(t, t->append_off);
			fine = true;
		}
	}
	if (fine) {
		t->letti += t->cont;
		t->funzione = none;
	}
	if (t->echo)
		sem_signal(t->mutex_w);
	if (fine)
		sem_signal(t->sincr);
}

void vterm_edit_eof(des_vterm *t, char c)
{
	if (t->funzione != input_n)
		return;

	vterm_move_cursor(t, t->append_off);
	t->letti += t->cont;
	t->funzione = none;
	vterm_output_status(t);
	sem_signal(t->sincr);
}

void vterm_decode(des_vterm* t, int v, natw code)
{
	if (code & 0x0080) 
		return;

	char c = vterm_decode_map[vterm_shift(vterm_global.flags)][v];
	if (!c)
		return;

	if ((vterm_global.capslock || (vterm_ctrl(vterm_global.flags))) && (c >= 'a' && c <= 'z'))
		c = c - 'a' + 'A';

	if (c >= '@' && c <= '_') {
		vterm_ctrl_func f = vterm_ctrl_map[vterm_ctrl(vterm_global.flags)][c - '@'];
		if (f) {
			(*f)(t, c);
			return;
		}
	}

	vterm_add_char(t, c);

}

void vterm_tocons(des_vterm* t, int v, natw code)
{
	vkbd_send(-2, code, false);
}

void vterm_do_ctrl(des_vterm *t, char c)
{
	vterm_add_char(t, c ^ 0x40);
}

void vterm_modif(des_vterm* t, int v, natw code)
{
	if (code & 0x0080) 
		vterm_global.flags &= ~v;	
	else
		vterm_global.flags |= v;
}
		
void vterm_motion(des_vterm* t, int v, natw code)
{
	if (code & 0x0080)
		return;

	int move = 0;
	sem_wait(t->mutex_w);
	switch (v) {
	case VTERM_MOTION_HOME:
		t->vmon_off = 0;
		break;
	case VTERM_MOTION_END:
		t->vmon_off = vterm_row(t, t->append_off) + t->video_max_x;
		break;
	case VTERM_MOTION_UP:
		move = -t->video_max_x;
		break;
	case VTERM_MOTION_DOWN:
		move = t->video_max_x;
		break;
	case VTERM_MOTION_PAGEUP:
		move = -t->vmon_size;
		break;
	case VTERM_MOTION_PAGEDOWN:
		move = t->vmon_size;
		break;
	}
	if (move < 0 && (natl)-move > t->vmon_off)
		t->vmon_off = 0;
	else if (t->vmon_off + move + t->vmon_size > vterm_row(t, t->append_off) + t->video_max_x)
		t->vmon_off = vterm_row(t, t->append_off) + t->video_max_x - t->vmon_size;
	else
		t->vmon_off += move;
	vterm_update_vmon(t, t->vmon_off, t->vmon_off + t->vmon_size);
	sem_signal(t->mutex_w);
}

void vterm_fnkey(des_vterm* t, int v, natw code)
{
	if (code & 0x00800)
		return;

	if (vterm_global.flags & VTERM_MODIF_ALT)
		vterm_switch(v + ((vterm_global.flags & VTERM_MODIF_SHIFT) ? 12 : 0));
}

void vterm_keypad(des_vterm* t, int v, natw code)
{
	if (code & 0x0080)
		return;

	if (vterm_global.numlock)
		vterm_add_char(t, (char)v);
	else
		vterm_edit(t, v, code);
}

static
void vterm_edit_insert(des_vterm *t, const char* v, int quanti)
{
	int last = t->cont - 1;
	if (quanti + t->cont > t->orig_cont)
		return;
	for (char *p = t->orig_punt + last; p >= t->punt; p--)
		*(p + quanti) = *p;
	for (int i = 0; i < quanti; i++)
		t->punt[i] = v[i];
	t->cont += quanti;
	if (t->echo) {
		vterm_write_chars(t, t->punt, quanti);
		t->pref_off = t->video_off;
		vterm_rewrite_chars(t, t->punt + quanti, (t->orig_punt + t->cont) - (t->punt + quanti));
	}
	t->punt += quanti;
}

static
void vterm_edit_replace(des_vterm *t, const char *v, int quanti)
{
	if (t->punt + quanti > t->orig_punt + t->orig_cont)
		return;
	for (int i = 0; i < quanti; i++)
		t->punt[i] = v[i];
	if (t->echo) {
		vterm_write_chars(t, t->punt, quanti);
		t->pref_off = t->video_off;
		vterm_rewrite_chars(t, t->punt + quanti, (t->orig_punt + t->cont) - (t->punt + quanti));
	}
	t->punt += quanti;
	if (t->punt - (t->orig_punt + t->cont) > 0)
		t->cont += t->punt - (t->orig_punt + t->cont);
}

static
void vterm_edit_remove(des_vterm *t, int quanti)
{
	if (t->punt + quanti > t->orig_punt + t->cont)
		return;
	for (char *p = t->punt; p + quanti < t->orig_punt + t->cont; p++)
		*p = *(p + quanti);
	t->cont -= quanti;
	if (t->echo) 
		vterm_rewrite_chars(t, t->punt, (t->orig_punt + t->cont) - t->punt);
}

static
bool vterm_edit_moveto(des_vterm *t, natl off)
{
	natl scan = t->orig_off, prec = scan;
	char *p = t->orig_punt;
	while (scan <= off && p < t->orig_punt + t->cont) {
		prec = scan;
		scan += vterm_delta_off(t, scan, *p);
		p++;
	}
	if (vterm_row(t, prec) != vterm_row(t, t->video_off)) {
		t->punt = p - 1;
		vterm_move_cursor(t, prec);
		return true;
	}
	return false;
}

void vterm_edit(des_vterm* t, int v, natw code)
{
	if (vterm_global.flags & VTERM_MODIF_SHIFT) {
		vterm_motion(t, v, code);
		return;
	}

	if ((code & 0x0080) || (t->funzione != input_n && t->funzione != input_ln) ) 
		return;
	
	if (t->echo)
		sem_wait(t->mutex_w);

	switch (v) {
	case VTERM_EDIT_HOME:
		t->pref_off = t->orig_off;
		vterm_move_cursor(t, t->pref_off);
		t->punt = t->orig_punt;
		break;
	case VTERM_EDIT_END:
		t->punt = t->orig_punt + t->cont;
		t->pref_off = vterm_calc_off(t, t->orig_off, t->orig_punt, t->punt);
		vterm_move_cursor(t, t->pref_off);
		break;
	case VTERM_EDIT_LEFT:
		if (t->punt > t->orig_punt) {
			t->punt--;
			t->pref_off = vterm_calc_off(t, t->orig_off, t->orig_punt, t->punt);
			vterm_move_cursor(t, t->pref_off);
		}
		break;
	case VTERM_EDIT_RIGHT:
		if (t->punt < t->orig_punt + t->cont) {
			t->punt++;
			t->pref_off = vterm_calc_off(t, t->orig_off, t->orig_punt, t->punt);
			vterm_move_cursor(t, t->pref_off);
		}
		break;
	case VTERM_EDIT_UP:
		if (vterm_edit_moveto(t, t->pref_off - t->video_max_x)) {
			t->pref_off -= t->video_max_x;
		} 
		break;
	case VTERM_EDIT_PAGEUP:
		if (t->pref_off > t->vmon_size) {
		    	t->pref_off -= t->vmon_size;
			vterm_edit_moveto(t, t->pref_off);
		}
		break;
	case VTERM_EDIT_DOWN:
		if (vterm_edit_moveto(t, t->pref_off + t->video_max_x)) {
			t->pref_off += t->video_max_x;
		}
		break;
	case VTERM_EDIT_PAGEDOWN:
		if ( t->pref_off + t->vmon_size <= vterm_row(t, t->append_off) + t->video_max_x) {
			t->pref_off += t->vmon_size;
			vterm_edit_moveto(t, t->pref_off);
		}
		break;
	case VTERM_EDIT_INSERT:
		t->insert = !t->insert;
		vmon_cursor_shape(t->vmon, (t->insert ? 0 : 1));
		break;
	case VTERM_EDIT_DELETE:
		if (t->punt < t->orig_punt + t->cont) 
			vterm_edit_remove(t, 1);
		break;
	}
	if (t->echo)
		sem_signal(t->mutex_w);

}
void vterm_special(des_vterm*, int v, natw code)
{ }
void vterm_numlock(des_vterm* t, int v, natw code)
{
	if (code & 0x0080)
		return;

	vterm_global.numlock = !vterm_global.numlock;
	vkbd_leds(t->vkbd, VKBD_LED_NUMLOCK, vterm_global.numlock);
}
void vterm_capslock(des_vterm* t, int v, natw code)
{
	if (code & 0x0080)
		return;

	vterm_global.capslock = !vterm_global.capslock;
	vkbd_leds(t->vkbd, VKBD_LED_CAPSLOCK, vterm_global.capslock);
}
void vterm_scrlock(des_vterm* t, int v, natw code)
{
	if (code & 0x0080)
		return;

	sem_wait(t->mutex_w);
	t->scroll_lock = !t->scroll_lock;
	vkbd_leds(t->vkbd, VKBD_LED_SCROLLOCK, t->scroll_lock);
	while (t->waiting_scroll > 0) {
		sem_signal(t->scroll_sincr);
		t->waiting_scroll--;
	}
	sem_signal(t->mutex_w);
}
void vterm_sysrq(des_vterm*, int v, natw code)
{
}

const natl N_VTERM = 100;

des_vterm vterm[N_VTERM];
des_vterm* vterm_active;


void input_term(int h)
{
	natw code;
	des_vterm *p_des;

	p_des = &vterm[h];
	vkbd_intr_enable(p_des->vkbd, true);
	vkbd_wfi(p_des->vkbd);

	for(;;) {
		code = vkbd_read(p_des->vkbd);

		if (code == VTERM_SHUTDOWN)
			terminate_p();
		
		bool ext = (code & 0xff00) == 0xe000;
		int c = (code & 0x007f);
		vterm_map* map = 0;
		if (code == KBD_PAUSE)
			; // pause
		else if (ext)
			map = vterm_emaps;
		else 
			map = vterm_maps;

		if (map && map[c].action)
			(*map[c].action)(p_des, map[c].arg, code);

		vkbd_wfi(p_des->vkbd);
	}
}

void input_console(int h)
{
	natw code;
	des_vterm *p_des;

	p_des = &vterm[h];
	vkbd_intr_enable(p_des->vkbd, true);
	vkbd_wfi(p_des->vkbd);

	for(;;) {
		code = vkbd_read(p_des->vkbd);

		if (code == VTERM_SHUTDOWN)
			terminate_p();
		
		int c = (code & 0x007f);
		vterm_map* map = console_maps;
		if (map && map[c].action)
			(*map[c].action)(p_des, map[c].arg, code);

		vkbd_wfi(p_des->vkbd);
	}
}

static
natw vterm_mks(char c, natb attr)
{
	return (((natw)attr) << 8) | c;
}

static
void vterm_update_vmoncursor(des_vterm *t)
{
	vmon_setcursor(t->vmon, t->video_off - t->vmon_off);
}

static
int vterm_lazy_clear(des_vterm* t, int end)
{
	natl last = t->vmon_off + t->vmon_size;

	if (t->uncleared >= last)
		return end;

	for (; t->uncleared < last; t->uncleared++)
		t->video[(t->base + t->uncleared) % t->video_size] =
			vterm_mks(' ', t->clear_attr);
	return last;
}


static
void vterm_update_vmon(des_vterm *t, natl first, natl end)
{
	if (first >= t->vmon_off + t->vmon_size || end <= t->vmon_off)
	    return;
	
	if (first < t->vmon_off)
		first = t->vmon_off;

	if (end > t->vmon_off + t->vmon_size)
		end = t->vmon_off + t->vmon_size;

	end = vterm_lazy_clear(t, end);
	natl abs_first = (t->base + first) % t->video_size;
	natl abs_end   = (t->base + end)   % t->video_size;
	natl off = first - t->vmon_off;

	if (abs_end > abs_first) {
		vmon_write_n(t->vmon, off, &t->video[abs_first],         abs_end - abs_first);
	} else {
		natl slice = t->video_size - abs_first;
		vmon_write_n(t->vmon, off,         &t->video[abs_first], slice);
		vmon_write_n(t->vmon, off + slice, &t->video[0],         abs_end);
	}
	vterm_update_vmoncursor(t);
}

static 
void vterm_redraw_vmon(des_vterm *t)
{
	vterm_update_vmon(t, t->vmon_off, t->vmon_off + t->vmon_size);
}

static natl vterm_row(const des_vterm *t, natl off)
{
	return (off / t->video_max_x) * t->video_max_x;
}

static
bool vterm_make_visible(des_vterm *t, natl off)
{
	if (off >= t->vmon_off && off < t->vmon_off + t->vmon_size)
		return false;

	if (off < t->vmon_off)
		t->vmon_off = vterm_row(t, off);
	else
		t->vmon_off = vterm_row(t, off) + t->video_max_x - t->vmon_size;
	return true;
}

static
void vterm_addline(des_vterm *t)
{
	t->base = (t->base + t->video_max_x) % t->video_size;
	t->video_off  -= t->video_max_x;
	t->append_off -= t->video_max_x;
	t->vmon_off   -= t->video_max_x;
	t->orig_off   -= t->video_max_x;
	t->pref_off   -= t->video_max_x;
	t->uncleared  -= t->video_max_x;
}

static
void vterm_putcode(des_vterm *t, char c)
{
	t->video[(t->base + t->video_off) % t->video_size] = vterm_mks(c, t->attr);
	if (++t->video_off >= t->video_size)
		vterm_addline(t);
}

static
int vterm_delta_off(const des_vterm *t, natl off, char c)
{
	int noff = off;
	switch (c) {
	case '\n':
		noff = off + t->video_max_x;
	case '\r':
		noff = vterm_row(t, noff);
		break;
	case '\t':
		noff = (off / t->tab + 1) * t->tab;
		break;
	case '\b':
		if (off > 0)
			noff = off - 1;
		break;
	default:
		noff = off + 1;
		break;
	}
	return noff - off;
}

static
int vterm_calc_off(const des_vterm *t, natl off, const char* start, const char* end)
{
	for (; start < end; start++)
		off += vterm_delta_off(t, off, *start);
	return off;
}
	

static
void vterm_write_chars(des_vterm * t, const char vetti[], int quanti)
{
	natl first, last;
	int delta;

	first = last = t->video_off;
	for (int i = 0; i < quanti; i++) {
		char c = vetti[i];
		delta = vterm_delta_off(t, t->video_off, c);
		if (c == '\n' || c == '\t')
			c = ' ';
		if (delta > 0) {
			for (int j = 0; j < delta; j++) {
				vterm_putcode(t, c);
			}
			if (t->video_off > last)
				last = t->video_off;
		} else {
			t->video_off += delta;
			if (t->video_off < first)
				first = t->video_off;
		}
	}
	if (last > first) 
		vterm_update_vmon(t, first, last);
	if (t->video_off > t->append_off) {
		t->append_off = t->video_off;
		if (t->append_off > t->uncleared)
			t->uncleared = t->append_off;
	}
}

static
void vterm_rewrite_chars(des_vterm *t, const char vetti[], int quanti, bool visible)
{
	char c = ' ';
	natl save1 = t->video_off;
	vterm_write_chars(t, vetti, quanti);
	natl save2 = t->video_off;
	while (t->video_off < t->append_off)
		vterm_write_chars(t, &c, 1);
	vterm_move_cursor(t, save1);
	t->append_off = save2;
	if (visible && vterm_make_visible(t, t->append_off))
		vterm_redraw_vmon(t);
}

unsigned char vterm_mkattr(int fgcol, int bgcol, bool blink)
{
	return (fgcol & 0xf) | (bgcol & 0x7) << 4 | (blink ? 0x80 : 0x00);
}


void vterm_setcolor(natl v, int fgcol, int bgcol, bool blink)
{
	if(v >= N_VTERM) {
		flog(LOG_WARN, "vterm: terminale inesitente: %d", v);
		return;
	}

	des_vterm *t = &vterm[v];

	t->attr = vterm_mkattr(fgcol, bgcol, blink);
}

static
void vterm_move_cursor(des_vterm *t, natl off)
{
	t->video_off = off;
	if (vterm_make_visible(t, t->video_off)) 
		vterm_redraw_vmon(t);
	else
		vterm_update_vmoncursor(t);
	if (t->video_off > t->append_off)
		t->append_off = t->video_off;
}



void writevterm_n(natl v, const char vetti[], int quanti)
{
	if(v < 1 || v >= N_VTERM) {
		flog(LOG_WARN, "vterm: terminale inesitente: %d", v);
		return;
	}

	des_vterm* t = &vterm[v];

	sem_wait(t->mutex_w);

	while (t->scroll_lock) {
		t->waiting_scroll++;
		sem_signal(t->mutex_w);
		sem_wait(t->scroll_sincr);
		sem_wait(t->mutex_w);
	}

	vterm_move_cursor(t, t->append_off);

	vterm_write_chars(t, vetti, quanti);
	if (vterm_make_visible(t, t->video_off)) 
		vterm_redraw_vmon(t);

	if (t->funzione != none && t->echo) {
		t->orig_off = t->video_off;
		t->orig_punt += t->cont;
		t->pref_off = t->orig_off;
		t->punt = t->orig_punt;
		t->orig_cont -= t->cont;
		t->letti += t->cont;
		t->cont = 0;
	}
	sem_signal(t->mutex_w);
}

static void vterm_input_status(des_vterm *t, vterm_edit_status* stat)
{
	natl quanti = t->orig_cont;
	natl validi = (stat->validi > quanti ? quanti : stat->validi);
	natl cursore = (stat->cursore > validi ? validi : stat->cursore);
	sem_wait(t->mutex_w);
	vterm_write_chars(t, t->orig_punt, cursore);
	vterm_rewrite_chars(t, t->orig_punt + cursore, validi - cursore);
	sem_signal(t->mutex_w);
	t->cont = validi;
	t->punt += cursore;
}

static void vterm_output_status(des_vterm *t)
{
	t->stat.validi = t->cont;
	t->stat.cursore = t->punt - t->orig_punt;
}


static
void startvterm_in(des_vterm *p_des, char vetti[], int quanti, funz op, struct vterm_edit_status* stat)
{
	p_des->orig_cont = quanti;
	if (op == input_ln)
		p_des->orig_cont--;
	p_des->letti = p_des->cont = 0;
	p_des->orig_punt = p_des->punt = vetti;
	p_des->funzione = op;
	p_des->insert = true;
	p_des->echo = true;
	p_des->pref_off = p_des->orig_off = p_des->video_off;
	if (stat == VTERM_NOECHO)
		p_des->echo = false;
	else if (stat && stat->validi > 0)
		vterm_input_status(p_des, stat);
}


int readvterm_n(natl v, char vetti[], int quanti, struct vterm_edit_status* stat)
{
	des_vterm *p_des;
	natl letti;

	if (v < 1 || v >= N_VTERM) {
		flog(LOG_WARN, "vterm: terminale inesitente: %d", v);
		return 0;
	}

	if (quanti <= 0)
		return 0;

	p_des = &vterm[v];
	sem_wait(p_des->mutex_r);
	startvterm_in(p_des, vetti, quanti, input_n, stat);
	sem_wait(p_des->sincr);
	letti = p_des->letti;
	if (stat && stat != VTERM_NOECHO) 
		*stat = p_des->stat;
	sem_signal(p_des->mutex_r);
	return letti;
}

int readvterm_ln(natl v, char vetti[], int quanti, struct vterm_edit_status* stat)
{
	des_vterm *p_des;
	natl letti;

	if (v < 1 || v >= N_VTERM) {
		flog(LOG_WARN, "vterm: terminale inesitente: %d", v);
		return 0;
	}

	if (quanti <= 0)
		return 0;

	p_des = &vterm[v];
	sem_wait(p_des->mutex_r);
	startvterm_in(p_des, vetti, quanti, input_ln, stat);
	sem_wait(p_des->sincr);
	letti = p_des->letti;
	if (stat && stat != VTERM_NOECHO) 
		*stat = p_des->stat;
	sem_signal(p_des->mutex_r);
	return letti;
}


void vterm_switch(natl v)
{
	if (v >= N_VTERM)
		return;
	des_vterm *t = &vterm[v];
	if (t->num < 0)
		return;
	vterm_active = t;
	vkbd_leds(vterm_active->vkbd, VKBD_LED_NUMLOCK,  vterm_global.numlock);
	vkbd_leds(vterm_active->vkbd, VKBD_LED_CAPSLOCK, vterm_global.capslock);
	vkbd_switch(vterm_active->vkbd);
	vmon_switch(vterm_active->vmon);
}

bool vterm_setresident(natl v)
{
	if (v >= N_VTERM)
		return false;

	des_vterm *t = &vterm[v];

	natl size = t->video_size * sizeof(*t->video);

	if (! resident(t->video, size)) {
		flog(LOG_WARN, "vterm: buffer non residente");
		return false;
	}

	return true;
}

void vterm_clear(natl v)
{
	if (v >= N_VTERM)
		return;
	
	des_vterm *t = &vterm[v];


	sem_wait(t->mutex_w);
	if (t->funzione == none) {
		t->base = 0;
		t->video_off = t->append_off = t->vmon_off = 0;
		t->uncleared = 0;
		vterm_redraw_vmon(t);
	}
	sem_signal(t->mutex_w);
}

// inizializzazione

void vterm_iniconsole()
{
	natl maxx = 80, maxy = 25;
	des_vterm* p_des = &vterm[0];

	p_des->num = 0;
	if (!vmon_getsize(0, maxx, maxy)) {
		flog(LOG_WARN, "verm: vmon(0) non presente");
		return;
	}

	if ( (p_des->id = activate_p(input_console, 0, 100, LIV_UTENTE)) == 0xFFFFFFFF) {
		flog(LOG_WARN, "vterm: impossibile creare input_console");
		return;
	}
	p_des->vkbd = 0;
	p_des->vmon = 0;
	flog(LOG_INFO, "vterm: console initializzata");
}

bool vterm_init()
{
	natl maxx = 80, maxy = 25;
	natl i;

	for (i = 1; i < N_VTERM; i++) {
		des_vterm* p_des = &vterm[i];
		p_des->num = -1;
	}

	vterm_iniconsole();

	for (i = 1; i < N_VTERM; i++) {
		des_vterm* p_des = &vterm[i];

		p_des->num = i;
		if (!vmon_getsize(i, maxx, maxy))
			break;

		if ( (p_des->mutex_r = sem_ini(1)) == 0xFFFFFFFF) {
			flog(LOG_WARN, "vterm: impossibile creare mutex_r %d", i);
			return false;
		}
		if ( (p_des->mutex_w = sem_ini(1)) == 0xFFFFFFFF) {
			flog(LOG_WARN, "vterm: impossibile creare mutex_w %d", i);
			return false;
		}
		if ( (p_des->sincr = sem_ini(0)) == 0xFFFFFFFF) {
			flog(LOG_WARN, "vterm: impossibile creare sincr %d", i);
			return false;
		}
		p_des->video_max_x = maxx;
		p_des->video_max_y = maxy * 10;
		p_des->video_size = p_des->video_max_x * p_des->video_max_y;
		p_des->vmon_size  = p_des->video_max_x * maxy;
		if ( (p_des->video = (natw*)mem_alloc(p_des->video_size * sizeof(short))) == 0) {
			flog(LOG_WARN, "vterm: memoria terminata");
			return false;
		}
		if ( (p_des->id = activate_p(input_term, i, 100, LIV_UTENTE)) == 0xFFFFFFFF) {
			flog(LOG_WARN, "vterm: impossibile creare processo %d", i);
			return false;
		}

		p_des->scroll_lock = false;
		p_des->waiting_scroll = 0;
		if ( (p_des->scroll_sincr = sem_ini(0)) == 0xFFFFFFFF) {
			flog(LOG_WARN, "vterm: impossibile creare scroll_sincr %d", i);
			return false;
		}
		p_des->vkbd = i;
		p_des->vmon = i;
		p_des->funzione = none;
		p_des->tab = 8;
		p_des->clear_attr = vterm_mkattr(COL_LIGHTGRAY, COL_BLACK, false);
		vterm_setcolor(i, COL_LIGHTGRAY, COL_BLACK);
		vterm_clear(i);

	}
	flog(LOG_INFO, "vterm: creati %d terminali virtuali", i-1);

	vterm_switch(1);
	return true;
}

void vterm_shutdown()
{
	vkbd_send(-1, VTERM_SHUTDOWN, true);
}

// log formattato
void flog(log_sev sev, const char *fmt, ...)
{
	va_list ap;
	char buf[LOG_MSG_SIZE];

	va_start(ap, fmt);
	int l = vsnprintf(buf, LOG_MSG_SIZE, fmt, ap);
	va_end(ap);

	log(sev, buf, l);
}
struct des_mem {
	natl dimensione;
	des_mem* next;
};

des_mem* memlibera = 0;
natl mem_mutex;

void* mem_alloc(natl dim)
{
	natl quanti = (dim % sizeof(int) == 0) ? dim : ((dim + sizeof(int) - 1) / sizeof(int)) * sizeof(int);

	sem_wait(mem_mutex);
	
	des_mem *prec = 0, *scorri = memlibera;
	while (scorri != 0 && scorri->dimensione < quanti) {
		prec = scorri;
		scorri = scorri->next;
	}

	addr p = 0;
	if (scorri != 0) {
		p = scorri + 1; // puntatore al primo byte dopo il descrittore

		if (scorri->dimensione - quanti >= sizeof(des_mem) + sizeof(int)) {

			addr pnuovo = static_cast<natb*>(p) + quanti;
			des_mem* nuovo = static_cast<des_mem*>(pnuovo);

			nuovo->dimensione = scorri->dimensione - quanti - sizeof(des_mem);
			scorri->dimensione = quanti;

			nuovo->next = scorri->next;
			if (prec != 0) 
				prec->next = nuovo;
			else
				memlibera = nuovo;

		} else {

			if (prec != 0)
				prec->next = scorri->next;
			else
				memlibera = scorri->next;
		}
		
		scorri->next = reinterpret_cast<des_mem*>(0xdeadbeef);
		
	}

	sem_signal(mem_mutex);

	return p;
}

void free_interna(addr indirizzo, natl quanti);

void mem_free(void* p)
{
	if (p == 0) return;
	des_mem* des = reinterpret_cast<des_mem*>(p) - 1;
	sem_wait(mem_mutex);
	free_interna(des, des->dimensione + sizeof(des_mem));
	sem_signal(mem_mutex);
}

void free_interna(addr indirizzo, natl quanti)
{
	if (quanti == 0) return;
	des_mem *prec = 0, *scorri = memlibera;
	while (scorri != 0 && scorri < indirizzo) {
		prec = scorri;
		scorri = scorri->next;
	}
	if (prec != 0 && (natb*)(prec + 1) + prec->dimensione == indirizzo) {
		if (scorri != 0 && static_cast<natb*>(indirizzo) + quanti == (addr)scorri) {
			
			prec->dimensione += quanti + sizeof(des_mem) + scorri->dimensione;
			prec->next = scorri->next;

		} else {

			prec->dimensione += quanti;
		}
	} else if (scorri != 0 && static_cast<natb*>(indirizzo) + quanti == (addr)scorri) {
		des_mem salva = *scorri; 
		des_mem* nuovo = reinterpret_cast<des_mem*>(indirizzo);
		*nuovo = salva;
		nuovo->dimensione += quanti;
		if (prec != 0) 
			prec->next = nuovo;
		else
			memlibera = nuovo;
	} else if (quanti >= sizeof(des_mem)) {
		des_mem* nuovo = reinterpret_cast<des_mem*>(indirizzo);
		nuovo->dimensione = quanti - sizeof(des_mem);
		nuovo->next = scorri;
		if (prec != 0)
			prec->next = nuovo;
		else
			memlibera = nuovo;
	}
}

extern "C" natl end;

extern "C" bool lib_init()
{
	mem_mutex = sem_ini(1);
	free_interna(&end, 0xC0000000 - (natl)(&end));
	return vterm_init();
}

void resume(int channel)
{
		sndcommand(channel, 1);	
}
void pause(int channel)
{
		sndcommand(channel, 2);	
}
void stop(int channel)
{
		sndcommand(channel, 0);	
}
