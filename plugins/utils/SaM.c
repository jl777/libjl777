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

#define TRIT signed char

#define TRIT_FALSE 1
#define TRIT_UNKNOWN 0
#define TRIT_TRUE -1

#define SAM_HASH_SIZE 243
#define SAM_STATE_SIZE (SAM_HASH_SIZE * 3)
#define SAM_NUMBER_OF_ROUNDS 9
#define SAM_DELTA 254

#define SAMHIT_LIMIT 7625597484987 // 3 ** 27
#define MAX_CRYPTO777_HIT ((1LL << 62) / 1000)

#include "bits777.c"
#include "utils777.c"
#include "../common/system777.c"
#define MAX_INPUT_SIZE ((int32_t)(65536 - sizeof(bits256) - 2*sizeof(uint32_t)))

struct SaM_info {  bits384 bits; TRIT trits[SAM_STATE_SIZE],hash[SAM_HASH_SIZE]; };
struct SaMhdr { bits384 sig; uint32_t timestamp,nonce; uint8_t numrounds,leverage; };

void SaM_Initialize(struct SaM_info *state);
int32_t SaM_Absorb(struct SaM_info *state,const uint8_t *input,const uint32_t inputSize,const uint8_t *input2,const uint32_t inputSize2);
bits384 SaM_emit(struct SaM_info *state);
bits384 SaM_encrypt(uint8_t *dest,uint8_t *src,int32_t len,bits384 password,uint32_t timestamp);
uint64_t SaM_threshold(int32_t leverage);
uint64_t SaM(bits384 *sigp,uint8_t *input,int32_t inputSize,uint8_t *input2,int32_t inputSize2);
uint32_t SaM_nonce(void *data,int32_t datalen,int32_t leverage,int32_t maxmillis,uint32_t nonce);
//uint64_t SaMnonce(bits384 *sigp,uint32_t *noncep,uint8_t *buf,int32_t len,uint64_t threshold,uint32_t rseed,int32_t maxmillis);
#endif
#else
#ifndef crypto777_SaM_c
#define crypto777_SaM_c

#ifndef crypto777_SaM_h
#define DEFINES_ONLY
#include "SaM.c"
#undef DEFINES_ONLY
#endif

static int32_t SAM_INDICES[SAM_STATE_SIZE];

void SaM_PrepareIndices()
{
	int32_t i,nextIndex,currentIndex = 0;
	for (i=0; i<SAM_STATE_SIZE; i++)
    {
		nextIndex = (currentIndex + SAM_DELTA) % SAM_STATE_SIZE;
		SAM_INDICES[i] = nextIndex;
		currentIndex = nextIndex;
	}
}

TRIT SaM_Bias(const TRIT a, const TRIT b) { return a == 0 ? 0 : (a == -b ? a : -a); }
TRIT SaM_Sum(const TRIT a, const TRIT b) { return a == b ? -a : (a + b); }

void SaM_SplitAndMerge(struct SaM_info *state)
{
    static const TRIT SAMSUM[3][3] = { { 1, -1, 0, }, { -1, 0, 1, }, { 0, 1, -1, } };
    static const TRIT SAMBIAS[3][3] = { { 1, 1, -1, }, { 0, 0, 0, }, { 1, -1, -1, } };
	struct SaM_info leftPart,rightPart;
	int32_t i,nextIndex,round,currentIndex = 0;
	for (round=0; round<SAM_NUMBER_OF_ROUNDS; round++)
    {
		for (i=0; i<SAM_STATE_SIZE; i++)
        {
			nextIndex = SAM_INDICES[i];
			//leftPart.trits[i] = SaM_Bias(state->trits[currentIndex],state->trits[nextIndex]);
			//rightPart.trits[i] = SaM_Bias(state->trits[nextIndex],state->trits[currentIndex]);
			leftPart.trits[i] = SAMBIAS[state->trits[currentIndex]+1][1+state->trits[nextIndex]];
			rightPart.trits[i] = SAMBIAS[state->trits[nextIndex]+1][1+state->trits[currentIndex]];
			currentIndex = nextIndex;
		}
		for (i=0; i<SAM_STATE_SIZE; i++)
        {
			nextIndex = SAM_INDICES[i];
			//state->trits[i] = SaM_Sum(leftPart.trits[currentIndex],rightPart.trits[nextIndex]);
			state->trits[i] = SAMSUM[leftPart.trits[currentIndex]+1][1+rightPart.trits[nextIndex]];
			currentIndex = nextIndex;
		}
	}
}

