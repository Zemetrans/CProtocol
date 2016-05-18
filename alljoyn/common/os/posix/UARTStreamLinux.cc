/**
 * @file
 *
 * This file implements a UART based physical link for communication.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#if !defined(QCC_OS_DARWIN)
#include <qcc/UARTStream.h>
#include <fcntl.h>
#include <errno.h>

#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>

#include <linux/types.h>
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define QCC_MODULE "UART"

using namespace qcc;
using namespace std;

namespace qcc {

QStatus UART(const qcc::String& devName, uint32_t baud, qcc::UARTFd& fd)
{
    return UART(devName, baud, 8, "none", 1, fd);
}

QStatus UART(const qcc::String& devName, uint32_t baud, uint8_t databits, const qcc::String& parity, uint8_t stopbits, qcc::UARTFd& fd)
{
    QCC_DbgTrace(("UART(devName=%s,baud=%d,databits=%d,parity=%s,stopbits=%d)",
                  devName.c_str(), baud, databits, parity.c_str(), stopbits));
    fd = -1;

    /*
     * Validate and set parameters
     */
    struct termios ttySettings;
    memset(&ttySettings, 0, sizeof(ttySettings));
    ttySettings.c_cflag |= CLOCAL | CREAD;

    /*
     * Set input and output baudrate
     */
    speed_t speed;
    switch (baud) {
    case 2400:
        speed = B2400;
        break;

    case 9600:
        speed = B9600;
        break;

    case 19200:
        speed = B19200;
        break;

    case 38400:
        speed = B38400;
        break;

    case 57600:
        speed = B57600;
        break;

    case 115200:
        speed = B115200;
        break;

    case 230400:
        speed = B230400;
        break;

    case 460800:
        speed = B460800;
        break;

    case 921600:
        speed = B921600;
        break;

    case 1000000:
        speed = B1000000;
        break;

    case 1152000:
        speed = B1152000;
        break;

    case 1500000:
        speed = B1500000;
        break;

    case 2000000:
        speed = B2000000;
        break;

    case 2500000:
        speed = B2500000;
        break;

    case 3000000:
        speed = B3000000;
        break;

    case 3500000:
        speed = B3500000;
        break;

    case 4000000:
        speed = B4000000;
        break;

    default:
        QCC_LogError(ER_BAD_ARG_2, ("Invalid baud %d", baud));
        return ER_BAD_ARG_2;
    }
    cfsetospeed(&ttySettings, speed);
    cfsetispeed(&ttySettings, speed);

    switch (databits) {
    case 5:
        ttySettings.c_cflag |= CS5;
        break;

    case 6:
        ttySettings.c_cflag |= CS6;
        break;

    case 7:
        ttySettings.c_cflag |= CS7;
        break;

    case 8:
        ttySettings.c_cflag |= CS8;
        break;

    default:
        QCC_LogError(ER_BAD_ARG_3, ("Invalid databits %d", databits));
        return ER_BAD_ARG_3;
    }

    switch (parity[0]) {
    case 'n':
        ttySettings.c_cflag &= ~(PARENB | PARODD);
#ifdef CMSPAR
        ttySettings.c_cflag &= ~CMSPAR;
#endif
        break;

    case 'e':
        ttySettings.c_iflag |= INPCK;
        ttySettings.c_cflag |= PARENB;
        break;

    case 'o':
        ttySettings.c_iflag |= INPCK;
        ttySettings.c_cflag |= PARENB | PARODD;
        break;

#ifdef CMSPAR
    case 'm':
        ttySettings.c_iflag |= INPCK;
        ttySettings.c_cflag |= PARENB | CMSPAR | PARODD;
        break;

    case 's':
        ttySettings.c_iflag |= INPCK;
        ttySettings.c_cflag |= PARENB | CMSPAR;
        break;
#endif

    default:
        QCC_LogError(ER_BAD_ARG_4, ("Invalid parity %s", parity.c_str()));
        return ER_BAD_ARG_4;
    }

    switch (stopbits) {
    case 1:
        ttySettings.c_cflag &= ~CSTOPB;
        break;

    case 2:
        ttySettings.c_cflag |= CSTOPB;
        break;

    default:
        QCC_LogError(ER_BAD_ARG_5, ("Invalid stopbits %d", stopbits));
        return ER_BAD_ARG_5;
    }

    //int ret = open(devName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    struct sockaddr_can addr;
	//struct can_frame frame;
	struct ifreq ifr;
	
	int ret = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (ret == -1) {
        QCC_LogError(ER_OS_ERROR, ("failed to open serial device %s. ret = %d, %d - %s", devName.c_str(), ret, errno, strerror(errno)));
        goto error;
    }
    fd = ret;
    
    strcpy(ifr.ifr_name, "vcan0");
	ioctl(fd, SIOCGIFINDEX, &ifr);
	
    addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	
	if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		goto error;
	}
	printf("SocketCan create and bind seccess\n");
	//Write Just for test. It will be deleted later
	//frame.can_id = 0xFFB;
	//frame.can_dlc = 1;
	//frame.data[0] = 0xFF;
	//write(fd, &frame, sizeof(struct can_frame));
	
	
    /* Lock this FD, to ensure exclusive access to this serial port. */
    /*ret = flock(fd, LOCK_EX | LOCK_NB);
    if (ret) {
        QCC_LogError(ER_OS_ERROR, ("Lock fd %d failed with '%s'", fd, strerror(errno)));
        goto error;
    }

    QCC_DbgPrintf(("opened serial device %s successfully. ret = %d", devName.c_str(), ret));

    ret = tcflush(fd, TCIOFLUSH);
    if (ret) {
        QCC_LogError(ER_OS_ERROR, ("Flush fd %d failed with '%s'", fd, strerror(errno)));
        goto error;
    }*/

    /**
     * Set the new options on the port
     */
    /*ret = tcsetattr(fd, TCSANOW, &ttySettings);
    if (ret) {
        QCC_LogError(ER_OS_ERROR, ("Set parameters fd %d failed with '%s'", fd, strerror(errno)));
        goto error;
    }

    ret = tcflush(fd, TCIOFLUSH);
    if (ret) {
        QCC_LogError(ER_OS_ERROR, ("Flush fd %d failed with '%s'", fd, strerror(errno)));
        goto error;
    }*/

    return ER_OK;

