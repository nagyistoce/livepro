/*	dummy.c
 *
 *	Example program for using a videoloopback device in zero-copy mode.
 *	Copyright 2000 by Jeroen Vreeken (pe1rxq@amsat.org)
 *	Copyright 2005 by Angel Carpintero (ack@telefonica.net)
 *	This software is distributed under the GNU public license version 2
 *	See also the file 'COPYING'.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <linux/videodev.h>

/* all seem reasonable, or not? */
#define MAXIOCTL 1024
#define MAXWIDTH 640
#define MAXHEIGHT 480
int width;
int height;
int fmt = 0;
char ioctlbuf[MAXIOCTL];
int v4ldev;
char *image_out;
int noexit = 1;	

int get_frame(void)
{
	int i;
	char colour = 0;

	memset(image_out, 0x128, width * height * 3);
	
	for (i = 10; i < width - 10; i++) {
		image_out[10 *width *3 + i *3] = colour++;
		image_out[10 *width *3 + i *3 + 1] = 0;
		image_out[10 *width *3 + i *3 + 2] =-colour;
	}

	for (i = 10; i < width - 10; i++) {
		image_out[(height - 10) * width *3 + i * 3] = colour;
		image_out[(height - 10) * width *3 + i * 3 + 1] = 0;
		image_out[(height - 10) * width *3 + i * 3 + 2] =-colour++;
	}

	usleep(500); /* BIG XXX */
	return 0;
}

char *v4l_create (int dev, int memsize)
{
	char *map;
	
	map = mmap(0, memsize, PROT_READ|PROT_WRITE, MAP_SHARED, dev, 0);

	if ((unsigned char *)-1 == (unsigned char *)map)
		return NULL;

	return map;	
}

int v4l_ioctl(unsigned long int cmd, void *arg)
{
	int i;
	switch (cmd) {
	case VIDIOCGCAP:
	{
		struct video_capability *vidcap = arg;

		sprintf(vidcap->name, "Jeroen's dummy v4l driver");
		vidcap->type = VID_TYPE_CAPTURE;
		vidcap->channels = 1;
		vidcap->audios = 0;
		vidcap->maxwidth = MAXWIDTH;
		vidcap->maxheight = MAXHEIGHT;
		vidcap->minwidth = 20;
		vidcap->minheight = 20;
		return 0;
	}
	case VIDIOCGCHAN:
	{
		struct video_channel *vidchan = (struct video_channel *)arg;
			
		printf("VIDIOCGCHAN called\n");
		if (vidchan->channel != 0)
				;//return 1;
		vidchan->channel = 0;
		vidchan->flags = 0;
		vidchan->tuners = 0;
		vidchan->norm = 0;
		vidchan->type = VIDEO_TYPE_CAMERA;
		strcpy(vidchan->name, "Loopback");
			
		return 0;
	}
	case VIDIOCSCHAN:
	{
		int *v = arg;
			
		if (v[0] != 0)
			return 1;
		return 0;
	}
	case VIDIOCGTUNER:
	{
		struct video_tuner *v = arg;

		if (v->tuner) {
			printf("VIDIOCGTUNER: Invalid Tuner, was %d\n", v->tuner);
			//return -EINVAL;
		}
		v->tuner = 0;
		strcpy(v->name, "Format");
		v->rangelow = 0;
		v->rangehigh = 0;
		v->flags = 0;
		v->mode = VIDEO_MODE_AUTO;
		return 1;
	}
	case VIDIOCGPICT:
	{
		struct video_picture *vidpic = arg;

		vidpic->colour = 0x8000;
		vidpic->hue = 0x8000;
		vidpic->brightness = 0x8000;
		vidpic->contrast = 0x8000;
		vidpic->whiteness = 0x8000;
		vidpic->depth = 0x8000;
		vidpic->palette = fmt;
		return 0;
	}
	case VIDIOCSPICT:
	{
		struct video_picture *vidpic = arg;
		
		if (vidpic->palette != fmt)
			return 1;
		return 0;
	}
	case VIDIOCGWIN:
	{
		struct video_window *vidwin = arg;

		vidwin->x = 0;
		vidwin->y = 0;
		vidwin->width = width;
		vidwin->height = height;
		vidwin->chromakey = 0;
		vidwin->flags = 0;
		vidwin->clipcount = 0;
		return 0;
	}
	case VIDIOCSWIN:
	{
		struct video_window *vidwin = arg;
			
		if (vidwin->width > MAXWIDTH ||
		    vidwin->height > MAXHEIGHT )
			return 1;
		if (vidwin->flags)
			return 1;
		width = vidwin->width;
		height = vidwin->height;
		printf("new size: %dx%d\n", width, height);
		return 0;
	}
	case VIDIOCGMBUF:
	{
		struct video_mbuf *vidmbuf = arg;

		vidmbuf->size = width * height * 3;
		vidmbuf->frames = 1;
		for (i = 0; i < vidmbuf->frames; i++)
			vidmbuf->offsets[i] = i * vidmbuf->size;
		return 0;
	}
	case VIDIOCMCAPTURE:
	{
		struct video_mmap *vidmmap = arg;

		//return 0;
		if (vidmmap->height>MAXHEIGHT ||
		    vidmmap->width>MAXWIDTH ||
		    vidmmap->format != fmt )
			return 1;
		if (vidmmap->height != height ||
		    vidmmap->width != width) {
			height = vidmmap->height;
			width = vidmmap->width;
			printf("new size: %dx%d\n", width, height);
		}
		// check if 'vidmmap->frame' is valid
		// initiate capture for 'vidmmap->frame' frames
		return 0;
	}
	case VIDIOCSYNC:
	
		//struct video_mmap *vidmmap=arg;

		// check if frames are ready.
		// wait until ready.
		get_frame();
		return 0;
	
	default:
		printf("unknown ioctl: %ld\n", cmd & 0xff);
		return 1;
	
	}
	return 0;
}

