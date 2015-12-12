/*
 * Judge.c - a simple text-only Pinewood derby lane judge
 * Copyright 2013-2015 Mitch Williams
 * Licensed under GPL v 2.0 - see the license file in this repository
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <string.h>

#define MAX_RACERS 50
#define MAX_LANES 4
#define TIMER_BAUD B1200
#define TIMER_BITS CS7
#define TIMER_STOP CSTOPB /* 2 stop bits */
#define TIMER_PARITY  0 /* none */
#define LINE_LEN 80
#define NAME_LEN 20

#define max(a, b) ((a) > (b) ? (a) : (b))

/* if config is true, then configure the serial port. Good for debugging.*/
FILE *open_timer(char *portname, int config)
{
	FILE *fd;
	int file;
	struct termios options;

	fd = fopen(portname, "r+");
	if (!fd)
		return;
	if (config) {
		file = fileno(fd);
		tcgetattr(file, &options);
		cfsetispeed(&options, TIMER_BAUD);
		cfsetospeed(&options, TIMER_BAUD);
		options.c_cflag |= (CLOCAL | CREAD);
		options.c_cflag &= ~(CSIZE | CSTOPB | PARENB); /* mask char size bits */
		options.c_cflag |= TIMER_BITS | TIMER_STOP | TIMER_PARITY;
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* raw input */
		options.c_oflag &= ~OPOST; /* raw output */
		/* set new options */
		tcsetattr(file, TCSANOW, &options);
	}
	return fd;
}

void close_timer(FILE *fd)
{
	fclose(fd);
}

/* returns number of lanes, or 0 if error */
int init_timer(FILE *fd)
{
	char timer_id[LINE_LEN];
	int retval;
	printf("\nPlease reset the timer. Hold switch closed for 1 second, then open.\n");
	if (!fgets(timer_id, LINE_LEN, fd)) {
		fprintf(stderr, "Error reading from port! Please check timer.\n");
		return 0;
	}

	/* Timer id string is formatted like: "SuperDuper Timer v9.3 - 4 Lanes found" */
	/* Search backward from end of string for a digit to get the number of lanes. */
	if (strrchr(timer_id, '8'))
		retval = 8;
	else if (strrchr(timer_id, '4'))
		retval = 4;
	else if (strrchr(timer_id, '2'))
		retval = 2;
	else {
		fprintf(stderr, "Error! unable to parse timer string\n\t%s\n", timer_id);
		return 0;
	}

	printf("Timer initialized. Timer reports \n\t%s\n", timer_id);
	printf("Found %d lanes\n", retval);
	return retval;
}

FILE *reinit_timer(FILE *port, char *name, int config, int lanes_expected)
{
	FILE *fd;
	char tmp[LINE_LEN];
	int lanes;

	printf("Wow, something's messed with your timer. Let's try again.\n");
	close_timer(port);
	printf("Please power cycle your timer.\n");
	printf("If you have a USB timer, just unplug it, wait a few seconds, and reconnect\n");
	printf("it to the SAME port.\n");
	printf("\nWhen you're done, press Enter and we'll try to get reconnected.\n");
retry:
	fgets(tmp, LINE_LEN, stdin);
	printf("Opening port.\n");
	fd = open_timer(name, config);
	if (!fd) {
		printf("Eek! Unable to open timer port! Please power cycle it again.\n");
		printf("Double-check that you plugged your USB timer into the same port!\n");
		printf("Press Enter to try again. (Ctrl-C to give up - you will lose all results.)\n");
		goto retry;
	}
	/* reset timer */
	lanes = init_timer(fd);
	if (lanes != lanes_expected) {
		close_timer(fd);
		printf("Eek! Invalid number of lanes! (Expected %d, got %d from timer.\n", lanes_expected, lanes);
		printf("Please power cycle your timer again and check all sensor connections.\n");
		printf("Press Enter to try again. (Ctrl-C to give up - you will lose all results.)\n");
		goto retry;
	}
	return fd;
}

/* returns -1 if error, 0 for do-over, or winning lane */
int get_times(FILE *fd, int lanes, float *results, int debug)
{
	char time_data[LINE_LEN];
	int i;
    int retval = 0;
	char *next = time_data;
	int lane;
	float current;

	if (!fgets(time_data, LINE_LEN, fd)) {
		fprintf(stderr, "Eek! Error reading from serial port! Please check timer.\n");
		return -1;
	}

	memset(results, 0, sizeof(float) * lanes);
	for (i = 0; i < lanes; i++) {
		lane = strtol(next, &next, 0);
        if (i == 0)
            retval = lane;
		current = strtof(next, &next);
		/* If > 10 seconds have elapsed, the timer returns 0 for all
		 * unfinished lanes.
		 * This usually means either that one car didn't finish, or that the
		 * gate was opened with no cars.
		 */
		if (current == 0.0)
			break;
		results[lane - 1] = current;
	}
	return retval;
}

