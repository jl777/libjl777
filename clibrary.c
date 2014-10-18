#include "picoc.h"
#include "interpreter.h"

/* the picoc version string */
static const char *VersionString = NULL;

/* endian-ness checking */
static const int __ENDIAN_CHECK__ = 1;

static int BigEndian = 0;
static int LittleEndian = 0;

static void *pM1,*pM2,*pM3,*pM4,*pM5,*pM10,*pM15,*pM30,*pH1;
static void *_M1,*_M2,*_M3,*_M4,*_M5,*_M10,*_M15,*_M30,*_H1;
static void *_inv_M1,*_inv_M2,*_inv_M3,*_inv_M4,*_inv_M5,*_inv_M10,*_inv_M15,*_inv_M30,*_inv_H1;
static void *pBids,*pAsks,*_Bids,*_Asks,*_inv_Bids,*_inv_Asks;
static void *pBidvols,*pAskvols,*_Bidvols,*_Askvols,*_inv_Bidvols,*_inv_Askvols;
static int Numbids,Numasks,_Numbids,_Numasks,Maxbars,Changed,Event;
static uint32_t Jdatetime;
static uint64_t *pBidnxt,*_Bidnxt,*pAsknxt,*_Asknxt;
static char *Base,*Rel,*_Base,*_Rel,*Exchange;

void set_jl777vars(uint32_t jdatetime,int event,int changed,char *exchange,char *base,char *rel,double *bids,double *inv_bids,int numbids,double *asks,double *inv_asks,int numasks,float *m1,float *m2,float *m3,float *m4,float *m5,float *m10,float *m15,float *m30,float *h1,float *inv_m1,float *inv_m2,float *inv_m3,float *inv_m4,float *inv_m5,float *inv_m10,float *inv_m15,float *inv_m30,float *inv_h1,int maxbars,double *bidvols,double *inv_bidvols,double *askvols,double *inv_askvols,uint64_t *bidnxt,uint64_t *asknxt)
{
    Jdatetime = jdatetime; Event = event; Changed = changed; Exchange = exchange; _Base = base; _Rel = rel;
    _Bids = bids; _inv_Bids = inv_bids; _Numbids = numbids; _Asks = asks; _inv_Asks = inv_asks; _Numasks = numasks;
    _Bidvols = bidvols; _inv_Bidvols = inv_bidvols; _Askvols = askvols; _inv_Askvols = inv_askvols;
    _Bidnxt = bidnxt; _Asknxt = asknxt;
    _M1 = m1; _M2 = m2; _M3 = m3; _M4 = m4; _M5 = m5; _M10 = m10; _M30 = m30; _H1 = h1; Maxbars = maxbars;
    _inv_M1 = inv_m1; _inv_M2 = inv_m2; _inv_M3 = inv_m3; _inv_M4 = inv_m4; _inv_M5 = inv_m5; _inv_M10 = inv_m10; _inv_M30 = inv_m30; _inv_H1 = inv_h1;
    //printf("top of orderbook (%f %f %f %f) (%p %p %p %p)\n",bids[0],inv_bids[0],asks[0],inv_asks[0],bids,inv_bids,asks,inv_asks);
    //pBids = _Bids; pAsks = _Asks; pBidvols = _Bidvols; pAskvols = _Askvols;
}

