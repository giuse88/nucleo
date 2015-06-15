#! /bin/bash



################################################################
# Programma per la creazione dei volumi formatati con il fat32 #
################################################################

NEED_PROGRAM="dd:losetup:mkfs.msdos:fdisk:awk" 
SIZE_PART=1000     # 100M   grandezza in megabite  
NAME_DISK=Fat32.bin 
TYPE="p" # primaria o logica 
INPUT_FDISK=__command_input_fdisk_


# Verifichiamo che l'utente sia logato come root 
function isRoot { 
  if [ `id -u` != "0" ]
  then
      echo "Devi essere root per eseguire questo programma!"
      exit -1
  fi
}

# Verfico la presenza dei programmi neccessari per l'utilizzo dello script 
function checkProgram { 

    OIFS=$IFS
    IFS=":" 
    
    for prog in $NEED_PROGRAM   # ciclo uno nome   
    do 
	exist=0 

	for path in $PATH       # ciclo due path  
	do
 	    programma=$path'/'$prog
            if [ -f $programma ]
	    then 
		exist=1  
	    break 
	    fi 
	done

	if [ $exist -eq 0 ]
	then 
	    echo "Il programma $prog e' neccessario per poter usare $0."
	    exit -1 
	fi 
    done 

    IFS=$OIFS
}

function isNumber {
    
    if [[ $1 = *[!0-9]* ]]; then
	return 0
    else
	return 1
    fi
}

function checkParam { 
    
    if [ $# != 1  ]; then 
	go_exit
    fi 
    
    isNumber $1 
    
    if  [ $? == 0 ]; then 
	go_exit 
    fi

    N_PART=$1

    return 0 
}
 function go_exit {
     	echo "Parametri Errati"
	usage 
	exit -1 
}

function createFile { 
 
    size=$(( $N_PART * $SIZE_PART + 10 ))    #grandezza disco in mega 

    echo "Inizio creazione del file $NAME_DISK...." 
    dd if=/dev/zero of=$NAME_DISK count=$size bs=1M &> /dev/null  
    chmod a+xrw $NAME_DISK 
    echo "Fine crazione file size $size M"
	

}

function usage { 
    echo "Usage : $0 "
    echo -e " \\t - Numero Partizoni " 
}

function createExtend { 

    echo "n" >> $INPUT_FDISK 
    echo "e" >> $INPUT_FDISK
    echo "1" >> $INPUT_FDISK
    echo ""  >> $INPUT_FDISK
    echo ""  >> $INPUT_FDISK
}

function createPartition { 
    
    echo "n"               >> $INPUT_FDISK #nuova
    echo "$TYPE"           >> $INPUT_FDISK # primaria o logica
    echo ""                >> $INPUT_FDISK # invio 
   
    if [ $TYPE != "l" ]; then 
	echo ""                >> $INPUT_FDISK # invio 
    fi 

    echo "+${SIZE_PART}M"  >> $INPUT_FDISK # grandezza 
    echo "t"               >> $INPUT_FDISK # tipo 
    
    if [ $1 -ne 1 ];then 
	echo "$1"          >> $INPUT_FDISK # numero 
    fi 

    echo "b"               >> $INPUT_FDISK # FAT32
}

function writeMBR { 
     echo "w" >> $INPUT_FDISK 
     fdisk  $NAME_DISK < $INPUT_FDISK &> /dev/null

     if [ $? -ne 0 ]; then 
	 echo "Erroe Fdisk "
	 go_exit
	 rm $INPUT_FDISK 
     fi 
     

}
function createMBR { 
  
    echo "Creazione MBR...... "

    if [ -e  $INPUT_FDISK ]; then 
	rm $INPUT_FDISK 
    fi 

    touch $INPUT_FDISK 
    number_partition=1
 
    if [ $N_PART -gt 4 ] 
	then
	createExtend
	TYPE="l"
	number_partition=5 
    fi 
    
    i=0   
 
    while [ $i -lt $N_PART  ]; do  
 	createPartition $number_partition
	i=$(( $i + 1 )) 
	number_partition=$(( $number_partition + 1 )) 
    done 

    writeMBR 

    rm  $INPUT_FDISK

    echo "Fine creazione MBR."

}


function printTableDisk { 
    fdisk -l $NAME_DISK
}


function formatVolume {

    #posso usare k 
 #   echo $1 
    off=$(( $1 / 2 ))k  
 #   echo $3
     loop=`losetup -f` 
    
    if [ $2 != "b" ]; then 
	return 
    fi 

    losetup -o $off $loop  $NAME_DISK &> /dev/null

    if [ $? -ne 0 ]; then 
	 echo "Erroe bind losetup"
	 exit -1
    fi 
    
    mkfs.msdos -F32 $loop &> /dev/null

    if [ $? -ne 0 ]; then 
	 echo "Erroe formatazione losetup"
	 exit -1
    fi 

    sleep 1 

    losetup -d $loop &> /dev/null 
    
     if [ $? -ne 0 ]; then 
	 echo "Erroe delete losetup"
	 exit -1
    fi 
    
    
    

}


function formatMSDOS { 

    echo "Formatazione Volumi in corso...." 
    
    tmp=`fdisk -l $NAME_DISK | tail -n $N_PART`

    OIFS=$IFS
    IFS=$'\n' 

    for str in $tmp  
    do  
	a=`echo $str|awk '{print $1}'`
	n_part=${a#${a%?}}
        offset=`echo $str | awk '{print $2}'`
	type=`echo $str | awk '{print $5}'`
	formatVolume $offset $type
    done 

    IFS=$OIFS
    
    echo "Formatazione Conclusa." 

}


#main 
function main { 
    isRoot             # Diritti di root 
    checkProgram       # Programmi che utilizzo 
    checkParam $@      # Correttezza parametri
    createFile         # creo il file 
    createMBR          # creo l'MBR 
    printTableDisk     # stampo la tabella delle partizioni
    formatMSDOS        # formato i vari volumi 
}

main  $@


