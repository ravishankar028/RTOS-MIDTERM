/**
* Mid term project: Pulse Audio IP phone implementation.
* Ravi Shankar (MS2016009)
* References are from Pulse Audio documentaion.
* File: IP_Phone_Speaker.c
* Command line arguments: PORT
* Compile : cc -lpulse -lpulse-simple -lrt IP_Phone_Speaker.c -o Periodic_Speaker.out
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Common includes */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

/* Include pulse audio header files */
#include <pulse/simple.h>
#include <pulse/error.h>

/* Include socket header files */
#include <sys/socket.h>
#include <arpa/inet.h>

/* Timer includes */
#include <time.h>
#include <sys/time.h>


/* Declare buffer size */
#define BUFSIZE 4096
#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN



pa_simple *s_out = NULL; /* PA stream for recording */
				
int ret = 1;
int error;

/* Server socket variables */
int listenfd = 0, connfd = 0;
	
uint8_t buf[BUFSIZE];
ssize_t r;

static void timer_handler (int signum) {
	/* Receive audio data...*/
	if((r = recv(connfd, buf, sizeof(buf), 0)) <= 0)
	{
		if(r == 0)
		{
			printf("Connection terminated...\n");
			return;

		}
		perror("read() failed :");
		
		goto finish;
	}
	
	/* Play the audio stream */
	if (pa_simple_write(s_out, buf, (size_t) r, &error) < 0) 
	{
		perror("pa_simple_write() failed:");
		goto finish;
	}
	
	/* Make sure that every single sample was played */
    if (pa_simple_drain(s_out, &error) < 0) 
	{
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        goto finish;
    }
	
	finish : 
	/*
	* Its mandatory to clean up the pa streams
	*/
    if (s_out)
	{
        pa_simple_free(s_out);
	}
}
int main(int argc, char*argv[]) 
{

    /* Define sample format */
    static const pa_sample_spec ss = 
	{
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
  
	struct sockaddr_in serv_addr;
 
	/*Timer variables*/
	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its;
	struct sigaction sa;

	/* Timer interrupt handler declaration*/
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = timer_handler;
	sigaction(SIG, &sa, NULL);
	
	char sendBuff[BUFSIZE];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	printf("Listening for incoming audio...\n");

	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(sendBuff, '0', sizeof(sendBuff));
	  
	serv_addr.sin_family = AF_INET;    
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	serv_addr.sin_port = htons(atoi(argv[1]));    

	bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));

	if(listen(listenfd, 10) == -1)
	{
	  printf("Failed to listen\n");
	  return -1;
	}

	connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL); // accept awaiting request

	/* Create a playback stream */
    if (!(s_out = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) 
	{
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;
    }
	
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;
	  
	 /*Create timer*/ 
	if (timer_create(CLOCKID, &sev, &timerid) == -1) {
	     perror("timer_create");
	}

	 /*Set values : period = 20 ms*/
	its.it_value.tv_sec = 0 ;
	its.it_value.tv_nsec =  20;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 20000000;	

	/*Start the 20ms timer*/
	if (timer_settime(timerid, 0, &its, NULL) == -1)
		perror("timer_settime");  

	for(;;){} // Stay alive...

    ret = 0;

	
finish:
	/*
	* Its mandatory to clean up the pa streams
	*/
    if (s_out)
	{
        pa_simple_free(s_out);
	}
	
	
    return ret;
}


