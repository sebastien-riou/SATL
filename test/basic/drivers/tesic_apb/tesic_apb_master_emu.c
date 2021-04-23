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

#include "../../../../implementations/c99/basic/drivers/tesic_apb/tesic_apb.h"

extern int sockfd;

typedef enum emu_state_enum_t {
    CTPDU_HDR=0,
    CTPDU_DAT=1,
    CTPDU_FINAL_DAT=2,
    RTPDU_HDR=10,
    RTPDU_DAT=11,
    RTPDU_FINAL_DAT=12
} emu_state_t;
emu_state_t emu_state;


//#define EMU_APB_OVER_SOCKET

#ifdef EMU_APB_OVER_SOCKET
//here we communicate with the other side of the APB emu
void get_updated_state(TESIC_APB_t*ctx){
    uint8_t buf[1024];
    int ret = recv(sockfd, buf, sizeof(buf), MSG_PEEK);
    assert(ret <= TESIC_APB_BUF_LEN+8);
    if(ret>0){//get updated state
        recv(sockfd, ctx, sizeof(TESIC_APB_t),MSG_WAITALL);//note: if we use signals this will be broken
    }
}

void set_updated_state(TESIC_APB_t*ctx){
    send(sockfd, ctx, sizeof(TESIC_APB_t),0);
}

#else

//here we communicate with whatever SATL implementation with compatible parameters
uint32_t emu_fl=0;
uint32_t emu_remaining=0;
unsigned int emu_rx_pos=0;
uint32_t emu_CFG=0;

void get_ack(TESIC_APB_t*ctx){
    uint8_t buf[1024];
    //printf("wait ack");fflush(stdout);
    recv(sockfd, buf, 1,MSG_WAITALL);
    //printf(", received\n");
    ctx->CNT=0;
    ctx->CFG &= ~TESIC_APB_CFG_MASTER_STS_MASK;
    ctx->CFG |=  TESIC_APB_CFG_SLAVE_STS_MASK;
    ctx->CFG &= ~TESIC_APB_CFG_OWNER_MASK;//only master can change this bit
}
//#include <sys/ioctl.h>
//#include <linux/sockios.h>
//assume we are master
void get_updated_state(TESIC_APB_t*ctx){
    uint8_t buf[1024];
    int ret;
    ret=rx_bytes_available(sockfd);
    //ioctl(sockfd, SIOCINQ, &ret);
    //printf("ret=%d\n",ret);
    assert(ret <= TESIC_APB_BUF_LEN);
    if(ret>0){//get updated state
        //printf("receiving at least %u bytes, state=%u\n",ret,emu_state);
        switch(emu_state){
            case CTPDU_DAT:
            case CTPDU_FINAL_DAT:{
                get_ack(ctx);
                return;
            }
        }
        int already_received = recv(sockfd, (uint8_t*)ctx->BUF, 4,MSG_WAITALL);
        unsigned int expected_len=0;
        switch(emu_state){
            case RTPDU_HDR:{
                assert(already_received>=4);
                emu_fl = ctx->BUF[0];
                //printf("RTPDU_HDR fl = %u\n",emu_fl );
                emu_rx_pos=0;
                emu_remaining = emu_fl;
                expected_len = emu_fl;
                if(expected_len>TESIC_APB_BUF_LEN){
                    expected_len = TESIC_APB_BUF_LEN;
                }
                break;
            }
            case RTPDU_DAT:{
                //printf("RTPDU_DAT\n");
                expected_len = TESIC_APB_BUF_LEN;
                break;
            }
            case RTPDU_FINAL_DAT:{
                //printf("RTPDU_FINAL_DAT\n");
                expected_len = emu_remaining;
                break;
            }
        }
        unsigned int remaining = expected_len - already_received;
        uint8_t*b8 = (uint8_t*)ctx->BUF;
        if(remaining % 4) remaining += 4-(remaining%4);
        if(remaining){
            //printf("waiting for %u bytes\n",remaining);
            ret = recv(sockfd, b8+already_received, remaining,MSG_WAITALL);
            assert(remaining==ret);

            //printf("received %u bytes\n",expected_len);
            //for(unsigned int i=0;i<expected_len;i++){
            //    printf("%02X ",b8[i]);
            //}
            //printf("\n");
        }
        //printf("update: CNT=%u\n",expected_len);
        ctx->CNT = expected_len;
        ctx->CFG&=~TESIC_APB_CFG_MASTER_STS_MASK;
        ctx->CFG|=TESIC_APB_CFG_SLAVE_STS_MASK;
        switch(emu_state){
            case RTPDU_HDR:{
                assert(ctx->CNT>=6);
                emu_remaining = emu_fl - ctx->CNT;
                if(0==emu_remaining){
                    emu_state = CTPDU_HDR;
                }else {
                    if(emu_remaining<=TESIC_APB_BUF_LEN){
                        emu_state = RTPDU_FINAL_DAT;
                    }else{
                        emu_state = RTPDU_DAT;
                    }
                }
                break;
            }
            case RTPDU_DAT:{
                assert(TESIC_APB_BUF_LEN==ctx->CNT);
                emu_remaining -= ctx->CNT;
                if(0==emu_remaining){
                    emu_state = CTPDU_HDR;
                }else {
                    if(emu_remaining<=TESIC_APB_BUF_LEN){
                        emu_state = RTPDU_FINAL_DAT;
                    }else{
                        emu_state = RTPDU_DAT;
                    }
                }
                break;
            }
            case RTPDU_FINAL_DAT:{
                assert(emu_remaining == ctx->CNT);
                emu_state = CTPDU_HDR;
                break;
            }
        }
    }
    emu_CFG = ctx->CFG;
}

