#ifndef __SATL_SERIAL_H__
#define __SATL_SERIAL_H__

#define SATL_SERIAL_BUF_LEN 1
#define SATL_ACK 1
#define SATL_MBLEN SATL_SERIAL_BUF_LEN
#define SATL_SBLEN SATL_SERIAL_BUF_LEN
#define SATL_SFR_GRANULARITY 1

#define SATL_SUPPORT_SLAVE
#define SATL_SUPPORT_MASTER

typedef struct SATL_driver_ctx_struct_t {//both shall block as long as needed
    void (*tx)(uint8_t val);
    uint8_t (*rx)();
} SATL_driver_ctx_t;

static uint32_t SATL_get_rx_level(const SATL_driver_ctx_t *ctx){
    return 0;//we don't buffer anything
}

static uint32_t SATL_slave_init_driver( SATL_driver_ctx_t*const ctx, void *hw){
    memcpy(ctx,hw,sizeof(SATL_driver_ctx_t));
    //skip initialization for now
    return SATL_SERIAL_BUF_LEN;
}
static uint32_t SATL_master_init_driver( SATL_driver_ctx_t*const ctx, void *hw){
    memcpy(ctx,hw,sizeof(SATL_driver_ctx_t));
    //skip initialization for now
    return SATL_SERIAL_BUF_LEN;
}

static void SATL_switch_to_tx(SATL_driver_ctx_t *const ctx){}

static void SATL_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    //printf("SATL_tx %u\n",len);
    const uint8_t*buf8=(const uint8_t*)buf;
    for(unsigned int i=0;i<len;i++){
        ctx->tx(buf8[i]);
    }
}

static void SATL_final_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    //printf("SATL_final_tx ");
    SATL_tx(ctx,buf,len);
}

static void SATL_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    //printf("SATL_rx %u\n",len);
    uint8_t*buf8=(uint8_t*)buf;
    for(unsigned int i=0;i<len;i++){
        buf8[i]=ctx->rx();
    }
}

static void SATL_final_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    //printf("SATL_final_rx ");
    SATL_rx(ctx,buf,len);
}

static void SATL_tx_ack(SATL_driver_ctx_t *const ctx){
    //printf("SATL_tx_ack\n");
    ctx->tx('3');
}

static void SATL_rx_ack(SATL_driver_ctx_t *const ctx){
    //printf("SATL_rx_ack ");
    uint8_t ack=ctx->rx();
    (void)ack;
    //assert('A' == ctx->rx());// Shall be ignored but it is useful to check it for debug
    //printf("%02x\n",ack);
}

#endif //__SATL_SERIAL_H__
