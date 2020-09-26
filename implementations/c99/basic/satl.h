//assume little endian platform
//expect stdint.h to be included in the C file, or uint8_t and uint32_t types to be defined somehow

//Functions you shall call:
//1. Initialization
//  SATL_master_init OR SATL_slave_init
//
//2.a APDU exchanges (full size):
//  SATL_master_tx_full OR SATL_slave_tx_full
//  SATL_master_rx_full OR SATL_slave_rx_full
//
//2.b APDU exchanges (arbitrary chunk size API):
//  Master side:
//      SATL_master_tx_hdr
//      SATL_master_tx_dat
//      SATL_master_rx_le
//      SATL_master_rx_dat
//      SATL_master_rx_sw
//
//  Slave side:
//      SATL_slave_rx_hdr
//      SATL_slave_rx_dat
//      SATL_slave_tx_le
//      SATL_slave_tx_dat
//      SATL_slave_tx_sw


#ifndef __SATL_H__TYPES
#define __SATL_H__TYPES

typedef struct SATL_capdu_header_struct_t {
    uint8_t CLA;
    uint8_t INS;
    uint8_t P1;
    uint8_t P2;
} SATL_capdu_header_t;

typedef struct SATL_rapdu_sw_struct_t {
    uint8_t SW1;
    uint8_t SW2;
} SATL_rapdu_sw_t;

typedef struct SATL_ctx_struct_t SATL_ctx_t;

#endif //__SATL_H__TYPES

#ifndef SATL_TYPES_ONLY

#ifndef __SATL_H__
#define __SATL_H__

#ifndef SATL_ACK
#error "SATL_ACK not defined. Set to 1 if control flow must be handled by software, otherwise 0"
#endif

#if SATL_ACK
    #ifndef SATL_MBLEN
    #error "SATL_MBLEN not defined. This is the maximum number of bytes that can be received by the master in a row."
    #endif

    #ifndef SATL_SBLEN
    #error "SATL_SBLEN not defined. This is the maximum number of bytes that can be received by the slave in a row."
    #endif
#else
    #define SATL_MBLEN (((uint32_t)1)<<31)
    #define SATL_SBLEN (((uint32_t)1)<<31)
#endif

#if (SATL_SFR_GRANULARITY!=1) && (SATL_SFR_GRANULARITY!=2) && (SATL_SFR_GRANULARITY!=4)
#error "this implementation supports only 1,2 and 4 for SATL_SFR_GRANULARITY"
#endif

#if defined(SATL_VERBOSE)
    #ifndef SATL_PRINTF
        #define SATL_PRINTF(str,...) printf(str __VA_OPT__(,) __VA_ARGS__)
    #endif
#else
    #undef SATL_PRINTF
    #define SATL_PRINTF(str,...)
#endif

typedef struct SATL_ctx_struct_t {
    SATL_driver_ctx_t driver_ctx;//keep this as first member to avoid constant addition each time we call driver functions
    uint32_t fl;//remaining bytes in this frame
    uint32_t remaining;//remaining bytes in this exchange
    uint32_t buf_level;//remaining bytes in the SFR access level buffer
    uint32_t buf_len;//negotiated max exchange size
    uint32_t flags;
    uint8_t buf[SATL_SFR_GRANULARITY];//SFR access level buffer
} SATL_ctx_t;

#define SATL_LE_SENT 1

#define SATL_LEN_LEN (sizeof(uint32_t))
#define SATL_DATA_SIZE_LIMIT  (((uint32_t)1)<<16)
#define SATL_FRAME_SIZE_LIMIT (SATL_DATA_SIZE_LIMIT+2*SATL_LEN_LEN+sizeof(SATL_capdu_header_t))
#define DCTX (&(ctx->driver_ctx))

