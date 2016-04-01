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
	canid_t client_id = 0x4BA;
	struct sockaddr_can addr;
	struct can_frame frame;
	struct ifreq ifr;

	char *ifname = "vcan0";

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
	//Верхняя граница стандартного ID 0x7FF
	//Опознаёмся на сервере
	int i;
	frame.can_id  = 0x8000000 | AJ_NEW_CLIENT_MASK | client_id;
	frame.can_dlc = 8;
	frame.data[0] = 0xFF;
	frame.data[1] = 0xFF;
	frame.data[2] = 0xFF;
	frame.data[3] = 0xFF;
	frame.data[4] = 0xCA;
	frame.data[5] = 0xF3;
	frame.data[6] = 0x04;
	frame.data[7] = 0xB0;
	nbytes = write(s, &frame, sizeof(struct can_frame));
	if (nbytes <= 0) {
		perror("Error in socket write");
	} else {
		printf("Wrote %d bytes\n", nbytes);
	}
	//Ждём ответного сообщения
	nbytes = read(s, (char *)&frame, sizeof(struct can_frame));
	if (nbytes <= 0) {
		perror("Error in socket read");
	} else {
		printf("Wrote %d bytes\n", nbytes);
	}
	//Разбираем кадр, проверяем его
	if ((frame.can_dlc == 6) & (frame.data[0] == 0xFF) & (frame.data[1] == 0xFF) & (frame.data[2] == 0xFF) &
		(frame.data[3] == 0xFF)) {
		canid_t tmp = ((frame.data[4] & 0x7) << 8) | frame.data[5];
		if (tmp == client_id) {
			printf("Get Server session acknowledge frame!\n");
			client_id = 0x80000000 | client_id | (frame.can_id & 0x1FFFF800) ;
		}
		//получаем свой новый CAN_ID
	} else {
		printf("Get not server acknowledge frame!\n");
	}
	printf("Client Session ID: %x\n", client_id);
	//}
	//frame.can_id = client_id;
	//nbytes = write(s, &frame, sizeof(struct can_frame));
	close(s);
	
	return 0;
}