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

/*
 * Controller Area Network Identifier structure
 *
 * bit 0-28	: CAN identifier (11/29 bit)
 * bit 29	: error message frame flag (0 = data frame, 1 = error message)
 * bit 30	: remote transmission request flag (1 = rtr frame)
 * bit 31	: frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
 */

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
		if (((frame.can_id & AJ_NEW_CLIENT_MASK) == AJ_NEW_CLIENT_MASK)) {
			//Нужно проверить data часть кадра
			printf("IF AJ_NEW_CLIENT_MASK\n");
			if ((frame.can_dlc == 8)  & (frame.data[0] == 0xFF) & (frame.data[1] == 0xFF) & (frame.data[2] == 0xFF) &
			(frame.data[3] == 0xFF) & (frame.data[4] == 0xCA) & (frame.data[5] == 0xF3) & (frame.data[6] == 0x04) &
			(frame.data[7] == 0xB0)) {
				frame.can_dlc = 6;
				printf("Have new AJ Client!\n");
				int i;
				//Заносим новые данные 4 байта MAGIC_AJ
				for (i = 0; i < 4; i++) {
					frame.data[i] = 0xFF;
				}
				//CLI_ID в двух байтах
				frame.data[4] = (frame.can_id & 0x700) >> 8;
				frame.data[5] = frame.can_id & 0xFF;
				//делаем в кадре новый ID
				frame.can_id = 0x80000000 | server_id | ((SessionID) << 11);
				printf("Gave SessionID: %x\n", SessionID);
				//Эмулируем Пул
				SessionID++;

			}
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
	close(s);
	return 0;
}