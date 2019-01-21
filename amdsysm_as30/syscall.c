//
// Syscall setup for hooking driver
//
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>

//#if CONFIG_MODVERSIONS==1
//        #define MODVERSIONS
//        #include <linux/modversions.h>
//#endif

#include <sys/syscall.h>
#include <linux/sched.h>

//#ifndef KERNEL_VERSION
//        #define KERNEL_VERSION(a,b,c) ((a)*65536+(b)*256+(c))
//#endif

//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
//        #include <asm/uaccess.h>
//#endif

#define MAXBUFSZ        4096
#define MAXMASK         64
#define MODULE_NAME "AMDSYSM"
//extern void *sys_call_table[];
extern int loops_per_jiffy;
unsigned long **sct=NULL;

static char datbuf[MAXBUFSZ];
static int datbufnxt;
static int uid;
static int MaxMasks;
struct semaphore globalbuf;
static char PathIncludeFilters[MAXMASK][128]={"/etc/amdmon.conf",""};
static int Alert[MAXMASK];
static int Lockdown[MAXMASK];


//static int myget_info(char *buf,char **start, off_t offset, int len, int unused);


struct proc_dir_entry *myproc=NULL;
struct proc_dir_entry *input=NULL;
struct proc_dir_entry *myroot=NULL;


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
        MODULE_PARM(uid,"i");
#endif

asmlinkage int (*original_link)(const char *,const char *);             	/* Orginal Link call */

asmlinkage int (*original_unlink)(const char *);            		        /* Orginal UnLink call */

asmlinkage int (*original_open)(const char *,int,int);                  	/* Orginal Open call */

asmlinkage long (*original_close)(unsigned int fd);

asmlinkage ssize_t (*original_read)(unsigned int fd, char * buf, size_t count);

asmlinkage long (*original_chmod)(const char *fn,mode_t mode);

asmlinkage long (*original_fchown)(unsigned int fd, uid_t user, gid_t group);

asmlinkage long (*original_chown)(const char * filename, uid_t user, gid_t group);

asmlinkage long (*original_lchown)(const char * filename, uid_t user, gid_t group);

asmlinkage int (*getuid_call)();

int SaveEvent(char *fname, char *process, char *p,char *call);            // Save an event away

int trim(char *data);

static int proc_read(char *buf,char **start, off_t offset,int count,int *eof, void *data);
static int proc_write(struct file *file,const char *buf,size_t count, loff_t *p);



int IsMatch(char *fullname);

int LoadConf();

//
// Hooked call. OS will call us First
//
asmlinkage int hook_sys_lchown(const char * filename, uid_t user, gid_t group)
{
char cuid[8];

         uid=getuid_call();
         sprintf(cuid,"%d",uid);
         SaveEvent(filename,cuid,current->comm,"LCHOWN");

return -EPERM;
}

//
// Hooked call. OS will call us First
//
asmlinkage int hook_sys_chown(const char * filename, uid_t user, gid_t group)
{
char cuid[8];

         uid=getuid_call();
         sprintf(cuid,"%d",uid);
         SaveEvent(filename,cuid,current->comm,"CHOWN");

return -EPERM;
}
//
// Hooked call. OS will call us First
//
asmlinkage int hook_sys_fchown(unsigned int fd, uid_t user, gid_t group)
{
char cuid[8];
char fid[8];

         uid=getuid_call();
         sprintf(cuid,"%d",uid);
	sprintf(fid,"%d",fd);
         SaveEvent(fid,cuid,current->comm,"FCHOWN");

return -EPERM;
}

//
// Hooked call. OS will call us First
//
asmlinkage int hook_sys_chmod(const char *filename,mode_t mode)
{
char cuid[8];
struct dentry *root;
struct vfsmount *mnt;
struct dentry *pwdroot;
struct vfsmount *pwdmnt;

char *f;
char buf[2048];
int bufsz=2048;
int idx=-1;


        if(filename[0]!='/') {
                pwdroot = dget(current->fs->pwd);
                pwdmnt = mntget(current->fs->pwdmnt);
	        mnt = mntget(current->fs->rootmnt);
	        root = dget(current->fs->root);
                f=__d_path(pwdroot,pwdmnt,root,mnt,buf,bufsz);
                if(!f) printk(KERN_EMERG "AMDSYSM: No Root Built for File Name\n");
                sprintf(buf,"%s/%s",f,filename);
                }
         else strcpy(buf,filename);

         if( (idx=IsMatch(buf)) < 0 ) return original_chmod(filename,mode); // Not something we are interested in
         uid=getuid_call();
         sprintf(cuid,"%d",uid);
         SaveEvent(buf,cuid,current->comm,"CHMOD");

return -EPERM;
}

