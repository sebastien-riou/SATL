#ifndef __SATL_TESIC_APB_SLAVE_H__
#define __SATL_TESIC_APB_SLAVE_H__

#if defined(SATL_TESIC_APB_SLAVE_VERBOSE)

    #define STR(s) #s
    #define XSTR(s) STR(s)

    #ifdef __PRINT_H__
        #define PRINT_APB_STATE(str) do{\
                    print(str);\
                    print32x(" CFG=0x",SATL_TESIC_APB_GET_CFG()," ");\
                    println32x("CNT=0x",SATL_TESIC_APB_GET_CNT());\
                }while(0)

        #define SATL_TESIC_APB_PRINTF(str,...) print(str)
        #define SATL_TESIC_APB_PRINT_CTX(str) do{\
                    print(str);\
                    print32x(" rx_pos=0x",ctx->rx_pos," ");\
                    print32x("rx_buf_level=0x",ctx->rx_buf_level," ");\
                    println32x("rx_buf=0x",ctx->rx_buf);\
                }while(0)
        #define SATL_TESIC_APB_PRINT_HEX32(name) do{\
                    print(#name);\
                    print32x("\t= 0x",*((uint32_t*)(&(name)))," ");\
                }while(0)
        #define SATL_TESIC_APB_PRINT_HEXUI(name) do{\
                    print(#name);\
                    print32x("\t= 0x",*((unsigned int*)(&(name)))," ");\
                }while(0)
    #else
        #define PRINT_APB_STATE(str) do{\
                    printf("%s",str);\
                    printf(" CFG=0x%08x ",SATL_TESIC_APB_GET_CFG());\
                    printf("CNT=0x%08x\n",SATL_TESIC_APB_GET_CNT());\
                }while(0)
        #define SATL_TESIC_APB_PRINTF(str,...) printf(str __VA_OPT__(,) __VA_ARGS__)
        #define SATL_TESIC_APB_PRINT_CTX(str)
        #define SATL_TESIC_APB_PRINT_HEX32(name)
        #define SATL_TESIC_APB_PRINT_HEXUI(name)
    #endif

    #define PRINT_ABP_BUF() do{\
            unsigned int nwords = (SATL_TESIC_APB_GET_CNT() + 3)/ 4;\
            println32x("nwords=0x",nwords);\
            for(unsigned int i=0;i<nwords;i++){\
                print8d("BUF[",i,"] = ");\
                println32x("0x",SATL_TESIC_APB_GET_BUF(i));\
            }\
        }while(0)
    #define PRINT_BUF(msg,buf,nbytes) println_bytes(msg,buf,nbytes);
#else
    #define PRINT_APB_STATE(str)
    #define SATL_TESIC_APB_PRINTF(str,...)
    #define SATL_TESIC_APB_PRINT_CTX(str)
    #define SATL_TESIC_APB_PRINT_HEX32(name)
    #define SATL_TESIC_APB_PRINT_HEXUI(name)
    #define PRINT_ABP_BUF()
    #define PRINT_BUF(msg,buf,nbytes)
#endif

#ifdef SATL_USE_IRQ
    #define SATL_SLAVE_IRQ (TESIC_APB_CFG_SLAVE_ITEN_MASK)
    #ifndef SATL_WAIT_RX_EVENT
        #error "When SATL_USE_IRQ is defined, user must define SATL_WAIT_RX_EVENT()"
    #endif
    #define SATL_WAIT_RX_EVENT_YIELD()
#else
    #define SATL_SLAVE_IRQ ((uint32_t)0)
    #ifndef SATL_WAIT_RX_EVENT_YIELD
    #define SATL_WAIT_RX_EVENT_YIELD()
    #endif
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
    volatile int rx_event_flag;
} SATL_driver_ctx_t;

#define SATL_TESIC_APB_INVALID 0xFFFFFFFF
#define SATL_TESIC_APB_RX_EVENT_FLAG ((uint32_t)1)

static uint32_t SATL_get_rx_level(const SATL_driver_ctx_t *ctx){
    PRINT_APB_STATE("SATL_get_rx_level");
    if(!SATL_TESIC_APB_OWNER_IS_SLAVE()) return 0;
    //assert(SATL_TESIC_APB_INVALID!=ctx->rx_pos);
    if(SATL_TESIC_APB_INVALID==ctx->rx_pos) return 0;//we are in TX mode OR RX before actual reception of the frame
    return SATL_TESIC_APB_GET_CNT()-ctx->rx_pos+ctx->rx_buf_level;
}

static uint32_t SATL_get_tx_level(const SATL_driver_ctx_t *ctx){
    PRINT_APB_STATE("SATL_get_tx_level");
    return TESIC_APB_BUF_LEN-SATL_TESIC_APB_GET_CNT();
}

