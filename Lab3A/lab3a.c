/*
NAME: Jonathan Chon, William Tan
EMAIL: jonchon@gmail.com, willtan510@gmail.com
ID: 104780881, 104770108
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include "ext2_fs.h"

#define EXT2_S_IFLNK 0xA000
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFDIR 0x4000

int fsFD;
struct ext2_super_block bigBlock;  
struct ext2_group_desc groupInfo;
int fatal;
int block_size = 0;

void supBlock()
{  
  if (pread(fsFD,&bigBlock,sizeof(struct ext2_super_block),EXT2_MIN_BLOCK_SIZE) < 0)
    {
      fatal = errno;
      fprintf(stderr, "%s", strerror(fatal));
      exit(2);
    }

  if (bigBlock.s_magic != EXT2_SUPER_MAGIC)
    {
      fprintf(stderr, "Not ext2 filesystem");
      exit(1);
    }

  block_size = EXT2_MIN_BLOCK_SIZE << bigBlock.s_log_block_size;
  
  fprintf(stdout,"SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
	  bigBlock.s_blocks_count, //2 
	  bigBlock.s_inodes_count, //3
	  block_size, //4
	  bigBlock.s_inode_size, //5
	  bigBlock.s_blocks_per_group, //6
	  bigBlock.s_inodes_per_group, //7
	  bigBlock.s_first_ino); //8
}

void group()
{
  if (pread(fsFD,&groupInfo,sizeof(struct ext2_group_desc),EXT2_MIN_BLOCK_SIZE + sizeof(struct ext2_super_block)) < 0)
    {
      fatal = errno;
      fprintf(stderr, "%s", strerror(fatal));
      exit(2);
    }
  
  fprintf(stdout,"GROUP,0,%d,%d,%d,%d,%d,%d,%d\n",
	  //2 is always 0 since specs state there will be only a single group in the images
	  bigBlock.s_blocks_count, //3
	  bigBlock.s_inodes_count, //4
	  groupInfo.bg_free_blocks_count, //5
	  groupInfo.bg_free_inodes_count, //6
	  groupInfo.bg_block_bitmap, //7
	  groupInfo.bg_inode_bitmap, //8
	  groupInfo.bg_inode_table ); //9
  //missing 3,4,9
}

void freeBlocks()
{
  for(int i=0;i<block_size;i++)  //0 to blocksize
    {
      int byte;
      if(pread(fsFD,&byte,1,block_size*groupInfo.bg_block_bitmap+i) < 0)
	{
	  fatal = errno;
	  fprintf(stderr, "%s", strerror(fatal));
	  exit(2);
	}
      for(int j=0;j<8;j++)
	{
	  if((byte&(1<<j))>>j==0)  //if bit is free
	    {
	      fprintf(stdout,"BFREE,%d\n",(i*8)+(j+1));  //account for byte # and bit #
	    }
	}
    }
}

void freeInodes()
{
  for (int i = 0; i < block_size; i++)
    {
      int byte;
      pread(fsFD, &byte,1,block_size*groupInfo.bg_inode_bitmap+i);
      for (int j = 0; j < 8; j++)
        {
	  if ((byte&(1<<j))>>j==0)
            {
	      fprintf(stdout,"IFREE,%d\n",(i*8)+(j+1));
            }
        }
    }
}

void indirectBlock(int level, int node_num, int block_num, int offset)
{
  int num_entries = block_size/sizeof(uint32_t);
  int entries[num_entries];
  memset(entries, 0, num_entries*sizeof(int));
  if (pread(fsFD, entries, block_size, 1024 + (block_num - 1) * block_size) < 0)
    {
      fatal = errno;
      fprintf(stderr, "%s", strerror(fatal));
      exit(2);
    }
  for (int i = 0; i < num_entries; i++)
    {
      if (entries[i] == 0)
	{
	  continue;
	}
      if (level == 1)
	{
	  offset+=i;
	}
      if (level == 2)
	{
	  offset+=i*256;
	  indirectBlock(level-1, node_num, entries[i], offset);
	}
      if (level == 3)
	{
	  offset+=i*65536;
	  indirectBlock(level-1, node_num, entries[i], offset);
	}
       fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
	      node_num, //2
	      level, //3
	      offset, //4
	      block_num, //5
	      entries[i]); //6
    }
}

void inodeSummary()
{
  struct ext2_inode node;
  
  for(unsigned int i=0;i<bigBlock.s_inodes_count;i++)
    {
      int inode_offset = 5 * 1024 + i * sizeof(struct ext2_inode);
      pread(fsFD,&node,sizeof(struct ext2_inode),inode_offset);
      if(node.i_mode==0||node.i_links_count==0)
	continue;
      else 
	{
	  char fileType;
	  if(node.i_mode&EXT2_S_IFREG)
	    fileType='f';
	  else if(node.i_mode&EXT2_S_IFDIR)
	    fileType='d';
	  else if(node.i_mode&EXT2_S_IFLNK)
	    fileType='s';
	  else
	    fileType='?';
      
	  int mask= 0b111111111111;
	  int mode=mask&node.i_mode;  
	  int ownerID=node.i_uid;
	  int groupID=node.i_gid;
	  int linkCount=node.i_links_count;
      
	  int createTime=node.i_ctime;
	  char createString[30];
	  int modTime=node.i_mtime;
	  char modString[30];
	  int accessTime=node.i_atime;
	  char accessString[30];
      
	  time_t total=createTime;
	  struct tm timeStruct=*gmtime(&total);
	  strftime(createString,30,"%m/%d/%y %H:%M:%S", &timeStruct);
	  total=modTime;
	  timeStruct=*gmtime(&total);
	  strftime(modString,30,"%m/%d/%y %H:%M:%S", &timeStruct);
	  total=accessTime;
	  timeStruct=*gmtime(&total);
	  strftime(accessString,30,"%m/%d/%y %H:%M:%S", &timeStruct);
      
      int fSize=node.i_size;
      int numBlocks=node.i_blocks;
      
      fprintf(stdout,"INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
	      i+1,
	      fileType,
	      mode,
	      ownerID,
	      groupID,
	      linkCount,
	      createString,
	      modString,
	      accessString,
	      fSize,
	      numBlocks);
      for(int j=0;j<EXT2_N_BLOCKS;j++)
        fprintf(stdout,",%d",node.i_block[j]);
      fprintf(stdout,"\n");

      //Directory Entries
      if (fileType == 'd')
	{
	  struct ext2_dir_entry directory_entry;
	  for (int k = 0; k < EXT2_NDIR_BLOCKS; k++)
	    {
	      if (node.i_block[k] == 0)
		{
		  break;
		}
	      int offset = 0;
	      while (offset < 1024)
		{
		  if (pread(fsFD, &directory_entry, sizeof(struct ext2_dir_entry), offset + node.i_block[k]*1024) < 0)
		    {
		      fatal = errno;
		      fprintf(stderr, "%s", strerror(fatal));
		      exit(2);
		    }
		  if (directory_entry.name_len == 0)
		    {
		      break;
		    }
		  char * name = malloc((directory_entry.name_len+2) * sizeof(char));
		  name[0] = '\'';
		  for (int j = 0; j < directory_entry.name_len; j++)
		    {
		      name[j+1] = directory_entry.name[j];
		    }
		  name[directory_entry.name_len+1] = '\'';
		  name[directory_entry.name_len+2] = '\0';
		  fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,%s\n",
			  i+1, //2
			  offset, //3
			  directory_entry.inode, //4
			  directory_entry.rec_len, //5
			  directory_entry.name_len, //6
			  name); //7
		  offset += directory_entry.rec_len;
		}
	    }	  
	}
      //Indirect      
      if (node.i_block[EXT2_IND_BLOCK] > 0)
	{
      	  indirectBlock(1, i+1, node.i_block[EXT2_IND_BLOCK], 12);
	}
      //Double Indirect
      if (node.i_block[EXT2_DIND_BLOCK] > 0)
	{
	  indirectBlock(2, i+1, node.i_block[EXT2_DIND_BLOCK], 268);
	}
      //Triple Indirect
      if (node.i_block[EXT2_TIND_BLOCK] > 0)
	{
	  indirectBlock(3, i+1, node.i_block[EXT2_TIND_BLOCK], 65804);
	}
    }
  }
}

int main (int argc, char** argv)
{
  if(argc!=2)
  {
    fprintf(stderr,"Error: incorrect number of command-line arguments\n");
    exit(1);
  }
 
  //img file to be observed
  fsFD=open(argv[1],O_RDONLY);
  if(fsFD<0)
  {
    fatal=errno;
    fprintf(stderr,"%s\n",strerror(fatal));
    exit(1);
  }
  //following lines, idk yet
  supBlock();
  group();
  freeBlocks();
  freeInodes();
  inodeSummary();
}
