#ifndef __SATL_TESIC_APB_MASTER_H__
#define __SATL_TESIC_APB_MASTER_H__

#if defined(SATL_TESIC_APB_MASTER_VERBOSE)
    #ifdef __PRINT_H__
        #define PRINT_APB_STATE(str) do{\
                    print(str);\
                    print32x(" CFG=",SATL_TESIC_APB_GET_CFG()," ");\
                    println32x("CNT=",SATL_TESIC_APB_GET_CNT());\
                }while(0)

        #define SATL_TESIC_APB_PRINTF(str,...) print(str)
        //#define TESIC_APB_GET_BUF_SPY(ctx,idx) TESIC_APB_GET_BUF(ctx,idx)
    #else
        //#define SATL_TESIC_APB_PRINTF(str,...) printf(str __VA_OPT__(,) __VA_ARGS__)
        #define SATL_TESIC_APB_PRINTF printk
        #define PRINT_APB_STATE(str) do{\
                    SATL_TESIC_APB_PRINTF("%s",str);\
                    SATL_TESIC_APB_PRINTF(" CFG=%08x ",SATL_TESIC_APB_GET_CFG());\
                    SATL_TESIC_APB_PRINTF("CNT=%08x\n",SATL_TESIC_APB_GET_CNT());\
                }while(0)


        //#define TESIC_APB_GET_BUF_SPY(ctx,idx) tesic_apb_get_buf_spy(ctx,idx)
    #endif
#else
#define PRINT_APB_STATE(str)
#define SATL_TESIC_APB_PRINTF(str,...)
//#define TESIC_APB_GET_BUF_SPY(ctx,idx) TESIC_APB_GET_BUF(ctx,idx)
#endif

#define TESIC_APB_MASTER_SIDE
#include "tesic_apb.h"

#ifdef SATL_USE_IRQ
    #define SATL_MASTER_IRQ (TESIC_APB_CFG_MASTER_ITEN_MASK | TESIC_APB_CFG_MASTER_REQ_LOCK_MASK)
    #ifndef SATL_WAIT_RX_EVENT
        #error "When SATL_USE_IRQ is defined, user must define SATL_WAIT_RX_EVENT()"
    #endif
#else
    #define SATL_MASTER_IRQ 0
    #ifndef SATL_WAIT_RX_EVENT_YIELD
    #define SATL_WAIT_RX_EVENT_YIELD()
    #endif
#endif

#define SATL_ACK 1
#define SATL_MBLEN TESIC_APB_BUF_LEN
#define SATL_SBLEN TESIC_APB_BUF_LEN
#define SATL_SFR_GRANULARITY 4

//this file support only master operation
#define SATL_SUPPORT_MASTER

//assume TESIC_APB_ADDR is defined to based address of TESIC_APB peripheral
//#define SATL_TESIC_APB ((TESIC_APB_t*)(TESIC_APB_ADDR))
#define SATL_TESIC_APB (ctx->hw)

#define SATL_TESIC_APB_GET_CFG()          TESIC_APB_GET_CFG(SATL_TESIC_APB)
#define SATL_TESIC_APB_SET_CFG(val)       TESIC_APB_SET_CFG(SATL_TESIC_APB,val)
#define SATL_TESIC_APB_GET_CNT()          TESIC_APB_GET_CNT(SATL_TESIC_APB)
#define SATL_TESIC_APB_SET_CNT(val)       TESIC_APB_SET_CNT(SATL_TESIC_APB,val)
#if defined(SATL_TESIC_APB_MASTER_VERBOSE)
    static uint32_t tesic_apb_get_buf_spy(TESIC_APB_t*ctx,unsigned int idx){
        uint32_t out = TESIC_APB_GET_BUF(ctx,idx);
        SATL_TESIC_APB_PRINTF("READ WORD at %3u: 0x%08x\n",idx*4,out);
        return out;
    }
    static void tesic_apb_set_buf_spy(TESIC_APB_t*ctx,unsigned int idx, uint32_t val){
        TESIC_APB_SET_BUF(ctx,idx,val);
        SATL_TESIC_APB_PRINTF("WRITE WORD at %3u: 0x%08x\n",idx*4,val);
    }
    #define SATL_TESIC_APB_GET_BUF(idx)       tesic_apb_get_buf_spy(SATL_TESIC_APB,idx)
    #define SATL_TESIC_APB_SET_BUF(idx,val)   tesic_apb_set_buf_spy(SATL_TESIC_APB,idx,val)
#else
    #define SATL_TESIC_APB_GET_BUF(idx)       TESIC_APB_GET_BUF(SATL_TESIC_APB,idx)
    #define SATL_TESIC_APB_SET_BUF(idx,val)   TESIC_APB_SET_BUF(SATL_TESIC_APB,idx,val)
#endif
#define SATL_TESIC_APB_OWNER_IS_MASTER()  TESIC_APB_OWNER_IS_MASTER(SATL_TESIC_APB)
#define SATL_TESIC_APB_OWNER_IS_SLAVE()   TESIC_APB_OWNER_IS_SLAVE(SATL_TESIC_APB)

