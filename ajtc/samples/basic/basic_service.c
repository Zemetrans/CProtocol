/**
 * @file
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
#define AJ_MODULE BASIC_SERVICE

#include <stdio.h>
#include <ajtcl/aj_debug.h>
#include <ajtcl/alljoyn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define CONNECT_ATTEMPTS   10
static const char ServiceName[] = "org.alljoyn.Bus.sample";
static const char ServicePath[] = "/sample";
static const uint16_t ServicePort = 25;

uint8_t dbgBASIC_SERVICE = 0;
/**
 * The interface name followed by the method signatures.
 *
 * See also .\inc\aj_introspect.h
 */
static const char* const sampleInterface[] = {
    "org.alljoyn.Bus.sample",   /* The first entry is the interface name. */
    "?Dummy foo<i",             /* This is just a dummy entry at index 0 for illustration purposes. */
    "?getSensorsCount outStr>i", /* Method at index 1. */
    "?getTemp num<i temp>i",
    "?getName num<i name>s",
    "?lightOff lie>i",
    NULL
};

/**
 * A NULL terminated collection of all interfaces.
 */
static const AJ_InterfaceDescription sampleInterfaces[] = {
    sampleInterface,
    NULL
};

/**
 * Objects implemented by the application. The first member in the AJ_Object structure is the path.
 * The second is the collection of all interfaces at that path.
 */
static const AJ_Object AppObjects[] = {
    { ServicePath, sampleInterfaces },
    { NULL }
};

/*
 * The value of the arguments are the indices of the
 * object path in AppObjects (above), interface in sampleInterfaces (above), and
 * member indices in the interface.
 * The 'cat' index is 1 because the first entry in sampleInterface is the interface name.
 * This makes the first index (index 0 of the methods) the second string in
 * sampleInterface[] which, for illustration purposes is a dummy entry.
 * The index of the method we implement for basic_service, 'cat', is 1 which is the third string
 * in the array of strings sampleInterface[].
 *
 * See also .\inc\aj_introspect.h
 */
#define BASIC_SERVICE_GETSENSORSCOUNT AJ_APP_MESSAGE_ID(0, 0, 1)
#define BASIC_SERVICE_GETTEMP AJ_APP_MESSAGE_ID(0, 0, 2)
#define BASIC_SERVICE_GETNAME AJ_APP_MESSAGE_ID(0, 0, 3)
#define BASIC_SERVICE_LIGHTOFF AJ_APP_MESSAGE_ID(0, 0, 4)
/*
 * Use async version of API for reply
 */
//static uint8_t asyncForm = TRUE;
//int tempMass[64]; //It`s not good

static AJ_Status AppHandleGetSensorsCount(AJ_Message* msg)
{
#define BUFFER_SIZE 20
    char buf[BUFFER_SIZE];
    //int32_t TempSensorCount;
    AJ_Message reply;
    AJ_Arg replyArg;
      
    AJ_MarshalReplyMsg(msg, &reply);
    
    AJ_InitArg(&replyArg, AJ_ARG_STRING, 0, buf, 0);
    //char buf[256];
    char path[] = "/sys/class/hwmon/hwmon1/device/temp";
    char endPath[256];
    char digit[10];
    //int rd;
    int fd = 1;
    int i = 1;
    //int tempMass[64];
    while (1) {//Получаем температуру
        snprintf(digit, 10, "%d", i);
        strncpy(endPath, path, 256);
        strncat(endPath, digit,5);
        strncat(endPath,"_input",6);
        printf("string after strncat: %s\n", endPath);
        fd = open(endPath, 0);
        if (fd < 0) {
            perror("open error");
            break;
        }
        //rd = read(fd, buf, 256);
       // buf[rd] ='\0';
       // printf("buf: %d\n", atoi(buf));
       // tempMass[i] = atoi(buf);
        ++i;
        endPath[0] = '\0'; 
        close(fd);   
    }
    //tempMass[0] = i - 1;
    AJ_MarshalArgs(&reply, "i", i - 1);
    
    return AJ_DeliverMsg(&reply);
    
#undef BUFFER_SIZE
}
static AJ_Status AppHandleGetTemp(AJ_Message* msg) {
#define BUFFER_SIZE 256
    int num;
    char buf[BUFFER_SIZE];
    AJ_Message reply;
    AJ_Arg replyArg;
    
    AJ_UnmarshalArgs(msg, "i", &num);
    AJ_MarshalReplyMsg(msg, &reply);
    
    
    AJ_InitArg(&replyArg, AJ_ARG_STRING, 0, buf, 0);
    
    char path[] = "/sys/class/hwmon/hwmon1/device/temp";
    char endPath[256];
    char digit[10];
    int rd = -1;
    int fd = 1;
    snprintf(digit, 10, "%d", num);
    strncpy(endPath, path, 256);
    strncat(endPath, digit,5);
    strncat(endPath,"_input",6);
    printf("string after strncat: %s\n", endPath);
    fd = open(endPath, 0);
    if (fd < 0) {
        perror("open error");
    }
    rd = read(fd, buf, 256);
    buf[rd] ='\0';
    printf("buf: %d\n", atoi(buf));
    num = atoi(buf);
    endPath[0] = '\0'; 
    close(fd);   
    
    AJ_MarshalArgs(&reply, "i", num);
    return AJ_DeliverMsg(&reply);
    
#undef BUFFER_SIZE
}

