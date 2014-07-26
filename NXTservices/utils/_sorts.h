//
//  _sorts.h
//  only meant as include from sorts.h
//
//  Created by jl777 on 7/25/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

void sortnetwork(sorttype *sortbuf,int num,int dir)
{
	sorttype A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A30,A31,A32,A33,tmp;
	sorttype A10,A11,A12,A13,A14,A15,A16,A17,A18,A19;
	sorttype A20,A21,A22,A23,A24,A25,A26,A27,A28,A29;
    A0 = A1 = A2 = A3 = A4 = A5 = A6 = A7 = A8 = A9 = A10 = A11 = A12 = A13 = A14 = A15 = A16 =
    A17 = A18 = A19 = A20 = A21 = A22 = A23 = A24 = A25 = A26 = A27 = A28 = A29 = A30 = A31 = A32 = A33 = 0;
	// jl777: validate edge case of num.34
	switch ( num-1 )
	{
		default:
		case 33: A33 = sortbuf[33];
		case 32: if ( dir*sortbuf[32] < dir*A33 ) A32 = A33; else A32 = sortbuf[32];
		case 31: if ( dir*sortbuf[31] < dir*A32 ) A31 = A32; else A31 = sortbuf[31]; num = 32;
		case 30: A30 = sortbuf[30];
		case 29: A29 = sortbuf[29];
		case 28: A28 = sortbuf[28];
		case 27: A27 = sortbuf[27];
		case 26: A26 = sortbuf[26];
		case 25: A25 = sortbuf[25];
		case 24: A24 = sortbuf[24];
		case 23: A23 = sortbuf[23];
		case 22: A22 = sortbuf[22];
		case 21: A21 = sortbuf[21];
		case 20: A20 = sortbuf[20];
		case 19: A19 = sortbuf[19];
		case 18: A18 = sortbuf[18];
		case 17: A17 = sortbuf[17];
		case 16: A16 = sortbuf[16];
		case 15: A15 = sortbuf[15];
		case 14: A14 = sortbuf[14];
		case 13: A13 = sortbuf[13];
		case 12: A12 = sortbuf[12];
		case 11: A11 = sortbuf[11];
		case 10: A10 = sortbuf[10];
		case 9: A9 = sortbuf[9];
		case 8: A8 = sortbuf[8];
		case 7: A7 = sortbuf[7];
		case 6: A6 = sortbuf[6];
		case 5: A5 = sortbuf[5];
		case 4: A4 = sortbuf[4];
		case 3: A3 = sortbuf[3];
		case 2: A2 = sortbuf[2];
		case 1: A1 = sortbuf[1];
		case 0: A0 = sortbuf[0];
	}
	if ( dir < 0 )
	{
#define CSWAP(i,j) if ( i > j ) { tmp = i; i = j; j = tmp; }
#include "sortnetworks.h"
#undef CSWAP
	}
	else
	{
#define CSWAP(i,j) if ( i < j ) { tmp = i; i = j; j = tmp; }
#include "sortnetworks.h"
#undef CSWAP
	}
	switch ( num-1 )
	{
		default:
		case 33: sortbuf[33] = A31;
		case 32: sortbuf[32] = A31;
		case 31: sortbuf[31] = A31;
		case 30: sortbuf[30] = A30;
		case 29: sortbuf[29] = A29;
		case 28: sortbuf[28] = A28;
		case 27: sortbuf[27] = A27;
		case 26: sortbuf[26] = A26;
		case 25: sortbuf[25] = A25;
		case 24: sortbuf[24] = A24;
		case 23: sortbuf[23] = A23;
		case 22: sortbuf[22] = A22;
		case 21: sortbuf[21] = A21;
		case 20: sortbuf[20] = A20;
		case 19: sortbuf[19] = A19;
		case 18: sortbuf[18] = A18;
		case 17: sortbuf[17] = A17;
		case 16: sortbuf[16] = A16;
		case 15: sortbuf[15] = A15;
		case 14: sortbuf[14] = A14;
		case 13: sortbuf[13] = A13;
		case 12: sortbuf[12] = A12;
		case 11: sortbuf[11] = A11;
		case 10: sortbuf[10] = A10;
		case 9: sortbuf[9] = A9;
		case 8: sortbuf[8] = A8;
		case 7: sortbuf[7] = A7;
		case 6: sortbuf[6] = A6;
		case 5: sortbuf[5] = A5;
		case 4: sortbuf[4] = A4;
		case 3: sortbuf[3] = A3;
		case 2: sortbuf[2] = A2;
		case 1: sortbuf[1] = A1;
		case 0: sortbuf[0] = A0;
	}
}
