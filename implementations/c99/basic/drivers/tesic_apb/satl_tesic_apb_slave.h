#ifndef __SATL_TESIC_APB_SLAVE_H__
#define __SATL_TESIC_APB_SLAVE_H__

#if defined(SATL_TESIC_APB_SLAVE_VERBOSE)
    #ifdef __PRINT_H__
        #define PRINT_APB_STATE(str) do{\
                    print(str);\
                    print32x(" CFG=",SATL_TESIC_APB_GET_CFG()," ");\
                    println32x("CNT=",SATL_TESIC_APB_GET_CNT());\
                }while(0)

        #define SATL_TESIC_APB_PRINTF(str,...) print(str)
    #else
        #define PRINT_APB_STATE(str) do{\
                    printf("%s",str);\
                    printf(" CFG=%08x ",SATL_TESIC_APB_GET_CFG());\
                    printf("CNT=%08x\n",SATL_TESIC_APB_GET_CNT());\
                }while(0)
        #define SATL_TESIC_APB_PRINTF(str,...) printf(str __VA_OPT__(,) __VA_ARGS__)
    #endif
#else
#define PRINT_APB_STATE(str)
#define SATL_TESIC_APB_PRINTF(str,...)
#endif

#include "tesic_apb.h"

#define SATL_ACK 1
#define SATL_MBLEN TESIC_APB_BUF_LEN
#define SATL_SBLEN TESIC_APB_BUF_LEN
#define SATL_SFR_GRANULARITY 4

//this file support only slave operation
#define SATL_SUPPORT_SLAVE

#define SATL_TESIC_APB (ctx->hw)

#define SATL_TESIC_APB_GET_CFG()          TESIC_APB_GET_CFG(SATL_TESIC_APB)
#define SATL_TESIC_APB_SET_CFG(val)       TESIC_APB_SET_CFG(SATL_TESIC_APB,val)
#define SATL_TESIC_APB_GET_CNT()          TESIC_APB_GET_CNT(SATL_TESIC_APB)
#define SATL_TESIC_APB_SET_CNT(val)       TESIC_APB_SET_CNT(SATL_TESIC_APB,val)
#define SATL_TESIC_APB_GET_BUF(idx)       TESIC_APB_GET_BUF(SATL_TESIC_APB,idx)
#define SATL_TESIC_APB_SET_BUF(idx,val)   TESIC_APB_SET_BUF(SATL_TESIC_APB,idx,val)
#define SATL_TESIC_APB_OWNER_IS_MASTER()  TESIC_APB_OWNER_IS_MASTER(SATL_TESIC_APB)
#define SATL_TESIC_APB_OWNER_IS_SLAVE()   TESIC_APB_OWNER_IS_SLAVE(SATL_TESIC_APB)

typedef struct SATL_driver_ctx_struct_t {
    uint32_t rx_buf;
    uint32_t rx_pos;
    uint32_t rx_buf_level;
    TESIC_APB_t*hw;
} SATL_driver_ctx_t;

#define SATL_TESIC_APB_INVALID 0xFFFFFFFF

static uint32_t SATL_get_rx_level(const SATL_driver_ctx_t *ctx){
    PRINT_APB_STATE("SATL_get_rx_level");
    if(!SATL_TESIC_APB_OWNER_IS_SLAVE()) return 0;
    if(SATL_TESIC_APB_INVALID==ctx->rx_pos) return 0;//we are in TX mode
    return SATL_TESIC_APB_GET_CNT()-ctx->rx_pos+ctx->rx_buf_level;
}

static uint32_t SATL_get_tx_level(const SATL_driver_ctx_t *ctx){
    PRINT_APB_STATE("SATL_get_tx_level");
    return TESIC_APB_BUF_LEN-SATL_TESIC_APB_GET_CNT();
}

