#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <stdint.h>
#include "prand32.h"

int sockfd = 0;

//#define SATL_VERBOSE
//#define SATL_TESIC_APB_MASTER_VERBOSE

#define SATL_TEST_MASTER
#include "satl_test_com.h"

#define SATL_SOCKET_ONLY_CUSTOM_FUNCS
#include "../../../../implementations/c99/basic/drivers/socket/satl_socket.h"

uint8_t refdat[(1<<16)+1];
uint8_t refdatle[(1<<16)+1];

SATL_ctx_t SATL_ctx;

uint32_t tx_chunk_size=0;
uint32_t rx_chunk_size=0;

void master_tx_apdu(SATL_ctx_t*const ctx, SATL_capdu_header_t*hdr,uint32_t lc, uint32_t le, uint8_t*data){
    if(0==tx_chunk_size){
        //read the whole APDU in one go
        SATL_master_tx_full(ctx, hdr,lc, le, data);
    }else{
        //simulate environment with limited buffering capability
        SATL_master_tx_hdr(ctx, hdr,lc, le);
        uint32_t remaining = lc;
        while(remaining>tx_chunk_size){
            SATL_master_tx_dat(ctx, data, tx_chunk_size);
            data+=tx_chunk_size;
            remaining-=tx_chunk_size;
        }
        SATL_master_tx_dat(ctx, data, remaining);
    }
}

void master_rx_apdu(SATL_ctx_t*const ctx, uint32_t *le, void *const dat,SATL_rapdu_sw_t* const sw){
    uint8_t * data = (uint8_t *const) dat;
    if(0==rx_chunk_size){
        //process the whole APDU in one go
        SATL_master_rx_full(ctx, le, data, sw);
    }else if(0xFFFFFFFF==rx_chunk_size){
        //process the whole APDU in one go
        //SATL_master_rx_full(ctx, le, data, sw);
        //split into several random sized reads
        SATL_master_rx_le(ctx, le);
        uint32_t remaining = *le;
        //printf("master rx read_size: ");
        while(remaining){
            uint32_t read_size = prand32_in_range(0,remaining);
            //printf("%4u ",read_size);
            SATL_master_rx_dat(ctx, data, read_size);
            data+=read_size;
            remaining-=read_size;
        }
        //printf("master rx done\n");
        SATL_master_rx_sw(ctx, sw);
    }else{
        //simulate environment with limited buffering capability
        SATL_master_rx_le(ctx, le);
        uint32_t remaining = *le;
        while(remaining>rx_chunk_size){
            SATL_master_rx_dat(ctx, data, rx_chunk_size);
            data+=rx_chunk_size;
            remaining-=rx_chunk_size;
        }
        SATL_master_rx_dat(ctx, data, remaining);
        SATL_master_rx_sw(ctx, sw);
    }
}


void test_case(uint32_t lc, uint32_t le, uint32_t rle){
    //printf("lc=%u, le=%u, rle=%u\n",lc,le,rle);
    //printf("refdatle[0]=%02x\n",refdatle[0]);

    assert(lc<=1<<16);
    assert(le<=1<<16);
    assert(rle<=le);
    uint8_t buf[(1<<16)+1];
    memcpy(buf,refdat,lc);
    SATL_capdu_header_t hdr;
    hdr.CLA=0x01;
    hdr.INS=rle>>16;
    hdr.P1 =rle>>8;
    hdr.P2 =rle;
    /*for(unsigned int i=0;i<lc;i++){
        printf("%02X ",buf[i]);
    }
    printf("\n");
    printf("refdatle[0]=%02x\n",refdatle[0]);*/
    master_tx_apdu(&SATL_ctx, &hdr,lc,le,buf);
    //printf("refdatle[0]=%02x\n",refdatle[0]);
    SATL_rapdu_sw_t sw;
    uint32_t actual_le=le;
    uint8_t buf2[(1<<16)+1];
    master_rx_apdu(&SATL_ctx, &actual_le,buf2,&sw);
    assert(rle==actual_le);
    assert(sw.SW1==0x90);
    assert(sw.SW2==0x00);
    uint32_t l = lc<rle ? lc : rle;
    /*printf("l=%u, rle=%u\n",l,rle);
    for(unsigned int i=0;i<actual_le;i++){
        printf("%02X ",buf2[i]);
    }
    printf("\n");*/
    assert(0==memcmp(buf,buf2,l));
    assert(0==memcmp(refdat,buf2,l));
    //printf("refdatle[0]=%02x\n",refdatle[0]);
    assert(0==memcmp(refdatle,buf2+l,rle-l));
}