#define VIDIOCSINVALID	_IO('v',BASE_VIDIOCPRIVATE+1)

void sighandler_exit(int signo)
{
    noexit = 0;
}    

void sighandler(int signo)
{
	int size, ret;
	unsigned long int cmd;
	struct pollfd ufds;

	if (signo != SIGIO)
		return;

	ufds.fd = v4ldev;
	ufds.events = POLLIN;
	ufds.revents = 0;
	poll(&ufds, 1, 1000);
	
	if (!ufds.revents & POLLIN) {
		printf("Received signal but got negative on poll?!?!?!?\n");
		return;
	}

	size = read(v4ldev, ioctlbuf, MAXIOCTL);

	if (size >= sizeof(unsigned long int)) {
		memcpy(&cmd, ioctlbuf, sizeof(unsigned long int));
		if (cmd == 0) {
			printf("Client closed device\n");
			return;
		}
		ret = v4l_ioctl(cmd, ioctlbuf+sizeof(unsigned long int));
		if (ret) {
			memset(ioctlbuf + sizeof(unsigned long int), MAXIOCTL - sizeof(unsigned long int), 0xff);
			printf("ioctl %lx unsuccesfull, lets issue VIDIOCSINVALID (%x)\n", cmd, VIDIOCSINVALID);
			ioctl(v4ldev, VIDIOCSINVALID);
		} else {
			ioctl(v4ldev, cmd, ioctlbuf + sizeof(unsigned long int));
        }
	}
	return;
}


