
FILE=/home/giuseppe/WARNING/fat32.bin

head=$((2145280 +$1)) 
tail=$1

#echo $head $tail 
head -c$head $FILE > tmp
tail -c$1 tmp  > esa  
rm tmp 
