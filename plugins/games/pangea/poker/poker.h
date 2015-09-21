#ifndef POKER_INCLUDED
#define POKER_INCLUDED

#define CLUB_SUIT              (1)
#define DIAMOND_SUIT           (2)
#define HEART_SUIT             (4)
#define SPADE_SUIT             (8)

#define RANK_SHL              (27)
#define SUBR_SHL              (13)

#define STRAIGHT_FLUSH_SCORE  (8 << RANK_SHL)
#define FOUR_KIND_SCORE       (7 << RANK_SHL)
#define FULL_HOUSE_SCORE      (6 << RANK_SHL)
#define FLUSH_SCORE           (5 << RANK_SHL)
#define STRAIGHT_SCORE        (4 << RANK_SHL)
#define THREE_KIND_SCORE      (3 << RANK_SHL)
#define TWO_PAIR_SCORE        (2 << RANK_SHL)
#define TWO_KIND_SCORE        (1 << RANK_SHL)

#define ONE_PAIR_SCORE        (TWO_KIND_SCORE)

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned long u16;
typedef signed long s16;
typedef unsigned long u32;
typedef signed long s32;

typedef struct {
    int len;
    u8 entry[52];
} CardPileType;

#ifdef __cplusplus
extern "C" {
#endif

extern u32 CardValue[52];
extern u32 CardSuit[52];
extern u32 CardMask[52];

extern  u32 SevenCardDrawScoreSlow (const u8 * h);
extern  u32 SevenCardDrawScore (const u8 * h);
extern  u32 FiveCardDrawScore (const u8 * h);
extern void Shuffle (CardPileType * c);
extern  int Deal (CardPileType * h, CardPileType * d, int n);

#ifdef __cplusplus
}
#endif
#endif