//
// Hooked call. OS will call us First
//
asmlinkage int hook_sys_open(const char *filename,int flags,int mode)
{
char cuid[8];
struct dentry *root;
struct vfsmount *mnt;
struct dentry *pwdroot;
struct vfsmount *pwdmnt;

char *f;
char buf[2048];
int bufsz=2048;
int idx=-1;


        if(filename[0]!='/') {
                pwdroot = dget(current->fs->pwd);
                pwdmnt = mntget(current->fs->pwdmnt);
	        mnt = mntget(current->fs->rootmnt);
	        root = dget(current->fs->root);
                f=__d_path(pwdroot,pwdmnt,root,mnt,buf,bufsz);
                if(!f) printk(KERN_EMERG "AMDSYSM: No Root Built for File Name\n");
                sprintf(buf,"%s/%s",f,filename);
                }
         else strcpy(buf,filename);

         if( (idx=IsMatch(buf)) < 0 ) return original_open(filename,flags,mode); // Not something we are interested in
         flags=O_RDONLY;
         uid=getuid_call();
         sprintf(cuid,"%d",uid);
         SaveEvent(buf,cuid,current->comm,"OPEN");

return original_open(filename,flags,mode);
}

//
// Hooked call. OS will call us First
//
asmlinkage int hook_sys_link(const char *old,const char *new)
{
char cuid[8];
struct dentry *root;
struct vfsmount *mnt;
struct dentry *pwdroot;
struct vfsmount *pwdmnt;

char *f;
char buf[2048];
int bufsz=2048;
int idx=-1;


        if(old[0]!='/') {
                pwdroot = dget(current->fs->pwd);
                pwdmnt = mntget(current->fs->pwdmnt);
	        mnt = mntget(current->fs->rootmnt);
	        root = dget(current->fs->root);
                f=__d_path(pwdroot,pwdmnt,root,mnt,buf,bufsz);
                if(!f) printk(KERN_EMERG "AMDSYSM: No Root Built for File Name\n");
                sprintf(buf,"%s/%s",f,old);
                }
         else strcpy(buf,old);

         if( (idx=IsMatch(buf)) < 0 ) return original_link(old,new); // Not something we are interested in
         uid=getuid_call();
         sprintf(cuid,"%d",uid);
         SaveEvent(buf,cuid,current->comm,"LINK");

 return EACCES;
}

//
// Hooked call. OS will call us First
//
asmlinkage int hook_sys_unlink(const char *old)
{
char cuid[8];
struct dentry *root;
struct vfsmount *mnt;
struct dentry *pwdroot;
struct vfsmount *pwdmnt;

char *f;
char buf[2048];
int bufsz=2048;
int idx=-1;


        if(old[0]!='/') {
                pwdroot = dget(current->fs->pwd);
                pwdmnt = mntget(current->fs->pwdmnt);
	        mnt = mntget(current->fs->rootmnt);
	        root = dget(current->fs->root);
                f=__d_path(pwdroot,pwdmnt,root,mnt,buf,bufsz);
                if(!f) printk(KERN_EMERG "AMDSYSM: No Root Built for File Name\n");
                sprintf(buf,"%s/%s",f,old);
                }
         else strcpy(buf,old);

         if( (idx=IsMatch(buf)) < 0 ) return original_unlink(old); // Not something we are interested in
         uid=getuid_call();
         sprintf(cuid,"%d",uid);
         SaveEvent(buf,cuid,current->comm,"UNLINK");

 return EACCES;
}


//
// Driver Entery
//l
int init_module()
{

unsigned long ptr;

	MODULE_LICENSE("GPL");
	for(ptr=(unsigned long) &loops_per_jiffy;  ptr < (unsigned long) &boot_cpu_data; ptr += sizeof(void *))
	{
	unsigned long *p;
	p = (unsigned long *)ptr;
	if(p[__NR_close] == (unsigned long) sys_close) {
		sct=(unsigned long **)p;
		break;		
		}
	}
	if(!sct) {printk(KERN_EMERG "Unable to load system table\n"); return 1; }
        printk(KERN_EMERG "We are loding the driver now\n");
        sema_init(&globalbuf,1);                            // Set the semphore for getting
        printk(KERN_EMERG "Global Semepohre has been set\n");
        original_open = sct[__NR_open];				// Hook the open 
        sct[__NR_open] = hook_sys_open;
        original_link = sct[__NR_link];			// Hook the link
        sct[__NR_link] = hook_sys_link;
        original_unlink = sct[__NR_unlink];			// Hook the link
         sct[__NR_unlink] = hook_sys_unlink;
	original_chmod = sct[__NR_chmod];			// Hook the chmod
         sct[__NR_chmod] = hook_sys_chmod;

	original_fchown = sct[__NR_fchown];			
         sct[__NR_fchown] = hook_sys_fchown;		// Hook the fchown
	original_chown = sct[__NR_chown32];			
         sct[__NR_chown32] = hook_sys_chown;		// Hook the chown
	original_lchown = sct[__NR_lchown];			
         sct[__NR_lchown] = hook_sys_lchown;		// Hook the lchown

        getuid_call = sct[__NR_getuid];
        original_close = sct[__NR_close];
        original_read = sct[__NR_read];

        datbufnxt=0;                                        // put next event in buffer at this location
        MaxMasks=1;                                         // We always lock ourselv at least
        Alert[0]=1; Lockdown[0]=1;                          // Set our up
        memset(datbuf,0,MAXBUFSZ);

        printk(KERN_EMERG "Establish proc entry\n");
        myroot=proc_mkdir(MODULE_NAME,NULL);
        if(myroot==NULL) return ENOMEM;
        printk(KERN_EMERG "Root Created\n");

        myproc=create_proc_entry("amdmon",777,myroot);
        if(myproc==NULL) return ENOMEM;
        myproc->read_proc=&proc_read;
        myproc->owner=THIS_MODULE;
        myproc->uid=0;
        myproc->gid=0;

        input=create_proc_entry("amdmonin",777,myroot);
        if(input==NULL) return ENOMEM;
        input->write_proc=&proc_write;
        input->owner=THIS_MODULE;
        input->uid=0;
        input->gid=0;

        printk(KERN_EMERG "Proc completed\n");

        printk(KERN_EMERG "Init complete for AMDSYSM V1.2\n");
return 0;
}

