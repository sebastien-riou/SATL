# Basic C implementation
This is a basic implementation that covers simple cases in which a blocking
interface is acceptable and the characteristics of both the master and the slave
are known in advance and can be hard coded.

# High level API
Master and slave communication is covered by 4 functions, their signature and
usage is independent of the underlying communication interface.

## Sending a C-APDU on a master
A single function call send the whole command APDU:

    void SATL_master_tx(SATL_capdu_header_t*hdr,uint32_t lc, uint32_t le, uint8_t*data);

## Receiving a C-APDU on a slave
A single function call receive the whole command APDU:

    void SATL_slave_rx(SATL_capdu_header_t*hdr,uint32_t *lc, uint32_t *le, uint8_t*data);

## Sending a R-APDU on a slave
A single function call send the whole response APDU:

    void SATL_slave_tx(uint32_t le, uint8_t*data,SATL_rapdu_sw_t*sw);

## Receiving a R-APDU on a master
A single function call receive the whole response APDU:

    void SATL_master_rx(uint32_t *le, uint8_t*data,SATL_rapdu_sw_t*sw);

# Piecemeal API
The high level functions handling full APDUs may not be suitable for low cost
embedded systems.
In this case the "Piecemeal" API allows to process data piece by piece.

## Sending a C-APDU on a master
First **SATL_master_tx_hdr** shall be called.

Optionally call **SATL_master_tx_dat** one or several times.
The sum of **len** for all calls to **SATL_master_tx_dat** shall be equal to the **lc** passed to **SATL_master_tx_hdr**.

    void SATL_master_tx_hdr(SATL_ctx_t*const ctx, const SATL_capdu_header_t*const hdr, uint32_t lc, uint32_t le);
    void SATL_master_tx_dat(SATL_ctx_t*const ctx, const void *const data, uint32_t len);

## Receiving a C-APDU on a slave
First **SATL_slave_rx_hdr** shall be called.

Optionally call **SATL_slave_rx_dat** one or several times.
The sum of **len** for all calls to **SATL_slave_rx_dat** shall be equal to the **lc** passed to **SATL_slave_rx_hdr**.

    void SATL_slave_rx_hdr(SATL_ctx_t*const ctx, SATL_capdu_header_t*hdr,uint32_t *lc, uint32_t *le);
    void SATL_slave_rx_dat(SATL_ctx_t*const ctx, void*const data, uint32_t len);

## Sending a R-APDU on a slave
First **SATL_slave_tx_le** shall be called.

Optionally call **SATL_slave_tx_dat** one or several times.
The sum of **len** for all calls to **SATL_slave_tx_dat** shall be equal to the **le** passed to **SATL_slave_tx_le**.

Finally **SATL_slave_tx_sw** shall be called.

    void SATL_slave_tx_le (SATL_ctx_t*const ctx, uint32_t le);
    void SATL_slave_tx_dat(SATL_ctx_t*const ctx, const void *const data, uint32_t len);
    void SATL_slave_tx_sw (SATL_ctx_t*const ctx, const SATL_rapdu_sw_t*const sw);


## Receiving a R-APDU on a master
First **SATL_master_rx_le** shall be called.

Optionally call **SATL_master_rx_dat** one or several times.
The sum of **len** for all calls to **SATL_master_rx_dat** shall be equal to the **le** returned by **SATL_master_rx_le**.

Finally **SATL_master_rx_sw** shall be called.

    void SATL_master_rx_le (SATL_ctx_t*const ctx, uint32_t*const le);
    void SATL_master_rx_dat(SATL_ctx_t*const ctx, void*const dat, uint32_t len);
    void SATL_master_rx_sw (SATL_ctx_t*const ctx, SATL_rapdu_sw_t*const sw);

# How to integrate in a project ?
Define common integer types uint8_t, uint16_t, uint32. Typically this is done
by including "stdint.h". If it is not available on your platform you can still
define them using a typedefs.

Include the communication interface header file.

Include "satl.h".

    #include "stdint.h"
    #include "satl_com16.h"
    #include "satl.h"

NOTE: SATL is a "header" only library, to use it with different
interfaces within a single project, you just need to do those steps in two
different c files.

# How to create a new communication interface header file
Define low level functions to your hardware communication interface in a dedicated
header file. This section details what this file shall contain.

NOTES:
* The guidelines and constraints discussed here apply only when using the basic
C implementation "satl.h". Other implementation may have very different approaches.
* Only your functions access the hardware communication interface.

## Define interface properties
Define the control flow and buffering capabilities of your hardware.

If the control flow is managed by hardware then define SATL_ACK to 0.

If software control flow is needed, define SATL_ACK to 1. In this case you also
need to define the hardware buffer size for each sides, in bytes.

### Example 1: Interface without control flow
In this case SATL_MBLEN and SATL_SBLEN shall be defined.

    #define SATL_ACK 1
    #define SATL_MBLEN 268
    #define SATL_SBLEN 268

### Example 2: Interface with control flow
In this case the defines for buffer size are not required.

    #define SATL_ACK 0

## Define SFR granularity
The macro SATL_SFR_GRANULARITY defines the minimum access size supported by the
hardware peripheral. It is usually 1,2 or 4 bytes.
Typical value for APB peripherals is 4.

    #define SATL_SFR_GRANULARITY 4

## Define Master/Slave support
If your implementation support slave functions, define **SATL_SUPPORT_SLAVE**.

If your implementation support master functions, define **SATL_SUPPORT_MASTER**.

NOTES:
* An header file can support one of them or both.
* You can also support slave functions in one header file and master functions
in a separate header file.


    #define SATL_SUPPORT_SLAVE
    #define SATL_SUPPORT_MASTER

## Initialization functions
There is two initialization function entry point to distinguish master and slave
operations. They both take a pointer to an arbitrary structure to pass the
hardware specific parameters.

NOTE: You do not need to implement **SATL_slave_init_driver** if your header file
does not defines **SATL_SUPPORT_SLAVE**. Same remark applies to **SATL_master_init_driver** and **SATL_SUPPORT_MASTER**.

    //Initialization
    static uint32_t SATL_slave_init_driver (SATL_driver_ctx_t *const ctx, void *hw){}
    static uint32_t SATL_master_init_driver(SATL_driver_ctx_t *const ctx, void *hw){}

## TX functions
**SATL_tx** and **SATL_final_tx** can be blocking or not, on half duplex links, **SATL_final_tx** is required to
change the communication direction to **SATL_rx** before it exit.
**SATL_tx** function is called only with **len** being a multiple of **SATL_SFR_GRANULARITY**.
Note however that **buf** may not be aligned.

    //TX functions
    static void     SATL_switch_to_tx      (SATL_driver_ctx_t *const ctx){}
    static void     SATL_tx                (SATL_driver_ctx_t *const ctx, const void *const buf, unsigned int len){}
    static void     SATL_final_tx          (SATL_driver_ctx_t *const ctx, const void *const buf, unsigned int len){}

## RX functions
**SATL_rx** and **SATL_final_rx** are required to be blocking.

    //RX functions
    static void     SATL_rx                (SATL_driver_ctx_t *const ctx, void *buf, unsigned int len){}
    static void     SATL_final_rx          (SATL_driver_ctx_t *const ctx, void *buf, unsigned int len){}
    static uint32_t SATL_get_rx_level      (const SATL_driver_ctx_t *ctx){}

## ACK signal functions
If there is hardware flow control, simply leave those functions empty.

    //ACK signal functions
    static void     SATL_tx_ack            (SATL_driver_ctx_t *const ctx){}
    static void     SATL_rx_ack            (SATL_driver_ctx_t *const ctx){}