error:
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    return ER_OS_ERROR;
}

} /* namespace qcc */

UARTStream::UARTStream(UARTFd fd) :
    fd(fd),
    sourceEvent(new Event(fd, Event::IO_READ)),
    sinkEvent(new Event(*sourceEvent, Event::IO_WRITE, false))
{

}

UARTStream::~UARTStream()
{
    delete sourceEvent;
    delete sinkEvent;
}

/* This frame size is chosen so that most of the SLAP packets fit into one frame.
 * If the packet doesnt fit within this, it will be read using two calls to read().
 */
#define RX_BUFSIZE  640
static uint8_t RxBuffer[RX_BUFSIZE];

QStatus UARTStream::PullBytes(void* buf, size_t numBytes, size_t& actualBytes, uint32_t timeout)
{
    QCC_UNUSED(timeout);
    QStatus status = ER_OK;
    struct can_frame frame;
    //int ret = read(fd, buf, numBytes);
    printf("UARTStream::PullBytes. Want to get %lu bytes\n", numBytes);
    size_t i;
    int offset = 0;
    int ret = -1;
    for (i = 0; i < numBytes; ++i) {
        ret = read(fd, (char *)&frame, sizeof(struct can_frame));
        if (ret == -1) {
            printf("Error in PullBytes\n");
            status = ER_OS_ERROR;
            break;
        }
        memcpy((char *)buf + offset, frame.data, 1);   
    }
    actualBytes = numBytes;
    /*if (ret == -1) {
        if (errno == EAGAIN) {
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
            QCC_DbgHLPrintf(("UARTStream::PullBytes (fd = %u): %d - %s", fd, errno, strerror(errno)));
        }
    } else {
        actualBytes = ret;
    }*/
    return status;
}

void UARTStream::Close()
{
    QCC_DbgPrintf(("Uart::close()"));
    if (fd != -1) {
        /* Release the lock on this FD */
        flock(fd, LOCK_UN);
        close(fd);
        fd = -1;
    }
}

QStatus UARTStream::PushBytes(const void* buf, size_t numBytes, size_t& actualBytes)
{

    struct can_frame frame;
    printf("UARTStream::PushBytes. Want to send %lu bytes\n", numBytes);
    QStatus status = ER_OK;
    //int ret = write(fd, buf, numBytes);
    int offset = 0;
    int ret = -1;
    size_t i;
    for (i = 0; i < numBytes; ++i) {
        memcpy(frame.data, (char *)buf + offset, 1);
        ret = write(fd, (char *)&frame, sizeof(struct can_frame));
        if (ret == -1) {
            printf("Error in PullBytes\n");
            status = ER_OS_ERROR;
            break;
        }   
    }
    actualBytes = numBytes;
    /*if (ret == -1) {
        if (errno == EAGAIN) {
            status = ER_WOULDBLOCK;
        } else {
            status = ER_OS_ERROR;
            QCC_DbgHLPrintf(("UARTStream::PushBytes (fd = %u): %d - %s", fd, errno, strerror(errno)));
        }
    } else {
        actualBytes = ret;
    }*/
    return status;
}

UARTController::UARTController(UARTStream* uartStream, IODispatch& iodispatch, UARTReadListener* readListener) :
    m_uartStream(uartStream), m_iodispatch(iodispatch), m_readListener(readListener), exitCount(0)
{
}

QStatus UARTController::Start()
{
    return m_iodispatch.StartStream(m_uartStream, this, NULL, this, true, false);
}

QStatus UARTController::Stop()
{
    return m_iodispatch.StopStream(m_uartStream);
}

QStatus UARTController::Join()
{
    while (!exitCount) {
        qcc::Sleep(100);
    }
    return ER_OK;
}

QStatus UARTController::ReadCallback(Source& source, bool isTimedOut)
{
    QCC_UNUSED(source);
    QCC_UNUSED(isTimedOut);
    size_t actual;
    QStatus status = m_uartStream->PullBytes(RxBuffer, RX_BUFSIZE, actual);
    QCC_ASSERT(status == ER_OK);
    m_readListener->ReadEventTriggered(RxBuffer, actual);
    m_iodispatch.EnableReadCallback(m_uartStream);
    return status;
}

void UARTController::ExitCallback()
{
    m_uartStream->Close();
    exitCount = 1;
}

#endif
