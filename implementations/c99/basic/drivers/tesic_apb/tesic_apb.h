#ifndef __TESIC_APB_H__
#define __TESIC_APB_H__

#define TESIC_APB_CFG_ID 0x0AAB

#define TESIC_APB_CFG_OWNER_POS 16
#define TESIC_APB_CFG_OWNER_MASK (((uint32_t)1)<<TESIC_APB_CFG_OWNER_POS)

#define TESIC_APB_CFG_SLAVE_STS_POS 17
#define TESIC_APB_CFG_SLAVE_STS_MASK (((uint32_t)1)<<TESIC_APB_CFG_SLAVE_STS_POS)

#define TESIC_APB_CFG_MASTER_STS_POS 18
#define TESIC_APB_CFG_MASTER_STS_MASK (((uint32_t)1)<<TESIC_APB_CFG_MASTER_STS_POS)

#define TESIC_APB_CFG_SLAVE_ITEN_POS 19
#define TESIC_APB_CFG_SLAVE_ITEN_MASK (1<<TESIC_APB_CFG_SLAVE_ITEN_POS)

#define TESIC_APB_CFG_MASTER_ITEN_POS 20
#define TESIC_APB_CFG_MASTER_ITEN_MASK (1<<TESIC_APB_CFG_MASTER_ITEN_POS)

#define TESIC_APB_CFG_MASTER_REQ_LOCK_POS 21
#define TESIC_APB_CFG_MASTER_REQ_LOCK_MASK (((uint32_t)1)<<TESIC_APB_CFG_MASTER_REQ_LOCK_POS)

#define TESIC_APB_BUF_DWORDS 67
#define TESIC_APB_BUF_LEN (4*TESIC_APB_BUF_DWORDS)

typedef struct TESIC_APB_struct_t{
    volatile uint32_t CFG;
    volatile uint32_t CNT;
    volatile uint32_t BUF[TESIC_APB_BUF_DWORDS];
} TESIC_APB_t;

#ifdef TESIC_APB_EMU
#define TESIC_APB_GET_CFG(ctx) emu_rd_cfg(ctx)
#define TESIC_APB_SET_CFG(ctx,val) emu_wr_cfg(ctx,val)
#define TESIC_APB_GET_CNT(ctx) emu_rd_cnt(ctx)
#define TESIC_APB_SET_CNT(ctx,val) emu_wr_cnt(ctx,val)
#define TESIC_APB_GET_BUF(ctx,idx) emu_rd_buf(ctx,idx)
#define TESIC_APB_SET_BUF(ctx,idx,val) emu_wr_buf(ctx,idx,val)
#else
#define TESIC_APB_GET_CFG(ctx) (ctx->CFG)
#if defined(CPU_IS_TAM16)
#define TESIC_APB_SET_CFG(ctx,val) do{((volatile uint16_t*)&(ctx->CFG))[1]=val>>16;}while(0)
#else
#define TESIC_APB_SET_CFG(ctx,val) do{ctx->CFG=val;}while(0)
#endif
#define TESIC_APB_GET_CNT(ctx) (ctx->CNT)
#define TESIC_APB_SET_CNT(ctx,val) do{ctx->CNT=val;}while(0)
#define TESIC_APB_GET_BUF(ctx,idx) (ctx->BUF[idx])
#define TESIC_APB_SET_BUF(ctx,idx,val) do{ctx->BUF[idx]=val;}while(0)
#endif

#define TESIC_APB_OWNER_IS_MASTER(ctx) (0 == (TESIC_APB_GET_CFG(ctx) & TESIC_APB_CFG_OWNER_MASK))
#define TESIC_APB_OWNER_IS_SLAVE(ctx) (TESIC_APB_CFG_OWNER_MASK == (TESIC_APB_GET_CFG(ctx) & TESIC_APB_CFG_OWNER_MASK))

#endif //__TESIC_APB_H__