//
// Driver Exit code
//
void cleanup_module()
{
        if(sct[__NR_open] != hook_sys_open) {
                printk(KERN_EMERG "Unable to Unload Driver - Secondary hook\n");
                return;
                }
        sct[__NR_open] = original_open;
        sct[__NR_link] = original_link;
        sct[__NR_unlink] = original_unlink;
        sct[__NR_chmod] = original_chmod;
        sct[__NR_fchown] = original_fchown;
        sct[__NR_chown32] = original_chown;
        sct[__NR_lchown] = original_lchown;


        remove_proc_entry("amdmon",myproc);
        remove_proc_entry("amdmonin",input);
        remove_proc_entry(MODULE_NAME,NULL);

        printk(KERN_EMERG "File Open Driver Suspended\n");
}

//
// Save the event please
//
int SaveEvent(char *fname, char *process, char *p,char *call)
{
char tbuf[2048];
int dsize=0;

        if(datbufnxt>=MAXBUFSZ) return 1;                       // can't do anything
        if(!fname) return 1;
	dsize=sprintf(tbuf,"%s;%s;%s;%s\n",call,fname,process,p);
        down(&globalbuf);
	if( (dsize+datbufnxt) > MAXBUFSZ ) { dsize=MAXBUFSZ-datbufnxt; datbufnxt=MAXBUFSZ;} // are we over
	memcpy(&datbuf[datbufnxt],tbuf,dsize);
	datbufnxt += dsize+1;					// no not over just add in and substract from max
        up(&globalbuf);

return 1;
}
//
// Remove all of the garbage chars
//
int trim(char *data)
{
int x=0;

	x=strlen(data);
	if(x<1) return 0;
	x--;
	while((data[x]<=' ') && x) {data[x]=0; x--;}
return x;
}

//
// Proc read routine
//
int proc_read(char *buf,char **start, off_t offset,int count,int *eof, void *data)
{
int len;
        printk(KERN_EMERG "In proc Read routine length [%d]\n",datbufnxt);
        *eof=1;
        if(datbufnxt==0) return 0;
        down(&globalbuf);
        len=datbufnxt-1;
        memcpy(buf,datbuf,len);
        datbufnxt=0;
        memset(datbuf,0,MAXBUFSZ);
        up(&globalbuf);
return len;
}
//
// Proc write routine
//
int proc_write(struct file *file,const char *buf,size_t count, loff_t *p)
{
int len=(int)count;
char *c;
char *s;
char *l;
int x=1;

        if(!len || len<0) return 0;
        printk(KERN_EMERG "In proc Write routine buf [%.80s] count[%d]\n",buf,count);
        s=(char *)buf;
        l=s;
        while(len) {
                while(*(l)>' ' && len) {len--; l++;}
                Alert[x]=0; Lockdown[x]=0;
                c=strstr(s,";");
                if(!c) return 0;
                *(c)='\0';
                strcpy(PathIncludeFilters[x],s);

                s=c+1;                                  // Next field please
                c=strstr(s,";");
                 if(!c) return 0;
                *(c)='\0';
                if(*(s)=='1') Alert[x]=1;

                s=c+1;
                if(*(s)=='1') Lockdown[x]=1;

                x++;  s++;
                while( (*(s) < ' ') && len) {s++; len--;}  // next line please
                l=s;
                }
        MaxMasks=x;                             // Reset the max
        printk(KERN_EMERG "Masks have been updated Number of Masks [%d]\n",MaxMasks);

return count;
}
//
// See if the name is a match
//
int IsMatch(char *fullname)
{
int x,y,i;

	    for( i = 0; i < MaxMasks; i++ ) {
		for(x=0, y=0;PathIncludeFilters[i][x] > ' '; x++,y++)
			{
			if(PathIncludeFilters[i][x]=='*') {												// Section Wild card
				while(PathIncludeFilters[i][x]>' ' && PathIncludeFilters[i][x] != '/') x++;
				while(fullname[y]> ' ' && fullname[y] != '/') y++;
				}
			if(PathIncludeFilters[i][x] != fullname[y]) break;								// Not this guy
			if(PathIncludeFilters[i][x]=='\0') { return i;}	// Match for this mask
			}
		if(PathIncludeFilters[i][x]=='\0') { return i;}	// Match for this mask
                }
return -1;
}










