/*
 * print.h
 *
 *  Created on: 24 mars 2020
 *      Author: sriou
 */

#ifndef __PRINT_H__
#define __PRINT_H__

//shall be included after the header which declare print_impl function
//void print_impl(const char*msg);

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

static bool print_enabled=1;
static void print_cfg(bool enable){print_enabled=enable;}

static void print(const char*msg){if(!print_enabled) return;print_impl(msg);}


static void print_uint_as_hex(uint64_t num, unsigned int nhexdigits){
  if(!print_enabled) return;
  const char hex[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

  uint8_t car;
  nhexdigits = nhexdigits < 16 ? nhexdigits : 16;
  uint8_t i = nhexdigits;
  char str_num[17];
  str_num[i] = 0;
  while (i > 0){
    --i;
    car = num & 0xF;
    str_num[i] = hex[car];
    num >>= 4;
  }
  print(str_num);
}

static void print_uint_as_dec(uint64_t num, unsigned int ndecdigits){
  if(!print_enabled) return;
  char res[21];
  char* resptr = &res[20];
  *resptr=0;
  uint64_t t=num;
  unsigned int l=0;
  char zero='0';
  do{
    uint64_t d=t/10;
    uint64_t m=t-10*d;
    *--resptr = zero+m;
    l++;
    zero=d ? zero : ' ';//replace leading zeroes by blank
    t=d;
  } while (t!=0);
  while(l<ndecdigits){
      *--resptr = ' ';
      l++;
  }
  print(resptr);
}

static void println(const char*msg){
  if(!print_enabled) return;
  print(msg);print("\n");
}
static void print8x(const char*msg1,uint8_t val,const char*msg2){
  if(!print_enabled) return;
  if(msg1) print(msg1);
  print_uint_as_hex(val,2);
  if(msg2) print(msg2);
}
static void println8x(const char*msg1,uint8_t val){
  if(!print_enabled) return;
  print8x(msg1,val,"\n");
}

static void print8d(const char*msg1,uint32_t val,const char*msg2){
  if(!print_enabled) return;
  if(msg1) print(msg1);
  print_uint_as_dec(val,3);
  if(msg2) print(msg2);
}
static void println8d(const char*msg1,uint32_t val){
  if(!print_enabled) return;
  print8d(msg1,val,"\n");
}


static void print16x(const char*msg1,uint16_t val,const char*msg2){
  if(!print_enabled) return;
  if(msg1) print(msg1);
  print_uint_as_hex(val,4);
  if(msg2) print(msg2);
}
static void println16x(const char*msg1,uint16_t val){
  if(!print_enabled) return;
  print16x(msg1,val,"\n");
}

static void print16d(const char*msg1,uint32_t val,const char*msg2){
  if(!print_enabled) return;
  if(msg1) print(msg1);
  print_uint_as_dec(val,5);
  if(msg2) print(msg2);
}
static void println16d(const char*msg1,uint32_t val){
  if(!print_enabled) return;
  print16d(msg1,val,"\n");
}


static void print32x(const char*msg1,uint32_t val,const char*msg2){
  if(!print_enabled) return;
  if(msg1) print(msg1);
  print_uint_as_hex(val,8);
  if(msg2) print(msg2);
}
static void println32x(const char*msg1,uint32_t val){
  if(!print_enabled) return;
  print32x(msg1,val,"\n");
}

static void print32d(const char*msg1,uint32_t val,const char*msg2){
  if(!print_enabled) return;
  if(msg1) print(msg1);
  print_uint_as_dec(val,10);
  if(msg2) print(msg2);
}
static void println32d(const char*msg1,uint32_t val){
  if(!print_enabled) return;
  print32d(msg1,val,"\n");
}

static void print64x(const char*msg1,uint64_t val,const char*msg2){
  if(!print_enabled) return;
  if(msg1) print(msg1);
  print_uint_as_hex(val,16);
  if(msg2) print(msg2);
}
static void println64x(const char*msg1,uint64_t val){
  if(!print_enabled) return;
  print64x(msg1,val,"\n");
}

static void print64d(const char*msg1,uint32_t val,const char*msg2){
  if(!print_enabled) return;
  if(msg1) print(msg1);
  print_uint_as_dec(val,10);
  if(msg2) print(msg2);
}
static void println64d(const char*msg1,uint32_t val){
  if(!print_enabled) return;
  print64d(msg1,val,"\n");
}

static void print_bytes_sep(const char*msg1,const void*const buf,unsigned int size, const char*msg2, const char*const separator){
  if(!print_enabled) return;
  const uint8_t*const buf8=(const uint8_t*const)buf;
  if(msg1) print(msg1);
  const char*sep=0;
  for(unsigned int i=0;i<size;i++){
      print8x(sep,buf8[i],0);
      sep=separator;
  }
  if(msg2) print(msg2);
}

static void print_bytes(const char*msg1,const void*const buf,unsigned int size, const char*msg2){
  if(!print_enabled) return;
  print_bytes_sep(msg1,buf,size,msg2," ");
}
static void println_bytes(const char*msg1,const void*const buf,unsigned int size){
  if(!print_enabled) return;
  print_bytes(msg1,buf,size,"\n");
}

static void print_bytes_0x(const char*msg1,const void*const buf,unsigned int size, const char*msg2){
  if(!print_enabled) return;
  print(msg1);
  print_bytes_sep("0x",buf,size,msg2,",0x");
}
static void println_bytes_0x(const char*msg1,const void*const buf,unsigned int size){
  if(!print_enabled) return;
  print_bytes_0x(msg1,buf,size,"\n");
}

static void print_array32x_0xln(const void*const buf, unsigned int n_elements){
  if(!print_enabled) return;
  const uint32_t *elements=(const uint32_t*)buf;
  for(unsigned int i=0;i<n_elements;i++){
      print32d("",i,"");
      println32x(": 0x",elements[i]);
  }
}

#define PRINTLN16X_VAR(name) do{\
    print(#name);\
    println16x(" = 0x",name);\
  }while(0)
#define PRINTLN16D_VAR(name) do{\
    print(#name);\
    println16d(" = ",name);\
  }while(0)

#endif // __PRINT_H__
