//
//  SaM.c
//  crypto777
//
//  Created by jl777 on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//  based on SaM code by Come-from-Beyond

#ifdef DEFINES_ONLY
#ifndef crypto777_SaM_h
#define crypto777_SaM_h
#include <stdio.h>
#include <memory.h>

#define TRIT char
#define TRIT_FALSE -1
#define TRIT_UNKNOWN 0
#define TRIT_TRUE 1

#define SAM_HASH_SIZE 243
#define SAM_STATE_SIZE (SAM_HASH_SIZE * 3)
#define SAM_MAGIC_NUMBER 10
#define SAMHIT_LIMIT 7625597484987 // 3 ** 27
#define MAX_CRYPTO777_HIT ((1LL << 62) / 1000)

#include "bits777.c"
#include "utils777.c"
#include "system777.c"
#define MAX_INPUT_SIZE ((int32_t)(4096 - sizeof(bits256) - 2*sizeof(uint32_t)))

struct SaM_info { TRIT trits[SAM_STATE_SIZE],hash[SAM_HASH_SIZE]; bits384 bits; int SAM_INDICES[SAM_STATE_SIZE]; };
struct SaMhdr { bits384 sig; uint32_t timestamp,nonce; uint8_t numrounds,leverage; };

void SaM_Initialize(struct SaM_info *state);
int32_t SaM_Absorb(struct SaM_info *state,uint64_t numrounds,const uint8_t *input,const uint32_t inputSize,const uint8_t *input2,const uint32_t inputSize2);
bits384 SaM_emit(struct SaM_info *state,uint64_t numrounds);
uint64_t calc_SaMthreshold(int32_t leverage);
bits384 SaM_chain(char *email,bits384 hash,int32_t chainid,int32_t hashi,int32_t maxiter,int32_t numrounds);
uint64_t calc_SaM(bits384 *sigp,uint8_t *input,int32_t inputSize,uint8_t *input2,int32_t inputSize2,uint64_t numrounds);
uint64_t SaMnonce(bits384 *sigp,uint32_t *noncep,uint8_t *buf,int32_t len,uint64_t numrounds,uint64_t threshold,uint32_t rseed,int32_t maxmillis);
#endif
#else
#ifndef crypto777_SaM_c
#define crypto777_SaM_c

#ifndef crypto777_SaM_h
#define DEFINES_ONLY
#include "SaM.c"
#undef DEFINES_ONLY
#endif

static const TRIT SAM_TRITS[729] = {
    -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1
};
//static int SAM_INDICES[SAM_STATE_SIZE];

void SaM_PrepareIndices(struct SaM_info *state)
{
    int32_t i,nextIndex,currentIndex = 0;
    for (i=0; i<SAM_STATE_SIZE; i++)
    {
        nextIndex = ((currentIndex + 1) * SAM_MAGIC_NUMBER) % SAM_STATE_SIZE;
        state->SAM_INDICES[currentIndex] = nextIndex;
        currentIndex = nextIndex;
    }
}

void SaM_Initialize(struct SaM_info *state)
{
    //if ( SAM_INDICES[0] == 0 )
    SaM_PrepareIndices(state);//, printf("SAM_INDICES[0] -> %d\n",SAM_INDICES[0]);
    memset(state->hash,0,sizeof(state->hash));
    memcpy(state->trits,SAM_TRITS,sizeof(state->trits));
}

static TRIT SaM_Bias(const TRIT a, const TRIT b) { return a == 0 ? 0 : a == -b ? a : -a; }

void SaM_SplitAndMerge(struct SaM_info *state,uint64_t numrounds)
{
    //static const TRIT SAMANY[3][3] = { { -1, -1, 0, }, { -1, 0, 1, }, { 0, 1, 1, } };
    static const TRIT SAMSUM[3][3] = { { 1, -1, 0, }, { -1, 0, 1, }, { 0, 1, -1, } };
    static const TRIT SAMBIAS[3][3] = { { 1, 1, -1, }, { 0, 0, 0, }, { 1, -1, -1, } };
    struct SaM_info leftPart,rightPart;
    uint64_t round;
	int32_t i,nextIndex,current,next,currentIndex = 0;
    if ( 0 )
    {
        int a,b;
        for (a=-1; a<=1; a++)
        {
            printf("{ ");
            for (b=-1; b<=1; b++)
                printf("%d, ",SaM_Bias(a,b));
            printf("}, ");
        }
        getchar();
    }
    if ( numrounds <= 0 )
        return;
	for (round=1; round<=numrounds; round++)
    {
		for (i=0; i<SAM_STATE_SIZE; i++)
        {
            nextIndex = state->SAM_INDICES[currentIndex];
            current = state->trits[currentIndex] + 1, next = state->trits[nextIndex] + 1;
            leftPart.trits[i] = SAMBIAS[current][next];//SaM_Bias(current,next);;//
            rightPart.trits[i] = SAMBIAS[next][current];//SaM_Bias(next,current);//
            currentIndex = state->SAM_INDICES[nextIndex];
 		}
		for (i=0; i<SAM_STATE_SIZE; i++)
        {
            nextIndex = state->SAM_INDICES[currentIndex];
			state->trits[i] = SAMSUM[leftPart.trits[currentIndex]+1][rightPart.trits[nextIndex]+1];
			currentIndex = state->SAM_INDICES[nextIndex];
		}
	}
}

