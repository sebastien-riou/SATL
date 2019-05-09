#ifndef __SATL_COM16_H__
#define __SATL_COM16_H__


#include "com16.h"

//assume we have built-in flow control
#define SATL_ACK 0

#define SATL_SFR_GRANULARITY 2

#define SATL_SUPPORT_SLAVE
#define SATL_SUPPORT_MASTER

//assume COM16_ADDR is defined to based address of COM16 peripheral
//#define SATL_COM16 ((COM16_t*)(COM16_ADDR))
#define SATL_COM16 (ctx->hw)

typedef struct SATL_driver_ctx_struct_t {
    COM16_t*hw;
    uint16_t buf;
    uint32_t buf_level;
} SATL_driver_ctx_t;

static uint32_t SATL_get_rx_level(const SATL_driver_ctx_t *ctx){
    return ctx->buf_level;
}

static uint32_t SATL_slave_init_driver( SATL_driver_ctx_t*const ctx, void *hw){
    ctx->buf = 0;
    ctx->buf_level = 0;
    ctx->hw = hw;
    return 0;
}

static uint32_t SATL_master_init_driver( SATL_driver_ctx_t*const ctx, void *hw){
    return SATL_slave_init_driver(ctx,hw);
}

static void SATL_switch_to_tx(SATL_driver_ctx_t *const ctx){
    //printf("SATL_switch_to_tx\n");
    ctx->buf_level=0;//sometime a padding byte is left unread
    assert(0==COM16_RX_IS_READY(ctx->hw));
}

static void __SATL_tx16(const SATL_driver_ctx_t *ctx,uint16_t dat){
    while(0==COM16_TX_IS_READY(ctx->hw));
    COM16_SET_TX(ctx->hw,dat);
}

static uint16_t __SATL_rx16(SATL_driver_ctx_t *const ctx){
    while(0==COM16_RX_IS_READY(ctx->hw));
    return COM16_GET_RX(ctx->hw);
}

static void SATL_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    //printf("SATL_tx %u\n",len);
    uint8_t*buf8 = (uint8_t*)buf;
    if(ctx->buf_level && len>=1){
        ctx->buf |= ((uint16_t)buf8[0])<<8;
        __SATL_tx16(ctx,ctx->buf);
        ctx->buf_level=0;
        len--;
        buf8++;
    }
    for(unsigned int i=0;i<len/2;i++){
        uint16_t dat = buf8[2*i+1];
        dat = dat<<8;
        dat |= buf8[2*i];
        __SATL_tx16(ctx,dat);
    }
    if(len%2){
        ctx->buf_level=1;
        ctx->buf = buf8[len-1];
    }
}

static void SATL_final_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    SATL_tx(ctx,buf,len);
    //printf("SATL_final_tx ctx->buf_level=%u\n",ctx->buf_level);
    if(ctx->buf_level){
        __SATL_tx16(ctx,ctx->buf);
        ctx->buf_level=0;
    }
}

static void SATL_generic_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    //printf("SATL_generic_rx len=%u\n",len);
    if(0==len) return;
    uint8_t*buf8 = (uint8_t*)buf;
    if(ctx->buf_level){
        buf8[0] = ctx->buf;
        len--;
        buf8++;
        ctx->buf_level=0;
    }
    for(unsigned int i=0;i<len/2;i++){
        ctx->buf = __SATL_rx16(ctx);
        buf8[2*i+0] = ctx->buf;
        buf8[2*i+1] = ctx->buf>>8;
    }
    if(len%2){
        ctx->buf = __SATL_rx16(ctx);
        buf8[len-1] = ctx->buf;
        ctx->buf_level=1;
        ctx->buf = ctx->buf>>8;
    }
}

static void SATL_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    //printf("SATL_rx %u\n",len);
    SATL_generic_rx(ctx,buf,len);
}

static void SATL_final_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    //printf("SATL_final_rx %u\n",len);
    SATL_generic_rx(ctx,buf,len);
}

static void SATL_tx_ack(SATL_driver_ctx_t *const ctx){
    //printf("SATL_tx_ack\n");
}

static void SATL_rx_ack(SATL_driver_ctx_t *const ctx){
    //printf("SATL_rx_ack ");
}

#endif //__SATL_COM16_H__
