#ifndef __SATL_SOCKET_H__
#define __SATL_SOCKET_H__
//driver to transport APDU over POSIX sockets

#include "stdio.h"
#include "stdlib.h"

//#include <sys/ioctl.h>
//#include <linux/sockios.h>
#if defined _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
static int inet_pton(int af, const char *src, void *dst){
  struct sockaddr_storage ss;
  int size = sizeof(ss);
  char src_copy[INET6_ADDRSTRLEN+1];

  ZeroMemory(&ss, sizeof(ss));
  strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
  src_copy[INET6_ADDRSTRLEN] = 0;

  if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
    switch(af) {
      case AF_INET:
    *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
    return 1;
      case AF_INET6:
    *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
    return 1;
    }
  }
  return 0;
}
static void init_socket_api(void){
    WSADATA mywsadata; //your wsadata struct, it will be filled by WSAStartup
    WSAStartup(0x0202,&mywsadata); //0x0202 refers to version of sockets we want to use.
}
static long unsigned int rx_bytes_available(int sockfd){
    long unsigned int bytes_available;
    ioctlsocket(sockfd,FIONREAD,&bytes_available);
    return bytes_available;
}
#else
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
static void init_socket_api(void){}
static long unsigned int rx_bytes_available(int sockfd){
    long unsigned int bytes_available;
    ioctl(sockfd,FIONREAD,&bytes_available);
    return bytes_available;
}
#endif

typedef struct SATL_socket_params_struct_t {
    const char*address;//"127.0.0.1"
    uint32_t port;//5000
} SATL_socket_params_t;

static int SATL_socket_master_init(void *hw){
    SATL_socket_params_t*params = (SATL_socket_params_t*)hw;

    printf("SATL_socket_master_init %s : %u\n",params->address,params->port);

    struct sockaddr_in serv_addr;
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Error : Could not create socket \n");
        return -1;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(params->port);
    if(inet_pton(AF_INET, params->address, &serv_addr.sin_addr)<=0){
        printf("\n inet_pton error occured\n");
        return -2;
    }
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
       printf("\n Error : Connect Failed \n");
       return -3;
    }
    return sockfd;
}

static int SATL_socket_slave_init(void *hw){
    SATL_socket_params_t*params = (SATL_socket_params_t*)hw;
    int listenfd = 0;
    struct sockaddr_in serv_addr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(params->port);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 1);//listen to a single connection

    int sockfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
    return sockfd;
}

#ifndef SATL_SOCKET_ONLY_CUSTOM_FUNCS

#ifndef SATL_ACK
#define SATL_ACK 0
#endif
#define SATL_SFR_GRANULARITY 1
#define SATL_SUPPORT_SLAVE
#define SATL_SUPPORT_MASTER

typedef struct SATL_driver_ctx_struct_t {
    int sockfd;
    uint8_t buf[66000];
    uint32_t buf_level;
    uint32_t rx_read_pos;
} SATL_driver_ctx_t;

//Initialization
static uint32_t SATL_slave_init_driver (SATL_driver_ctx_t *const ctx, void *hw){
    ctx->sockfd = SATL_socket_slave_init(hw);
    assert(0<ctx->sockfd);
    ctx->buf_level=0;
    ctx->rx_read_pos=0;
    memset(ctx->buf,0,sizeof(ctx->buf));
    return 0;
}
static uint32_t SATL_master_init_driver(SATL_driver_ctx_t *const ctx, void *hw){
    ctx->sockfd = SATL_socket_master_init(hw);
    assert(0<ctx->sockfd);
    ctx->buf_level=0;
    ctx->rx_read_pos=0;
    memset(ctx->buf,0,sizeof(ctx->buf));
    return 0;
}

//TX functions
static void     SATL_switch_to_tx      (SATL_driver_ctx_t *const ctx){
    ctx->buf_level=0;
    memset(ctx->buf,0,sizeof(ctx->buf));
}
static void     SATL_tx                (SATL_driver_ctx_t *const ctx, const void *const buf, unsigned int len){
    //printf("SATL_tx %u\n",len);
    assert(ctx->buf_level+len<sizeof(ctx->buf));
    memcpy(ctx->buf+ctx->buf_level,buf,len);
    ctx->buf_level+=len;
    //printf("ctx->buf_level=%u\n",ctx->buf_level);
}
static void     SATL_final_tx          (SATL_driver_ctx_t *const ctx, const void *const buf, unsigned int len){
    //printf("SATL_final_tx %u\n",len);
    SATL_tx(ctx, buf, len);
    send(ctx->sockfd, ctx->buf, ctx->buf_level,0);
    ctx->buf_level=0;
    ctx->rx_read_pos=0;
}

//RX functions
static void     SATL_rx                (SATL_driver_ctx_t *const ctx, void *buf, unsigned int len){
    //printf("SATL_rx %u\n",len);
    if(len){
        if(0==ctx->rx_read_pos){//start of frame, fill buffer
            uint32_t fl;
            const unsigned int SATL_LEN_LEN = 4;
            recv(ctx->sockfd, ctx->buf, SATL_LEN_LEN, MSG_WAITALL);//get FL
            memcpy(&fl,ctx->buf,sizeof(fl));
            assert(fl>SATL_LEN_LEN);
            assert(fl<=sizeof(ctx->buf));
            //printf("fl=%u\n",fl);
            recv(ctx->sockfd, ctx->buf+SATL_LEN_LEN, fl-SATL_LEN_LEN, MSG_WAITALL);//get remaining of the frame
            ctx->buf_level = fl;
        }
        memcpy(buf,ctx->buf+ctx->rx_read_pos,len);
        ctx->rx_read_pos+=len;
        ctx->buf_level  -=len;
    }
}
static void     SATL_final_rx          (SATL_driver_ctx_t *const ctx, void *buf, unsigned int len){
    SATL_rx(ctx, buf, len);
}
static uint32_t SATL_get_rx_level      (const SATL_driver_ctx_t *ctx){
    //printf("SATL_get_rx_level\n");
    return ctx->buf_level;
}

//ACK signal functions
#if SATL_ACK
static void     SATL_tx_ack            (SATL_driver_ctx_t *const ctx){
    uint8_t ack='A';
    send(ctx->sockfd, &ack, 1,0);
}
static void     SATL_rx_ack            (SATL_driver_ctx_t *const ctx){
    uint8_t ack;
    recv(ctx->sockfd, &ack, 1, MSG_WAITALL);
}
#else
static void     SATL_tx_ack            (SATL_driver_ctx_t *const ctx){assert(0);}
static void     SATL_rx_ack            (SATL_driver_ctx_t *const ctx){assert(0);}
#endif

#endif //SATL_SOCKET_ONLY_CUSTOM_FUNCS

#endif //__SATL_SOCKET_H__