void Initjl777vars()
{
    const char *definition = "\
#define TRADEBOT_PRICECHANGE 1\n\
#define TRADEBOT_NEWMINUTE 2\n\
\
#define BARI_FIRSTBID 0\n\
#define BARI_FIRSTASK 1\n\
#define BARI_LOWBID 2\n\
#define BARI_HIGHASK 3\n\
#define BARI_HIGHBID 4\n\
#define BARI_LOWASK 5\n\
#define BARI_LASTBID 6\n\
#define BARI_LASTASK 7\n\
\
#define BARI_ARBBID 8\n\
#define BARI_ARBASK 9\n\
#define BARI_MINBID 10\n\
#define BARI_MAXASK 11\n\
#define BARI_VIRTBID 10\n\
#define BARI_VIRTASK 11\n\
#define BARI_AVEBID 12\n\
#define BARI_AVEASK 13\n\
#define BARI_MEDIAN 14\n\
#define BARI_AVEPRICE 15\n\
\
#define Bid(i) ((double *)pBids)[i]\n\
#define Ask(i) ((double *)pAsks)[i]\n\
#define Bidvol(i) ((double *)pBidvols)[i]\n\
#define Askvol(i) ((double *)pAskvols)[i]\n\
#define Bidid(i) ((unsigned long *)pBidnxt)[i]\n\
#define Askid(i) ((unsigned long *)pAsknxt)[i]\n\
#define M1(i,bari) ((float *)pM1)[((i) << 4) + (bari)]\n\
#define M2(i,bari) ((float *)pM2)[((i) << 4) + (bari)]\n\
#define M3(i,bari) ((float *)pM3)[((i) << 4) + (bari)]\n\
#define M4(i,bari) ((float *)pM4)[((i) << 4) + (bari)]\n\
#define M5(i,bari) ((float *)pM5)[((i) << 4) + (bari)]\n\
#define M10(i,bari) ((float *)pM10)[((i) << 4) + (bari)]\n\
#define M15(i,bari) ((float *)pM15)[((i) << 4) + (bari)]\n\
#define M30(i,bari) ((float *)pM30)[((i) << 4) + (bari)]\n\
#define H1(i,bari) ((float *)pH1)[((i) << 4) + (bari)]\n\
\
int init_PTL(int *eventp,int *changedp,char *exchange,char *base,char *rel)\n\
{\n\
    *eventp = Event; *changedp = Changed;\n\
    if ( exchange != 0 && strcmp(exchange,Exchange) != 0 )\n\
        return(0);\n\
    if ( strcmp(base,_Base) == 0 && strcmp(rel,_Rel) == 0 )\n\
    {\n\
        pBids = _Bids; Numbids = _Numbids; pAsks = _Asks; Numasks = _Numasks; Base = _Base; Rel = _Rel;\n\
        pBidvols = _Bidvols; pBidnxt = _Bidnxt; pAskvols = _Askvols; pAsknxt = _Asknxt;\n\
        pM1 = _M1; pM2 = _M2; pM3 = _M3; pM4 = _M4; pM5 = _M5; pM10 = _M10; pM15 = _M15; pM30 = _M30; pH1 = _H1;\n\
        return(1);\n\
    }\n\
    else if ( strcmp(base,_Rel) == 0 && strcmp(rel,_Base) == 0 )\n\
    {\n\
        pBids = _inv_Bids; Numbids = _Numasks; pAsks = _inv_Asks; Numasks = _Numbids; Base = _Rel; Rel = _Base;\n\
        pBidvols = _inv_Bidvols; pBidnxt = _Asknxt; pAskvols = _inv_Askvols; pAsknxt = _Bidnxt;\n\
        pM1 = _inv_M1; pM2 = _inv_M2; pM3 = _inv_M3; pM4 = _inv_M4; pM5 = _inv_M5; pM10 = _inv_M10; pM15 = _inv_M15; pM30 = _inv_M30; pH1 = _inv_H1;\n\
        return(-1);\n\
    }\n\
    else return(0);\n\
}\
";

    //printf("PicoParse(%s)\n",definition);
    PicocParse("jl777lib",definition,(int)strlen(definition),TRUE,TRUE,FALSE);
    VariableDefinePlatformVar(NULL,"Exchange",CharPtrType,(union AnyValue *)&Exchange,FALSE);
    VariableDefinePlatformVar(NULL,"Jdatetime",&IntType,(union AnyValue *)&Jdatetime,FALSE);
    VariableDefinePlatformVar(NULL,"Changed",&IntType,(union AnyValue *)&Changed,FALSE);
    VariableDefinePlatformVar(NULL,"Event",&IntType,(union AnyValue *)&Event,FALSE);
    VariableDefinePlatformVar(NULL,"Maxbars",&IntType,(union AnyValue *)&Maxbars,FALSE);
    
    VariableDefinePlatformVar(NULL,"Numbids",&IntType,(union AnyValue *)&Numbids,TRUE);
    VariableDefinePlatformVar(NULL,"Numasks",&IntType,(union AnyValue *)&Numasks,TRUE);
    VariableDefinePlatformVar(NULL,"_Numbids",&IntType,(union AnyValue *)&_Numbids,FALSE);
    VariableDefinePlatformVar(NULL,"_Numasks",&IntType,(union AnyValue *)&_Numasks,FALSE);
  
    VariableDefinePlatformVar(NULL,"Base",CharPtrType,(union AnyValue *)&Base,TRUE);
    VariableDefinePlatformVar(NULL,"Rel",CharPtrType,(union AnyValue *)&Rel,TRUE);
    VariableDefinePlatformVar(NULL,"_Base",CharPtrType,(union AnyValue *)&_Base,FALSE);
    VariableDefinePlatformVar(NULL,"_Rel",CharPtrType,(union AnyValue *)&_Rel,FALSE);
 
    VariableDefinePlatformVar(NULL,"pBidnxt",VoidPtrType,(union AnyValue *)&pBidnxt,TRUE);
    VariableDefinePlatformVar(NULL,"pAsknxt",VoidPtrType,(union AnyValue *)&pAsknxt,TRUE);
    VariableDefinePlatformVar(NULL,"_Bidnxt",VoidPtrType,(union AnyValue *)&_Bidnxt,FALSE);
    VariableDefinePlatformVar(NULL,"_Asknxt",VoidPtrType,(union AnyValue *)&_Asknxt,FALSE);

    VariableDefinePlatformVar(NULL,"pBidvols",VoidPtrType,(union AnyValue *)&pBidvols,TRUE);
    VariableDefinePlatformVar(NULL,"pAskvols",VoidPtrType,(union AnyValue *)&pAskvols,TRUE);
    VariableDefinePlatformVar(NULL,"_Bidvols",VoidPtrType,(union AnyValue *)&_Bidvols,FALSE);
    VariableDefinePlatformVar(NULL,"_Askvols",VoidPtrType,(union AnyValue *)&_Askvols,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_Bidvols",VoidPtrType,(union AnyValue *)&_inv_Bidvols,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_Askvols",VoidPtrType,(union AnyValue *)&_inv_Askvols,FALSE);

    VariableDefinePlatformVar(NULL,"pM1",VoidPtrType,(union AnyValue *)&pM1,TRUE);
    VariableDefinePlatformVar(NULL,"pM2",VoidPtrType,(union AnyValue *)&pM2,TRUE);
    VariableDefinePlatformVar(NULL,"pM3",VoidPtrType,(union AnyValue *)&pM3,TRUE);
    VariableDefinePlatformVar(NULL,"pM4",VoidPtrType,(union AnyValue *)&pM4,TRUE);
    VariableDefinePlatformVar(NULL,"pM5",VoidPtrType,(union AnyValue *)&pM5,TRUE);
    VariableDefinePlatformVar(NULL,"pM10",VoidPtrType,(union AnyValue *)&pM10,TRUE);
    VariableDefinePlatformVar(NULL,"pM15",VoidPtrType,(union AnyValue *)&pM15,TRUE);
    VariableDefinePlatformVar(NULL,"pM30",VoidPtrType,(union AnyValue *)&pM30,TRUE);
    VariableDefinePlatformVar(NULL,"pH1",VoidPtrType,(union AnyValue *)&pH1,TRUE);
    VariableDefinePlatformVar(NULL,"pBids",VoidPtrType,(union AnyValue *)&pBids,TRUE);
    VariableDefinePlatformVar(NULL,"pAsks",VoidPtrType,(union AnyValue *)&pAsks,TRUE);
    
    VariableDefinePlatformVar(NULL,"_M1",VoidPtrType,(union AnyValue *)&_M1,FALSE);
    VariableDefinePlatformVar(NULL,"_M2",VoidPtrType,(union AnyValue *)&_M2,FALSE);
    VariableDefinePlatformVar(NULL,"_M3",VoidPtrType,(union AnyValue *)&_M3,FALSE);
    VariableDefinePlatformVar(NULL,"_M4",VoidPtrType,(union AnyValue *)&_M4,FALSE);
    VariableDefinePlatformVar(NULL,"_M5",VoidPtrType,(union AnyValue *)&_M5,FALSE);
    VariableDefinePlatformVar(NULL,"_M10",VoidPtrType,(union AnyValue *)&_M10,FALSE);
    VariableDefinePlatformVar(NULL,"_M15",VoidPtrType,(union AnyValue *)&_M15,FALSE);
    VariableDefinePlatformVar(NULL,"_M30",VoidPtrType,(union AnyValue *)&_M30,FALSE);
    VariableDefinePlatformVar(NULL,"_H1",VoidPtrType,(union AnyValue *)&_H1,FALSE);
    VariableDefinePlatformVar(NULL,"_Bids",VoidPtrType,(union AnyValue *)&_Bids,FALSE);
    VariableDefinePlatformVar(NULL,"_Asks",VoidPtrType,(union AnyValue *)&_Asks,FALSE);
    
    VariableDefinePlatformVar(NULL,"_inv_M1",VoidPtrType,(union AnyValue *)&_inv_M1,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_M2",VoidPtrType,(union AnyValue *)&_inv_M2,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_M3",VoidPtrType,(union AnyValue *)&_inv_M3,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_M4",VoidPtrType,(union AnyValue *)&_inv_M4,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_M5",VoidPtrType,(union AnyValue *)&_inv_M5,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_M10",VoidPtrType,(union AnyValue *)&_inv_M10,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_M15",VoidPtrType,(union AnyValue *)&_inv_M15,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_M30",VoidPtrType,(union AnyValue *)&_inv_M30,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_H1",VoidPtrType,(union AnyValue *)&_inv_H1,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_Bids",VoidPtrType,(union AnyValue *)&_inv_Bids,FALSE);
    VariableDefinePlatformVar(NULL,"_inv_Asks",VoidPtrType,(union AnyValue *)&_inv_Asks,FALSE);
    //printf("Parsed pBids.%p %f  pAsks.%p %f | _Bids %p _Asks %p | _inv_Bids %p _inv_Asks %p\n",pBids,((double *)_Bids)[0],pAsks,((double *)_Asks)[0],_Bids,_Asks,_inv_Bids,_inv_Asks);
}

/* global initialisation for libraries */
void LibraryInit()
{
    /* define the version number macro */
    VersionString = TableStrRegister(PICOC_VERSION);
    VariableDefinePlatformVar(NULL, "PICOC_VERSION", CharPtrType, (union AnyValue *)&VersionString, FALSE);

    /* define endian-ness macros */
    BigEndian = ((*(char*)&__ENDIAN_CHECK__) == 0);
    LittleEndian = ((*(char*)&__ENDIAN_CHECK__) == 1);

    VariableDefinePlatformVar(NULL, "BIG_ENDIAN", &IntType, (union AnyValue *)&BigEndian, FALSE);
    VariableDefinePlatformVar(NULL, "LITTLE_ENDIAN", &IntType, (union AnyValue *)&LittleEndian, FALSE);
}

/* add a library */
void LibraryAdd(struct Table *GlobalTable, const char *LibraryName, struct LibraryFunction *FuncList)
{
    struct ParseState Parser;
    int Count;
    char *Identifier;
    struct ValueType *ReturnType;
    struct Value *NewValue;
    void *Tokens;
    const char *IntrinsicName = TableStrRegister("c library");
    
    /* read all the library definitions */
    for (Count = 0; FuncList[Count].Prototype != NULL; Count++)
    {
        Tokens = LexAnalyse(IntrinsicName, FuncList[Count].Prototype, (int)strlen((char *)FuncList[Count].Prototype), NULL);
        LexInitParser(&Parser, FuncList[Count].Prototype, Tokens, IntrinsicName, TRUE);
        TypeParse(&Parser, &ReturnType, &Identifier, NULL);
        NewValue = ParseFunctionDefinition(&Parser, ReturnType, Identifier);
        NewValue->Val->FuncDef.Intrinsic = FuncList[Count].Func;
        HeapFreeMem(Tokens);
    }
}

/* print a type to a stream without using printf/sprintf */
void PrintType(struct ValueType *Typ, IOFILE *Stream)
{
    switch (Typ->Base)
    {
        case TypeVoid:          PrintStr("void", Stream); break;
        case TypeInt:           PrintStr("int", Stream); break;
        case TypeShort:         PrintStr("short", Stream); break;
        case TypeChar:          PrintStr("char", Stream); break;
        case TypeLong:          PrintStr("long", Stream); break;
        case TypeUnsignedInt:   PrintStr("unsigned int", Stream); break;
        case TypeUnsignedShort: PrintStr("unsigned short", Stream); break;
        case TypeUnsignedLong:  PrintStr("unsigned long", Stream); break;
#ifndef NO_FP
        case TypeFP:            PrintStr("double", Stream); break;
#endif
        case TypeFunction:      PrintStr("function", Stream); break;
        case TypeMacro:         PrintStr("macro", Stream); break;
        case TypePointer:       if (Typ->FromType) PrintType(Typ->FromType, Stream); PrintCh('*', Stream); break;
        case TypeArray:         PrintType(Typ->FromType, Stream); PrintCh('[', Stream); if (Typ->ArraySize != 0) PrintSimpleInt(Typ->ArraySize, Stream); PrintCh(']', Stream); break;
        case TypeStruct:        PrintStr("struct ", Stream); PrintStr(Typ->Identifier, Stream); break;
        case TypeUnion:         PrintStr("union ", Stream); PrintStr(Typ->Identifier, Stream); break;
        case TypeEnum:          PrintStr("enum ", Stream); PrintStr(Typ->Identifier, Stream); break;
        case TypeGotoLabel:     PrintStr("goto label ", Stream); break;
        case Type_Type:         PrintStr("type ", Stream); break;
    }
}


#ifdef BUILTIN_MINI_STDLIB

/* 
 * This is a simplified standard library for small embedded systems. It doesn't require
 * a system stdio library to operate.
 *
 * A more complete standard library for larger computers is in the library_XXX.c files.
 */
 
IOFILE *CStdOut;
IOFILE CStdOutBase;

static int TRUEValue = 1;
static int ZeroValue = 0;

void BasicIOInit()
{
    CStdOutBase.Putch = &PlatformPutc;
    CStdOut = &CStdOutBase;
}

/* initialise the C library */
void CLibraryInit()
{
    /* define some constants */
    VariableDefinePlatformVar(NULL, "NULL", &IntType, (union AnyValue *)&ZeroValue, FALSE);
    VariableDefinePlatformVar(NULL, "TRUE", &IntType, (union AnyValue *)&TRUEValue, FALSE);
    VariableDefinePlatformVar(NULL, "FALSE", &IntType, (union AnyValue *)&ZeroValue, FALSE);
}

/* stream for writing into strings */
void SPutc(unsigned char Ch, union OutputStreamInfo *Stream)
{
    struct StringOutputStream *Out = &Stream->Str;
    *Out->WritePos++ = Ch;
}

/* print a character to a stream without using printf/sprintf */
void PrintCh(char OutCh, struct OutputStream *Stream)
{
    (*Stream->Putch)(OutCh, &Stream->i);
}

/* print a string to a stream without using printf/sprintf */
void PrintStr(const char *Str, struct OutputStream *Stream)
{
    while (*Str != 0)
        PrintCh(*Str++, Stream);
}

/* print a single character a given number of times */
void PrintRepeatedChar(char ShowChar, int Length, struct OutputStream *Stream)
{
    while (Length-- > 0)
        PrintCh(ShowChar, Stream);
}

/* print an unsigned integer to a stream without using printf/sprintf */
void PrintUnsigned(unsigned long Num, unsigned int Base, int FieldWidth, int ZeroPad, int LeftJustify, struct OutputStream *Stream)
{
    char Result[33];
    int ResPos = sizeof(Result);

    Result[--ResPos] = '\0';
    if (Num == 0)
        Result[--ResPos] = '0';
            
    while (Num > 0)
    {
        unsigned long NextNum = Num / Base;
        unsigned long Digit = Num - NextNum * Base;
        if (Digit < 10)
            Result[--ResPos] = '0' + Digit;
        else
            Result[--ResPos] = 'a' + Digit - 10;
        
        Num = NextNum;
    }
    
    if (FieldWidth > 0 && !LeftJustify)
        PrintRepeatedChar(ZeroPad ? '0' : ' ', FieldWidth - (sizeof(Result) - 1 - ResPos), Stream);
        
    PrintStr(&Result[ResPos], Stream);

    if (FieldWidth > 0 && LeftJustify)
        PrintRepeatedChar(' ', FieldWidth - (sizeof(Result) - 1 - ResPos), Stream);
}

/* print an integer to a stream without using printf/sprintf */
void PrintSimpleInt(long Num, struct OutputStream *Stream)
{
    PrintInt(Num, -1, FALSE, FALSE, Stream);
}

/* print an integer to a stream without using printf/sprintf */
void PrintInt(long Num, int FieldWidth, int ZeroPad, int LeftJustify, struct OutputStream *Stream)
{
    if (Num < 0)
    {
        PrintCh('-', Stream);
        Num = -Num;
        if (FieldWidth != 0)
            FieldWidth--;
    }
    
    PrintUnsigned((unsigned long)Num, 10, FieldWidth, ZeroPad, LeftJustify, Stream);
}

#ifndef NO_FP
/* print a double to a stream without using printf/sprintf */
void PrintFP(double Num, struct OutputStream *Stream)
{
    int Exponent = 0;
    int MaxDecimal;
    
    if (Num < 0)
    {
        PrintCh('-', Stream);
        Num = -Num;    
    }
    
    if (Num >= 1e7)
        Exponent = log10(Num);
    else if (Num <= 1e-7 && Num != 0.0)
        Exponent = log10(Num) - 0.999999999;
    
    Num /= pow(10.0, Exponent);    
    PrintInt((long)Num, 0, FALSE, FALSE, Stream);
    PrintCh('.', Stream);
    Num = (Num - (long)Num) * 10;
    if (abs(Num) >= 1e-7)
    {
        for (MaxDecimal = 6; MaxDecimal > 0 && abs(Num) >= 1e-7; Num = (Num - (long)(Num + 1e-7)) * 10, MaxDecimal--)
            PrintCh('0' + (long)(Num + 1e-7), Stream);
    }
    else
        PrintCh('0', Stream);
        
    if (Exponent != 0)
    {
        PrintCh('e', Stream);
        PrintInt(Exponent, 0, FALSE, FALSE, Stream);
    }
}
#endif

/* intrinsic functions made available to the language */
void GenericPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs, struct OutputStream *Stream)
{
    char *FPos;
    struct Value *NextArg = Param[0];
    struct ValueType *FormatType;
    int ArgCount = 1;
    int LeftJustify = FALSE;
    int ZeroPad = FALSE;
    int FieldWidth = 0;
    char *Format = Param[0]->Val->Pointer;
    
    for (FPos = Format; *FPos != '\0'; FPos++)
    {
        if (*FPos == '%')
        {
            FPos++;
            if (*FPos == '-')
            {
                /* a leading '-' means left justify */
                LeftJustify = TRUE;
                FPos++;
            }
            
            if (*FPos == '0')
            {
                /* a leading zero means zero pad a decimal number */
                ZeroPad = TRUE;
                FPos++;
            }
            
            /* get any field width in the format */
            while (isdigit((int)*FPos))
                FieldWidth = FieldWidth * 10 + (*FPos++ - '0');
            
            /* now check the format type */
            switch (*FPos)
            {
                case 's': FormatType = CharPtrType; break;
                case 'd': case 'u': case 'x': case 'b': case 'c': FormatType = &IntType; break;
#ifndef NO_FP
                case 'f': FormatType = &FPType; break;
#endif
                case '%': PrintCh('%', Stream); FormatType = NULL; break;
                case '\0': FPos--; FormatType = NULL; break;
                default:  PrintCh(*FPos, Stream); FormatType = NULL; break;
            }
            
            if (FormatType != NULL)
            { 
                /* we have to format something */
                if (ArgCount >= NumArgs)
                    PrintStr("XXX", Stream);   /* not enough parameters for format */
                else
                {
                    NextArg = (struct Value *)((char *)NextArg + MEM_ALIGN(sizeof(struct Value) + TypeStackSizeValue(NextArg)));
                    if (NextArg->Typ != FormatType && 
                            !((FormatType == &IntType || *FPos == 'f') && IS_NUMERIC_COERCIBLE(NextArg)) &&
                            !(FormatType == CharPtrType && (NextArg->Typ->Base == TypePointer || 
                                                             (NextArg->Typ->Base == TypeArray && NextArg->Typ->FromType->Base == TypeChar) ) ) )
                        PrintStr("XXX", Stream);   /* bad type for format */
                    else
                    {
                        switch (*FPos)
                        {
                            case 's':
                            {
                                char *Str;
                                
                                if (NextArg->Typ->Base == TypePointer)
                                    Str = NextArg->Val->Pointer;
                                else
                                    Str = &NextArg->Val->ArrayMem[0];
                                    
                                if (Str == NULL)
                                    PrintStr("NULL", Stream); 
                                else
                                    PrintStr(Str, Stream); 
                                break;
                            }
                            case 'd': PrintInt(ExpressionCoerceInteger(NextArg), FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'u': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 10, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'x': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 16, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'b': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 2, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'c': PrintCh(ExpressionCoerceUnsignedInteger(NextArg), Stream); break;
#ifndef NO_FP
                            case 'f': PrintFP(ExpressionCoerceFP(NextArg), Stream); break;
#endif
                        }
                    }
                }
                
                ArgCount++;
            }
        }
        else
            PrintCh(*FPos, Stream);
    }
}

/* printf(): print to console output */
void LibPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream ConsoleStream;
    
    ConsoleStream.Putch = &PlatformPutc;
    GenericPrintf(Parser, ReturnValue, Param, NumArgs, &ConsoleStream);
}

