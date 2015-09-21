#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "poker.h"

static CardPileType Deck, Hand;

static void InitDeck (CardPileType * deck) {
u8 i;
	deck->len = 52;
	for (i=0; i < 52; i++) deck->entry[i] = i;
}

/* char suitdisp[9] = { 0, 5, 4, 0, 3, 0, 0, 0, 6 }; */

static char suitdisp[9] = { 0, 'c', 'd', 0, 'h', 0, 0, 0, 's' };

static void DisplayCard (u8 c, char out[]) {
char s[4];

	s[0] = "        1    "[CardValue[c]];
	s[1] = "234567890JQKA"[CardValue[c]];
	s[2] = suitdisp[CardSuit[c]];
	s[3] = '\0';

    strcat (out, " ");
    strcat (out, s);
}

void DisplayHand5 (const CardPileType * h) {
char out[128];
int i;

	out[0] = '\0';
	for (i=0; i < 5; i++) DisplayCard (h->entry[i], out);
	sprintf (out + strlen (out), " => %08X\n", (int)FiveCardDrawScore (&h->entry[0]));
	printf ("%s", out);
}

void DisplayHand7 (CardPileType * h) {
char out[128];
int i, j;

	out[0] = '\0';
	for (i=0; i < 7; i++) DisplayCard (h->entry[i], out);

	i = SevenCardDrawScore (&h->entry[0]);
	j = SevenCardDrawScoreSlow (&h->entry[0]);
	if (i != j) {
		sprintf (out + strlen (out), " => %08X | %08X\n", i, j);
	} else {
		sprintf (out + strlen (out), " => %08X\n", i);
	}
	printf ("%s", out);
}

#if 0
#define DETERMINISTIC
#endif

int main () {
int c;

	srand (0);
	InitDeck (&Deck);
	Shuffle (&Deck);		/* Shuffle Deck. */

#if 1
	printf ("STRAIGHT_FLUSH_SCORE\t%08x\n",STRAIGHT_FLUSH_SCORE);
	printf ("FOUR_KIND_SCORE\t\t%08x\n",FOUR_KIND_SCORE);
	printf ("FULL_HOUSE_SCORE\t%08x\n",FULL_HOUSE_SCORE);
	printf ("FLUSH_SCORE\t\t%08x\n",FLUSH_SCORE);
	printf ("STRAIGHT_SCORE\t\t%08x\n",STRAIGHT_SCORE);
	printf ("THREE_KIND_SCORE\t%08x\n",THREE_KIND_SCORE);
	printf ("TWO_PAIR_SCORE\t\t%08x\n",TWO_PAIR_SCORE);
	printf ("TWO_KIND_SCORE\t\t%08x\n",TWO_KIND_SCORE);

	printf ("\n");

	for (c=0; c < 8; c++) {
		Deal (&Hand, &Deck, 5);
		DisplayHand5 (&Hand);
		Deal (&Deck, &Hand, 5);
		Shuffle (&Deck);
	}

	for (c=0; c < 8; c++) {
		Deal (&Hand, &Deck, 7);
		DisplayHand7 (&Hand);
		Deal (&Deck, &Hand, 7);
		Shuffle (&Deck);
	}

	printf("\nRigged hands\n\n");

	Hand.len = 5;
	Hand.entry[0] = 12;
	Hand.entry[1] = 12+13;
	Hand.entry[2] = 12+26;
	Hand.entry[3] = 12+39;
	Hand.entry[4] = 11;
	DisplayHand5 (&Hand);

	Hand.entry[0] = 12;
	Hand.entry[1] = 12+13;
	Hand.entry[2] = 12+26;
	Hand.entry[3] = 11+39;
	Hand.entry[4] = 11;
	DisplayHand5 (&Hand);

	Hand.entry[0] = 12;
	Hand.entry[1] =  0+13;
	Hand.entry[2] =  1+26;
	Hand.entry[3] =  2+39;
	Hand.entry[4] =  3;
	DisplayHand5 (&Hand);

	printf ("\n");
#endif

#ifndef DETERMINISTIC
	srand ((unsigned int) time ((void *) 0));
#endif
	Shuffle (&Deck);		/* Shuffle Deck. */
	Hand.len = 0;

	c = 0;

	printf ("7 Card Draw test\n\n");

	for (;;) {
		Deal (&Hand, &Deck, 7);
		if (SevenCardDrawScore (&Hand.entry[0]) != SevenCardDrawScoreSlow (&Hand.entry[0])) break;
		Deal (&Deck, &Hand, 7);	/* Add hand back into Deck. */
		Shuffle (&Deck);		/* Reshuffle Deck.          */
		c++;
		if ((c & 0xFFFFF) == 0) {
#ifndef DETERMINISTIC
			srand ((unsigned int) time ((void *) 0));
#endif
			printf ("%d \t(no errors)\n", c);
		}
	}

	/* There must be an error ! */

	DisplayHand7 (&Hand);
	return 0;
}