#define NUM_ELEMS(a) (sizeof(a)/sizeof 0[a])
int main(int argc, char *argv[]){

    //printf("sizeof(SATL_ctx)=%u\n",sizeof(SATL_ctx));

    unsigned int long_test=0;
    uint32_t port = 5000;
    if(argc>1) port = strtol(argv[1], 0, 0);
    if(argc>2) tx_chunk_size = strtol(argv[2], 0, 0);
    if(argc>3) rx_chunk_size = strtol(argv[3], 0, 0);
    if(argc>4) long_test = strtol(argv[4], 0, 0);

    for(unsigned int i=0;i<(1<<16)+1;i++){
        refdat[i] = i;
        refdatle[i] = i ^ 0xFF;
    }
    //printf("refdatle[0]=%02x\n",refdatle[0]);
    SATL_socket_params_t params;
    params.address="127.0.0.1";
    params.port = port;

    init_socket_api();
    #ifndef SATL_TEST_SOCKET
        sockfd = SATL_socket_master_init(&params);
        emu_init(&com_peripheral);
    #else
        memcpy(&com_peripheral,&params,sizeof(params));
    #endif
    uint32_t sblen = SATL_master_init(&SATL_ctx,&com_peripheral);
    printf("Slave buffer size = %u\n",sblen);
    if(SATL_ACK){
        assert(SATL_MBLEN==sblen);
    }else{
        assert(0==sblen);
    }

    {
        SATL_capdu_header_t hdr;
        hdr.CLA=0x01;
        hdr.INS=0x02;
        hdr.P1 =0x03;
        hdr.P2 =0x04;
        SATL_master_tx_full(&SATL_ctx, &hdr,0,0,0);
        SATL_rapdu_sw_t sw;
        uint32_t le=0;
        SATL_master_rx_full(&SATL_ctx, &le,0,&sw);
        assert(sw.SW1==0x90);
        assert(sw.SW2==0x00);
        assert(0==le);
    }

    if(0xFFFFFFFF==rx_chunk_size){
        for(unsigned int i=0;i<1000;i++){
            test_case(0x14,0x14,0x14);
        }
    }
    //test_case(0,263,262);
    test_case(257,0,0);
    test_case(0,1,1);

    uint32_t test_lengths[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,254,255,256,257,258,259,260,261,262,263,264,265,266,267,268,269,65533,65534,65535,65536};
    for(unsigned int i=0;i<NUM_ELEMS(test_lengths);i++){
        uint32_t lc = test_lengths[i];
        printf("%u\n",lc);
        for(unsigned int j=0;j<NUM_ELEMS(test_lengths);j++){
            uint32_t le = test_lengths[j];
            for(unsigned int rle=0;rle<4;rle++){
                if(rle>le) break;
                test_case(lc,le,rle);
            }
            uint32_t rle = le-1;
            while(rle<le){
                test_case(lc,le,rle);
                rle--;
                if(rle<le-3) break;
            }
        }
    }
    if(long_test){
        //test all lengths
        for(unsigned int l=0;l<(1<<16)+1;l++){
            printf("%u\n",l);
            test_case(l,l,l);
        }
        //cross test for all lengths < 1000
        for(unsigned int lc=0;lc<1000;lc++){
            printf("%u\n",lc);
            for(unsigned int le=0;le<1000;le++){
                test_case(lc,le,le);
            }
        }
    }
    {
        SATL_capdu_header_t hdr;
        hdr.CLA=0xFF;
        hdr.INS=0x02;
        hdr.P1 =0x03;
        hdr.P2 =0x04;
        SATL_master_tx_full(&SATL_ctx, &hdr,0,0,0);
        SATL_rapdu_sw_t sw;
        uint32_t le=0;
        SATL_master_rx_full(&SATL_ctx, &le,0,&sw);
        assert(sw.SW1==0x90);
        assert(sw.SW2==0x00);
        assert(0==le);
        sleep(1);
        printf("done\n");
    }
    return 0;
}
