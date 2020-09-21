//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
//#include <sys/types.h>
#include <stdint.h>
#include <time.h>

#include <assert.h>
int sockfd = 0;


#define SATL_TEST_SLAVE
#include "satl_test_com.h"

#define SATL_SOCKET_ONLY_CUSTOM_FUNCS
#include "../../../../implementations/c99/basic/drivers/socket/satl_socket.h"

SATL_ctx_t SATL_ctx;

uint32_t tx_chunk_size=0;
uint32_t rx_chunk_size=0;

void slave_rx_apdu(SATL_ctx_t*const ctx, SATL_capdu_header_t*hdr,uint32_t *lc, uint32_t *le, uint8_t*data){
    if(0==rx_chunk_size){
        //read the whole APDU in one go
        SATL_slave_rx_full(ctx, hdr,lc, le, data);
    }else{
        //simulate environment with limited buffering capability
        SATL_slave_rx_hdr(ctx, hdr,lc, le);
        uint32_t remaining = *lc;
        while(remaining>rx_chunk_size){
            SATL_slave_rx_dat(ctx, data, rx_chunk_size);
            data+=rx_chunk_size;
            remaining-=rx_chunk_size;
        }
        SATL_slave_rx_dat(ctx, data, remaining);
    }
}

void slave_tx_apdu(SATL_ctx_t*const ctx, uint32_t le, const uint8_t *const dat,const SATL_rapdu_sw_t*const sw){
    const uint8_t * data = (const uint8_t *) dat;
    if(0==tx_chunk_size){
        //write the whole APDU in one go
        SATL_slave_tx_full(ctx, le, data, sw);
    }else{
        //simulate environment with limited buffering capability
        SATL_slave_tx_le(ctx, le);
        uint32_t remaining = le;
        while(remaining>tx_chunk_size){
            SATL_slave_tx_dat(ctx, data, tx_chunk_size);
            data+=tx_chunk_size;
            remaining-=tx_chunk_size;
        }
        SATL_slave_tx_dat(ctx, data, remaining);
        SATL_slave_tx_sw(ctx, sw);
    }
}
int main(int argc, char *argv[]){
    uint32_t port = 5000;
    if(argc>1) port = strtol(argv[1], 0, 0);
    if(argc>2) tx_chunk_size = strtol(argv[2], 0, 0);
    if(argc>3) rx_chunk_size = strtol(argv[3], 0, 0);

    uint8_t refdatle[(1<<16)+1];
    uint8_t refdat[(1<<16)+1];
    for(unsigned int i=0;i<(1<<16)+1;i++){
        refdat[i] = i;
        refdatle[i] = i ^ 0xFF;
    }

    SATL_socket_params_t params;
    params.address=0;
    params.port = port;

    init_socket_api();
    #ifndef SATL_TEST_SOCKET
        sockfd = SATL_socket_slave_init(&params);
        emu_init(&com_peripheral);
    #else
        memcpy(&com_peripheral,&params,sizeof(params));
    #endif

    uint32_t mblen = SATL_slave_init(&SATL_ctx,&com_peripheral);
    if(SATL_ACK){
        assert(SATL_SBLEN==mblen);
    }else{
        assert(0==mblen);
    }

    while(1){
        SATL_capdu_header_t hdr;
        uint32_t lc;
        uint32_t le;
        uint8_t data[SATL_DATA_SIZE_LIMIT];
        SATL_rapdu_sw_t sw;
        sw.SW1 = 0x90;
        sw.SW2 = 0x00;
        slave_rx_apdu(&SATL_ctx, &hdr,&lc, &le, data);
        /*printf("C-APDU %02X %02X %02X %02X",hdr.CLA,hdr.INS,hdr.P1,hdr.P2);
        if(lc) {
            printf(" - LC=%5u DATA: ",lc);
        }
        if(le){
            printf(" - LE=%5u",le);
        }
        printf("\n");*/

        if(0xFE==hdr.CLA){
            //in this case we just answer status word
            sw.SW1 = 0x69;
            le=0;
        } else {
            if(le){
                uint32_t rle = ((uint32_t)hdr.INS)<<16;
                rle += ((uint32_t)hdr.P1)<<8;
                rle += hdr.P2;
                assert(le>=rle);
                uint32_t l = lc<rle ? lc : rle;
                assert(0==memcmp(refdat,data,l));
                memcpy(data+lc,refdatle,rle-l);
                le = rle;
            }
        }
        /*printf("R-APDU %02X %02X",sw.SW1,sw.SW2);
        if(le){
            printf(" - LE=%5u DATA: ",le);
        }
        printf("\n");*/
        slave_tx_apdu(&SATL_ctx, le, data, &sw);
        if(0xFF==hdr.CLA){
            break;
        }
    }
    sleep(1);
    //printf("closing socket\n");
    close(sockfd);
    //printf("socket closed\n");
    printf("slave done\n");
    
}
