// util.c
#include "type.h"
/*********** globals in main.c ***********/
extern PROC   proc[NPROC];   // process table
extern PROC   *running;     // pointer to the currently running process

extern MINODE minode[NMINODE];   // In-memory Inodes structure array
extern MINODE *freeList;         // List of free inodes
extern MINODE *cacheList;        // List of inodes that are currently in use

extern MINODE *root; // Pointer to root directory inode

extern OFT    oft[NOFT]; // Open File Table

extern char gline[256];   // global line hold token strings of pathname
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings                    

// Variables for holding file system metadata
extern int ninodes, nblocks;
extern int bmap, imap, inodes_start, iblk;  // bitmap, inodes block numbers

// Variables for file descriptor and command processing
extern int  fd, dev;
extern char cmd[16], pathname[128], parameter[128];
extern int  requests, hits; // Variables for caching information

/**************** util.c file **************/

int get_block(int dev, int blk, char buf[ ]) // read a block of data from a device
{
  lseek(dev, blk*BLKSIZE, SEEK_SET); // set the read/write position at the beginning of the block
  int n = read(fd, buf, BLKSIZE); // read the block of data
  return n;  // return the number of bytes read
}

int put_block(int dev, int blk, char buf[ ])
{
  lseek(dev, blk*BLKSIZE, SEEK_SET); // set the read/write position at the beginning of the block
  int n = write(fd, buf, BLKSIZE); // write the block of data
  return n;  // return the number of bytes written
}    

//This whole function below is likely to get removed.
int sort_list(){
  MINODE *mip;
  MINODE* mipArr[NMINODE];
  int i, n = 0;
  mip = cacheList;

  if (!mip)
    return(0);

  printf("Resorting cacheList...\n");
  while (mip){
    mipArr[n] = mip;
    mip = mip->next;
    n++;
  }

  for(i = 0; i < n - 1; i++){ //Bubblesort used razor leaf!
    for (int j = 0; j < n - i - 1; j++){
      if (mipArr[j]->cacheCount > mipArr[j + 1]->cacheCount){
        mip = mipArr[j];
        mipArr[j] = mipArr[j+1];
        mipArr[j+1] = mip;
      }
    }
  }
  cacheList = mip = mipArr[0];

  for(i = 1; i < n && mip; i++)
  {
    mip->next = mipArr[i];
    mip = mip->next;
  }
  printf("cacheList resorted\n");
}

MINODE *mialloc() // allocate a FREE minode for use
{
  int i;
  for (i=0; i<NMINODE; i++){ // iterate over all minodes
    MINODE *mp = &minode[i];
    if (mp->shareCount == 0){ // if shareCount is 0, the minode is free
      mp->shareCount = 1; // set shareCount to 1 and return the minode
      return mp;
    }
  }
  printf("FS panic: out of minodes\n"); // if no free minodes, print an error message and return 0
  return 0;
}

int midalloc(MINODE *mip) // release a used minode
{
  mip->shareCount = 0;  // set shareCount to 0 to release the minode
}

int tokenize(char *pathname) // Takes a pathname and tokenizes it into an array of names
{
  int i, n = 0;
  char *s;
  printf("tokenize %s\n", pathname); // print what is being tokenized

  strcpy(gline, pathname); // copy pathname into global variable gpath

  s = strtok(gline, "/"); // tokenize gpath

  while(s){
    name[n++] = s;
    s = strtok(0, "/"); // continue tokenizing
  }
  name[n] = 0; // set last element in array to 0 (null terminating)

  return n; // return number of names
}

MINODE *enqueue(MINODE **list, MINODE *mip)
{
  //*list = 0;
  MINODE *m = *list;
  printf("mip ino = %d\n", mip->ino);
  
  while(m){
    if (m == mip)
      return 0;
    m = m->next; 
  }

  m = *list;
  if (m == 0 || mip->cacheCount < m->cacheCount){
    mip->next = *list;
    *list = mip;
    return mip;
  }

  while (m->next && mip->cacheCount >= m->next->cacheCount)
    m = m->next;
  mip->next = m->next;
  m->next = mip;
  return mip;
}

MINODE *dequeue(MINODE **list)
{
  MINODE *mip = *list;
  if (mip)
    *list = (*list)->next;

  return mip;
}

MINODE *iget(int dev, int ino) // return minode pointer of (dev, ino)
{
  MINODE *mip = cacheList;
  INODE *ip;
  int i, blk, offset;
  char buf[BLKSIZE];

    while(mip){
      if (mip->dev == dev && mip->ino == ino){
      //printf("found minode %d [%d %d] in cache\n", mip->id, dev, ino);
        mip->cacheCount++;
        mip->shareCount++;
        return mip;
      }
      mip = mip->next;
    }

  mip = dequeue(&freeList);
  if (mip){    // unused minodes are available
    mip->cacheCount = mip->shareCount = 1; mip->modified = 0;
    mip->dev = dev; mip->ino = ino; // assign to (dev, ino)

    blk = (ino-1) / 8 + iblk; // disk block containing this inode
    offset= (ino-1) % 8; // which inode in this block
    get_block(dev, blk, buf);

    ip = (INODE*)buf + offset;
    mip->INODE = *ip;

    enqueue(&cacheList, mip);
    return mip;
  }
  else{ 
    
    // sort_list; // start by reordering cachelist
    mip = cacheList;
    while(mip->shareCount != 0) // find minode from cacheList with sharecount=0 and smallest cacheCount
      mip = mip->next;
    mip->cacheCount = mip->shareCount = 1; mip->modified = 0;
    mip->dev = dev; mip->ino = ino; // assign to (dev, ino)
    return mip;
  }
}