static uint32_t SATL_slave_init_driver( SATL_driver_ctx_t*const ctx, void *hw){
    ctx->rx_buf = 0;
    ctx->rx_pos = 0;
    ctx->rx_buf_level = 0;
    ctx->hw = hw;
    PRINT_APB_STATE("init");
    //skip initialization phase as APB is meant for intra SOC communication
    return TESIC_APB_BUF_LEN;
}

static void SATL_switch_to_tx(SATL_driver_ctx_t *const ctx){
    PRINT_APB_STATE("SATL_switch_to_tx entry");
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    assert(ctx->rx_pos == (SATL_TESIC_APB_GET_CNT()+ctx->rx_buf_level));//check that all data sent by master has been read
    SATL_TESIC_APB_SET_CNT(0);
    ctx->rx_pos=SATL_TESIC_APB_INVALID;
    ctx->rx_buf_level=SATL_TESIC_APB_INVALID;
    PRINT_APB_STATE("SATL_switch_to_tx exit");
}

//use CNT register to track progress of buffer write
//this allows to support several calls to SATL_tx to prepare the buffer
//buffer is actually sent when SATL_tx_flush_switch_to_rx is called
static void SATL_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    //println32x("SATL_tx 0x",len);
    PRINT_APB_STATE("SATL_tx entry");
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    assert(0 == (len % sizeof(uint32_t) ));//check SFR granularity requirement
    assert(SATL_TESIC_APB_GET_CNT()+len<=TESIC_APB_BUF_LEN);//check buffer length requirement
    uint32_t stack_buf[TESIC_APB_BUF_DWORDS];
    const uint32_t *aligned_buf;
    if(((uintptr_t)buf) & 0x3){//need to align the data, do it assuming stack is large and memcpy optimized
        memcpy(stack_buf,buf,len);
        aligned_buf = stack_buf;
    } else {
        aligned_buf = (uint32_t*const) buf;
    }
    const unsigned int nwords = len / sizeof(uint32_t);
    const unsigned int base = SATL_TESIC_APB_GET_CNT() / sizeof(uint32_t);
    for(unsigned int i=0;i<nwords;i++){
        SATL_TESIC_APB_SET_BUF(base+i,aligned_buf[i]);
    }
    SATL_TESIC_APB_SET_CNT((base+nwords)*sizeof(uint32_t));
    if(TESIC_APB_BUF_LEN == SATL_TESIC_APB_GET_CNT()){//buffer full, send data
        ctx->rx_pos=0;
        ctx->rx_buf_level=0;
        SATL_TESIC_APB_SET_CFG(TESIC_APB_CFG_SLAVE_STS_MASK);//set status and give buffer ownership to master side
    }
    PRINT_APB_STATE("SATL_tx exit");
}

static void SATL_final_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    //println32x("SATL_final_tx 0x",len);
    PRINT_APB_STATE("SATL_final_tx entry");
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    unsigned int safe_len = len & 0x1FFFC;
    SATL_tx(ctx, buf,safe_len);
    unsigned int remaining = len & 0x3;
    if(remaining){//write the last word
        uint32_t w;
        const uint8_t*const buf8 = (const uint8_t*const)buf;
        memcpy(&w,buf8+safe_len,remaining);
        const unsigned int base = SATL_TESIC_APB_GET_CNT() / sizeof(uint32_t);
        SATL_TESIC_APB_SET_BUF(base,w);
        SATL_TESIC_APB_SET_CNT(base * sizeof(uint32_t)+remaining);
    }
    ctx->rx_pos=0;
    ctx->rx_buf_level=0;
    SATL_TESIC_APB_SET_CFG(TESIC_APB_CFG_SLAVE_STS_MASK);//set status and give buffer ownership to master side
    PRINT_APB_STATE("SATL_final_tx exit");
}