/* sprintf(): print to a string */
void LibSPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream StrStream;
    
    StrStream.Putch = &SPutc;
    StrStream.i.Str.Parser = Parser;
    StrStream.i.Str.WritePos = Param[0]->Val->Pointer;

    GenericPrintf(Parser, ReturnValue, Param+1, NumArgs-1, &StrStream);
    PrintCh(0, &StrStream);
    ReturnValue->Val->Pointer = *Param;
}

/* get a line of input. protected from buffer overrun */
void LibGets(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = PlatformGetLine(Param[0]->Val->Pointer, GETS_BUF_MAX, NULL);
    if (ReturnValue->Val->Pointer != NULL)
    {
        char *EOLPos = strchr(Param[0]->Val->Pointer, '\n');
        if (EOLPos != NULL)
            *EOLPos = '\0';
    }
}

void LibGetc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = PlatformGetCharacter();
}

void LibExit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    PlatformExit(Param[0]->Val->Integer);
}

#ifdef PICOC_LIBRARY
void LibSin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = sin(Param[0]->Val->FP);
}

void LibCos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = cos(Param[0]->Val->FP);
}

void LibTan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = tan(Param[0]->Val->FP);
}

void LibAsin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = asin(Param[0]->Val->FP);
}

void LibAcos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = acos(Param[0]->Val->FP);
}