void rearm_timer(FILE *fd, int debug)
{
	/* send a space to get the timer ready for the next run */
	if (!debug)
		fputc(' ', fd);
}

void display_winner(int lane)
{
    char cmdline[80];
    system("clear");
    snprintf(cmdline, 79, "toilet -f bigmono12 -F border \"Lane %d\"", lane);
    system(cmdline);
}

void display_times(int lanes, float* times)
{
    int i;
    int pos;
    char cmdline[80];
    pos = snprintf(cmdline, 80, "toilet -f future");
    for (i = 0; i < lanes; i++) {
        pos += snprintf(cmdline + pos, 80 - pos, " %1.4f", times[i]);
    }
    system(cmdline);
}
void usage(void)
{
	fprintf(stderr, "Invalid command line - you must specify a port name or debug option!\n");
	fprintf(stderr, "Valid options:\n");
	fprintf(stderr, "\t-p <filename>    -  serial port device path, e.g. /dev/ttyS0\n");
	fprintf(stderr, "\t                    *** Make sure you have write access to the port!\n");
	fprintf(stderr, "\t-d               - debug mode; do not intialize serial port\n");
}

int main(int argc, char **argv)
{
	FILE *port;
	char *filename;
	int c, i, j, tmp;
	int debugmode = 0;
	int lanes;
    float times[MAX_LANES];
	float prev_times[MAX_LANES][4];
    int prev_head = 0;\
	char *tmpbuf[10];
    
	/* parse command line options */
	if (argc == 1) {
		usage();
		return 0;
	}

	opterr = 0;
	while ((c = getopt (argc, argv, "dp:")) != -1) {
		switch (c) {
		case 'd':
			printf("Debugging mode enabled\n");
			debugmode = 1;
			break;
		case 'p':
			filename = optarg;
			break;
		case '?':
		default:
			usage();
			return 1;
		}
	}

	/* open timer */
	port = open_timer(filename, !debugmode);
	if (!port) {
		fprintf(stderr, "Eek! Unable to open timer port! Cannot continue.\n");
		fprintf(stderr, "Possible problems:\n");
		fprintf(stderr, "\tbad filename for port (if USB timer, check dmesg for port id\n");
		fprintf(stderr, "\tno write access to device (check permissions or run as root)\n");
		fprintf(stderr, "\tUSB timer not connected\n");
		return 1;
	}
	/* reset timer */
	lanes = init_timer(port);
	if (!lanes) {
		fprintf(stderr, "Eek! Cannot initialize timer! Cannot continue!\n");
		fprintf(stderr, "Make sure your timer is supported by this program.\n");
		close_timer(port);
		return 1;
	}

	memset(prev_times, 0, sizeof(float) * 3 * MAX_LANES);
    printf("Press enter to continue.\n");
    fgets(tmpbuf, 10, stdin);

	/* main loop */
	while (1) {
		printf("Begin racing when ready.\n");
		tmp = get_times(port, lanes, times, debugmode);
		printf("Run is complete. Results:\n");
		if (tmp > 0) {
			/* good race! */
            display_winner(tmp);
            printf("\n");
            // save current time, drop last time
            display_times(lanes, times);
			for (j = 0; j < lanes; j++) {
                prev_times[j][prev_head] = times[j];
			}
			printf("\nPrevious results:\n");
            i = prev_head - 1;
            if (i < 0)
                i = 3;
            while (i != prev_head) {
                for (j = 0; j < lanes; j++)
                    printf("\t%1.4f", prev_times[j][i]);
                printf("\n");
                i--;
                if (i < 0)
                    i = 3;
            }

            prev_head++;
            if (prev_head == 4)
                prev_head = 0;
		}
		if (tmp == 0) {
			printf("Null race result! Please redo this run.\n");
		}
		if (tmp < 0) {
			port = reinit_timer(port, filename, !debugmode, lanes);
			/* if this returns, we've succeeded */
		}
        printf("Press enter to continue, X to exit.\n");
		fgets(tmpbuf, 10, stdin);
        if (strchr(tmpbuf, 'x') || strchr(tmpbuf, 'X'))
			break;
		rearm_timer(port, debugmode);
	}

	/* all done */
	printf("\nDone.\n");
	close_timer(port);
	return 0;
}