//track read progress for rx operation: position of first unread byte
//CNT register track remaining number of bytes
//static unsigned int SATL_rx_pos=0;
//static uint32_t SATL_generic_rx_buf;
//static unsigned int SATL_generic_rx_level=0;
typedef struct SATL_driver_ctx_struct_t {
    TESIC_APB_t*hw;
    uint32_t rx_buf;
    uint32_t rx_pos;
    uint32_t rx_buf_level;
} SATL_driver_ctx_t;

#define SATL_TESIC_APB_INVALID 0xFFFFFFFF

static uint32_t SATL_get_rx_level(const SATL_driver_ctx_t *ctx){
    PRINT_APB_STATE("SATL_get_rx_level");
    //if(!SATL_TESIC_APB_OWNER_IS_MASTER()) return 0;
    if(SATL_TESIC_APB_INVALID==ctx->rx_pos) return 0;//we are in TX mode
    return SATL_TESIC_APB_GET_CNT()-ctx->rx_pos+ctx->rx_buf_level;
}

static uint32_t SATL_master_init_driver( SATL_driver_ctx_t*const ctx, void *hw){
    SATL_TESIC_APB_PRINTF("SATL_master_init_driver\n");
    ctx->rx_buf = 0;
    ctx->rx_pos=SATL_TESIC_APB_INVALID;
    ctx->rx_buf_level=SATL_TESIC_APB_INVALID;
    ctx->hw = hw;
    SATL_TESIC_APB_SET_CNT(0);
    SATL_TESIC_APB_SET_CFG(SATL_MASTER_IRQ);
    //skip initialization phase as APB is meant for intra SOC communication
    return TESIC_APB_BUF_LEN;
}

static void SATL_switch_to_tx(SATL_driver_ctx_t *const ctx){
    SATL_TESIC_APB_PRINTF("SATL_switch_to_tx");
    assert(ctx->rx_pos == (SATL_TESIC_APB_GET_CNT()+ctx->rx_buf_level));//check that all data sent by slave has been read
    SATL_TESIC_APB_SET_CNT(0);
    SATL_TESIC_APB_SET_CFG(SATL_MASTER_IRQ);
    ctx->rx_pos=SATL_TESIC_APB_INVALID;
    ctx->rx_buf_level=SATL_TESIC_APB_INVALID;
    //println(" done");
}

//use CNT register to track progress of buffer write
//this allows to support several calls to SATL_tx to prepare the buffer
//buffer is actually sent when SATL_tx_flush_switch_to_rx is called
static void SATL_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    PRINT_APB_STATE("SATL_tx");
    SATL_TESIC_APB_PRINTF("len=%u\n",len);
    assert(len);
    SATL_TESIC_APB_PRINTF("SATL_TESIC_APB_GET_CFG()=0x%08x\n",SATL_TESIC_APB_GET_CFG());
    assert(SATL_TESIC_APB_OWNER_IS_MASTER());
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
        SATL_TESIC_APB_SET_CFG(SATL_MASTER_IRQ | TESIC_APB_CFG_MASTER_STS_MASK | TESIC_APB_CFG_OWNER_MASK);//set status and give buffer ownership to slave side
    }
}

static void SATL_final_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    PRINT_APB_STATE("SATL_final_tx");
    SATL_TESIC_APB_PRINTF("len=%u\n",len);
    SATL_TESIC_APB_PRINTF("SATL_TESIC_APB_GET_CFG()=0x%08x\n",SATL_TESIC_APB_GET_CFG());
    assert(SATL_TESIC_APB_OWNER_IS_MASTER());
    unsigned int safe_len = len & 0x1FFFC;
    if(safe_len) SATL_tx(ctx, buf,safe_len);
    unsigned int remaining = len & 0x3;
    if(remaining){//write the last word
        uint32_t w;
        const uint8_t*const buf8 = (const uint8_t*const)buf;
        memcpy(&w,buf8+safe_len,remaining);
        const unsigned int base = SATL_TESIC_APB_GET_CNT() / sizeof(uint32_t);
        SATL_TESIC_APB_SET_BUF(base,w);
        SATL_TESIC_APB_SET_CNT(base * sizeof(uint32_t)+remaining);
    }
    ctx->rx_pos=SATL_TESIC_APB_INVALID;
    ctx->rx_buf_level=SATL_TESIC_APB_INVALID;
    PRINT_APB_STATE("SATL_final_tx exit (before giving ownership");
    SATL_TESIC_APB_SET_CFG(SATL_MASTER_IRQ | TESIC_APB_CFG_MASTER_STS_MASK | TESIC_APB_CFG_OWNER_MASK);//set status and give buffer ownership to slave side
    PRINT_APB_STATE("SATL_final_tx exit (after giving ownership");
}