void LibAtan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = atan(Param[0]->Val->FP);
}

void LibSinh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = sinh(Param[0]->Val->FP);
}

void LibCosh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = cosh(Param[0]->Val->FP);
}

void LibTanh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = tanh(Param[0]->Val->FP);
}

void LibExp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = exp(Param[0]->Val->FP);
}

void LibFabs(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = fabs(Param[0]->Val->FP);
}

void LibLog(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = log(Param[0]->Val->FP);
}

void LibLog10(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = log10(Param[0]->Val->FP);
}

void LibPow(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = pow(Param[0]->Val->FP, Param[1]->Val->FP);
}

void LibSqrt(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = sqrt(Param[0]->Val->FP);
}

void LibRound(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = floor(Param[0]->Val->FP + 0.5);   /* XXX - fix for soft float */
}

void LibCeil(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = ceil(Param[0]->Val->FP);
}

void LibFloor(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->FP = floor(Param[0]->Val->FP);
}
#endif

#ifndef NO_STRING_FUNCTIONS
void LibMalloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = malloc(Param[0]->Val->Integer);
}

#ifndef NO_CALLOC
void LibCalloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = calloc(Param[0]->Val->Integer, Param[1]->Val->Integer);
}
#endif

#ifndef NO_REALLOC
void LibRealloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = realloc(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}
#endif

