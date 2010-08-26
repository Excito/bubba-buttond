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

#ifndef DEVICE
#define DEVICE 				"/dev/input/by-path/platform-gpio-keys-event"
#endif

#ifndef PIDFILE
#define PIDFILE				"/var/run/buttond.pid"
#endif

#ifndef REBOOTCMD
#define REBOOTCMD			"/sbin/reboot"
#endif

#ifndef MTD_PART
#define MTD_PART 			"/dev/mtd2"
#endif

#define BLOCK_START			0x110000
#define BLOCK_SIZE			0x10000
#define REBOOT_MAGIC		0xdeadbeef

#define B_KEY_DOWN			0x1
#define B_KEY_UP			0x0
#define POWER_DOWN_DELAY	2

// Forwards
static int write_magic();

static int dorun = 1;

#ifdef DEBUG
void devinfo(int fd){
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
}

void dump_event(struct input_event *ev){
	printf("Time %ld.%06ld ",ev->time.tv_sec,ev->time.tv_usec);
	printf("Type : %d ",ev->type);
	printf("Code : %d ",ev->type);
	printf("Value: %d\n",ev->value);
}
#else
#define devinfo(x)
#define dump_event(x)
#endif /*DEBUG*/

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

void sigshutdown(int signum){
	if (signum == SIGALRM){
		syslog(LOG_NOTICE, "Shuting down system");
		if(write_magic()==0){
			system(REBOOTCMD);
		}
		dorun=0;
	}else{
		syslog(LOG_NOTICE, "Invalid signal received: %d", signum);
	}
}


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
	signal(SIGTERM, sighandler);
	signal(SIGALRM, sigshutdown);


	if((fd=open(DEVICE,O_RDONLY))<0){
		syslog(LOG_ERR, "Failed to open device " DEVICE " %m");
		return 1;
	}

	devinfo(fd);

	while( dorun && ((r=read(fd,&ev,sizeof(ev)))>0)){
		if(r==sizeof(ev)){
			if(ev.type!=EV_KEY){
				continue;
			}
			dump_event(&ev);
			if(ev.value == B_KEY_DOWN){
				syslog(LOG_DEBUG, "Start shutdown timer");
				alarm(POWER_DOWN_DELAY);
			}else if(ev.value == B_KEY_UP){
				syslog(LOG_DEBUG, "Cancel shutdown timer");
				alarm(0);
			}

		}else{
			syslog(LOG_DEBUG,"Got event %zu bytes",r);
		}	
	}

	close(fd);
	syslog(LOG_NOTICE, "Daemon terminating");
	closelog();

	unlink(PIDFILE);

	return 0;
}
