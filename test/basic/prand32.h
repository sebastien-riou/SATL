#ifndef __PRAND32_H__
#define __PRAND32_H__

static uint32_t lfsr32(uint32_t state) {
  const uint32_t tapsMask = 0x80000057;
  if(0==state) state = 0x12345678;
  uint32_t out = state>>1;
  if(state & 1){
    out = out ^ tapsMask;
  }
  return out;
}

static uint32_t prand32(void){
  static uint32_t state=0;
  state = lfsr32(state);
  return state;
}

static uint32_t prand32_in_range(uint32_t min, uint32_t max){
    uint32_t r = prand32();
    uint32_t range = max+1-min;
    r = r%range;
    r += min;
    assert(r>=min);
    assert(r<=max);
    return r;
}

#endif