void LibFree(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    free(Param[0]->Val->Pointer);
}

void LibStrcpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *To = (char *)Param[0]->Val->Pointer;
    char *From = (char *)Param[1]->Val->Pointer;
    
    while (*From != '\0')
        *To++ = *From++;
    
    *To = '\0';
}

void LibStrncpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *To = (char *)Param[0]->Val->Pointer;
    char *From = (char *)Param[1]->Val->Pointer;
    int Len = Param[2]->Val->Integer;
    
    for (; *From != '\0' && Len > 0; Len--)
        *To++ = *From++;
    
    if (Len > 0)
        *To = '\0';
}

void LibStrcmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Str1 = (char *)Param[0]->Val->Pointer;
    char *Str2 = (char *)Param[1]->Val->Pointer;
    int StrEnded;
    
    for (StrEnded = FALSE; !StrEnded; StrEnded = (*Str1 == '\0' || *Str2 == '\0'), Str1++, Str2++)
    {
         if (*Str1 < *Str2) { ReturnValue->Val->Integer = -1; return; } 
         else if (*Str1 > *Str2) { ReturnValue->Val->Integer = 1; return; }
    }
    
    ReturnValue->Val->Integer = 0;
}

void LibStrncmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Str1 = (char *)Param[0]->Val->Pointer;
    char *Str2 = (char *)Param[1]->Val->Pointer;
    int Len = Param[2]->Val->Integer;
    int StrEnded;
    
    for (StrEnded = FALSE; !StrEnded && Len > 0; StrEnded = (*Str1 == '\0' || *Str2 == '\0'), Str1++, Str2++, Len--)
    {
         if (*Str1 < *Str2) { ReturnValue->Val->Integer = -1; return; } 
         else if (*Str1 > *Str2) { ReturnValue->Val->Integer = 1; return; }
    }
    
    ReturnValue->Val->Integer = 0;
}