int open_vidpipe(void)
{
	int pipe_fd = -1;
	FILE *vloopbacks;
	char pipepath[255];
	char buffer[255];
	char *loop;
	char *input;
	char *istatus;
	char *output;
	char *ostatus;
	char *major;
	char *minor;
	struct utsname uts;

	if (uname(&uts) < 0) {
		printf("Unable to execute uname\nError[%s]\n", strerror(errno));
		return -1;
	}
	
	major = strtok(uts.release, ".");
	minor = strtok(NULL, ".");
	if ((major == NULL) || (minor == NULL) || (strcmp(major, "2"))) {
		printf("Unable to decipher OS version\n");
		return -1;
	}

	if (strcmp(minor, "5") < 0) {
		vloopbacks = fopen("/proc/video/vloopback/vloopbacks", "r");
		if (!vloopbacks) {
			printf ("Failed to open '/proc/video/vloopback/vloopbacks");
			return -1;
		}

		/* Read vloopback version */
		fgets(buffer, 255, vloopbacks);
		printf("%s", buffer);

		/* Read explaination line */
		fgets(buffer, 255, vloopbacks);

		while (fgets(buffer, 255, vloopbacks)) {
			
            if (strlen(buffer)>1) {
				buffer[strlen(buffer)-1] = 0;
				loop = strtok(buffer, "\t");
				input = strtok(NULL, "\t");
				istatus = strtok(NULL, "\t");
				output = strtok(NULL, "\t");
				ostatus = strtok(NULL, "\t");
				
                if (istatus[0] == '-') {
					snprintf(pipepath, 255, "/dev/%s", input);
					pipe_fd = open(pipepath, O_RDWR);

					if (pipe_fd >= 0) {
						printf("Input: /dev/%s\n", input);
						printf("Output: /dev/%s\n", output);
						return pipe_fd;
					}
				}
			} 
		}
	} else {
		DIR *dir;
		struct dirent *dirp;
		const char prefix[] = "/sys/class/video4linux/";
		char *ptr, *io;
		int fd;
		int low = 9999;
		int tfd;
		int tnum;

		if ((dir = opendir(prefix)) == NULL) {
			printf( "Failed to open '%s'", prefix);
			return -1;
		}

		while ((dirp = readdir(dir)) != NULL) {
            if (!strncmp(dirp->d_name, "video", 5)) {
                strncpy(buffer, prefix, 255 - strlen(prefix));
                strncat(buffer, dirp->d_name, 255 - strlen(buffer));
                strncat(buffer, "/name", 255 - strlen(buffer));
                
                if ((fd = open(buffer, O_RDONLY)) >= 0) {
                    if ((read(fd, buffer, sizeof(buffer)-1)) < 0) {
                        close(fd);
                        continue;
                    }
                    
                    ptr = strtok(buffer, " ");
                    
                    if (strcmp(ptr, "Video")) {
                        close(fd);
                        continue;
                    }
                    
                    major = strtok(NULL, " ");
                    minor = strtok(NULL, " ");
                    io  = strtok(NULL, " \n");

                    if (strcmp(major, "loopback") || strcmp(io, "input")) {
                        close(fd);
                        continue;
                    }

                    if ((ptr=strtok(buffer, " ")) == NULL) {
                        close(fd);
                        continue;
                    }

                    tnum = atoi(minor);
                    if (tnum < low) {
                        strcpy(buffer, "/dev/");
                        strcat(buffer, dirp->d_name);
                        if ((tfd = open(buffer, O_RDWR)) >= 0) {
                            strcpy(pipepath, buffer);
                            if (pipe_fd >= 0) 
                                close(pipe_fd);
                                
                             pipe_fd = tfd;
                             low = tnum;
                        }
                    }
                    close(fd);
                }
            }
        }
		

		closedir(dir);
        if (pipe_fd >= 0)
            printf("Opened input of %s", pipepath);
    }
	
	return pipe_fd;
}

int main (int argc, char **argv)
{
	char palette[10] = {'\0'};

	if (argc != 3) {
		printf("dummy.c\n");
		printf("A example for using a video4linux loopback in zero-copy mode\n");
		printf("Written by Jeroen Vreeken, 2000\n");
		printf("Updated to vloopback API v1.1\n\n");
		printf("Usage:\n\n");
		printf("dummy widthxheight rgb24|yuv420p\n\n");
		printf("example: dummy 352x288 yuv420p\n\n");
		exit(1);
	}
	
	sscanf(argv[1], "%dx%d", &width, &height);
	sscanf(argv[2], "%s", palette);

	if (!strcmp(palette, "rgb24")) 
		fmt = VIDEO_PALETTE_RGB24;
	else if (!strcmp(palette, "yuv420p")) 
		fmt = VIDEO_PALETTE_YUV420P;
	else fmt = VIDEO_PALETTE_RGB24;
	
	/* Default startup values, nothing special 
	width=352;
	height=288;
	*/

	v4ldev = open_vidpipe();

	if (v4ldev < 0) {
		printf ("Failed to open video loopback device\nError[%s]\n", strerror(errno));
		exit(1);
	}

	image_out = v4l_create(v4ldev, MAXWIDTH * MAXHEIGHT * 3);
	
	if (!image_out) {
		exit(1);
		printf ("Failed to set device to zero-copy mode\nError[%s]\n", strerror(errno));
	}

	signal(SIGIO, sighandler);
    signal(SIGTERM, sighandler_exit);
    signal(SIGINT, sighandler_exit);

	printf("\nListening.\n");
	
	while (noexit) 
		sleep(1000);
	
	close(v4ldev);
	munmap(image_out, MAXWIDTH * MAXHEIGHT * 3);

    printf("\nBye Bye ...\n");

	exit(0);
}
