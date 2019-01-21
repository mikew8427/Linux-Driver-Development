#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdarg.h>

int DumpDir(char *dirname);

int main (int argc, char *argv[])
{
DIR *dp;
struct dirent *dirp;

         if(argc != 2) {
                printf("Usage: amdscan /dirname \n");
                return 16;
                }
           DumpDir(argv[1]);

return 0;
}

int DumpDir(char *dirname)
{
DIR *dp;
struct dirent *dirp;
struct stat sbuf;
char ftype[16];
char fullname[PATH_MAX*2];
char atime[64];
char mtime[64];
char ctime[64];
char perm[8];
char owngrp[8];

         if( (dp = opendir(dirname)) == NULL) {
                printf("Error: Directory not found [%s]",dirname);
                return 8;
                }
         while( (dirp = readdir(dp)) != NULL) {
                sprintf(fullname,"%s/%s",dirname,dirp->d_name);
                if(dirp->d_name[0]=='.') continue;
                lstat(fullname,&sbuf);
                if( (S_ISREG(sbuf.st_mode)) == 1 ) strcpy(ftype,"F");
                else if( (S_ISLNK(sbuf.st_mode)) == 1 ) strcpy(ftype,"L");
                else if( (S_ISSOCK(sbuf.st_mode))== 1 ) strcpy(ftype,"S");
                else if( (S_ISDIR(sbuf.st_mode)) == 1 ) strcpy(ftype,"D");
                sprintf(atime,"%s",asctime(localtime(&sbuf.st_atime)));
                sprintf(mtime,"%s",asctime(localtime(&sbuf.st_mtime)));
                sprintf(ctime,"%s",asctime(localtime(&sbuf.st_ctime)));
                sprintf(perm,"0%o",sbuf.st_mode & (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO));
                sprintf(owngrp,"%d:%d",sbuf.st_uid,sbuf.st_gid);

                printf("File %s %s %s %.8d : %s\n",ftype,perm,owngrp,sbuf.st_size,fullname);

                printf("Atime %sMtime %sCtime %s\n",atime,mtime,ctime);
                if(ftype[0]=='D') DumpDir(fullname);
                }
return 1;
}

