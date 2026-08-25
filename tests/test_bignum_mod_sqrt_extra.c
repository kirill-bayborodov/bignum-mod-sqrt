#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bignum_mod_sqrt.h"
static bignum_t make(uint64_t x){bignum_t v={{0},0};if(x){v.words[0]=x;v.len=1;}return v;}
int main(void){
 bignum_t a=make(4),p=make(11),r=make(0),before;
 assert(bignum_mod_sqrt(&a,&p,&r)==0 && r.len==1 && (r.words[0]==3 || r.words[0]==9));
 before=r; p.words[0]=13; assert(bignum_mod_sqrt(&a,&p,&r)==0); assert(r.words[0]==11 || r.words[0]==2); assert(before.words[0]!=0);
 bignum_t non=make(2),bad=make(14),sentinel=make(0xabcdef); assert(bignum_mod_sqrt(&non,&bad,&sentinel)==BIGNUM_MOD_SQRT_ERROR_MODULUS); assert(sentinel.words[0]==0xabcdef);
 bignum_t wide={{0},0}; wide.len=2; wide.words[0]=4; wide.words[1]=1; p=make(11); assert(bignum_mod_sqrt(&wide,&p,&r)==BIGNUM_MOD_SQRT_SUCCESS); assert(r.len==1);
 puts("bignum_mod_sqrt extra tests: OK"); return 0;
}