void SaM_Initialize(struct SaM_info *state)
{
    int32_t i;
    for (i=SAM_HASH_SIZE; i<SAM_STATE_SIZE; i++)
		state->trits[i] = (i & 1) ? TRIT_FALSE : TRIT_TRUE;
}

void SaM_Squeeze(struct SaM_info *state,TRIT *output)
{
	memcpy(output,state->trits,SAM_HASH_SIZE * sizeof(TRIT));
	SaM_SplitAndMerge(state);
}

void _SaM_Absorb(struct SaM_info *state,const TRIT *input,const int32_t inputSize)
{
	int32_t size,i,remainder = inputSize;
	do
    {
		size = remainder >= SAM_HASH_SIZE ? SAM_HASH_SIZE : remainder;
		memcpy(state->trits,&input[inputSize - remainder],size);
		remainder -= SAM_HASH_SIZE;
        if ( size < SAM_HASH_SIZE )
            for (i=size; i<SAM_HASH_SIZE; i++)
                state->trits[i] = (i & 1) ? TRIT_FALSE : TRIT_TRUE;
		SaM_SplitAndMerge(state);
	} while ( remainder > 0 );
}

int32_t SaM_Absorb(struct SaM_info *state,const uint8_t *input,uint32_t inputSize,const uint8_t *input2,uint32_t inputSize2)
{
    //TRIT output[(MAX_INPUT_SIZE + sizeof(struct SaMhdr)) << 3];
    TRIT *trits,tritbuf[4096];
    int32_t i,size,n = 0;
    /*if ( inputSize + inputSize2 > sizeof(output) )
    {
        printf("SaM overflow (%d + %d) > %ld\n",inputSize,inputSize2,sizeof(output));
        if ( inputSize > MAX_INPUT_SIZE )
            inputSize = MAX_INPUT_SIZE;
        inputSize2 = 0;
    }*/
    size = (inputSize + inputSize2) << 3;
    trits = (size < sizeof(tritbuf)) ? tritbuf : malloc(size);
    if ( input != 0 && inputSize != 0 )
    {
        for (i=0; i<(inputSize << 3); i++)
            trits[n++] = ((input[i >> 3] & (1 << (i & 7))) != 0);
    }
    if ( input2 != 0 && inputSize2 != 0 )
    {
        for (i=0; i<(inputSize2 << 3); i++)
            trits[n++] = ((input2[i >> 3] & (1 << (i & 7))) != 0);
    }
    _SaM_Absorb(state,trits,n);
    if ( trits != tritbuf )
        free(trits);
    return(n);
}

static TRIT InputA[] = { 0 }; // zero len
static TRIT OutputA[] = { 1, -1, 1, 1, -1, -1, 0, -1, 0, 0, 0, 1, -1, 0, 1, 1, 0, -1, 1, 0, 0, 0, 1, 1, -1, -1, 0, 0, 1, -1, -1, 0, 0, -1, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 1, 0, 1, 0, -1, -1, -1, -1, 0, 1, -1, 1, -1, 0, 1, 1, 0, 0, -1, 0, 1, 1, -1, 1, 0, 0, 0, 1, 0, -1, 1, 1, 0, -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 0, 1, -1, 1, -1, 0, 0, 1, 1, 1, 1, -1, 1, 1, -1, 0, 0, 1, 1, 0, 0, -1, 1, 1, -1, 0, 0, -1, 0, 0, 1, 0, 0, 0, -1, 1, -1, 0, 1, -1, 0, -1, 1, 1, 1, -1, 0, 1, 1, -1, -1, 0, 0, 1, -1, -1, -1, 0, -1, -1, 1, 1, 0, 1, 0, 1, -1, 1, -1, -1, 0, 0, -1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, -1, 1, -1, 0, 0, 1, 0, -1, -1, -1, 1, -1, 1, -1, -1, 1, 0, 1, -1, 1, -1, 1, -1, 1, 0, 1, 0, 1, -1, -1, -1, -1, 1, 0, 0, -1, -1, 1, 0, 1, 1, -1, 1, -1, -1, -1, 0, 0, -1, 0, 1, 1, 1, 0, 1, 1, -1, 1, 1, 0, 1, 1, 1, 0, -1, 0, 0, -1, -1, -1 };