static void SATL_safe_tx(SATL_ctx_t*const ctx,const void*const buf,uint32_t len){
    SATL_PRINTF("SATL_safe_tx len=%u\n",len);
    const uint8_t* buf8 = (const uint8_t*)buf;
    if(ctx->buf_level){
        if(ctx->buf_level+len > SATL_SFR_GRANULARITY){
            unsigned int tx_len = SATL_SFR_GRANULARITY-ctx->buf_level;
            memcpy(ctx->buf+ctx->buf_level,buf8,tx_len);
            SATL_tx(DCTX,ctx->buf,SATL_SFR_GRANULARITY);
            len-=tx_len;
            buf8+=tx_len;
            ctx->buf_level=0;
        }
    }
    if(len>=SATL_SFR_GRANULARITY){
        unsigned int to_buf = len % SATL_SFR_GRANULARITY;
        uint32_t tx_len = len - to_buf;
        SATL_tx(DCTX,buf8,tx_len);
        len-=tx_len;
        buf8+=tx_len;
    }
    memcpy(ctx->buf+ctx->buf_level,buf8,len);
    ctx->buf_level+=len;
    if(ctx->buf_level==SATL_SFR_GRANULARITY){
        SATL_tx(DCTX,ctx->buf,SATL_SFR_GRANULARITY);
        ctx->buf_level=0;
    }
    SATL_PRINTF("safe tx buf level=%u\n",ctx->buf_level);
}

static void SATL_safe_final_tx(SATL_ctx_t*const ctx,const void*const buf,uint32_t len){
    SATL_PRINTF("SATL_safe_final_tx len=%u\n",len);
    const uint8_t* buf8 = (const uint8_t*)buf;
    //printf("ctx->buf_level=%u\n",ctx->buf_level);
    if(ctx->buf_level){
        if(ctx->buf_level+len > SATL_SFR_GRANULARITY){
            unsigned int tx_len = SATL_SFR_GRANULARITY-ctx->buf_level;
            memcpy(ctx->buf+ctx->buf_level,buf8,tx_len);
            SATL_tx(DCTX,ctx->buf,SATL_SFR_GRANULARITY);
            len-=tx_len;
            buf8+=tx_len;
            ctx->buf_level=0;
        }
    }
    if(len>SATL_SFR_GRANULARITY){
        unsigned int to_buf = len % SATL_SFR_GRANULARITY;
        if(0==to_buf) to_buf=SATL_SFR_GRANULARITY;//keep some data for the call to SATL_final_tx
        uint32_t tx_len = len - to_buf;
        SATL_tx(DCTX,buf8,tx_len);
        len-=tx_len;
        buf8+=tx_len;
    }
    memcpy(ctx->buf+ctx->buf_level,buf8,len);
    SATL_final_tx(DCTX,ctx->buf,ctx->buf_level+len);
    ctx->buf_level=0;
    //printf("safe final tx buf level=%u\n",ctx->buf_level);
}

static void SATL_rx_dat(SATL_ctx_t*const ctx, void*const dat, uint32_t data_len){
    SATL_PRINTF("SATL_rx_dat len=%u\n",data_len);
    SATL_PRINTF("ctx->remaining=%u,ctx->fl=%u\n",ctx->remaining,ctx->fl);
    uint8_t*data = (uint8_t*)dat;
    assert(ctx->remaining>=data_len);
    uint32_t remaining = data_len;
    if(remaining){
        uint32_t rx_level = SATL_get_rx_level(DCTX);
        SATL_PRINTF("rx_level=%u, remaining=%u\n",rx_level,remaining);
        if(rx_level){//get left over from previous read
            unsigned int l = remaining < rx_level ? remaining : rx_level;
            SATL_rx(DCTX,data,l);
            remaining -= l;
            data += l;
        }
        SATL_PRINTF("remaining=%u\n",remaining);
        while(remaining){//need to fill driver buffer
            if((data_len!=remaining)||(0!=ctx->fl)){
                //send ack except for the very first rx in the frame
                SATL_tx_ack(DCTX);
            }
            unsigned int l = remaining < ctx->buf_len ? remaining : ctx->buf_len;
            SATL_rx(DCTX,data,l);
            remaining -= l;
            data += l;
        }
        ctx->remaining -= data_len;
    }
    SATL_PRINTF("SATL_rx_dat return\n");
}

