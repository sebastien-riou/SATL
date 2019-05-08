#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <assert.h>

#include "../../../../implementations/basic/drivers/com16/com16.h"

extern int sockfd;

#include <sys/ioctl.h>
#include <linux/sockios.h>

void get_updated_state(COM16_t*const ctx){
    int ret;
    if(0==(ctx->CTRL & COM16_CTRL_RXR_MASK)){
        ioctl(sockfd, SIOCINQ, &ret);
        //if(ret) printf("ret=%d\n",ret);
        if(ret>=2){//get updated state
            uint16_t buf;
            int received = recv(sockfd, &buf, 2,MSG_WAITALL);
            ctx->RX = buf;
            assert(2==received);
            ctx->CTRL|=COM16_CTRL_RXR_MASK;
        }
    }
}

void emu_init(COM16_t*const ctx){
    ctx->CTRL = COM16_CTRL_TXR_MASK;
}

uint16_t emu_rd_ctrl(COM16_t*const ctx){
    get_updated_state(ctx);
    //printf("read  CTRL=0x%04X\n",ctx->CTRL);
    return ctx->CTRL;
}
uint16_t emu_rd_rx(COM16_t*const ctx){
    assert(COM16_RX_IS_READY(ctx));
    uint16_t out = ctx->RX;
    ctx->CTRL&=~COM16_CTRL_RXR_MASK;
    //printf("read  RX=0x%04X\n",out);
    get_updated_state(ctx);
    return out;
}
void emu_wr_tx(COM16_t*const ctx,uint16_t val){
    //printf("write TX=0x%04X\n",val);
    ctx->TX = val;
    write(sockfd, (uint8_t*)&val, 2);
}
