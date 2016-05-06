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

/*
*Структура, описывающая сессию
*/
struct Session {
 	canid_t ID;
 	canid_t SID;
 	struct can_frame buffer[10];
 	int numberOfFrames;
 	//SDesc - Session Descripter. Показывает состояние сессии. TODO
} session_struct;

int main() {
	int s;
	int nbytes;
	int Session_ID = 0x200;
	canid_t server_id = 0x112;
	struct sockaddr_can addr;
	struct can_frame frame;
	struct ifreq ifr;

	char buf[21];

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
	
	nbytes = read(s, (char *)&frame, sizeof(struct can_frame));
	if (nbytes <= 0) {
		perror("Error in socket read");
	} else {
		printf("Read %d bytes\n", nbytes);
	}
	if (((frame.data[0] & (AJ_CONTROL_FRAME << 4)) >> 4) == AJ_CONTROL_FRAME) {
		printf("Have Control Frame!\nКоличество кадров в серии = %d\n", frame.data[0] & 0xf);
		session_struct.numberOfFrames = frame.data[0] & 0xf;
		session_struct.ID = (frame.can_id & (0x7FF << 18)) >> 18;
		session_struct.SID = frame.can_id & 0x3FFFF;
		int i;
		for (i = 0; i < session_struct.numberOfFrames; ++i) {
			nbytes = read(s, (char *)&frame, sizeof(struct can_frame));
			if (nbytes <= 0) {
				perror("Socket Read");
			}
			session_struct.buffer[i] = frame;
		}
	} else {
		printf("-_-\n%x\n", frame.data[0] & (AJ_CONTROL_FRAME << 4));
	}
	
	printf("Struct data:\nNumber of frames: %d\ndata: %x\nCID: %x\nSID: %x\n", session_struct.numberOfFrames, session_struct.buffer[0].data[1], 
		session_struct.ID, session_struct.SID);
	int i;
	int offset = 0;
	for (i = 0; i < session_struct.numberOfFrames; ++i) {
		memcpy(buf + offset, session_struct.buffer[i].data + 1, session_struct.buffer[i].can_dlc - 1);
		offset += 7;
	}
	printf("%s\n", buf);
	close(s);
	return 0;
 }