static AJ_Status AppHandleGetName(AJ_Message* msg) {
#define BUFFER_SIZE 256
    int num;
    char buf[BUFFER_SIZE];
    char outTxt[BUFFER_SIZE];
    AJ_Message reply;
    AJ_Arg replyArg;
    
    AJ_UnmarshalArgs(msg, "i", &num);
    AJ_MarshalReplyMsg(msg, &reply);
    
    
    AJ_InitArg(&replyArg, AJ_ARG_STRING, 0, buf, 0);
    
    char path[] = "/sys/class/hwmon/hwmon1/device/temp";
    char endPath[256];
    char digit[10];
    int rd = -1;
    int fd = 1;
      
    snprintf(digit, 10, "%d", num);
    strncpy(endPath, path, 256);
    strncat(endPath, digit,5);
    strncat(endPath,"_label",6);
    printf("string after strncat: %s\n", endPath);
    fd = open(endPath, 0);
    if (fd < 0) {
        perror("open error");
    }
    rd = read(fd, outTxt, 256);
    outTxt[rd] ='\0';
    close(fd);       
    AJ_MarshalArgs(&reply, "s", outTxt);
    return AJ_DeliverMsg(&reply);
    
#undef BUFFER_SIZE
}
static AJ_Status AppHandleLightOff(AJ_Message* msg)
{
#define BUFFER_SIZE 20
    char buf[BUFFER_SIZE];
    ;
    AJ_Message reply;
    AJ_Arg replyArg;
      
    AJ_MarshalReplyMsg(msg, &reply);
    
    AJ_InitArg(&replyArg, AJ_ARG_STRING, 0, buf, 0);
    AJ_MarshalArgs(&reply, "i", 0);
    AJ_Status status = AJ_DeliverMsg(&reply);
    system("init 6");
    return status;
    
#undef BUFFER_SIZE
}

/* All times are expressed in milliseconds. */
#define CONNECT_TIMEOUT     (1000 * 60)
#define UNMARSHAL_TIMEOUT   (1000 * 5)
#define SLEEP_TIME          (1000 * 2)

int AJ_Main(void)
{
    AJ_Status status = AJ_OK;
    AJ_BusAttachment bus;
    uint8_t connected = FALSE;
    uint32_t sessionId = 0;

    /* One time initialization before calling any other AllJoyn APIs. */
    AJ_Initialize();

    /* This is for debug purposes and is optional. */
    AJ_PrintXML(AppObjects);

    AJ_RegisterObjects(AppObjects, NULL);

    while (TRUE) {
        AJ_Message msg;

        if (!connected) {
            status = AJ_StartService(&bus,
                                     NULL,
                                     CONNECT_TIMEOUT,
                                     FALSE,
                                     ServicePort,
                                     ServiceName,
                                     AJ_NAME_REQ_DO_NOT_QUEUE,
                                     NULL);

            if (status != AJ_OK) {
                continue;
            }

            AJ_InfoPrintf(("StartService returned %d, session_id=%u\n", status, sessionId));
            connected = TRUE;
        }

        status = AJ_UnmarshalMsg(&bus, &msg, UNMARSHAL_TIMEOUT);

        if (AJ_ERR_TIMEOUT == status) {
            continue;
        }

        if (AJ_OK == status) {
            switch (msg.msgId) {
            case AJ_METHOD_ACCEPT_SESSION:
                {
                    uint16_t port;
                    char* joiner;
                    AJ_UnmarshalArgs(&msg, "qus", &port, &sessionId, &joiner);
                    status = AJ_BusReplyAcceptSession(&msg, TRUE);
                    AJ_InfoPrintf(("Accepted session session_id=%u joiner=%s\n", sessionId, joiner));
                }
                break;

            case BASIC_SERVICE_GETSENSORSCOUNT:
                status = AppHandleGetSensorsCount(&msg);
                break;
            case BASIC_SERVICE_GETTEMP:
                status = AppHandleGetTemp(&msg);
                break;
            case BASIC_SERVICE_GETNAME:
                status = AppHandleGetName(&msg);
                break;
            case BASIC_SERVICE_LIGHTOFF:
                status = AppHandleLightOff(&msg);
                break;

            case AJ_SIGNAL_SESSION_LOST_WITH_REASON:
                {
                    uint32_t id, reason;
                    AJ_UnmarshalArgs(&msg, "uu", &id, &reason);
                    AJ_AlwaysPrintf(("Session lost. ID = %u, reason = %u\n", id, reason));
                }
                break;

            default:
                /* Pass to the built-in handlers. */
                status = AJ_BusHandleBusMessage(&msg);
                break;
            }
        }

        /* Messages MUST be discarded to free resources. */
        AJ_CloseMsg(&msg);

        if ((status == AJ_ERR_READ) || (status == AJ_ERR_WRITE)) {
            AJ_AlwaysPrintf(("AllJoyn disconnect.\n"));
            AJ_Disconnect(&bus);
            connected = FALSE;

            /* Sleep a little while before trying to reconnect. */
            AJ_Sleep(SLEEP_TIME);
        }
    }

    AJ_AlwaysPrintf(("Basic service exiting with status %d.\n", status));

    return status;
}

#ifdef AJ_MAIN
int main()
{
    return AJ_Main();
}
#endif
