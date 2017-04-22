/***
  This file is part of PulseAudio (http://pulseaudio.org)
  Name : Ravi Shankar (MS2016009)
  Mid-term project assignment (Term II)
  File: IP_Phone_Mic.c
  Compile: cc -lpulse -lpulse-simple IP_Phone_Mic.c -o g711_Mic.out
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

#include "g711.c"


#define BUFSIZE 1024

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

int main(int argc, char*argv[]) {

    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
	
    pa_simple *s = NULL;
    int ret = 1;
    int error;

	/* Client socket variables */
	int sockfd = 0;
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

    for (;;) {
        uint8_t buf[BUFSIZE];
		int k = 0;
		
        /* Record some data ... */
        if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
            goto finish;
        }

		/* Encoding using ulaw */
		while(k++ <= BUFSIZE)	buf[k-1] = (uint8_t)linear2ulaw((int)buf[k-1]);
		
        /* And send it to remote host */
        if (loop_write(sockfd, buf, sizeof(buf)) != sizeof(buf)) {
            fprintf(stderr, __FILE__": send() failed: %s\n", strerror(errno));
            goto finish;
        }
    }

    ret = 0;

finish:

    if (s)
        pa_simple_free(s);

    return ret;
}