int iput(MINODE *mip)  // release a mip
{
  INODE *ip;
  int i, block, offset;
  char buf[BLKSIZE];

  if (mip==0) return; // if mip is null, return
  
  mip->shareCount--;         // decrement shareCount to release the minode

  if (mip->shareCount > 0)   return; // if there are still users of the minode, return
  if (!mip->modified)        return; // if the minode has not been modified, return
     
  // calculate the block and offset of the minode's inode
  block = (mip->ino - 1) / 8 + mip->INODE.i_block;
  offset = (mip->ino - 1) % 8;

  // get block containing this inode
  get_block(mip->dev, block, buf);
  ip = (INODE *)buf + offset; // set ip to point to the inode

  *ip = mip->INODE; // copy the minode's INODE to the inode in the block
  put_block(mip->dev, block, buf); // write block back to disk
  midalloc(mip); // mip->refCount = 0; release the minode

  return(0); // return success

 
  // last user, INODE modified: MUST write back to disk
  //Use Mailman's algorithm to write minode.INODE back to disk)
  // NOTE: minode still in cacheList;

} 

int search(MINODE *mip, char *name)
{
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  // ASSUME only one data block i_block[0]
  // YOU SHOULD print i_block[0] number to see its value
  get_block(dev, mip->INODE.i_block[0], sbuf);

  dp = (DIR *)sbuf; // set the directory entry pointer to the start of sbuf
  cp = sbuf; // set the character pointer to the start of sbuf

  while(cp < sbuf + BLKSIZE){ // while there are still directory entries in the data block
    strncpy(temp, dp->name, dp->name_len); // copy the name from the directory entry to temp
    temp[dp->name_len] = 0;  // convert dp->name into a string

    printf("%8d%8d%8u       %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

    if (!strcmp(temp, name)){
      printf("found %s : ino = %d\n", dp->name, dp->inode); // print message indicating target found
      return dp->inode;
    }
    cp += dp->rec_len;      // advance cp by rec_len
    dp = (DIR *)cp;         // pull dp to cp
   }

   return(0);
}

MINODE *path2inode(char *pathname) 
{
  MINODE *mip;

  if (pathname[0] == '/') // If pathname starts with '/', set mip to root
    mip = root;
  else // Otherwise, set mip to the current working directory of the running process
    mip = running->cwd;

  n = tokenize(pathname); // Tokenize the pathname and store the number of tokens in n
  
  for (int i=0; i < n; i++){ // For each token in the pathname

    if (!S_ISDIR(mip->INODE.i_mode)){ // If the current MINODE is not a directory
      printf("Error: %s not a directory\n", name[i]); // Print error message and return 0
      return 0;
    }
    int ino = search(mip, name[i]); // Search for the directory entry matching the current token in the current directory
    if (ino==0)
      return 0;

    iput(mip);             // release current mip
    mip = iget(dev, ino);  // change to new mip of (dev, ino)
  }

  return mip; // Return the final MINOD
}   

int findmyname(MINODE *pip, int myino, char myname[ ]) 
{
  char sbuf[BLKSIZE];
  DIR *dp;
  char *cp;

  // ASSUME only one data block i_block[0]
  // YOU SHOULD print i_block[0] number to see its value
  get_block(dev, pip->INODE.i_block[0], sbuf);

  dp = (DIR *)sbuf; // set the directory entry pointer to the start of sbuf
  cp = sbuf; // set the character pointer to the start of sbuf

  while(cp < sbuf + BLKSIZE){ // while there are still directory entries in the data block
    if (dp->inode == myino){
    strncpy(myname, dp->name, dp->name_len); // copy the name from the directory entry to temp
    myname[dp->name_len] = 0;  // convert dp->name into a string

    // printf("%8d%8d%8u       %s\n", dp->inode, dp->rec_len, dp->name_len, *myname);
    printf("found %s : ino = %d\n", dp->name, dp->inode); // print message indicating target found
    return dp->inode;
    }
    cp += dp->rec_len;      // advance cp by rec_len
    dp = (DIR *)cp;         // pull dp to cp
  }
  printf("Error: inode not found\n");
  return(0);
}
 
int findino(MINODE *mip, int *myino) 
{
  char sbuf[BLKSIZE];
  DIR *dp;
  char *cp;

  // ASSUME only one data block i_block[0]
  // YOU SHOULD print i_block[0] number to see its value
  get_block(dev, mip->INODE.i_block[0], sbuf); // Read in the first data block of the inode into sbuf

  dp = (DIR *)sbuf; // set the directory entry pointer to the start of sbuf
  cp = sbuf; // set the character pointer to the start of sbuf

  *myino = dp->inode; // Get the inode number of the first directory entry and store it in myino
  cp += dp->rec_len; // Advance the character pointer by the length of the directory entry
  dp = (DIR *)cp; // Set the directory entry pointer to the next directory entry

  return dp->inode; // Return the inode number of the second directory entry

}