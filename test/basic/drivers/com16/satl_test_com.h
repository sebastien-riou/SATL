#ifdef COM16_EMU
#include "../../../../implementations/basic/drivers/com16/com16.h"
void emu_init(COM16_t*ctx);
uint16_t emu_rd_ctrl(COM16_t*ctx);
uint16_t emu_rd_rx(COM16_t*ctx);
void emu_wr_tx(COM16_t*ctx,uint16_t val);
COM16_t com_peripheral;
#include "../../../../implementations/basic/drivers/com16/satl_com16.h"
#endif

#include "../../../../implementations/basic/satl.h"
