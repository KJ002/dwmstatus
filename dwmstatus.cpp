/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *tzgmt = (char*)"Europe/London";

static Display *dpy;

char * smprintf(char *fmt, ...){
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = (char*)malloc(++len);
	if (ret == NULL) {
		perror((char*)"malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void settz(char *tzname){
	setenv((char*)"TZ", tzname, 1);
}

char * mktimes(char *fmt, char *tzname){
	char buf[129];
	time_t tim;
	struct tm *timtm;

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf((char*)"");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, (char*)"strftime == 0\n");
		return smprintf((char*)"");
	}

	return smprintf((char*)"%s", buf);
}

void setstatus(char *str){
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char * readfile(char *base, char *file){
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf((char*)"%s/%s", base, file);
	fd = fopen(path, (char*)"r");
	free(path);
	if (fd == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-1, fd) == NULL)
		return NULL;
	fclose(fd);

	return smprintf((char*)"%s", line);
}

char * getbattery(char *base){
	char *co, status;
	int descap, remcap;

	descap = -1;
	remcap = -1;

	co = readfile(base, (char*)"present");
	if (co == NULL)
		return smprintf((char*)"");
	if (co[0] != '1') {
		free(co);
		return smprintf((char*)"not present");
	}
	free(co);

	co = readfile(base, (char*)"charge_full_design");
	if (co == NULL) {
		co = readfile(base, (char*)"energy_full_design");
		if (co == NULL)
			return smprintf((char*)"");
	}
	sscanf(co, (char*)"%d", &descap);
	free(co);

	co = readfile(base, (char*)"charge_now");
	if (co == NULL) {
		co = readfile(base, (char*)"energy_now");
		if (co == NULL)
			return smprintf((char*)"");
	}
	sscanf(co, (char*)"%d", &remcap);
	free(co);

	co = readfile(base,(char*) "status");
	if (!strncmp(co, (char*)"Discharging", 11)) {
		status = '-';
	} else if(!strncmp(co, (char*)"Charging", 8)) {
		status = '+';
	} else {
		status = '?';
	}

	if (remcap < 0 || descap < 0)
		return smprintf((char*)"invalid");

	return smprintf((char*)"%.0f%%", ((float)remcap / (float)descap) * 100, status);
}

int main(void){
	char *status;
	char *bat;
	char *tmlndn;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, (char*)"dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(60)) {
		bat = getbattery((char*)"/sys/class/power_supply/BAT0");
		tmlndn = mktimes((char*)"Date: %a %d %b | Time: %H:%M %Y", tzgmt);

		status = smprintf((char*)"Battery: %s | %s",
				bat, tmlndn);

		setstatus(status);
		free(bat);
		free(status);
		free(tmlndn);
	}

	XCloseDisplay(dpy);

	return 0;
}

