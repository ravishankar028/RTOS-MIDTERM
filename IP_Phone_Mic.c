/***
  This file is part of PulseAudio (http://pulseaudio.org)
  Name : Ravi Shankar (MS2016009)
  Mid-term project assignment (Term II)
  File: IP_Phone_Mic.c
  Compile using: cc -lpulse -lpulse-simple -lrt IP_Phone_Mic.c -o Periodic_Mic.out
  Command line arguments: SERVER_IP PORT
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include <time.h>
#include <sys/time.h>

#define BUFSIZE 1024
#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

pa_simple *s = NULL;

int ret = 1;

int error;

int sockfd = 0;
	
/* Function to send data over socket in a loop */
static ssize_t loop_write(int fd, const void*data, size_t size) {
    ssize_t ret = 0;

    while (size > 0) {
        ssize_t r;

        if ((r = write(fd, data, size)) < 0)
            return r;

        if (r == 0)
            break;

        ret += r;
        data = (const uint8_t*) data + r;
        size -= (size_t) r;
    }

    return ret;
}

/* Timer handler function */
void timer_handler(int signum)
{
		uint8_t buf[BUFSIZE];

		if(pa_simple_read(s, buf, sizeof (buf), &error) < 0)
		{
			fprintf(stderr,__FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
			return;
		}
		/* Send data over socket */
		if(loop_write(sockfd, buf, sizeof(buf)) != sizeof(buf))
		{
			perror("send failed:");
		}
}

int main(int argc, char*argv[]) {

    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };

	/* Timer variables */
	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its;
	struct sigaction sa;

	/* Timer signal setting */
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = timer_handler;
	sigaction(SIG, &sa, NULL);
	
	/* Client socket variables */
	struct sockaddr_in serv_addr;


	/* Open socket */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
      printf("\n Error : Could not create socket \n");
      return 1;
    }
	
	serv_addr.sin_family = AF_INET;
	
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	
	serv_addr.sin_port = htons(atoi(argv[2]));
	
	/* Connect ot remote host */
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
		printf("\n Error : Connect Failed \n");
		return 1;
    }
	
    /* Create the recording stream */
    if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;
    }

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;

	/* Create Timer */
   if (timer_create(CLOCKID, &sev, &timerid) == -1){
		perror("timer_create");
   }

	/* Set values : Period = 20ms */
	its.it_value.tv_sec = 0 ;
	its.it_value.tv_nsec =  20;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 20000000;

	/* Start Timer */
	if (timer_settime(timerid, 0, &its, NULL) == -1) {
		perror("timer_settime");
		goto finish;
	}
		
    for (;;){} // Stay alive.

    ret = 0;

finish:

    if (s)
        pa_simple_free(s);

    return ret;
}