static void SATL_wait_rx_event(SATL_driver_ctx_t *const ctx){
    PRINT_APB_STATE("SATL_wait_rx_event");
    //assert(SATL_TESIC_APB_OWNER_IS_SLAVE());//cannot check that when using interrupts, sometimes the other side is faster

    #ifdef SATL_USE_IRQ
        SATL_WAIT_RX_EVENT();
    #else
        while(0 == (SATL_TESIC_APB_GET_CFG() & TESIC_APB_CFG_SLAVE_STS_MASK)){
            SATL_WAIT_RX_EVENT_YIELD();
        }
    #endif
    SATL_TESIC_APB_SET_CFG(SATL_MASTER_IRQ);//get control of CNT and BUF
    PRINT_APB_STATE("SATL_wait_rx_event exit");
}

static void SATL_generic_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    SATL_TESIC_APB_PRINTF("SATL_generic_rx len=%u\n",len);
    SATL_TESIC_APB_PRINTF("ctx->rx_buf_level=%u, ctx->rx_pos=%u\n",ctx->rx_buf_level,ctx->rx_pos);
    if((SATL_TESIC_APB_INVALID == ctx->rx_pos) && (SATL_TESIC_APB_INVALID==ctx->rx_buf_level)){//first rx for a frame, or after ack
        SATL_wait_rx_event(ctx);
        ctx->rx_pos=0;
        ctx->rx_buf_level=0;
    }else{
        SATL_TESIC_APB_PRINTF("skip wait for rx event");
    }
    assert(ctx->rx_pos+len-ctx->rx_buf_level<=SATL_TESIC_APB_GET_CNT());//check buffer length requirement
    uint8_t* buf8 = (uint8_t*)buf;

    while(ctx->rx_buf_level && len){
        buf8[0] = ctx->rx_buf;
        ctx->rx_buf=ctx->rx_buf>>8;
        buf8++;
        len--;
        ctx->rx_buf_level--;
    }

    const unsigned int nwords = len / sizeof(uint32_t);
    const unsigned int base = ctx->rx_pos / sizeof(uint32_t);
    if(((uintptr_t)buf8) & 0x3){//unaligned destination
        for(unsigned int i=0;i<nwords;i++){
            uint32_t w = SATL_TESIC_APB_GET_BUF(base+i);
            buf8[i*4+0] = w;
            buf8[i*4+1] = w>>8;
            buf8[i*4+2] = w>>16;
            buf8[i*4+3] = w>>24;
        }
    } else {
        uint32_t*aligned_buf = (uint32_t*)buf8;
        for(unsigned int i=0;i<nwords;i++){
            aligned_buf[i] = SATL_TESIC_APB_GET_BUF(base+i);
        }
    }
    ctx->rx_pos += 4*nwords;
    len -= 4*nwords;
    if(len){
        buf8 += 4*nwords;
        ctx->rx_buf = SATL_TESIC_APB_GET_BUF(base+nwords);
        ctx->rx_buf_level = 4-len;
        ctx->rx_pos += 4;
        memcpy(buf8,&ctx->rx_buf,len);
        ctx->rx_buf = ctx->rx_buf >> (len*8);
    }
    SATL_TESIC_APB_PRINTF("ctx->rx_buf_level=%u, ctx->rx_pos=%u\n",ctx->rx_buf_level,ctx->rx_pos);
    SATL_TESIC_APB_PRINTF("SATL_generic_rx exit\n");
}

static void SATL_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    SATL_TESIC_APB_PRINTF("SATL_rx %u",len);
    SATL_generic_rx(ctx,buf,len);
}

static void SATL_final_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    SATL_TESIC_APB_PRINTF("SATL_final_rx %u",len);
    SATL_generic_rx(ctx,buf,len);
}

static void SATL_tx_ack(SATL_driver_ctx_t *const ctx){
    PRINT_APB_STATE("SATL_tx_ack");
    SATL_TESIC_APB_PRINTF("ctx->rx_buf_level=%u, ctx->rx_pos=%u\n",ctx->rx_buf_level,ctx->rx_pos);
    //assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    assert(0==ctx->rx_buf_level);
    ctx->rx_pos=SATL_TESIC_APB_INVALID;
    ctx->rx_buf_level=SATL_TESIC_APB_INVALID;
    SATL_TESIC_APB_SET_CFG(SATL_MASTER_IRQ);//get control of CNT
    SATL_TESIC_APB_SET_CNT(0);
    SATL_TESIC_APB_SET_CFG(SATL_MASTER_IRQ | TESIC_APB_CFG_MASTER_STS_MASK | TESIC_APB_CFG_OWNER_MASK);//set status and give buffer ownership to slave side
}

static void SATL_rx_ack(SATL_driver_ctx_t *const ctx){
    SATL_TESIC_APB_PRINTF("SATL_rx_ack ");
    //assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    //while(0 == (SATL_TESIC_APB_GET_CFG() & TESIC_APB_CFG_SLAVE_STS_MASK));
    SATL_wait_rx_event(ctx);
    SATL_TESIC_APB_PRINTF("received\n");
    assert(0 == SATL_TESIC_APB_GET_CNT());
    SATL_TESIC_APB_SET_CFG(SATL_MASTER_IRQ);//get control of CNT and BUF
}

#endif //__SATL_TESIC_APB_H__
