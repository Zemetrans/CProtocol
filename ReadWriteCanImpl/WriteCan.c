#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>

#include <linux/types.h>
#include <inttypes.h>

int write_to_can(uint8_t *buffer, int len); // returns -1 or actual bytes written


int write_to_can(uint8_t *buffer, int len) {
    int fd = -1;
	int nbytes = -1;
	int written = 0;
	struct sockaddr_can addr;
	struct can_frame frame;
	struct ifreq ifr;

	char *ifname = "vcan0";
    if (len <= 0) {
        printf("Error: len <= 0\n");
        return -1;
    }
	if((fd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return -1;
	}

	strcpy(ifr.ifr_name, ifname);
	ioctl(fd, SIOCGIFINDEX, &ifr);
	
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex; 

	printf("%s at index %d\n", ifname, ifr.ifr_ifindex);
	
	if (ifr.ifr_ifindex < 0) {
	    printf("Error: ifr_index < 0\n");
	    return -1;
	}
	
	if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -1;
	}
	
	int offset = 0;
	frame.can_id = 0x4BA;
	int i;
	
	for (i = 0; i <= 31; i++) {
	    if (len <=7) {
	        frame.can_dlc = len + 1;
            frame.data[0] = len | (0xF8);
            memcpy(frame.data + 1, buffer + offset, len);
	    } else {
	        frame.can_dlc = 8;
	        frame.data[0] = 7 | (i << 3);
	        memcpy(frame.data + 1, buffer + offset, 7);
	    }
	    offset += 7;
	    nbytes = write(fd, &frame, sizeof(struct can_frame));
	    if (nbytes <= 0) {
	        printf("Error: written < 0\n");
	        return -1;
	    }
        if (len <= 7) {
            written += len;
            break;
        }
        written += 7;
        len -= 7;
    }
    return written;	
}

int main() {
    char buf0[1] = "A";
    char buf1[8];
    char buf2[224];
    char buf3[1024];
    int i;
    for (i = 0; i < 8; ++i) {
        buf1[i] = 'A';
    }
    for (i = 0; i < 224; ++i) {
        buf2[i] = 'A';
    }
    for (i = 0; i < 1024; ++i) {
        buf3[i] = 'A';
    }
    printf("Wrote 1, Written: %d\n", write_to_can(buf0, 1));
    sleep(2);
    printf("Wrote 8, Written: %d\n", write_to_can(buf1, 8));
    sleep(2);
    printf("Wrote 224, Written: %d\n", write_to_can(buf2, 224));
    sleep(2);
    printf("Wrote 1024, Written: %d\n", write_to_can(buf3, 1024));
    return 0; 
}
