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
#define AJ_NEW_CLIENT_MASK			0x3ff
#define AJ_CONTROL_FRAME			0xf
#define AJ_DATA_FRAME				0xC
#define AJ_ERROR_FRAME				0x7
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
	canid_t client_id = 0x92e80200;
	struct sockaddr_can addr;
	struct can_frame frame;
	struct ifreq ifr;

	char *ifname = "vcan0";
	char testString[46] = "This is test string__!!This is test string__!!";
	char cutTest[46];
	int stringDone = 0;
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
	int count; //Определяем, сколько информационных кадров будет
	int lastFrameDataNumber = 46 % 7; //46 потом заменить на sizeof(ARDP frame)
	if (lastFrameDataNumber) {
		count = 46 / 7 + 1;
	} else {
		count = 46 / 7;
	}
	printf("count = %d\n", count);
	if (count > 15) {
		printf("Houston, we have a problem\n");
	}
	//Нужна проверка, уместим ли всё в один кадр, будет ли последний кадр полным.

	frame.can_id = client_id;
	frame.can_dlc = 1;
	frame.data[0] = (AJ_CONTROL_FRAME << 4) | count;

	write(s, &frame, sizeof(struct can_frame));
	//frame.can_dlc = 8;
	//frame.data[0] = (AJ_DATA_FRAME << 4) | 0;
	//frame.data[1] = 0xCA;
	//write(s, &frame, sizeof(struct can_frame));
	
	int iter;
	int offset = 0;
	int bytesToCopy = 0;
	int testOffCp = 0;
	for (iter = 0; iter < count; ++iter) {
		if ((iter + 1) == count & lastFrameDataNumber != 0) {
			frame.can_dlc = lastFrameDataNumber + 1;
			bytesToCopy = lastFrameDataNumber;
		} else {
			frame.can_dlc = 8;
			bytesToCopy = 7;
			//testOffCp++;
		}
		/*if (iter + 1 == count) {
			bytesToCopy = 8;
		}*/
		frame.data[0] = (AJ_DATA_FRAME << 4) | iter;
		memcpy(frame.data + 1, testString + offset, bytesToCopy);
		memcpy(cutTest + offset, testString + offset, bytesToCopy);
		offset += 7;
		//testOffCp += 7;
		write(s, &frame, sizeof(struct can_frame));
		printf("wrote, iter: %d\n", iter);
	}
	printf("String after cuttng:\n%s\n", cutTest);
	close(s);
}
