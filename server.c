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
#define AJ_CAN_SERIES_START			0x0B
#define AJ_CAN_RESTART				0x8
#define AJ_CAN_SERIES_TERMINATION	0x6
#define AJ_CAN_SERIES_SUCCESS		1
#define AJ_CAN_SERIES_FAILURE		0

struct series_list {
	int sockId;
	__u8 checkSum;
	__u8 localCheckSum;
	__u8 framesQuantity;
	char *chunk;
	struct series_list *nextSeries;
};

int main(void) {
	int s;
	int nbytes;
	struct sockaddr_can addr;
    struct ifreq ifr;
    socklen_t len = sizeof(addr);
    struct can_frame frame;
    struct series_list sList;
    int waitDataSeries = 0;
    int success = 0;
    int restart = 0;

    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    

    addr.can_family = AF_CAN;
    //listen all interfaces
    addr.can_ifindex = 0;

    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    while (1) {

    	if (!restart) { //Пока костыль. Если пришёл новый кадр начала серии, то считывать ещё один кадр сразу не надо
    		nbytes = recvfrom(s, &frame, sizeof(struct can_frame),
                      		0, (struct sockaddr*)&addr, &len);

    		if (nbytes < sizeof(struct can_frame)) {
            		fprintf(stderr, "read: incomplete CAN frame\n");
            		return 1;
    		}
    	}
    	//Проверка на принадлежность протоколу
    	if (frame.data[0] & AJ_CAN_SIGNATURE) {
    		//Проверка на тип кадра
    		if (frame.data[0] & AJ_CAN_SERIES_START) {
    			printf("Received SERIES Start Frame! SERIES Length: %d\n", frame.data[1]);
    			sList.sockId = s;
    			sList.framesQuantity = frame.data[1];
    			sList.checkSum = frame.data[2];
    			//необходимо принять информационные кадры
    			int i;
    			int j = 0;
    			for (i = 0; i < sList.framesQuantity; i++) {
    				printf("First for. I = %d\n", i);
    				nbytes = recvfrom(s, &frame, sizeof(struct can_frame),
                      	0, (struct sockaddr*)&addr, &len);

    				printf("recvfrom?\n");
    				if (nbytes < sizeof(struct can_frame)) {
            			fprintf(stderr, "read: incomplete CAN frame\n");
            			return 1;
            		}
            		if (j == frame.data[0]) { //Если номер ожидаемого кадра соответствует номеру пришедшего кадра
            			//тут в будущем будет вытаскивание данных из нагрузки, а пока просто печать на экран
            			int k;
            			for (k = 0; k < frame.can_dlc; k++) {
    						printf("#%d: %#x\n", k, frame.data[k]);
            			}
	            	} else {
	            		printf("the sequence is broken\n");
	            		break;
	            	}
	            	++j;
	            	if (((frame.data[0] & AJ_CAN_SIGNATURE) == AJ_CAN_SIGNATURE) & ((frame.data[0] & AJ_CAN_SERIES_TERMINATION) == AJ_CAN_SERIES_TERMINATION)) {
	            		printf("Received termination frame!\n");
	            		//Место под очистку памяти в будущем
	            		break;
	            	}

	            	if ((frame.data[0] & AJ_CAN_SIGNATURE == AJ_CAN_SIGNATURE) & ((frame.data[0] & AJ_CAN_SERIES_START) == AJ_CAN_SERIES_START)) {
	            		printf("Received Restart frame!\n");
	            		printf("data 0: %#x\ndata 1: %#x\ndata 2: %#x\n", frame.data[0], frame.data[1], frame.data[2]);
    					restart = 1;
	            		break;
	            	}
	            	//Должны ещё быть проверки на дополнительные кадры.
    			}
    			if (j == sList.framesQuantity) {
	            		//тут надо будет вычислять контрольную сумму
	            	sList.localCheckSum = 0;
	            	if (sList.localCheckSum == sList.checkSum) {
	            		printf("Success!\n");
	            		success = AJ_CAN_SERIES_SUCCESS;
	            	} else {
	            		printf("Failure\n");
	            		success = AJ_CAN_SERIES_FAILURE;
	            	}
	            }
    		}
    	
    	} else {
    		printf("Received not a AJ_CAN frame\n");
    		continue;
    	}

    	/* get interface name of the received CAN frame */
    	/*ifr.ifr_ifindex = addr.can_ifindex;
    	ioctl(s, SIOCGIFNAME, &ifr);
    	printf("Received a CAN frame from interface %s\n", ifr.ifr_name);*/
    }
    close(s);
	return 0;
}