void _SaM_Absorb(struct SaM_info *state,TRIT *input,uint32_t inputSize,uint64_t numrounds)
{
    int32_t i,n,offset = 0;
    if ( numrounds < 2 )
        numrounds = 2;
    n = (inputSize / SAM_HASH_SIZE);
    if ( n > 0 )
    {
        for (i=0; i<n; i++,offset+=SAM_HASH_SIZE)
        {
            memcpy(state->trits,&input[offset],SAM_HASH_SIZE * sizeof(TRIT));
            SaM_SplitAndMerge(state,numrounds);
        }
    }
	if ( (i= (inputSize % SAM_HASH_SIZE)) != 0 )
    {
		memcpy(state->trits,&input[offset],i * sizeof(TRIT));
		SaM_SplitAndMerge(state,numrounds);
	}
    //for (i=0; i<SAM_HASH_SIZE; i++)
    //    printf("%d ",state->trits[i]);
    //printf("input trits\n");
}

int32_t SaM_Absorb(struct SaM_info *state,uint64_t numrounds,const uint8_t *input,const uint32_t inputSize,const uint8_t *input2,const uint32_t inputSize2)
{
    TRIT output[(MAX_INPUT_SIZE + sizeof(struct SaMhdr)) << 3];
    int32_t i,n = 0;
    if ( input != 0 && inputSize != 0 )
    {
        for (i=0; i<(inputSize << 3); i++)
            output[n++] = ((input[i >> 3] & (1 << (i & 7))) != 0);
    }
    if ( input2 != 0 && inputSize2 != 0 )
    {
        for (i=0; i<(inputSize2 << 3); i++)
            output[n++] = ((input2[i >> 3] & (1 << (i & 7))) != 0);
    }
    if ( state != 0 )
        _SaM_Absorb(state,output,n,numrounds);
    return(n);
}

void SaM_Squeeze(struct SaM_info *state,TRIT *output,uint64_t numrounds)
{
	SaM_SplitAndMerge(state,(numrounds < SAM_MAGIC_NUMBER) ? 2 : numrounds);
	memcpy(output,state->trits,SAM_HASH_SIZE * sizeof(TRIT));
	SaM_SplitAndMerge(state,(numrounds < SAM_MAGIC_NUMBER) ? 2 : numrounds);
}

int32_t SaM_test()
{
/*    extern char testvector[243*3*8],testhash[243];
    struct SaM_info state;
    int i;
    SaM_PrepareIndices();
    SaM_Initialize(&state);
    _SaM_Absorb(&state,testvector,243*3,SAM_MAGIC_NUMBER);
	SaM_SplitAndMerge(&state,SAM_MAGIC_NUMBER);
    for (i=0; i<243; i++)
        if ( testhash[i] != state.hash[i] )
            printf("(%d != %d).%d ",testhash[i],state.hash[i],i);
    printf("cmp.%d\n",memcmp(testhash,state.hash,243)); getchar();*/
    return(0);
}

