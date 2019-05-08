#ifndef __COM16_H__
#define __COM16_H__

#define COM16_CTRL_TXR_POS 0
#define COM16_CTRL_TXR_MASK (1<<COM16_CTRL_TXR_POS)

#define COM16_CTRL_RXR_POS 1
#define COM16_CTRL_RXR_MASK (1<<COM16_CTRL_RXR_POS)

typedef struct COM16_struct_t{
    volatile uint16_t CTRL;
    volatile uint16_t TX;
    volatile uint16_t RX;
} COM16_t;

#ifdef COM16_EMU
#define COM16_GET_CTRL(ctx) emu_rd_ctrl(ctx)
#define COM16_GET_RX(ctx) emu_rd_rx(ctx)
#define COM16_SET_TX(ctx,val) emu_wr_tx(ctx,val)
#else
#define COM16_GET_CTRL(ctx) (ctx->CTRL)
#define COM16_GET_RX(ctx) (ctx->RX)
#define COM16_SET_TX(ctx,val) do{ctx->TX=val;}while(0)
#endif

#define COM16_RX_IS_READY(ctx) (COM16_CTRL_RXR_MASK == (COM16_GET_CTRL(ctx) & COM16_CTRL_RXR_MASK))
#define COM16_TX_IS_READY(ctx) (COM16_CTRL_TXR_MASK == (COM16_GET_CTRL(ctx) & COM16_CTRL_TXR_MASK))

#endif //__COM16_H__
