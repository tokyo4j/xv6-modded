// On-disk file system format.
// Both the kernel and user programs use this header file.

#pragma once

#include <xv6/bio.h>
#include <xv6/file.h>
#include <xv6/stat.h>
#include <xv6/types.h>

#define ROOTINO 1 // root i-number

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint size;       // Size of file system image (blocks)
  uint nblocks;    // Number of data blocks
  uint ninodes;    // Number of inodes.
  uint nlog;       // Number of log blocks
  uint logstart;   // Block number of first log block
  uint inodestart; // Block number of first inode block
  uint bmapstart;  // Block number of first free map block
};

#define NDIRECT   12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE   (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;              // File type
  short major;             // Major device number (T_DEV only)
  short minor;             // Minor device number (T_DEV only)
  short nlink;             // Number of links to inode in file system
  uint size;               // Size of file (bytes)
  uint addrs[NDIRECT + 1]; // Data block addresses
};

// Inodes per block.
#define IPB (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb) ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB (BSIZE * 8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) (b / BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

// Exclude definitions below when included from mkfs.c
#ifndef MKFS

// in-memory copy of an inode
struct inode {
  uint dev;              // Device number
  uint inum;             // Inode number
  int ref;               // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;             // inode has been read from disk?
  short type;            // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT + 1];
};

void readsb(int dev, struct superblock *sb);
int dirlink(struct inode *dp, const char *name, uint inum);
struct inode *dirlookup(struct inode *dp, const char *name, uint *poff);
struct inode *ialloc(uint dev, short type);
struct inode *idup(struct inode *dp);
void iinit(int dev);
void ilock(struct inode *ip);
void iput(struct inode *);
void iunlock(struct inode *ip);
void iunlockput(struct inode *ip);
void iupdate(struct inode *ip);
int namecmp(const char *s, const char *t);
struct inode *namei(const char *path);
struct inode *nameiparent(const char *path, char *name);
int readi(struct inode *ip, char *dst, uint off, uint n);
void stati(struct inode *ip, struct stat *st);
int writei(struct inode *ip, const char *src, uint off, uint n);

#endif