static void SATL_generic_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    PRINT_APB_STATE("SATL_generic_rx");
    SATL_TESIC_APB_PRINTF("len=0x%x",len);
    while(0 == (SATL_TESIC_APB_GET_CFG() & TESIC_APB_CFG_MASTER_STS_MASK));
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    SATL_TESIC_APB_PRINTF("ctx->rx_pos=%u, ctx->rx_buf_level=%u, SATL_TESIC_APB_GET_CNT()=%u\n",ctx->rx_pos, ctx->rx_buf_level, SATL_TESIC_APB_GET_CNT());
    assert(ctx->rx_pos+len-ctx->rx_buf_level<=SATL_TESIC_APB_GET_CNT());//check buffer length requirement
    uint8_t* buf8 = (uint8_t*)buf;
    SATL_TESIC_APB_PRINTF("ctx->rx_buf_level=%u, len=%u, ctx->rx_pos=%u\n",ctx->rx_buf_level,len, ctx->rx_pos);
    while(ctx->rx_buf_level && len){
        buf8[0] = ctx->rx_buf;
        ctx->rx_buf=ctx->rx_buf>>8;
        buf8++;
        len--;
        ctx->rx_buf_level--;
    }
    SATL_TESIC_APB_PRINTF("ctx->rx_buf_level=%u, len=%u\n",ctx->rx_buf_level,len);
    const unsigned int nwords = len / sizeof(uint32_t);
    const unsigned int base = ctx->rx_pos / sizeof(uint32_t);
    if(((uintptr_t)buf) & 0x3){//unaligned destination
        for(unsigned int i=0;i<nwords;i++){
            uint32_t w = SATL_TESIC_APB_GET_BUF(base+i);
            buf8[i*4+0] = w;
            buf8[i*4+1] = w>>8;
            buf8[i*4+2] = w>>16;
            buf8[i*4+3] = w>>24;
        }
    } else {
        uint32_t*aligned_buf = (uint32_t*)buf;
        for(unsigned int i=0;i<nwords;i++){
            aligned_buf[i] = SATL_TESIC_APB_GET_BUF(base+i);
        }
    }
    ctx->rx_pos += 4*nwords;
    len -= 4*nwords;
    if(len){
        SATL_TESIC_APB_PRINTF("len=%u, ctx->rx_pos=%u\n",len, ctx->rx_pos);
        buf8 += 4*nwords;
        ctx->rx_buf = SATL_TESIC_APB_GET_BUF(base+nwords);
        ctx->rx_buf_level = 4-len;
        ctx->rx_pos += 4;
        memcpy(buf8,&ctx->rx_buf,len);
        ctx->rx_buf = ctx->rx_buf >> (len*8);
    }
    SATL_TESIC_APB_PRINTF("ctx->rx_buf_level=%u, ctx->rx_pos=%u\n",ctx->rx_buf_level,ctx->rx_pos);
}

static void SATL_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    //println32x("SATL_rx 0x",len);
    SATL_generic_rx(ctx,buf,len);
}

static void SATL_final_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    //println32x("SATL_final_rx 0x",len);
    SATL_generic_rx(ctx,buf,len);
}


static void SATL_tx_ack(SATL_driver_ctx_t *const ctx){
    PRINT_APB_STATE("SATL_tx_ack entry");
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    SATL_TESIC_APB_PRINTF("ctx->rx_buf_level=%u, ctx->rx_pos=%u\n",ctx->rx_buf_level,ctx->rx_pos);
    assert(0==ctx->rx_buf_level);
    ctx->rx_pos=0;
    ctx->rx_buf_level=0;
    SATL_TESIC_APB_SET_CNT(0);
    SATL_TESIC_APB_SET_CFG(TESIC_APB_CFG_SLAVE_STS_MASK);//set status and give buffer ownership to master side
    PRINT_APB_STATE("SATL_tx_ack exit");
}

static void SATL_rx_ack(SATL_driver_ctx_t *const ctx){
    PRINT_APB_STATE("SATL_rx_ack ");
    while(0 == (SATL_TESIC_APB_GET_CFG() & TESIC_APB_CFG_MASTER_STS_MASK));
    //println("received");
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    assert(0 == SATL_TESIC_APB_GET_CNT());
}

#endif //__SATL_TESIC_APB_H__