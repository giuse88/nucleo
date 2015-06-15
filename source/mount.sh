#!/bin/bash
sudo mount -t vfat  -oloop,sync,offset=16384,gid=1000,uid=1000 /home/giuseppe/WARNING/Fat32.bin /home/giuseppe/Scrivania/Nucleo/test1
sudo mount -t vfat  -oloop,sync,offset=699990016,gid=1000,uid=1000 /home/giuseppe/WARNING/Fat32.bin /home/giuseppe/Scrivania/Nucleo/test2
