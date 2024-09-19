***EXT2 FILE SYSTEM PROJECT***

**Getting Started**

First you must obtain the ext2_fs.h header file. To do this, use the command $sudo apt-get install e2fslibs-dev

Next, you must modify the mkdisk sh script. On the line 'sudo chown randall diskimage', change 'randall' to the user for your computer. 
NOTE: user must have superuser privileges for this project.

This repository contains code to implement file operations on an EXT2 File System. Supported operations include:
* cd
* ls
* pwd
* mkdir
* creat
* rmdir
* link
* unlink
* symlink
* chmod
* open
* close
* dup
* dup2
* pfd
* cat
* cp
* mv
* head
* tail
* exit
