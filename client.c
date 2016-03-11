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

#define AJ_CAN_SIGNATURE			0xCO
#define AJ_CAN_RESTART				0x8
#define AJ_CAN_SERIAL_TERMINATION	0x6

int main() {

	int s;
	int nbytes;
	struct sockaddr_can addr;
	struct can_frame frame;
	struct ifreq ifr;
	//Имя интерфейса
	/*char *ifname = "vcan0";
	//Создаем сокет типа кан, сырой сокет
	//int socket(домен, тип, протокол);
	if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return -1;
	}
	//Копируем имя интерфейса и получаем информацию о Кан устройстве?
	strcpy(ifr.ifr_name, ifname);
	//Надо ещё точнее поискать
	ioctl(s, SIOCGIFINDEX, &ifr);
	
	//вызываемый адрес
	addr.can_family  = AF_CAN;
	//Индекс интерфейса
	addr.can_ifindex = ifr.ifr_ifindex; 

	printf("%s at index %d\n", ifname, ifr.ifr_ifindex);*/
	//Биндим сокет
	if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return -1;
	}

	
    addr.can_ifindex = 0;
    addr.can_family  = AF_CAN;

	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -2;
	}

	strcpy(ifr.ifr_name, "vcan1");
    ioctl(s, SIOCGIFINDEX, &ifr);
    addr.can_ifindex = ifr.ifr_ifindex;
    //addr.can_family  = AF_CAN;

	//Айди сообщения/устройства?
	frame.can_id  = 0x123;
	//frame.can_id  = 0x123;
	if (frame.can_id & CAN_ERR_FLAG) {
		printf("Can error frame send\n");
	}

	strcpy(ifr.ifr_name, "vcan0");
    ioctl(s, SIOCGIFINDEX, &ifr);
	//Количество кусков данных
	frame.can_dlc = 1;
	//Сами данные
	frame.data[0] = 0x11;
	/*frame.data[1] = 0x22;
	frame.data[2] = 0xAA;
	frame.data[3] = 0xA5;*/
	//Собственно пишем
	nbytes = sendto(s, &frame, sizeof(struct can_frame),
                    0, (struct sockaddr*)&addr, sizeof(addr));
	//nbytes = write(s, &frame, sizeof(struct can_frame));

	printf("Wrote %d bytes\n", nbytes);
	
	return 0;
}