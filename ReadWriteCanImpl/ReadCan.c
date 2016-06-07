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

int read_from_can(uint8_t *buffer, int len); // returns -1 or actual bytes read

int read_from_can(uint8_t *buffer, int len) {
    int fd = -1;
	int nbytes = -1;
	int readFromCan = 0;
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
	int i = 0;
	
	while (len > 0) {
	    nbytes = read(fd, &frame, sizeof(struct can_frame));
	    if (nbytes <= 0) {
	        printf("Error: read < 0\n");
	        return -1;
	    }
        if ((frame.can_dlc - 1) != (frame.data[0] & 7)) {
            printf("Error: Frame.dlc != frame payload size. Iteration: %d frame payload size:%d\n", i, frame.data[0] & 7);
            return -1;
        }
        if ((frame.data[0] & 7) == 0) {
            printf("Error: payload size == 0\n");
            return -1;
        }
        if (len < (frame.data[0] & 7)) {
            printf("Error: len < payload size. Iteration: %d payload size: %d len: %d\n", i, frame.data[0] & 7, len);
            return -1;
        }
        //printf("frame.can_dlc = %d\n", frame.can_dlc);
        if ((i == (frame.data[0] >> 3)) | ((frame.data[0] >> 3) == 31)) {
            memcpy(buffer + offset, frame.data + 1, frame.can_dlc - 1);
            readFromCan = readFromCan + frame.can_dlc - 1;
            offset = offset + frame.can_dlc - 1;
            //printf("len in read before dec: %d\n", len);
            len = len - frame.can_dlc + 1;
            //printf("len in read after dec: %d\n", len);
            ++i;
            if ((frame.data[0] >> 3) == 31) {
                break;
            }
        } else {
            printf("Error: sequence number пришел не в том порядке. Ожидаемый номер sequence number: %d Полученный: %d\n", 
                    i, frame.data[0] >>3);
            return -1;
        }
    }
    return readFromCan;	
}

int main() {
    char buffer[256];
    //printf("Read %d bytes\n %s\n", read_from_can(buffer, 200), buffer);
    int i;
    for (i = 0; i < 4; ++i) {
        printf("Read %d bytes\n", read_from_can(buffer, 256));
    }
    return 0; 
}
