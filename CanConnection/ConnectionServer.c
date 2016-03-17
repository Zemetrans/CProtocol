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

#define AJ_CAN_SIGNATURE			0xC0
#define AJ_CAN_SERIAL_START			0x0B
#define AJ_CAN_RESTART				0x8
#define AJ_CAN_SERIAL_TERMINATION	0x6
#define AJ_SERIES_SUCCESS			1
#define AJ_SERIES_FAILURE			0
#define AJ_NEW_CLIENT_MASK			0x3ff << 11

int main() {
	int s;
	int nbytes;
	int SessionID = 0x200;
	canid_t server_id = 0x112;
	struct sockaddr_can addr;
	struct can_frame frame;
	struct ifreq ifr;

	char *ifname = "vcan0";
	printf("AJ_NEW_CLIENT_MASK: %x\n", AJ_NEW_CLIENT_MASK);

	if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return -1;
	}

	strcpy(ifr.ifr_name, ifname);
	ioctl(s, SIOCGIFINDEX, &ifr);
	
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex; 

	printf("%s at index %d\n", ifname, ifr.ifr_ifindex);

	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -2;
	}
	while(1) {
		nbytes = read(s, (char *)&frame, sizeof(struct can_frame));
		if (nbytes <= 0) {
			perror("Error in socket read");
		} else {
			printf("Read %d bytes\n", nbytes);
		}
		if (((frame.can_id & AJ_NEW_CLIENT_MASK) == AJ_NEW_CLIENT_MASK) & (frame.data[0] == 0x0B)) {
			printf("Have new AJ Client!\n");
			//должен быть нормальный пул + выдавать разным клиентам разный SessionID
			frame.can_id = ((frame.can_id & 0x7FF)<<11) | server_id | 0x80000000;
			frame.can_dlc = 2;
			frame.data[0] = (__u8)SessionID & 0xFF;
			frame.data[1] = (__u8)(SessionID >> 8) & 0x3F;
			printf("Gave SessionID: %x\n", SessionID);
			SessionID++;
			nbytes = write(s, &frame, sizeof(struct can_frame));
			if (nbytes <= 0) {
				perror("Error in socket write");
			} else {
				printf("Wrote %d bytes\n", nbytes);
			}
		} else {
			printf("Not initial AJ frame\n");
		}
	}
	//printf("frame.can_id & mask: %x\n",(frame.can_id & AJ_NEW_CLIENT_MASK) == AJ_NEW_CLIENT_MASK) ;
	//printf("frame.data: %x\n", frame.data[0]);
	close(s);
	return 0;
}