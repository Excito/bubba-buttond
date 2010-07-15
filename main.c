#include <stdio.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <stddef.h>

#define DEVICE 			"/dev/input/by-path/platform-gpio-keys-event"
#define PIDFILE			"/var/run/buttond.pid"
#define REBOOTCMD		"/sbin/reboot"
#define B_KEY_DOWN		0x1
#define B_KEY_UP		0x0
#define POWER_DOWN_DELAY	2

static struct timeval tim;
static int dorun = 1;
static int shutdown = 0;

void devinfo(int fd){
#ifdef DEBUG
	char name[256]= "Unknown";
	char phys[256]= "Unknown";

	if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
		perror("Failed get name of " DEVICE);
	}else{
		printf("Name of device: %s\n",name);
	}

	if(ioctl(fd, EVIOCGPHYS(sizeof(phys)), phys) < 0) {
	    perror("event ioctl");
	}else{
		printf("Path of device: %s\n",name);
	}
#endif
}

void dump_event(struct input_event *ev){
#ifdef DEBUG
	printf("Time %ld.%06ld ",ev->time.tv_sec,ev->time.tv_usec);
	printf("Type : %d ",ev->type);
	printf("Code : %d ",ev->type);
	printf("Value: %d\n",ev->value);
#endif
}

static void write_pidfile(void){
	FILE *fil;

	if((fil=fopen(PIDFILE,"w"))==NULL){
		perror("Failed to open pidfile for writing");
		return;
	}

	fprintf(fil,"%d\n",getpid());

	fclose(fil);
}

void sighandler(int signum){
	if (signum==SIGINT || signum==SIGTERM) {
		syslog(LOG_NOTICE, "Daemon terminating");
		exit(0);		
	}
}

#define MTD_PART 	"/dev/mtd2"
#define BLOCK_START	0x110000
#define BLOCK_SIZE	0x10000
#define REBOOT_MAGIC	0xdeadbeef

static int write_magic(){
	unsigned long value = REBOOT_MAGIC;
	struct erase_info_user erase;
	int fd;

	if( (fd = open(MTD_PART,O_RDWR)) == -1){
		syslog(LOG_ERR,"Failed to open device: %m");
		return 1;
	}

	erase.start = BLOCK_START;
	erase.length = BLOCK_SIZE;
	if (ioctl (fd, MEMERASE, &erase) != 0) {
		syslog(LOG_ERR,"Erase block failed: %m");
		return 1;
	}

	if(lseek(fd, BLOCK_START, SEEK_SET) == -1) {
		syslog(LOG_ERR,"Failed to seek to block: %m");
		return 1;
	}

	if( write(fd,&value,sizeof(value)) != sizeof(value)){
		syslog(LOG_ERR,"Failed to write value: %m");
		return 1;
	}
	
	close(fd);
	return 0;
} 


int main(int argc, char** argv){

	int fd;
	ssize_t r;

	struct input_event ev;
	
	openlog("buttond",LOG_PERROR,LOG_DAEMON);

	if (getuid()!=0) {
		syslog(LOG_ERR, "Started as non root terminating");
		return 1;
	}

	if(argc==2 && strncmp(argv[1],"-f",2)==0){
		syslog(LOG_NOTICE,"Running in foreground");	
	}else{
		syslog(LOG_NOTICE,"Daemonizing");	
		daemon(1,0);
		write_pidfile();
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, sighandler);
	signal(SIGTERM,sighandler);


	if((fd=open(DEVICE,O_RDONLY))<0){
		syslog(LOG_ERR, "Failed to open device " DEVICE " %m");
		return 1;
	}

	devinfo(fd);

	tim.tv_sec = 0;
	tim.tv_usec = 0;

	while( dorun && ((r=read(fd,&ev,sizeof(ev)))>0)){
		if(r==sizeof(ev)){
			if(ev.type!=EV_KEY){
				continue;
			}
			dump_event(&ev);
			if(ev.value == B_KEY_DOWN){
				tim.tv_sec = ev.time.tv_sec;
				tim.tv_usec = ev.time.tv_usec;
			}else if(ev.value == B_KEY_UP && tim.tv_sec > 0){
				
				if( ( ev.time.tv_sec - tim.tv_sec)>=POWER_DOWN_DELAY){
					syslog(LOG_NOTICE,"Power down");
					dorun = 0;
					shutdown = 1;
					break;
				}else{
					syslog(LOG_DEBUG,"Not long enough press");
				}
			}

		}else{
			syslog(LOG_NOTICE,"Got event %u bytes",r);
		}	
	}

	if(shutdown){
		// shut down system
		syslog(LOG_NOTICE, "Shut down system");
		if(write_magic()==0){
			system(REBOOTCMD);
		}
	}
	
	close(fd);
	syslog(LOG_NOTICE, "Daemon terminating");
	closelog();

	unlink(PIDFILE);

	return 0;
}