static void SATL_tx_dat(SATL_ctx_t*const ctx, const void *const data, uint32_t len){
    SATL_PRINTF("SATL_tx_dat len=%u\n",len);
    if(0==len) return;
    assert(ctx->fl>=len);
    const uint8_t *d=(const uint8_t *)data;
    //printf("ctx->remaining=%u\n",ctx->remaining);
    if(len>ctx->remaining){
        SATL_safe_tx(ctx,d,ctx->remaining);
        d += ctx->remaining;
        len-=ctx->remaining;
        ctx->fl-=ctx->remaining;
        SATL_rx_ack(DCTX);
        ctx->remaining=ctx->buf_len;
        if(len){
            uint32_t chunks = len/ctx->buf_len;
            if(0==(len%ctx->buf_len)) chunks--;
            //printf("chunks=%u\n",chunks);
            for(unsigned int i=0;i<chunks;i++){
                SATL_safe_tx(ctx,d,ctx->buf_len);
                d += ctx->buf_len;
                len -= ctx->buf_len;
                ctx->fl-= ctx->buf_len;
                SATL_rx_ack(DCTX);
            }
        }
    }
    //printf("ctx->fl=%u, len=%u\n",ctx->fl,len);
    if(ctx->fl>len){
        SATL_safe_tx(ctx,d,len);
    }else{
        SATL_safe_final_tx(ctx,d,len);
    }
    ctx->fl-=len;
    ctx->remaining-=len;
    if((0==ctx->remaining) && (0!=ctx->fl)) {
        SATL_rx_ack(DCTX);
        ctx->remaining = ctx->buf_len;
    }
    //printf("ctx->remaining=%u\n",ctx->remaining);
}

#ifdef SATL_SUPPORT_MASTER
static uint32_t SATL_master_init(SATL_ctx_t*const ctx, void *const hw){
    ctx->fl = 0;
    ctx->remaining = 0;
    ctx->buf_level=0;
    ctx->flags=0;
    uint32_t sblen = SATL_master_init_driver(DCTX,hw);
    #if SATL_ACK
        ctx->buf_len = sblen<SATL_MBLEN ? sblen : SATL_MBLEN;
        assert(0==(ctx->buf_len)%SATL_SFR_GRANULARITY);
        return ctx->buf_len;
    #else
        ctx->buf_len = SATL_FRAME_SIZE_LIMIT;
        return 0;
    #endif
}

static void SATL_master_tx_dat(SATL_ctx_t*const ctx, const void *const data, uint32_t len){
    SATL_tx_dat(ctx, data, len);
}

static uint32_t SATL_master_tx_lengths(SATL_ctx_t*const ctx, uint32_t lc, uint32_t le){
    SATL_PRINTF("SATL_master_tx_lengths lc=%u, le=%u\n",lc,le);
    uint32_t flle[2];
    flle[0] = lc + sizeof(SATL_capdu_header_t) + 2*SATL_LEN_LEN;
    flle[1] = le;
    SATL_switch_to_tx(DCTX);
    ctx->buf_level=0;
    ctx->fl = flle[0];
    ctx->remaining = ctx->buf_len;
    SATL_master_tx_dat(ctx,flle,sizeof(flle));
    return flle[0];
}

static void SATL_master_tx_hdr(SATL_ctx_t*const ctx, const SATL_capdu_header_t*const hdr,uint32_t lc, uint32_t le){
    SATL_master_tx_lengths(ctx,lc,le);
    SATL_master_tx_dat(ctx,hdr,sizeof(SATL_capdu_header_t));
}

static void SATL_master_tx_full(SATL_ctx_t*const ctx, const SATL_capdu_header_t*const hdr,uint32_t lc, uint32_t le, const void*const dat){
    SATL_PRINTF("SATL_master_tx_full\n");
    SATL_master_tx_hdr(ctx, hdr, lc, le);
    SATL_master_tx_dat(ctx, dat, lc);
}