static uint32_t SATL_slave_init_driver( SATL_driver_ctx_t*const ctx, void *hw){
    ctx->rx_buf = 0;
    ctx->rx_pos = SATL_TESIC_APB_INVALID;
    ctx->rx_buf_level = SATL_TESIC_APB_INVALID;
    ctx->hw = hw;
    ctx->rx_event_flag = 0;
    uint32_t master_sts = SATL_TESIC_APB_GET_CFG() & TESIC_APB_CFG_MASTER_STS_MASK;
    if(master_sts) ctx->rx_event_flag = 1;
    SATL_TESIC_APB_SET_CFG(SATL_SLAVE_IRQ | master_sts);
    PRINT_APB_STATE("init");
    //skip initialization phase as APB is meant for intra SOC communication
    return TESIC_APB_BUF_LEN;
}

static void SATL_switch_to_tx(SATL_driver_ctx_t *const ctx){
    PRINT_APB_STATE("SATL_switch_to_tx entry");
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    assert(ctx->rx_pos == (SATL_TESIC_APB_GET_CNT()+ctx->rx_buf_level));//check that all data sent by master has been read
    SATL_TESIC_APB_SET_CNT(0);
    SATL_TESIC_APB_SET_CFG(SATL_SLAVE_IRQ);
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    ctx->rx_pos=SATL_TESIC_APB_INVALID;
    ctx->rx_buf_level=SATL_TESIC_APB_INVALID;
    PRINT_APB_STATE("SATL_switch_to_tx exit");
}

//use CNT register to track progress of buffer write
//this allows to support several calls to SATL_tx to prepare the buffer
//buffer is actually sent when SATL_tx_flush_switch_to_rx is called
static void SATL_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    PRINT_APB_STATE("SATL_tx entry");
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    if(0==len) return;
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
        ctx->rx_buf_level=0;
        PRINT_APB_STATE("SATL_tx before giving buffer");
        PRINT_ABP_BUF();
        SATL_TESIC_APB_SET_CFG(SATL_SLAVE_IRQ | TESIC_APB_CFG_SLAVE_STS_MASK);//set status and give buffer ownership to master side
    }
    PRINT_APB_STATE("SATL_tx exit");
}

static void SATL_final_tx(SATL_driver_ctx_t *const ctx,const void*const buf,unsigned int len){
    PRINT_APB_STATE("SATL_final_tx entry");
    SATL_TESIC_APB_PRINT_HEXUI(len);
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    unsigned int safe_len = len & 0x1FFFC;
    ctx->rx_buf_level=SATL_TESIC_APB_INVALID;//used to detect if SATL_tx sends the buffer
    SATL_tx(ctx, buf,safe_len);
    unsigned int remaining = len & 0x3;
    if(remaining){//write the last word
        ctx->rx_buf_level=SATL_TESIC_APB_INVALID;
        uint32_t w;
        const uint8_t*const buf8 = (const uint8_t*const)buf;
        memcpy(&w,buf8+safe_len,remaining);
        const unsigned int base = SATL_TESIC_APB_GET_CNT() / sizeof(uint32_t);
        SATL_TESIC_APB_SET_BUF(base,w);
        SATL_TESIC_APB_SET_CNT(base * sizeof(uint32_t)+remaining);
    }
    if(SATL_TESIC_APB_INVALID==ctx->rx_buf_level){
        PRINT_APB_STATE("SATL_final_tx exit (before giving buffer)");
        PRINT_ABP_BUF();
        SATL_TESIC_APB_SET_CFG(SATL_SLAVE_IRQ | TESIC_APB_CFG_SLAVE_STS_MASK);//set status and give buffer ownership to master side
    }else{
        PRINT_APB_STATE("SATL_final_tx exit (buffer already given by SATL_tx)");
    }
    ctx->rx_pos=SATL_TESIC_APB_INVALID;
    ctx->rx_buf_level=SATL_TESIC_APB_INVALID;
}

static void SATL_wait_rx_event(SATL_driver_ctx_t *const ctx){
    PRINT_APB_STATE("SATL_wait_rx_event");
    #ifdef SATL_USE_IRQ
        //if(0 == (SATL_TESIC_APB_GET_CFG() & TESIC_APB_CFG_MASTER_STS_MASK)){
          SATL_WAIT_RX_EVENT();
        //}
        ctx->rx_event_flag = 0;
        SATL_TESIC_APB_SET_CFG(SATL_SLAVE_IRQ);//clear hardware flag

    #else
        while(0 == (SATL_TESIC_APB_GET_CFG() & TESIC_APB_CFG_MASTER_STS_MASK)){
            #ifdef SATL_TESIC_APB_SLAVE_VERBOSE_RX_LOOP
            PRINT_APB_STATE("wait rx loop");
            #endif
            SATL_WAIT_RX_EVENT_YIELD();
        }
    #endif
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    PRINT_APB_STATE("SATL_wait_rx_event exit");
    //while(1);
}