static TRIT InputB[] = { 0 };
static TRIT OutputB[] = { -1, -1, -1, 1, 0, 0, 1, 1, 0, 1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 1, 1, 0, -1, 1, 0, 1, 0, 1, -1, 0, -1, 0, 0, -1, 1, -1, -1, 0, 0, 1, -1, -1, 0, 0, -1, 1, 1, 0, 1, 0, 0, 1, -1, 1, 0, -1, -1, 1, -1, 0, -1, 1, -1, 0, 0, 0, 1, -1, 0, 1, -1, 1, 1, 1, 1, -1, 1, -1, -1, 1, 0, 1, -1, -1, -1, 0, 1, 0, 0, -1, 1, 1, 0, 0, -1, 1, 1, 0, -1, -1, 0, 0, 0, -1, 1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 1, 0, 1, 0, -1, 1, 0, -1, 1, 1, -1, 1, 0, 1, -1, -1, 1, 1, 0, -1, 0, -1, -1, -1, 1, -1, -1, 1, 1, 1, 1, 1, -1, -1, 1, 0, 0, 0, 0, -1, -1, 1, 1, 1, -1, 1, 0, -1, 1, 0, 1, 0, 0, -1, -1, 1, 1, 0, 0, 1, 0, 0, 0, 0, -1, 1, 0, 0, 1, 1, 0, -1, 1, -1, 1, 0, -1, 0, 0, 1, -1, -1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, -1, 1, -1, 1, 1, 1, -1, 0, 1, 0, -1, 1, 0, 1, 1, 0, -1, 1, 1, -1, 0, -1, 1, 1, 0, -1, -1, -1, -1, 1, 0, 0, -1, -1, -1, 0, 1 };

static TRIT InputC[] = { 1 };
static TRIT OutputC[] = { 1, -1, 1, 1, -1, -1, 0, -1, 0, 0, 0, 1, -1, 0, 1, 1, 0, -1, 1, 0, 0, 0, 1, 1, -1, -1, 0, 0, 1, -1, -1, 0, 0, -1, 1, -1, 0, 0, -1, -1, -1, -1, 0, 0, 0, -1, 1, 0, 1, 0, -1, -1, -1, -1, 0, 1, -1, 1, -1, 0, 1, 1, 0, 0, -1, 0, 1, 1, -1, 1, 0, 0, 0, 1, 0, -1, 1, 1, 0, -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 0, 1, -1, 1, -1, 0, 0, 1, 1, 1, 1, -1, 1, 1, -1, 0, 0, 1, 1, 0, 0, -1, 1, 1, -1, 0, 0, -1, 0, 0, 1, 0, 0, 0, -1, 1, -1, 0, 1, -1, 0, -1, 1, 1, 1, -1, 0, 1, 1, -1, -1, 0, 0, 1, -1, -1, -1, 0, -1, -1, 1, 1, 0, 1, 0, 1, -1, 1, -1, -1, 0, 0, -1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, -1, 1, -1, 0, 0, 1, 0, -1, -1, -1, 1, -1, 1, -1, -1, 1, 0, 1, -1, 1, -1, 1, -1, 1, 0, 1, 0, 1, -1, -1, -1, -1, 1, 0, 0, -1, -1, 1, 0, 1, 1, -1, 1, -1, -1, -1, 0, 0, -1, 0, 1, 1, 1, 0, 1, 1, -1, 1, 1, 0, 1, 1, 1, 0, -1, 0, 0, -1, -1, -1 };

