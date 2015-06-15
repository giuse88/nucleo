START_SISTEMA=   0x00100000
START_IO=        0x40400000
START_UTENTE=	 0x80000000
SWAP=		 swap.img

CXXFLAGS=-fleading-underscore -fno-exceptions -fno-rtti -fno-stack-protector -g -fcall-saved-esi -fcall-saved-edi -fcall-saved-ebx -Wall 
CPPFLAGS=-nostdinc -Iinclude -g -Wall 
CFLAGS=-fno-stack-protector -fleading-underscore -fno-exceptions -nostdinc -g -fcall-saved-esi -fcall-saved-edi -fcall-saved-ebx  
LIBFLAGS=-fno-stack-protector -fno-exceptions -nostdinc -g -fcall-saved-esi -fcall-saved-edi -fcall-saved-ebx
GRUB_MKRESCUE=grub-mkrescue

all: swap   build/sistema   build/parse     build/creatimg    cdrom.img 

boot/boot/sistema: build/sistema
	cp build/sistema boot/boot/sistema

cdrom.img: boot/boot/sistema
	${GRUB_MKRESCUE} --output=cdrom.img boot
     
build/sistema: sistema/sist_s.o sistema/sist_cpp.o
	ld -nostdlib -o build/sistema -Ttext $(START_SISTEMA) sistema/sist_s.o sistema/sist_cpp.o

build/io: io/io_s.o io/io_cpp.o  filesystem.o
	ld -nostdlib -o build/io -Ttext $(START_IO) io/io_s.o io/io_cpp.o fs/fat.o fs/volumi.o fs/data.o fs/direntry.o fs/filesystem.o lib/errno.o lib/string.o lib/wchar.o lib/stdarg.o   
build/utente: utente/uten_s.o utente/lib.o utente/uten_cpp.o
	ld -nostdlib -o build/utente -Ttext $(START_UTENTE) utente/uten_cpp.o utente/uten_s.o utente/lib.o

# compizione di sistema.s e sistema.cpp
sistema/sist_s.o: sistema/sistema.S include/costanti.h
	gcc $(CPPFLAGS) -c sistema/sistema.S -o sistema/sist_s.o

sistema/sist_cpp.o: sistema/sistema.cpp include/mboot.h include/costanti.h
	g++ $(CPPFLAGS) $(CXXFLAGS) -c sistema/sistema.cpp -o sistema/sist_cpp.o

# compilazione di io.s e io.cpp
io/io_s.o: io/io.S include/costanti.h
	gcc $(CPPFLAGS) -c io/io.S -o io/io_s.o 

io/io_cpp.o: io/io.cpp include/costanti.h  fat.o
	g++ $(CPPFLAGS) $(CXXFLAGS) -Ifs -c io/io.cpp -o io/io_cpp.o

#compilazione file system 
volumi.o : 
	gcc -c fs/volumi.c -g $(CFLAGS)  -o  fs/volumi.o  -Iinclude  -Ifs
fat.o : 
	gcc -c fs/fat.c -g  $(CFLAGS) -o  fs/fat.o -Ifs -Iinclude 
errno.o :
	gcc -c lib/errno.c -g $(CFLAGS)  -o lib/errno.o -Ifs -Iinclude
string.o : 
	gcc -c lib/string.c -g $(CFLAGS) -o lib/string.o -Ifs -Iinclude 
stdarg.o :	
	gcc -c lib/stdarg.c -g $(CFLAGS) -o lib/stdarg.o -Ifs -Iinclude
wchar.o : 
	gcc -c lib/wchar.c -g $(CFLAGS) -o lib/wchar.o  -Iinclude 
data.o : 
	gcc -c fs/data.c  -g $(CFLAGS) -o fs/data.o -Ifs -Iinclude 
direntry.o: fs/direntry.c fs/direntry.h
	gcc -c fs/direntry.c -g -fno-stack-protector -fleading-underscore  -o  fs/direntry.o -Ifs -Iinclude 

filesystem.o :   errno.o  string.o stdarg.o wchar.o direntry.o fat.o volumi.o data.o 
	gcc -c fs/filesystem.c  $(CFLAGS) -g  -Wall   -o fs/filesystem.o  -Ifs -Iinclude

utente/uten_s.o: utente/utente.S
	gcc $(CPPFLAGS) -c utente/utente.S -o utente/uten_s.o

utente/utente.cpp: build/parse utente/prog/*.in utente/include/* utente/prog
	build/parse -o utente/utente.cpp utente/prog/*.in

utente/uten_cpp.o: utente/utente.cpp
	g++ $(CXXFLAGS) $(CPPFLAGS) -Iutente/include -c utente/utente.cpp -o utente/uten_cpp.o

utente/lib.o: utente/lib.cpp utente/include/lib.h
	g++ $(CXXFLAGS) $(CPPFLAGS) -Iutente/include -c utente/lib.cpp -o utente/lib.o

# creazione di parse e createimg
build/parse: util/parse.c util/src.h
	gcc -g -o build/parse util/parse.c

util/coff.o: include/costanti.h util/interp.h util/coff.h util/dos.h util/coff.cpp
	g++ -c -g -o util/coff.o util/coff.cpp

util/elf.o:  include/costanti.h util/interp.h util/elf.h util/elf.cpp
	g++ -c -g  -o util/elf.o util/elf.cpp

util/interp.o: include/costanti.h util/interp.h util/interp.cpp
	g++ -c -g -o util/interp.o util/interp.cpp

util/swap.o: include/costanti.h util/swap.h util/swap.cpp
	g++ -c -g -o util/swap.o util/swap.cpp

util/fswap.o: include/costanti.h util/swap.h util/fswap.cpp
	g++ -c -g -o util/fswap.o util/fswap.cpp

util/creatimg.o: util/interp.h util/swap.h util/creatimg.cpp
	g++ -c -g -o util/creatimg.o util/creatimg.cpp

build/creatimg: util/creatimg.o util/elf.o util/coff.o util/interp.o util/swap.o util/fswap.o
	g++ -g -o build/creatimg util/creatimg.o util/elf.o util/coff.o util/interp.o util/swap.o util/fswap.o

.PHONY: swap clean reset
swap: build/creatimg build/io build/utente
	build/creatimg $(SWAP) build/io build/utente

clean:
	-rm -f sistema/*.o io/*.o utente/*.o util/*.o fs/*.o lib/*.o

reset: clean
	rm -f build/* swap 
qemu : 
	qemu -cdrom cdrom.img -hda swap.img -hdb fat32.bin -m 512M 

debug : 
	qemu -S -s -cdrom  cdrom.img -hda swap.img -hdb fat32.bin & 
	gdb
