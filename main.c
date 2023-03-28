#include "type.h"

/********** globals **************/
PROC   proc[NPROC];
PROC   *running;

MINODE minode[NMINODE];   // in memory INODES
MINODE *freeList;         // free minodes list
MINODE *cacheList;        // cached minodes list

MINODE *root;             // root minode pointer

OFT    oft[NOFT];         // for level-2 only

char gline[256];          // global line hold token strings of pathname
char *name[64];           // token string pointers
int  n;                   // number of token strings                    

int ninodes, nblocks;     // ninodes, nblocks from SUPER block
int bmap, imap, inodes_start, iblk;  // bitmap, inodes block numbers

int  fd, dev;
char cmd[16], pathname[128], parameter[128];
int  requests, hits;

// start up files
#include "util.c"
#include "cd_ls_pwd.c"

int init()
{
  int i, j;
  // initialize minodes into a freeList
  for (i=0; i<NMINODE; i++){
    MINODE *mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->id = i;
    mip->next = &minode[i+1];
  }
  minode[NMINODE-1].next = 0;
  freeList = &minode[0];       // free minodes list

  cacheList = 0;               // cacheList = 0

  for (i=0; i<NOFT; i++)
    oft[i].shareCount = 0;     // all oft are FREE
 
  for (i=0; i<NPROC; i++){     // initialize procs
     PROC *p = &proc[i];    
     p->uid = p->gid = i;      // uid=0 for SUPER user
     p->pid = i+1;             // pid = 1,2,..., NPROC-1

     for (j=0; j<NFD; j++)
       p->fd[j] = 0;           // open file descritors are 0
  }
  
  running = &proc[0];          // P1 is running
  requests = hits = 0;         // for hit_ratio of minodes cache
}

char *disk = "diskimage";

int main(int argc, char *argv[ ]) 
{
  char line[128];
  char buf[BLKSIZE];

  init();
  
  fd = dev = open(disk, O_RDWR);
  printf("dev = %d\n", dev);  // YOU should check dev value: exit if < 0
  if (dev == 0){
    printf("Failed to open %s \n", disk);
    exit (1);
  }

  // get super block of dev
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;  // you should check s_magic for EXT2 FS
  printf("s_magic=%x  ", sp->s_magic);
  if (sp->s_magic != 0xEF53){ // print magic number
      printf("magic = %x it's not an ext2 file system\n", sp->s_magic); // EF53 is number that verifies that it's an ext2 file system
      exit(1);
  }
  printf("OK\n");// It is an ext2 system


  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;
  printf("ninodes=%d  nblocks=%d\n", ninodes, nblocks);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = inodes_start = gp->bg_inode_table;

  printf("bmap=%d  imap=%d  iblk=%d\n", bmap, imap, iblk);
  root = iget(dev, 2);
  running->cwd = root; 
  
  while(1){
    printf("P%d running\n", running->pid);
    pathname[0] = parameter[0] = 0;
      
    printf("enter command [cd|ls|pwd|exit] : ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;    // kill \n at end

    if (line[0]==0)
      continue;
      
    sscanf(line, "%s %s %64c", cmd, pathname, parameter);
    printf("pathname=%s parameter=%s\n", pathname, parameter);
      
    if (strcmp(cmd, "ls")==0)
	    ls();
    if (strcmp(cmd, "cd")==0)
	    cd();
    if (strcmp(cmd, "pwd")==0)
      pwd();

     
    if (strcmp(cmd, "show")==0)
	    show_dir(running->cwd);
    if (strcmp(cmd, "hits")==0)
	    hit_ratio();
    if (strcmp(cmd, "exit")==0)
	    quit();
  }
}


int show_dir(MINODE *mip)  // show contents of mip DIR: same as in LAB5
{
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  // ASSUME only one data block i_block[0]
  // YOU SHOULD print i_block[0] number to see its value
  get_block(dev, mip->INODE.i_block[0], sbuf); // read the block containing directory entries

  dp = (DIR *)sbuf; // set dp to point to the first directory entry in the block
  cp = sbuf; // set the character pointer to the start of sbuf

  while(cp < sbuf + BLKSIZE){ // loop through all directory entries in the block
    strncpy(temp, dp->name, dp->name_len); // copy the directory name into temp
    temp[dp->name_len] = 0;  // convert dp->name into a string  

    printf("%8d%8d%8u       %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

    cp += dp->rec_len;      // advance cp by rec_len
    dp = (DIR *)cp;         // pull dp to cp
   }

}

int hit_ratio()
{

}

int quit()
{
   MINODE *mip = cacheList;
   while(mip){
     if (mip->shareCount){
        mip->shareCount = 1;
        iput(mip);    // write INODE back if modified
     }
     mip = mip->next;
   }
   exit(0);
}







