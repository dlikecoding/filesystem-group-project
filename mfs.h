/**************************************************************
* Class::  CSC-415-03 FALL 2024
* Name:: Danish Nguyen, Atharva Walawalkar, Arvin Ghanizadeh, Cheryl Fong
* Student IDs:: 923091933, 924254653, 922810925, 918157791
* GitHub-Name:: dlikecoding, AtharvaWal2002, arvinghanizadeh, cherylfong
* Group-Name:: 0xAACD
* Project:: Basic File System
*
* File:: mfs.h
*
* Description:: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/

#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"

#include "structs/VCB.h"
#include "fsLow.h"
#include "structs/ParsePath.h"

#define MAX_PATH_LENGTH 1024

#include <sys/stat.h>
#include <dirent.h>
#define FT_REGFILE	DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK	DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

/**
 * Parse a path string and return a ParserStruct containing the parent directory,
 * index of the last element, and the last element's name.
 * 
 * @param path The path string to parse (can be absolute or relative)
 * @param result Pointer to ParserStruct to store parsing results
 * @return 0 on success, -1 on failure
 */
int parsePath(const char* path, parsepath_st* parser);

char* cleanPath(const char* srcPath);
int isDirEmpty(directory_entry *de);
int deleteBlod(const char* pathname, int isDir);
int makeDirOrFile(parsepath_st parser, int isDir, directory_entry* newDir);


// This structure is returned by fs_readdir to provide the caller with information
// about each file as it iterates through a directory
struct fs_diriteminfo
	{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    	/* D for directory; F for file*/
    char d_name[256]; 			/* filename max filename is 255 characters */
	};

// This is a private structure used only by fs_opendir, fs_readdir, and fs_closedir
// Think of this like a file descriptor but for a directory - one can only read
// from a directory.  This structure helps you (the file system) keep track of
// which directory entry you are currently processing so that everytime the caller
// calls the function readdir, you give the next entry in the directory
typedef struct
	{
	unsigned short  d_reclen;			/* length of this record */
	unsigned short	dirEntryPosition;	/* which directory entry position, like file pos */
	directory_entry * de;				/* Pointer to the loaded directory you want to iterate */
	struct fs_diriteminfo * di;			/* Pointer to the structure you return from read */
	} fdDir;

// Key directory functions
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Misc directory functions
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise
int fs_delete(const char* filename);	//removes a file


// This is the strucutre that is filled in from a call to fs_stat
struct fs_stat
	{
	off_t     st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;   	/* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;   	/* time of last status change */
	};

int fs_stat(const char *path, struct fs_stat *buf);

#endif

