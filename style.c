// style.c
#include "type.h"
/*********** globals in main.c ***********/
extern PROC *running;

extern char cmd[16], pathname[128], parameter[128];

/**************** style.c file **************/

void black() { printf("\033[1;30m");}
void red() { printf("\033[1;31m");}
void green() { printf("\033[1;32m");}
void yellow() { printf("\033[1;33m");}
void blue() { printf("\033[1;34m");}
void purple() { printf("\033[1;35m");}
void cyan() { printf("\033[1;36m");}
void white() { printf("\033[1;37m");}

int do_command()
{
    int fd, gd;

    if (strcmp(cmd, "ls")==0) // list files
        ls();
    if (strcmp(cmd, "cd")==0) // change directory
        cd();
    if (strcmp(cmd, "pwd")==0) // print working directory
        pwd();

     
    if (strcmp(cmd, "show")==0) // show directoy of current running program
        show_dir(running->cwd);
    if (strcmp(cmd, "hits")==0) //
        hit_ratio();
    if (strcmp(cmd, "exit")==0) // exit the program
        quit();

    if (strcmp(cmd, "mkdir")==0)
        make_dir();
    if (strcmp(cmd, "creat")==0)
        creat_file();
    if (strcmp(cmd, "rmdir")==0)
        rmdir();

    if (strcmp(cmd, "link")==0)
        link();
    if (strcmp(cmd, "unlink")==0)
        unlink();
    if (strcmp(cmd, "symlink")==0)
        symlink();

    if (strcmp(cmd, "chmod")==0)
        do_chmod();

    if (strcmp(cmd, "open")==0)
        open_file();
    if (strcmp(cmd, "close")==0){
        fd = atoi(pathname);
        close_file(fd);
    }
    if (strcmp(cmd, "dup")==0){
        fd = atoi(pathname);
        dup(fd);
    }
    if (strcmp(cmd, "dup2")==0){
        fd = atoi(pathname);
        gd = atoi(parameter);
        dup2(fd, gd);
    }
    if (strcmp(cmd, "pfd")==0)
        pfd();

    if (strcmp(cmd, "read")==0)
        read_file();

    if (strcmp(cmd, "write")==0)
        write_file();

    if (strcmp(cmd, "cat")==0)
        mycat(pathname);
    if (strcmp(cmd, "cp")==0)
        mycp();
    if (strcmp(cmd, "mv")==0)
        mymv();

    if (strcmp(cmd, "head")==0)
        myhead();
    if (strcmp(cmd, "tail")==0)
        mytail();
}

int show_tux()
{
    white();printf("\n");printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$$$$$");black();printf(".88888888:.");white();printf("$$$$$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$$$$");black();printf("88888888.88888.");white();printf("$$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$$");black();printf(".8888888888888888.");white();printf("$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$$");black();printf("888888888888888888");white();printf("$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$$");black();printf("88' _`88'_  `88888");white();printf("$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$$");black();printf("88 88 88 88  88888");white();printf("$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$$");black();printf("88_88_");yellow();printf("::");black();printf("_88_:88888");white();printf("$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$$");black();printf("88");yellow();printf(":::,::,:::::");black();printf("8888");white();printf("$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$$");black();printf("88");yellow();printf("`:::::::::'`");black();printf("8888");white();printf("$$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$$");black();printf(".88  ");yellow();printf("`::::'");black();printf("    8:88.");white();printf("$$$$$$$$$$$$\n");
    printf("$$$$$$$$$$$$");black();printf("8888            `8:888.");white();printf("$$$$$$$$$$\n");
    printf("$$$$$$$$$$");black();printf(".8888'             `888888.");white();printf("$$$$$$$$\n");
    printf("$$$$$$$$$");black();printf(".8888:..  .::.  ...:'8888888:.");white();printf("$$$$$$\n");
    printf("$$$$$$$$");black();printf(".8888.'     :'     `'::`88:88888");white();printf("$$$$$\n");
    printf("$$$$$$$");black();printf(".8888        '         `.888:8888.");white();printf("$$$$\n");
    printf("$$$$$$");black();printf("888:8         .           888:88888");white();printf("$$$$\n");
    printf("$$$$");black();printf(".888:88        .:           888:88888:");white();printf("$$$\n");
    printf("$$$$");black();printf("8888888.       ::           88:888888");white();printf("$$$$\n");
    printf("$$$$");black();printf("`");yellow();printf(".::.");black();printf("888.      ::          .88888888");white();printf("$$$$$\n");
    printf("$$$");yellow();printf(".::::::.");black();printf("888.    ::         ");yellow();printf(":::");black();printf("`8888'");yellow();printf(".:.");white();printf("$$$\n");
    printf("$$");yellow();printf("::::::::::.");black();printf("888   '         .");yellow();printf("::::::::::::");white();printf("$$$\n");
    printf("$$");yellow();printf("::::::::::::.");black();printf("8    '      .:8");yellow();printf("::::::::::::.");white();printf("$$\n");
    printf("$");yellow();printf(".::::::::::::::.");black();printf("        .:888");yellow();printf(":::::::::::::");white();printf("$$\n");
    printf("$");yellow();printf(":::::::::::::::");black();printf("88:.__..:88888");yellow();printf(":::::::::::'");white();printf("$$$\n");
    printf("$$");yellow();printf("''.:::::::::::");black();printf("88888888888.88");yellow();printf(":::::::::'");white();printf("$$$$$\n");
    printf("$$$$$$$$");yellow();printf("'':::_:'");black();printf(" -- '' -'-' ''");yellow();printf(":_::::''");white();printf("$$$$$$$\n");
    printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n\n");
    printf("<Press enter to continue> ");
    char c = getchar();
    putchar('\n');
}