static void SATL_generic_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    PRINT_APB_STATE("SATL_generic_rx");
    #ifdef SATL_TESIC_APB_SLAVE_VERBOSE_RX_DATA
      const unsigned int input_len=len;
    #endif
    //if(SATL_TESIC_APB_INVALID==ctx->rx_buf_level){
    //    ctx->rx_pos=0;
    //    ctx->rx_buf_level=0;
    //}
    //SATL_TESIC_APB_PRINT_HEXUI(len);
    //SATL_wait_rx_event(ctx);

    if((SATL_TESIC_APB_INVALID == ctx->rx_pos) && (SATL_TESIC_APB_INVALID==ctx->rx_buf_level)){//first rx for a frame, or after ack
        SATL_wait_rx_event(ctx);
        ctx->rx_pos=0;
        ctx->rx_buf_level=0;
    }else{
        PRINT_APB_STATE("skip wait for rx event");
        assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    }


    PRINT_APB_STATE("\towner is slave:");
    SATL_TESIC_APB_PRINT_CTX("");
    assert(ctx->rx_pos+len-ctx->rx_buf_level<=SATL_TESIC_APB_GET_CNT());//check buffer length requirement
    uint8_t* outwr = (uint8_t*)buf;
    while(ctx->rx_buf_level && len){
        outwr[0] = ctx->rx_buf;
        ctx->rx_buf=ctx->rx_buf>>8;
        outwr++;
        len--;
        ctx->rx_buf_level--;
    }
    SATL_TESIC_APB_PRINT_HEXUI(len);
    SATL_TESIC_APB_PRINT_CTX("");
    const unsigned int nwords = len / sizeof(uint32_t);
    const unsigned int base = ctx->rx_pos / sizeof(uint32_t);
    SATL_TESIC_APB_PRINT_HEXUI(nwords);
    SATL_TESIC_APB_PRINT_HEXUI(base);
    if(((uintptr_t)outwr) & 0x3){//unaligned destination
        SATL_TESIC_APB_PRINTF("unaligned destination");
        for(unsigned int i=0;i<nwords;i++){
            uint32_t w = SATL_TESIC_APB_GET_BUF(base+i);
            outwr[i*4+0] = w;
            outwr[i*4+1] = w>>8;
            outwr[i*4+2] = w>>16;
            outwr[i*4+3] = w>>24;
        }
    } else {
        SATL_TESIC_APB_PRINTF("aligned destination");
        uint32_t*aligned_buf = (uint32_t*)outwr;
        for(unsigned int i=0;i<nwords;i++){
            aligned_buf[i] = SATL_TESIC_APB_GET_BUF(base+i);
        }
    }
    ctx->rx_pos += 4*nwords;
    len -= 4*nwords;
    if(len){
        SATL_TESIC_APB_PRINT_HEXUI(len);
        SATL_TESIC_APB_PRINT_CTX("");
        outwr += 4*nwords;
        ctx->rx_buf = SATL_TESIC_APB_GET_BUF(base+nwords);
        ctx->rx_buf_level = 4-len;
        ctx->rx_pos += 4;
        SATL_TESIC_APB_PRINTF("memcpy");
        memcpy(outwr,&ctx->rx_buf,len);
        ctx->rx_buf = ctx->rx_buf >> (len*8);
    }
    SATL_TESIC_APB_PRINT_HEXUI(len);
    SATL_TESIC_APB_PRINT_CTX("");
    #ifdef SATL_TESIC_APB_SLAVE_VERBOSE_RX_DATA
      PRINT_BUF("rx dat:",buf,input_len);
    #endif
}

static void SATL_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    SATL_generic_rx(ctx,buf,len);
}

static void SATL_final_rx(SATL_driver_ctx_t *const ctx,void*buf,unsigned int len){
    SATL_generic_rx(ctx,buf,len);
}


static void SATL_tx_ack(SATL_driver_ctx_t *const ctx){
    PRINT_APB_STATE("SATL_tx_ack entry");
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    SATL_TESIC_APB_PRINT_CTX("");
    assert(0==ctx->rx_buf_level);
    ctx->rx_pos=SATL_TESIC_APB_INVALID;
    ctx->rx_buf_level=SATL_TESIC_APB_INVALID;
    SATL_TESIC_APB_SET_CNT(0);
    PRINT_APB_STATE("SATL_tx_ack exit (before giving buffer)");
    SATL_TESIC_APB_SET_CFG(SATL_SLAVE_IRQ | TESIC_APB_CFG_SLAVE_STS_MASK);//set status and give buffer ownership to master side
}

static void SATL_rx_ack(SATL_driver_ctx_t *const ctx){
    PRINT_APB_STATE("SATL_rx_ack ");
    SATL_wait_rx_event(ctx);
    assert(SATL_TESIC_APB_OWNER_IS_SLAVE());
    assert(0 == SATL_TESIC_APB_GET_CNT());
}

#endif //__SATL_TESIC_APB_H__
