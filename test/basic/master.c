#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <assert.h>

int sockfd = 0;
/*
void SATL_error_handler(){
    printf("error handler\n");
    exit(-1);
}
*/
#define SATL_TEST_MASTER
#include "satl_test_com.h"

SATL_ctx_t SATL_ctx;

uint8_t refdat[(1<<16)+1];
uint8_t refdatle[(1<<16)+1];

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
        //write the whole APDU in one go
        SATL_master_rx_full(ctx, le, data, sw);
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
    master_tx_apdu(&SATL_ctx, &hdr,lc,le,buf);
    SATL_rapdu_sw_t sw;
    uint32_t actual_le=le;
    uint8_t buf2[(1<<16)+1];
    master_rx_apdu(&SATL_ctx, &actual_le,buf2,&sw);
    assert(rle==actual_le);
    assert(sw.SW1==0x90);
    assert(sw.SW2==0x00);
    uint32_t l = lc<rle ? lc : rle;
    //printf("l=%u, rle=%u\n",l,rle);
    //for(unsigned int i=0;i<actual_le;i++){
    //    printf("%02X ",buf2[i]);
    //}
    //printf("\n");
    assert(0==memcmp(buf,buf2,l));
    assert(0==memcmp(refdat,buf2,l));
    assert(0==memcmp(refdatle,buf2+l,rle-l));
}

#define NUM_ELEMS(a) (sizeof(a)/sizeof 0[a])
int main(int argc, char *argv[]){
    unsigned int long_test=0;
    uint32_t port = 5000;
    if(argc>1) port = atoi(argv[1]);
    if(argc>2) tx_chunk_size = atoi(argv[2]);
    if(argc>3) rx_chunk_size = atoi(argv[3]);
    if(argc>4) long_test = atoi(argv[4]);

    for(unsigned int i=0;i<(1<<16)+1;i++){
        refdat[i] = i;
        refdatle[i] = i ^ 0xFF;
    }

    int n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    }

    emu_init(&com_peripheral);
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
        printf("done\n");
    }
    return 0;
}
