/***
	Mid term project: Pulse Audio IP phone implementation.
	Ravi Shankar (MS2016009)
	References are from Pulse Audio documentaion.
	File: IP_Phone_Speaker.c
	Compile: cc -lpulse -lpulse-simple IP_Phone_Speaker.c -o Speaker.out
	Command line arguments: PORT
***/

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

/* Declare buffer size */
#define BUFSIZE 64

int main(int argc, char*argv[]) 
{

    /* Define sample format */
    static const pa_sample_spec ss = 
	{
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };

    pa_simple *s_out = NULL; /* PA stream for recording */
				
    int ret = 1;
    int error;
	
	/* Server socket variables */
	int listenfd = 0, connfd = 0;
  
	struct sockaddr_in serv_addr;
 
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

	// Loop forever... ( TODO: Handle SIGINT)
    for (;;)
	{
        uint8_t buf[BUFSIZE];
		ssize_t r;
		
		/* Receive audio data...*/
		if((r = recv(connfd, buf, sizeof(buf), 0)) <= 0)
		{
			if(r == 0)
			{
				printf("Connection terminated...\n");
				break;

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
        
    }
	/* Make sure that every single sample was played */
    if (pa_simple_drain(s_out, &error) < 0) 
	{
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        goto finish;
    }

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


