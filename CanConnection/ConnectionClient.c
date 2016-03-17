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
	for (i = 0; i < 3; i++) {
		frame.can_id  = 0x8000000 | (0x3FF << 11) | client_id;
		frame.can_dlc = 1;
		frame.data[0] = 0x0B;
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
		//Получаем свой новый Айди
		frame.can_id = (((frame.data[0] | (frame.data[1] << 8)) << 11) | 0x4BA) | 0x80000000 ;
		printf("Session ID: %x\n", frame.data[0] | (frame.data[1] << 8));
		//А дальше просто для проверки пишем
		frame.can_dlc = 1;
		frame.data[0] = 0xFF;

		nbytes = write(s, &frame, sizeof(struct can_frame));
		if (nbytes <= 0) {
			perror("Error in socket write");
		} else {
			printf("Wrote %d bytes\n", nbytes);
		}
	}
	close(s);
	
	return 0;
}
