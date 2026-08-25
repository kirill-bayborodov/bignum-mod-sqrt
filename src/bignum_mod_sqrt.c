#include "bignum_mod_sqrt.h"
#include <string.h>

typedef struct { uint64_t w[BIGNUM_CAPACITY]; size_t len; } bn_local;

static void norm(bn_local *x) { while (x->len && !x->w[x->len - 1]) --x->len; }
static int cmp(const bn_local *a, const bn_local *b) {
    size_t i; if (a->len != b->len) return a->len > b->len ? 1 : -1;
    for (i = a->len; i; --i) if (a->w[i-1] != b->w[i-1]) return a->w[i-1] > b->w[i-1] ? 1 : -1;
    return 0;
}
static void from_public(bn_local *d, const bignum_t *s) {
    memset(d, 0, sizeof(*d)); d->len = s->len <= BIGNUM_CAPACITY ? s->len : BIGNUM_CAPACITY;
    memcpy(d->w, s->words, d->len * sizeof(uint64_t)); norm(d);
}
static void to_public(bignum_t *d, const bn_local *s) {
    memset(d, 0, sizeof(*d)); d->len = s->len; memcpy(d->words, s->w, s->len * sizeof(uint64_t));
}
static void sub_bn(bn_local *a, const bn_local *b) {
    size_t i; uint64_t borrow = 0;
    for (i = 0; i < a->len; ++i) { uint64_t bi = i < b->len ? b->w[i] : 0; uint64_t old = a->w[i]; a->w[i] = old - bi - borrow; borrow = borrow ? old <= bi : old < bi; }
    norm(a);
}
static void add_mod(bn_local *r, const bn_local *a, const bn_local *b, const bn_local *m) {
    size_t i; uint64_t carry = 0;
    memset(r, 0, sizeof(*r)); r->len = m->len;
    for (i = 0; i < m->len; ++i) { __uint128_t t = (__uint128_t)(i<a->len?a->w[i]:0)+(i<b->len?b->w[i]:0)+carry; r->w[i]=(uint64_t)t; carry=(uint64_t)(t>>64); }
    norm(r); if (carry || cmp(r,m) >= 0) sub_bn(r,m);
}
static void shl1_mod(bn_local *r, const bn_local *a, const bn_local *m) { add_mod(r,a,a,m); }
static void shr1(bn_local *a) {
    size_t i; uint64_t carry=0;
    for (i=a->len; i; --i) { uint64_t x=a->w[i-1]; a->w[i-1]=(x>>1)|(carry<<63); carry=x&1; } norm(a);
}
static void inc_small(bn_local *a, uint64_t v) {
    size_t i=0; uint64_t c=v; while (c && i<BIGNUM_CAPACITY) { __uint128_t t=(__uint128_t)a->w[i]+c; a->w[i]=(uint64_t)t; c=(uint64_t)(t>>64); ++i; } if (i>a->len) a->len=i; norm(a);
}
static void mod_reduce(bn_local *r, const bn_local *x, const bn_local *m) {
    size_t i,j; memset(r,0,sizeof(*r));
    for (i=x->len; i; --i) for (j=64; j; --j) { bn_local t; shl1_mod(&t,r,m); if ((x->w[i-1]>>(j-1))&1) { bn_local one={{1},1}; add_mod(r,&t,&one,m); } else *r=t; }
}
static void mul_mod(bn_local *r, const bn_local *a, const bn_local *b, const bn_local *m) {
    size_t i,j; bn_local x,y,t; mod_reduce(&x,a,m); mod_reduce(&y,b,m); memset(r,0,sizeof(*r));
    for (i=0;i<y.len;++i) for (j=0;j<64;++j) { if ((y.w[i]>>j)&1) { add_mod(&t,r,&x,m); *r=t; } shl1_mod(&t,&x,m); x=t; }
}
static void pow_mod(bn_local *r, const bn_local *a, const bn_local *e, const bn_local *m) {
    size_t i,j; bn_local x,t,one={{1},1}; mod_reduce(&x,a,m); mod_reduce(r,&one,m);
    for (i=0;i<e->len;++i) for (j=0;j<64;++j) { if ((e->w[i]>>j)&1) { mul_mod(&t,r,&x,m); *r=t; } mul_mod(&t,&x,&x,m); x=t; }
}
static int is_one(const bn_local *a) { return a->len==1 && a->w[0]==1; }
static int is_zero(const bn_local *a) { return a->len==0; }
static uint64_t lowbit(const bn_local *a) { return a->len ? a->w[0]&1 : 0; }
static uint64_t u64_addmod(uint64_t a, uint64_t b, uint64_t m) { return (uint64_t)(((__uint128_t)a + b) % m); }
static uint64_t u64_mulmod(uint64_t a, uint64_t b, uint64_t m) { uint64_t r=0; while (b) { if (b&1) r=u64_addmod(r,a,m); a=u64_addmod(a,a,m); b>>=1; } return r; }
static uint64_t u64_powmod(uint64_t a, uint64_t e, uint64_t m) { uint64_t r=1%m; while(e){if(e&1)r=u64_mulmod(r,a,m);a=u64_mulmod(a,a,m);e>>=1;}return r; }
static int u64_sqrt(uint64_t a, uint64_t p, uint64_t *out) { uint64_t q=p-1,z,c,x,t,b; unsigned s=0,i,k; if(a==0){*out=0;return 1;} if(u64_powmod(a,q/2,p)!=1)return 0; if(p%4==3){*out=u64_powmod(a,(p+1)/4,p);return 1;} while(!(q&1)){q>>=1;++s;} for(z=2;u64_powmod(z,(p-1)/2,p)!=p-1;++z){} x=u64_powmod(a,(q+1)/2,p); c=u64_powmod(z,q,p); t=u64_powmod(a,q,p); while(t!=1){for(i=1,b=u64_mulmod(t,t,p);b!=1 && i<s;++i)b=u64_mulmod(b,b,p); if(i>=s)return 0; b=c;for(k=0;k<s-i-1;++k)b=u64_mulmod(b,b,p); t=u64_mulmod(t,u64_mulmod(b,b,p),p);c=u64_mulmod(b,b,p);x=u64_mulmod(x,b,p);s=i;}*out=x;return 1;}

