#! /bin/bash 

DIRECTORY=/mnt/giuseppe/Tesi
DIRECTORY_BACKUP=/mnt/giuseppe/BACKUP/ 
DATE=`date '+%m%d%y%H%M%S'`
NAME="${DIRECTORY_BACKUP}tesi_backup_${DATE}.tar.bz2"

echo "Backup della directory " $DIRECTORY 
tar --exclude=swap.img --exclude=cdrom.img -cjf $NAME  $DIRECTORY 
echo "Eseguito backup $NAME" 
