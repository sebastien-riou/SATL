#include "../../../../implementations/c99/basic/drivers/serial/satl_serial.h"
void emu_init(SATL_driver_ctx_t*ctx);
void emu_tx(uint8_t val);
uint8_t emu_rx();
SATL_driver_ctx_t com_peripheral={
    emu_tx,
    emu_rx
};

#include "../../../../implementations/c99/basic/satl.h"