//assume we are master
void set_updated_state(TESIC_APB_t*ctx){
    if(ctx->CFG & TESIC_APB_CFG_MASTER_STS_MASK){
        if(0==(emu_CFG & TESIC_APB_CFG_MASTER_STS_MASK)){//send data
            assert(0== (emu_CFG & TESIC_APB_CFG_OWNER_MASK));
            unsigned int padded_len = ((ctx->CNT + 3) / 4)*4;
            uint8_t *b8 = (uint8_t*)ctx->BUF;
            //printf("sending %u bytes\n",padded_len);
            //for(unsigned int i=0;i<padded_len;i++){
            //    printf("%02X ",b8[i]);
            //}
            //printf("\n");
            assert(padded_len<=TESIC_APB_BUF_LEN);
            if(0==padded_len) {
                //printf("emu_state=%u\n",emu_state);
                assert((CTPDU_DAT==emu_state)||(RTPDU_DAT==emu_state)||(RTPDU_FINAL_DAT==emu_state));
                padded_len = 1;//sending ack, generic socket impl expect a payload
            }
            switch(emu_state){
                case CTPDU_HDR:{
                    //printf("CTPDU_HDR\n");
                    assert(ctx->CNT>=12);
                    emu_fl = ctx->BUF[0];
                    emu_remaining = emu_fl - ctx->CNT;
                    if(0==emu_remaining){
                        emu_state = RTPDU_HDR;
                    }else {
                        if(emu_remaining<=TESIC_APB_BUF_LEN){
                            emu_state = CTPDU_FINAL_DAT;
                        }else{
                            emu_state = CTPDU_DAT;
                        }
                    }
                    break;
                }
                case CTPDU_DAT:{
                    //printf("CTPDU_DAT\n");
                    assert(TESIC_APB_BUF_LEN==ctx->CNT);
                    emu_remaining -= ctx->CNT;
                    if(0==emu_remaining){
                        emu_state = RTPDU_HDR;
                    }else {
                        if(emu_remaining<=TESIC_APB_BUF_LEN){
                            emu_state = CTPDU_FINAL_DAT;
                        }else{
                            emu_state = CTPDU_DAT;
                        }
                    }
                    break;
                }
                case CTPDU_FINAL_DAT:{
                    //printf("CTPDU_FINAL_DAT\n");
                    assert(emu_remaining == ctx->CNT);
                    emu_state = RTPDU_HDR;
                    break;
                }
            }
            send(sockfd, (uint8_t*)ctx->BUF, padded_len,0);
        }
    }
    emu_CFG = ctx->CFG;
}

#endif

void emu_init(TESIC_APB_t*ctx){
    emu_state = CTPDU_HDR;
    ctx->CFG = 0x0D000AAB;
    ctx->CNT = 0;
    for(unsigned int i=0;i<TESIC_APB_BUF_DWORDS;i++){
        ctx->BUF[i]=0;
    }
}

uint32_t emu_rd_cfg(TESIC_APB_t*ctx){
    get_updated_state(ctx);
    //printf("read  CFG=0x%08X\n",ctx->CFG);
    return ctx->CFG;
}
uint32_t emu_rd_cnt(TESIC_APB_t*ctx){
    get_updated_state(ctx);
    //printf("read  CNT=0x%08X\n",ctx->CNT);
    return ctx->CNT;
}
uint32_t emu_rd_buf(TESIC_APB_t*ctx, unsigned int idx){
    //printf("read  BUF[%u]=",idx);
    get_updated_state(ctx);
    //printf("read  BUF[%u]=0x%08X\n",idx,ctx->BUF[idx]);
    return ctx->BUF[idx];
}
void emu_wr_cfg(TESIC_APB_t*ctx,uint32_t val){
    //printf("write CFG=0x%08X\n",val);
    ctx->CFG = (val & 0xFF7F0000) | 0x0AAB | (ctx->CFG & TESIC_APB_CFG_SLAVE_ITEN_MASK);
    set_updated_state(ctx);
}
void emu_wr_cnt(TESIC_APB_t*ctx,uint32_t val){
    //printf("write CNT=0x%08X\n",val);
    assert(val < 1<<16);
    ctx->CNT = val;
    set_updated_state(ctx);
}
void emu_wr_buf(TESIC_APB_t*ctx, unsigned int idx, uint32_t val){
    ctx->BUF[idx] = val;
    set_updated_state(ctx);
}