static void SATL_master_rx_le(SATL_ctx_t*const ctx, uint32_t*const le){
    SATL_PRINTF("SATL_master_rx_le ----------------------- \n");
    const uint32_t min_fl = (sizeof(SATL_capdu_header_t)+2*SATL_LEN_LEN);
    ctx->remaining = min_fl;
    ctx->fl=0;
    SATL_rx_dat(ctx,&(ctx->fl),SATL_LEN_LEN);
    //SATL_rx(DCTX,&(ctx->fl),SATL_LEN_LEN);
    ctx->remaining = ctx->fl - SATL_LEN_LEN - sizeof(SATL_rapdu_sw_t);
    *le=ctx->remaining;
    //printf("rx fl = %u, le=%u\n",ctx->fl,*le);
    assert(ctx->fl>=(SATL_LEN_LEN + sizeof(SATL_rapdu_sw_t)));
}

static void SATL_master_rx_dat(SATL_ctx_t*const ctx, void*const dat, uint32_t len){
    SATL_PRINTF("SATL_master_rx_dat len=%u\n",len);
    SATL_rx_dat(ctx,dat,len);
}
static void SATL_master_rx_sw(SATL_ctx_t*const ctx, SATL_rapdu_sw_t*const sw){
    SATL_PRINTF("SATL_master_rx_sw\n");
    assert(0==ctx->remaining);
    SATL_PRINTF("ctx->fl=%u, ctx->buf_len=%u, ctx->fl mod ctx->buf_len = %u\n",ctx->fl,ctx->buf_len,ctx->fl % ctx->buf_len);
    switch(ctx->fl % ctx->buf_len){
        case 0:
            if(ctx->buf_len!=1) {SATL_final_rx(DCTX,sw,sizeof(SATL_rapdu_sw_t)); break;}  
            SATL_tx_ack(DCTX);//fall through
        case 1:  SATL_rx(DCTX,sw,1);SATL_tx_ack(DCTX);SATL_final_rx(DCTX,&(sw->SW2),1);break;
        case 2:  SATL_tx_ack(DCTX);SATL_final_rx(DCTX,sw,sizeof(SATL_rapdu_sw_t));break;
        default: SATL_final_rx(DCTX,sw,sizeof(SATL_rapdu_sw_t));
    }
    ctx->fl = 0;
    SATL_PRINTF("SATL_master_rx_sw done -------------------------\n");
}
static void SATL_master_rx_full(SATL_ctx_t*const ctx, uint32_t *const le, void*const data,SATL_rapdu_sw_t*const sw){
    SATL_PRINTF("SATL_master_rx_full\n");
    SATL_master_rx_le(ctx,le);
    //printf("le=%u\n",*le);
    SATL_master_rx_dat(ctx,data,*le);
    SATL_master_rx_sw(ctx,sw);
}
#endif //SATL_SUPPORT_MASTER

#ifdef SATL_SUPPORT_SLAVE
static uint32_t SATL_slave_init(SATL_ctx_t*const ctx, void *const hw){
    ctx->fl = 0;
    ctx->remaining = 0;
    ctx->buf_level=0;
    ctx->flags=0;
    uint32_t mblen=SATL_slave_init_driver(DCTX,hw);
    #if SATL_ACK
        ctx->buf_len = mblen<SATL_SBLEN ? mblen : SATL_SBLEN;
        assert(0==(ctx->buf_len)%SATL_SFR_GRANULARITY);
        return ctx->buf_len;
    #else
        (void)mblen;//remove unused warning
        ctx->buf_len = SATL_FRAME_SIZE_LIMIT;
        return 0;
    #endif
}

static void SATL_slave_rx_dat(SATL_ctx_t*const ctx, void*const data, uint32_t len){
    SATL_PRINTF("SATL_slave_rx_dat %u\n",len);
    SATL_rx_dat(ctx,data,len);
}

