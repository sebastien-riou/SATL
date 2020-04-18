//#include <sys/socket.h>
//#include <sys/types.h>
//#include <netinet/in.h>
//#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
//#include <arpa/inet.h>
#include <stdint.h>
#include <assert.h>

#define SATL_SOCKET_ONLY_CUSTOM_FUNCS
#include "../../../../implementations/c99/basic/drivers/socket/satl_socket.h"

#include "../../../../implementations/c99/basic/drivers/serial/satl_serial.h"

extern int sockfd;

//#include <sys/ioctl.h>
//#include <linux/sockios.h>

void emu_init(SATL_driver_ctx_t*const ctx){}

uint8_t emu_rx(){
    uint8_t out;
    recv(sockfd, &out, 1,MSG_WAITALL);
    return out;
}
void emu_tx(uint8_t val){
    int iResult = send(sockfd, &val, 1,0);
    if (iResult!=1) {
        printf("send returned %d\n",iResult);
    }
}