void LibStrcat(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *To = (char *)Param[0]->Val->Pointer;
    char *From = (char *)Param[1]->Val->Pointer;
    
    while (*To != '\0')
        To++;
    
    while (*From != '\0')
        *To++ = *From++;
    
    *To = '\0';
}

void LibIndex(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Pos = (char *)Param[0]->Val->Pointer;
    int SearchChar = Param[1]->Val->Integer;

    while (*Pos != '\0' && *Pos != SearchChar)
        Pos++;
    
    if (*Pos != SearchChar)
        ReturnValue->Val->Pointer = NULL;
    else
        ReturnValue->Val->Pointer = Pos;
}

void LibRindex(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Pos = (char *)Param[0]->Val->Pointer;
    int SearchChar = Param[1]->Val->Integer;

    ReturnValue->Val->Pointer = NULL;
    for (; *Pos != '\0'; Pos++)
    {
        if (*Pos == SearchChar)
            ReturnValue->Val->Pointer = Pos;
    }
}

void LibStrlen(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Pos = (char *)Param[0]->Val->Pointer;
    int Len;
    
    for (Len = 0; *Pos != '\0'; Pos++)
        Len++;
    
    ReturnValue->Val->Integer = Len;
}

void LibMemset(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    /* we can use the system memset() */
    memset(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

void LibMemcpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    /* we can use the system memcpy() */
    memcpy(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

void LibMemcmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    unsigned char *Mem1 = (unsigned char *)Param[0]->Val->Pointer;
    unsigned char *Mem2 = (unsigned char *)Param[1]->Val->Pointer;
    int Len = Param[2]->Val->Integer;
    
    for (; Len > 0; Mem1++, Mem2++, Len--)
    {
         if (*Mem1 < *Mem2) { ReturnValue->Val->Integer = -1; return; } 
         else if (*Mem1 > *Mem2) { ReturnValue->Val->Integer = 1; return; }
    }
    
    ReturnValue->Val->Integer = 0;
}
#endif


/* list of all library functions and their prototypes */
struct LibraryFunction CLibrary[] =
{
    { LibPrintf,        "void printf(char *, ...);" },
    { LibSPrintf,       "char *sprintf(char *, char *, ...);" },
    { LibGets,          "char *gets(char *);" },
    { LibGetc,          "int getchar();" },
    { LibExit,          "void exit(int);" },
#ifdef PICOC_LIBRARY
    { LibSin,           "float sin(float);" },
    { LibCos,           "float cos(float);" },
    { LibTan,           "float tan(float);" },
    { LibAsin,          "float asin(float);" },
    { LibAcos,          "float acos(float);" },
    { LibAtan,          "float atan(float);" },
    { LibSinh,          "float sinh(float);" },
    { LibCosh,          "float cosh(float);" },
    { LibTanh,          "float tanh(float);" },
    { LibExp,           "float exp(float);" },
    { LibFabs,          "float fabs(float);" },
    { LibLog,           "float log(float);" },
    { LibLog10,         "float log10(float);" },
    { LibPow,           "float pow(float,float);" },
    { LibSqrt,          "float sqrt(float);" },
    { LibRound,         "float round(float);" },
    { LibCeil,          "float ceil(float);" },
    { LibFloor,         "float floor(float);" },
#endif
    { LibMalloc,        "void *malloc(int);" },
#ifndef NO_CALLOC
    { LibCalloc,        "void *calloc(int,int);" },
#endif
#ifndef NO_REALLOC
    { LibRealloc,       "void *realloc(void *,int);" },
#endif
    { LibFree,          "void free(void *);" },
#ifndef NO_STRING_FUNCTIONS
    { LibStrcpy,        "void strcpy(char *,char *);" },
    { LibStrncpy,       "void strncpy(char *,char *,int);" },
    { LibStrcmp,        "int strcmp(char *,char *);" },
    { LibStrncmp,       "int strncmp(char *,char *,int);" },
    { LibStrcat,        "void strcat(char *,char *);" },
    { LibIndex,         "char *index(char *,int);" },
    { LibRindex,        "char *rindex(char *,int);" },
    { LibStrlen,        "int strlen(char *);" },
    { LibMemset,        "void memset(void *,int,int);" },
    { LibMemcpy,        "void memcpy(void *,void *,int);" },
    { LibMemcmp,        "int memcmp(void *,void *,int);" },
#endif
    { NULL,             NULL }
};

#endif /* BUILTIN_MINI_STDLIB */