bits384 SaM_emit(struct SaM_info *state,uint64_t numrounds)
{
    // i.12 531441 81bf1 0.68% numbits.19 mask.7ffff -> bias -0.0005870312
    TRIT *ptr;
    uint64_t bits64;
    uint32_t i,j,rawbits,bits19[20],mask = 0x7ffff;
	SaM_Squeeze(state,state->hash,numrounds);
    ptr = state->hash;
    for (i=0; i<SAM_HASH_SIZE/12; i++)
    {
        for (j=rawbits=0; j<12; j++)
            rawbits = (rawbits * 3 + *ptr++ + 1);
        bits19[i] = ((((uint64_t)rawbits<<19)/531441) & mask); // 3^12 == 531441 //bits19[i] = (rawbits & mask);
        //printf("%5x ",bits19[i]);
    }
    for (i*=12,rawbits=0; i<SAM_HASH_SIZE; i++) // 3 trits -> 27
        rawbits = (rawbits * 3 + *ptr++ + 1);
    rawbits = (((rawbits<<4)/27) & 0xf);
    //printf("%x -> Sam_emit\n",rawbits);
    for (bits64=i=0; i<20; i++)
    {
        memcpy(&state->bits.bytes[i*sizeof(uint16_t)],&bits19[i],sizeof(uint16_t));
        bits64 = (bits64 << 3) | ((bits19[i] >> 16) & 7);
    }
    bits64 = (bits64 << 4) | (rawbits & 0xf);
    memcpy(&state->bits.bytes[40],&bits64,sizeof(uint64_t));
    return(state->bits);
}

uint64_t SaM_hit(struct SaM_info *state)
{
    int32_t i; uint64_t hit = 0;
    for (i=0; i<27; i++)
 		hit = (hit * 3 + state->hash[i] + 1);
    return(hit);
}

uint64_t calc_SaM(bits384 *sigp,uint8_t *input,int32_t inputSize,uint8_t *input2,int32_t inputSize2,uint64_t numrounds)
{
    int32_t verify_SaM(TRIT *newhash,uint8_t *buf,const int n);
    struct SaM_info state;
    SaM_Initialize(&state);
    SaM_Absorb(&state,numrounds,input,inputSize,input2,inputSize2);
    //printf("len.%d: ",inputSize+inputSize2);
    *sigp = SaM_emit(&state,numrounds);
    if ( 0 && input2 == 0 && numrounds == SAM_MAGIC_NUMBER )
        verify_SaM(state.hash,(uint8_t *)input,inputSize);
    return(SaM_hit(&state));
}

bits384 SaM_chain(char *email,bits384 hash,int32_t chainid,int32_t hashi,int32_t maxiter,int32_t numrounds)
{
    struct SaM_info state;
    int32_t i;
    char chainid_email[512];
    sprintf(chainid_email,"%s.%d",email,chainid);
    SaM_Initialize(&state);
    for (i=hashi; i<maxiter; i++)
    {
        SaM_Absorb(&state,numrounds,hash.bytes,sizeof(hash),(uint8_t *)chainid_email,(int32_t)strlen((char *)chainid_email));
        hash = SaM_emit(&state,numrounds);
    }
    return(hash);
}

uint64_t calc_SaMthreshold(int32_t leverage)
{
    int32_t i;
    uint64_t threshold,divisor = 1;
    if ( leverage > 26 )
        leverage = 26;
    for (i=0; i<leverage; i++)
        divisor *= 3;
    threshold = (SAMHIT_LIMIT / divisor);
    return(threshold);
}

uint64_t SaMnonce(bits384 *sigp,uint32_t *noncep,uint8_t *buf,int32_t len,uint64_t numrounds,uint64_t threshold,uint32_t rseed,int32_t maxmillis)
{
    uint64_t hit = SAMHIT_LIMIT;
    double startmilli = 0;
    if ( maxmillis == 0 )
    {
        hit = calc_SaM(sigp,buf,len,0,0,numrounds);
        if ( hit >= threshold )
        {
            printf("nonce failure hit.%llu >= threshold.%llu\n",(long long)hit,(long long)threshold);
            return(threshold - hit);
        }
        else return(0);
    }
    else startmilli = milliseconds();
    while ( hit >= threshold )
    {
        if ( rseed == 0 )
            randombytes((uint8_t *)noncep,sizeof(*noncep));
        else _randombytes((uint8_t *)noncep,sizeof(*noncep),rseed);
        hit = calc_SaM(sigp,buf,len,0,0,numrounds);
        //printf("%llu %.2f%% (%s) len.%d numrounds.%lld threshold.%llu seed.%u\n",(long long)hit,100.*(double)hit/threshold,(char *)buf,len,(long long)numrounds,(long long)threshold,rseed);
        if ( maxmillis != 0 && milliseconds() > (startmilli + maxmillis) )
            return(0);
        if ( rseed != 0 )
            rseed = (uint32_t)(sigp->txid ^ hit);
    }
    //printf("%5.1f %14llu %7.2f%% numrounds.%lld threshold.%llu seed.%u\n",milliseconds()-startmilli,(long long)hit,100.*(double)hit/threshold,(long long)numrounds,(long long)threshold,rseed);
    return(hit);
}

#endif
#endif