static TRIT InputD[] = { -1 };
static TRIT OutputD[] = { -1, 0, 0, 1, 1, 0, -1, 1, 1, 0, 1, 0, -1, 1, -1, 0, 0, 1, 0, -1, 0, -1, 1, 1, 1, 1, -1, 1, -1, 1, -1, 0, 0, 0, -1, -1, 1, 1, -1, 1, -1, 0, -1, 1, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 1, -1, 1, 1, 0, -1, 1, -1, 0, 0, 1, -1, 1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 1, 0, 0, -1, 0, 0, -1, 0, 0, 1, -1, -1, -1, -1, 1, 1, 0, 0, -1, 1, -1, 1, 0, 0, -1, 1, -1, 0, 1, 1, -1, 1, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 1, -1, 0, -1, 1, -1, 1, 1, -1, -1, 0, 0, 0, -1, 1, -1, 1, -1, 1, 1, 1, 1, -1, 0, -1, 0, 1, 0, 0, -1, 1, -1, 0, 1, 0, 1, 1, -1, 0, 1, 1, 0, 0, -1, -1, -1, -1, 0, 1, 0, -1, -1, 0, 0, 1, 1, 1, 0, 0, -1, 1, -1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, -1, -1, 0, -1, -1, 0, -1, -1, 1, 0, 0, -1, -1, 1, 0, 0, 0, 1, 1, 0, -1, 1, -1, -1, 1, -1, -1, 1, 1, 0, 1, 0, 0, 0, -1, 1, 0, -1, -1, 0, 1, -1, 0, 0, 0, -1, -1, 1, 1 };