bignum_mod_sqrt_status_t bignum_mod_sqrt(const bignum_t *a, const bignum_t *modulus, bignum_t *root) {
    bn_local aa,p,q,qodd,z,c,x,t,b,tmp,exp,one={{1},1}; unsigned s=0;
    if (!a || !modulus || !root) return BIGNUM_MOD_SQRT_ERROR_NULL_ARG;
    if (a->len>BIGNUM_CAPACITY || modulus->len>BIGNUM_CAPACITY) return BIGNUM_MOD_SQRT_ERROR_OVERFLOW;
    from_public(&aa,a); from_public(&p,modulus);
    if (is_zero(&p) || !lowbit(&p)) return BIGNUM_MOD_SQRT_ERROR_MODULUS;
    if (p.len == 1U) { uint64_t av = aa.len ? aa.w[0] % p.w[0] : 0U, rv; if (!u64_sqrt(av,p.w[0],&rv)) return BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE; memset(root,0,sizeof(*root)); if(rv){root->words[0]=rv;root->len=1;} return BIGNUM_MOD_SQRT_SUCCESS; }
    { bn_local reduced; mod_reduce(&reduced,&aa,&p); aa = reduced; }
    if (is_zero(&aa)) { to_public(root,&aa); return BIGNUM_MOD_SQRT_SUCCESS; }
    q=p; sub_bn(&q,&one);
    exp=q; shr1(&exp); pow_mod(&tmp,&aa,&exp,&p);
    { bn_local pm1=q; if (cmp(&tmp,&one)!=0 && cmp(&tmp,&pm1)!=0) return BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE; }
    if (p.len==1 && p.w[0]%4==3) { exp=q; inc_small(&exp,1); shr1(&exp); shr1(&exp); pow_mod(&x,&aa,&exp,&p); to_public(root,&x); return BIGNUM_MOD_SQRT_SUCCESS; }
    qodd=q; while (!lowbit(&qodd)) { shr1(&qodd); ++s; }
    if (s==1) { exp=p; inc_small(&exp,3); shr1(&exp); shr1(&exp); pow_mod(&x,&aa,&exp,&p); to_public(root,&x); return BIGNUM_MOD_SQRT_SUCCESS; }
    z.len=1; z.w[0]=2;
    for (;;) { exp=q; shr1(&exp); pow_mod(&tmp,&z,&exp,&p); if (cmp(&tmp,&q)==0) break; inc_small(&z,1); }
    exp=qodd; inc_small(&exp,1); shr1(&exp); pow_mod(&x,&aa,&exp,&p);
    pow_mod(&c,&z,&qodd,&p); pow_mod(&t,&aa,&qodd,&p);
    while (!is_one(&t)) { unsigned i=1; tmp=t; while (!is_one(&tmp) && i<s) { mul_mod(&tmp,&tmp,&tmp,&p); ++i; } if (i>=s) return BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE; b=c; for (unsigned k=0;k<s-i-1;++k) mul_mod(&b,&b,&b,&p); { bn_local bb; mul_mod(&bb,&b,&b,&p); mul_mod(&t,&t,&bb,&p); c=bb; mul_mod(&x,&x,&b,&p); } s=i; }
    to_public(root,&x); return BIGNUM_MOD_SQRT_SUCCESS;
}
