#!/bin/bash

gcc -g -DHAVE_CONFIG_H -I. -I..  -o link link.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o access access.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o create create.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o lookup lookup.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o mkdir mkdir.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o mknod mknod.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o remove remove.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o rename rename.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o rmdir rmdir.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o symlink symlink.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs

gcc -g -DHAVE_CONFIG_H -I. -I..  -o create_without_cred create_without_cred.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o access_without_cred access_without_cred.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
gcc -g -DHAVE_CONFIG_H -I. -I..  -o rename_without_cred rename_without_cred.c ../common/frame_driver.c ../common/file_clean.c ../common/fh_generate.c -lnfs