static TRIT InputE[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static TRIT OutputE[] = { 0, 1, 0, 1, -1, -1, 1, -1, -1, 0, 0, 1, 1, -1, -1, -1, 0, 1, 0, 0, -1, -1, 1, 1, 1, -1, 0, -1, -1, -1, -1, -1, 1, -1, -1, -1, 0, 0, 1, 1, 0, 1, -1, -1, 0, -1, -1, 1, 1, 1, -1, 1, 1, 0, -1, 0, 1, -1, 1, -1, 1, 1, -1, 1, 0, -1, -1, -1, 0, 0, 1, 1, 0, -1, 0, 0, -1, 0, 0, 1, 1, -1, 0, 1, -1, -1, 1, -1, 1, -1, 0, 1, -1, 1, 0, 1, -1, -1, -1, 0, 1, -1, 0, 1, -1, 1, 0, -1, 1, -1, 1, 0, -1, -1, 1, 0, 1, 0, 0, 1, 1, 1, -1, 1, -1, -1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, -1, 1, 0, 0, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, 1, 0, 0, 1, 0, -1, -1, 0, 0, -1, -1, 1, -1, 0, -1, 1, -1, 0, 1, -1, 0, 1, 1, -1, 1, -1, 1, -1, 0, 0, 0, -1, 0, -1, 1, -1, 1, 1, 1, 1, 1, 0, -1, 0, -1, -1, 0, 0, -1, -1, 1, -1, -1, -1, 1, 0, 0, 0, 1, 0, 1, 0, 1, -1, 0, -1, -1, 1, -1, -1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, -1, -1, -1, -1, 1, 0, -1, 0, 0 };

bits384 SaM_emit(struct SaM_info *state)
{
    // i.12 531441 81bf1 0.68% numbits.19 mask.7ffff -> bias -0.0005870312
    TRIT *ptr;
    uint64_t bits64;
    uint32_t i,j,rawbits,bits19[20],mask = 0x7ffff;
	SaM_Squeeze(state,state->hash);
    ptr = state->hash;
    for (i=0; i<SAM_HASH_SIZE/12; i++)
    {
        for (j=rawbits=0; j<12; j++)
            rawbits = (rawbits * 3 + *ptr++ + 1);
        bits19[i] = ((((uint64_t)rawbits<<19)/531441) & mask); // 3^12 == 531441 //bits19[i] = (rawbits & mask);
        //printf("%05x ",bits19[i]);
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

int32_t _SaM_test(char *msg,TRIT *testvector,int32_t n,TRIT *checkvals)
{
    struct SaM_info state; int32_t i,errs;
    SaM_Initialize(&state);
    _SaM_Absorb(&state,testvector,n);
    SaM_emit(&state);
    for (i=errs=0; i<243; i++)
    {
        if ( state.hash[i] != checkvals[i] )
            errs++;
    }
    if ( errs != 0 )
    {
        for (i=0; i<243; i++)
            printf("%2d, ",state.hash[i]);
        printf("\nSaM_test.%s errs.%d vs output\n",msg,errs);
    }
    return(errs);
}

int32_t SaM_test()
{
    int32_t i,j,wt,iter,totalset,totalclr,setcount[48*8],clrcount[48*8],histo[16]; bits256 seed;
    struct SaM_info state;
    uint8_t buf[4096*2],bits[2][10][48];
    double startmilli = milliseconds();
    for (i=0; i<1000; i++)
    {
        _SaM_test("A",InputA,0,OutputA);
        _SaM_test("B",InputB,sizeof(InputB),OutputB);
        _SaM_test("C",InputC,sizeof(InputC),OutputC);
        _SaM_test("D",InputD,sizeof(InputD),OutputD);
        _SaM_test("E",InputE,sizeof(InputE),OutputE);
    }
    printf("per SaM %.3f\n",(milliseconds() - startmilli) / (5 * i));
    memset(seed.bytes,0,sizeof(seed));
    memcpy(seed.bytes,(uint8_t *)"12345678901",11);
    for (i=0; i<243*2; i++)
        buf[i] = 0;
    randombytes(buf,sizeof(buf));
    for (iter=0; iter<2; iter++)
    {
        memset(&state,0,sizeof(state));
        SaM_Initialize(&state);
        SaM_Absorb(&state,buf,243*2,0,0);
        memset(setcount,0,sizeof(setcount));
        memset(clrcount,0,sizeof(clrcount));
        memset(histo,0,sizeof(histo));
        for (i=0; i<5; i++)
        {
            if ( 0 && (i % 100) == 99 )
            {
                for (j=0; j<32; j++)
                    seed.bytes[j] = rand() >> 8;
                SaM_Absorb(&state,seed.bytes,sizeof(seed),0,0);
            }
            memset(bits[iter][i],0,sizeof(bits[iter][i]));
            SaM_emit(&state);
            memcpy(bits[iter][i],state.bits.bytes,sizeof(bits[iter][i]));
            for (j=0; j<48; j++)
            {
                histo[bits[iter][i][j] & 0xf]++;
                histo[(bits[iter][i][j]>>4) & 0xf]++;
                printf("%02x ",bits[iter][i][j]);
            }
            printf("\n");
            for (j=0; j<48*8; j++)
            {
                if ( GETBIT(bits[iter][i],j) != 0 )
                    setcount[j]++;
                else clrcount[j]++;
            }
        }
        for (i=0; i<16; i++)
            printf("%8d ",histo[i]);
        printf("hex histogram\n");
        seed.bytes[0] ^= 1;
        buf[0] ^= 1;
    }
    for (i=0; i<5; i++)
    {
        for (j=wt=0; j<48; j++)
        {
            wt += bitweight(bits[0][i][j] ^ bits[1][i][j]);
            printf("%02x",bits[0][i][j] ^ bits[1][i][j]);
        }
        printf(" i.%d diff.%d\n",i,wt);
    }
    //set.19090245 clr.19309755 -0.0057
    //total set.19200072 clr.19199928 0.0000037500
    // total set.19191713 clr.19208287 -0.0004316146
    for (totalset=totalclr=j=0; j<48*8; j++)
    {
        totalset += setcount[j];
        totalclr += clrcount[j];
        printf("%.2f ",(double)(setcount[j]-clrcount[j])/i);
    }
    printf("total set.%d clr.%d %.10f\n",totalset,totalclr,(double)(totalset-totalclr)/(totalset+totalclr));
    return(0);
}

bits384 SaM_encrypt(uint8_t *dest,uint8_t *src,int32_t len,bits384 password,uint32_t timestamp)
{
    bits384 xorpad; int32_t i;  struct SaM_info XORpad;
    SaM_Initialize(&XORpad), SaM_Absorb(&XORpad,password.bytes,sizeof(password),(void *)&timestamp,sizeof(timestamp));
    while ( len >= 0 )
    {
        SaM_emit(&XORpad);
        for (i=0; i<sizeof(xorpad) && len>=0; i++,len--)
        {
            xorpad.bytes[i] = (XORpad.bits.bytes[i] ^ *src++);
            if ( dest != 0 )
                *dest++ = xorpad.bytes[i];
        }
    }
    return(xorpad);
}

uint64_t SaM_hit(struct SaM_info *state)
{
    int32_t i; uint64_t hit = 0;
    for (i=0; i<27; i++)
 		hit = (hit * 3 + state->hash[i] + 1);
    return(hit);
}

uint64_t SaM(bits384 *sigp,uint8_t *input,int32_t inputSize,uint8_t *input2,int32_t inputSize2)
{
    int32_t verify_SaM(TRIT *newhash,uint8_t *buf,const int n);
    struct SaM_info state;
    SaM_Initialize(&state);
    SaM_Absorb(&state,input,inputSize,input2,inputSize2);
    //printf("len.%d: ",inputSize+inputSize2);
    *sigp = SaM_emit(&state);
    //if ( 0 && input2 == 0 && numrounds == SAM_MAGIC_NUMBER )
    //    verify_SaM(state.hash,(uint8_t *)input,inputSize);
    return(SaM_hit(&state));
}

uint64_t SaM_threshold(int32_t leverage)
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

uint32_t SaM_nonce(void *data,int32_t datalen,int32_t leverage,int32_t maxmillis,uint32_t nonce)
{
    uint64_t hit,threshold; bits384 sig; double endmilli;
    if ( leverage != 0 )
    {
        threshold = SaM_threshold(leverage);
        if ( maxmillis == 0 )
        {
            if ( (hit= SaM(&sig,data,datalen,(void *)&nonce,sizeof(nonce))) >= threshold )
            {
                printf("nonce failure hit.%llu >= threshold.%llu\n",(long long)hit,(long long)threshold);
                if ( (threshold - hit) > ((uint64_t)1L << 32) )
                    return(0xffffffff);
                else return((uint32_t)(threshold - hit));
            }
        }
        else
        {
            endmilli = (milliseconds() + maxmillis);
            while ( milliseconds() < endmilli )
            {
                randombytes((void *)&nonce,sizeof(nonce));
                if ( (hit= SaM(&sig,data,datalen,(void *)&nonce,sizeof(nonce))) < threshold )
                    return(nonce);
            }
        }
    }
    return(0);
}

/*uint64_t SaMnonce(bits384 *sigp,uint32_t *noncep,uint8_t *buf,int32_t len,uint64_t threshold,uint32_t rseed,int32_t maxmillis)
{
    uint64_t hit = SAMHIT_LIMIT;
    double startmilli = 0;
    if ( maxmillis == 0 )
    {
        hit = calc_SaM(sigp,buf,len,0,0);
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
        hit = calc_SaM(sigp,buf,len,0,0);
        //printf("%llu %.2f%% (%s) len.%d numrounds.%lld threshold.%llu seed.%u\n",(long long)hit,100.*(double)hit/threshold,(char *)buf,len,(long long)numrounds,(long long)threshold,rseed);
        if ( maxmillis != 0 && milliseconds() > (startmilli + maxmillis) )
            return(0);
        if ( rseed != 0 )
            rseed = (uint32_t)(sigp->txid ^ hit);
    }
    //printf("%5.1f %14llu %7.2f%% numrounds.%lld threshold.%llu seed.%u\n",milliseconds()-startmilli,(long long)hit,100.*(double)hit/threshold,(long long)numrounds,(long long)threshold,rseed);
    return(hit);
}*/

#endif
#endif