static void SATL_slave_rx_hdr(SATL_ctx_t*const ctx, SATL_capdu_header_t*hdr,uint32_t *lc, uint32_t *le){
    SATL_PRINTF("SATL_slave_rx_hdr entry\n");
    const uint32_t min_fl = (sizeof(SATL_capdu_header_t)+2*SATL_LEN_LEN);
    ctx->remaining = min_fl;
    ctx->fl=0;
    SATL_rx_dat(ctx,&(ctx->fl),SATL_LEN_LEN);
    SATL_PRINTF("ctx->fl=%u\n",ctx->fl);
    ctx->remaining = ctx->fl - SATL_LEN_LEN;
    *lc = ctx->fl-sizeof(SATL_capdu_header_t)-2*SATL_LEN_LEN;
    assert(ctx->fl>=min_fl);
    SATL_slave_rx_dat(ctx,le,SATL_LEN_LEN);
    SATL_slave_rx_dat(ctx,hdr,sizeof(SATL_capdu_header_t));
    SATL_PRINTF("SATL_slave_rx_hdr exit\n");
}

static void SATL_slave_rx_full(SATL_ctx_t*const ctx, SATL_capdu_header_t*hdr,uint32_t *lc, uint32_t *le, uint8_t*data){
    SATL_slave_rx_hdr(ctx, hdr,lc,le);
    SATL_slave_rx_dat(ctx, data, *lc);
}

static void SATL_slave_tx_le(SATL_ctx_t*const ctx, uint32_t le){
    SATL_PRINTF("SATL_slave_tx_le le=%u\n",le);
    SATL_switch_to_tx(DCTX);
    uint32_t fl = SATL_LEN_LEN+le+sizeof(SATL_rapdu_sw_t);
    ctx->remaining = ctx->buf_len;;
    SATL_tx_dat(ctx,&fl,SATL_LEN_LEN);
    //SATL_safe_tx(ctx,&fl,SATL_LEN_LEN);
    //ctx->remaining = SATL_MBLEN - SATL_LEN_LEN;
    ctx->fl = fl - SATL_LEN_LEN;
    //if(le && (0==ctx->remaining)) {
    //    SATL_rx_ack(DCTX);
    //    ctx->remaining = SATL_MBLEN;
    //}
    ctx->flags|=SATL_LE_SENT;
}

static void SATL_slave_tx_dat(SATL_ctx_t*const ctx, const void *const data, uint32_t len){
    SATL_tx_dat(ctx, data, len);
}

static void SATL_slave_tx_sw(SATL_ctx_t*const ctx, const SATL_rapdu_sw_t*const sw){
    SATL_PRINTF("SATL_slave_tx_sw\n");
    //printf("fl mod SATL_MBLEN = %u\n",ctx->fl%SATL_MBLEN);
    assert(2==sizeof(SATL_rapdu_sw_t));//if this change the code below needs update
    //printf("ctx->remaining=%u\n",ctx->remaining);
    //uint32_t rx_level = SATL_get_rx_level(DCTX);
    //SATL_PRINTF("rx_level=%u\n",rx_level);
    //assert(0==rx_level);
    if(0==(ctx->flags & SATL_LE_SENT)) SATL_slave_tx_le(ctx,0);
    if(ctx->remaining==1){
        SATL_safe_tx(ctx,&(sw->SW1),1);
        SATL_rx_ack(DCTX);
        SATL_safe_final_tx(ctx,&(sw->SW2),1);
    }else{
        SATL_safe_final_tx(ctx,sw,sizeof(SATL_rapdu_sw_t));
    }
    ctx->remaining=0;
    ctx->flags&=~SATL_LE_SENT;
}

static void SATL_slave_tx_full(SATL_ctx_t*const ctx, uint32_t le, const void *const data,const SATL_rapdu_sw_t*const sw){
    SATL_slave_tx_le(ctx, le);
    SATL_slave_tx_dat(ctx, data, le);
    SATL_slave_tx_sw(ctx, sw);
}

#endif //SATL_SUPPORT_SLAVE

#endif //__SATL_H__

#endif //SATL_TYPES_ONLY
