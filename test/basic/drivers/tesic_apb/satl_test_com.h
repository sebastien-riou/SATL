#define TESIC_APB_EMU
#include "../../../../implementations/c99/basic/drivers/tesic_apb/tesic_apb.h"
void emu_init(TESIC_APB_t*ctx);
uint32_t emu_rd_cfg(TESIC_APB_t*ctx);
uint32_t emu_rd_cnt(TESIC_APB_t*ctx);
uint32_t emu_rd_buf(TESIC_APB_t*ctx, unsigned int idx);
void emu_wr_cfg(TESIC_APB_t*ctx,uint32_t val);
void emu_wr_cnt(TESIC_APB_t*ctx,uint32_t val);
void emu_wr_buf(TESIC_APB_t*ctx, unsigned int idx, uint32_t val);
TESIC_APB_t com_peripheral;
#ifdef SATL_TEST_SLAVE
#include "../../../../implementations/c99/basic/drivers/tesic_apb/satl_tesic_apb_slave.h"
#else
#include "../../../../implementations/c99/basic/drivers/tesic_apb/satl_tesic_apb_master.h"
#endif

#include "../../../../implementations/c99/basic/satl.h"
