#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define SZEJSET

#include "poldit.h"

struct chardic {
    uint32_t znak;
    const char *str;
};

// typy znaków

#define ZX_LOWER 0x200
#define ZX_UPPER 0x400
#define ZX_DIGIT 0x800
#define ZX_SPACE 0x1000
#define ZX_NSB 0x2000 // no space before
#define ZX_PSA 0x4000 // provide space after
#define ZX_NCHAR 0x8000

// główne flagi konwertera i jednostek

#define FLG_FEMALE 1
#define FLG_COLL 2
#define FLG_DIVD 4
#define FLG_SPELCOMMA 8
#define FLG_NATURAL 0x10
#define FLG_BASEUNIT 0x20
#define FLG_BASEIGNORE 0x40

#define UNIT_ALL 0x7fffff00


struct unitDefs {
    const char *unit;
    int flags;
    const char *vals[4];
};



#include "poldit_data.h"
#include "outpush.h"


struct numberForm {
    const char *starter;
    const char *num1;
    const char *inner;
    const char *num2;
    uint8_t starterlen;
    uint8_t num1len;
    uint8_t innerlen;
    uint8_t num2len;
    int32_t intv1;
    int32_t intv2;
    const struct unitDefs *unit;
    uint8_t subtyp1;
    uint8_t subtyp2;
    uint8_t type;
    uint8_t mode;
};

// typ danych w numberForm.type
enum {
    DT_NUM = 1,
    DT_HOUR,
    DT_IPV4,
    DT_FMT,
    DT_DATE,
    DT_MAX
};

// typ danych w numberForm.subtype1 i numberForm.subtype2

enum {
    SDT_INT = 1,
    SDT_FXD,
    SDT_HOUR,
    SDT_FMT,
    SDT_DATE,
    SDT_MAX
};

// flagi parsera miesięcy

#define MONFLG_ARABIC 1
#define MONFLG_NATURAL 2
#define MONFLG_NOMINAL 4
#define MONFLG_ROMAN 8
#define MONFLG_FORCE 0x10
#define MONFLG_DECLINE 0x20
#define MONFLG_ISODATE 0x40

#define MON_FORCE_DATE (MONFLG_FORCE | MONFLG_NATURAL | MONFLG_ROMAN | MONFLG_ISODATE)

#define MONTH_NONE 15
#define YEAR_NONE 4095

// typ zwracany przez getNextNumberType

#define NTYP_NONE 0
#define NTYP_PLAIN 1
#define NTYP_UNIT 2
#define NTYP_HOUR 4
#define NTYP_DATE 8
#define NTYP_IPV4 0x10
#define NTYP_NPLAIN (0xff & ~(NTYP_PLAIN))
#define DAF_NOMONTH 0x100
#define DAF_NOYEAR  0x200

// typy formatu

enum {
    FMT_POS=0,
    FMT_CNT,
    FMT_NORM,
    FMT_SPELL,
    FMT_SPELLALL,
    FMT_MAX};

static const struct unitDefs *getUnit(poldit *buffer,const char *str, const char **rest);
static size_t spellFmt(poldit *buffer, const char *str, int lstr);
static size_t getNumTriplet(poldit *buffer,int n, int female, uint8_t uzero, uint8_t prezero);
static uint8_t getDate(poldit *buffer,const char *str, const char **rest,
    uint8_t *day, uint8_t *month, uint16_t *year, int flags, uint8_t *outflags);
static int getDitMonth(const char *str, const char **rest, uint8_t flags, uint8_t *oflags);
static int getNextNumberType(poldit *buffer, const char *str, uint16_t *dateflags);

static uint32_t zx_tolower(uint32_t a)
{
    if (a >= 0x180) return a;
    return charCode[a] & 511;
}

static uint8_t zx_islower(uint32_t a)
{
    if (a >= 0x180) return 0;
    return (charCode[a] & ZX_LOWER) != 0;
}

static uint8_t zx_isdigit(uint32_t a)
{
    if (a >= 0x180) return 0;
    return (charCode[a] & ZX_DIGIT) != 0;
}

static uint8_t zx_isspace(uint32_t a)
{
    if (a >= 0x180) return 0;
    return (charCode[a] & ZX_SPACE) != 0;
}

static uint8_t zx_isalnum(uint32_t a)
{
    if (a >= 0x180) return 0;
    return (charCode[a] & (ZX_UPPER | ZX_LOWER | ZX_DIGIT)) != 0;
}

static uint8_t zx_isalpha(uint32_t a)
{
    if (a >= 0x180) return 0;
    return (charCode[a] & (ZX_UPPER | ZX_LOWER)) != 0;
}
static int zx_isdash(uint32_t a)
{
    return a == '-' || (a>=0x2012 && a <=0x2014);
}

static uint16_t zx_flags(uint32_t a)
{
    if (a >= 0x180) return ZX_NCHAR;
    return charCode[a] & 0xfe00;
}

static const char *zx_ismap(uint32_t a)
{
    int i;
    for (i=0;charDic[i].znak;i++) if (a == charDic[i].znak) return charDic[i].str;
    return NULL;
}


static uint32_t get_unichar(const char *str, const char **rest)
{
    uint32_t znak; int n,m;
    if (!*str) return 0;
    znak=(*str++) & 255;
    if (!(znak & 0x80)) {
        if (rest) *rest=str;
        return znak;
    }
    if ((znak & 0xe0)==0xc0) {n=1;znak &= 0x3f;}
    else if ((znak & 0xf0)==0xe0) {n=2;znak &=0x1f;}
    else if ((znak & 0xf8)==0xf0) {n=3;znak &=0x0f;}
    else if ((znak & 0xfc)==0xe0) {n=4;znak &=0x07;}
    else if ((znak & 0xfe)==0xe0) {n=5;znak &=0x03;}
    else {
            return 0;
    }
    while (n--) {
            m=*str++ & 255;
            if ((m & 0xc0)!=0x80) {
                    return 0;
            }
            znak=(znak<<6) | (m & 0x3f);
    }
    if (rest) *rest=str;
    return znak;
}

static const char *unblank(const char *str)
{
    const char *estr;uint32_t znak;
    while (*str) {
        znak=get_unichar(str,&estr);
        if (!zx_isspace(znak)) break;
        str=estr;
    }
    return str;
}

static const struct {
    const char *pat;
    const char *rep;
} _nats[] = {
    {"ęćdz","eńdz"},
    {"eśćdz","eźdz"},
    {"eśćse",
 #ifdef SZEJSET
 "ejse"
 #else   
    "eśse"
#endif    
    },
    {"ęć","eńć"},
    {"ętn","etn"},
    {NULL, NULL}};
    

static const char *_naturalize(const char *str, int doit)
{
    static char buf[32];
    if (!doit) return str;
    char *b = buf;const char *c,*d;
    int i;
    while (*str) {
        for (i=0;_nats[i].pat;i++) {
            c=str;
            d=_nats[i].pat;
            while (*c && *d) {
                if (*c != *d) break;
                c++;
                d++;
            }
            if (!*d) break;
        }
        if (!_nats[i].pat) {
            *b++ = *str++;
            continue;
        }
        str=c;
        for (d=_nats[i].rep;*d;) *b++=*d++;
    }
    *b=0;
    return buf;
}

static uint8_t _needBlank(const char *c)
{
    if (!*c) return 0;
    uint32_t znak = get_unichar(c, &c);
    if (znak >= 0x180 || znak == '-') return 1;
    if (zx_flags(znak) & (ZX_NSB | ZX_SPACE)) return 0;
    return 1;
}

static size_t _lastBlank(const char *ci, size_t len)
{
    uint32_t znak;
    if (len == 0) return 0;
    const unsigned char *c = (const unsigned char *)ci;
    if (len == 1) {
        znak=c[len-1];
    }
    else {
        znak = c[len-1];
        if (znak & 0x80) {
            if ((c[len-2] & 0xE0) != 0xA0) return 0;
            znak = (znak & 0x3f) | ((c[len-2] & 0x1f) << 6);
        }
    }
    if (zx_flags(znak) & (ZX_SPACE | ZX_PSA)) return 1;
    //if (znak >= 0x7f) return 0;
    //if (strchr("[({", znak)) return 1;
    return 0;
}


static uint8_t cmpCharPat(uint32_t znak, uint32_t pat)
{
    if (zx_islower(pat) && zx_tolower(znak) == pat) return 1;
    return znak == pat;
}


static uint8_t _eow(const char *c, const char **rest)
{
    if (*c) {
        const char *d;
        uint32_t znak=get_unichar(c,&d);
        if (zx_flags(znak) & (ZX_LOWER | ZX_UPPER | ZX_DIGIT)) return 0;
        while(zx_flags(znak) & ZX_SPACE) {
            c=d;
            znak=get_unichar(c,&d);
        }
    }
    if (rest) *rest = c;
    return 1;

}

static uint8_t _eon(const char *c, const char **rest)
{
    if ((*c == '.' || *c == ',') && isdigit(c[1])) return 0;
    return _eow(c, rest);
}

static uint8_t getHour(poldit *buffer,const char *str, const char **rest, uint8_t *h, uint8_t *m, uint8_t *s)
{
    const char *c=str, *d;
    uint8_t _h, _m=0xff, _s=0xff;
    uint8_t rc=1;
    if (!isdigit(*c)) return 0;
    _h=*c++-'0';
    if (isdigit(*c)) {
        _h = 10*_h + *c++ - '0';
    }
    if (_h > 23 || isdigit(*c)) return 0;
    d=c; // koniec godziny
    if (*c != ':') { // czy sama godzina?
        if (getUnit(buffer, c, &d)) {
            return 0;
        }
        goto finale;
    }

    //if (*c++!=':') return 0;
    c++;
    if (!isdigit(*c) || !isdigit(c[1])) return 0;
    _m = 10* (*c - '0') + c[1]-'0';
    if (_m > 59) return 0;
    c += 2;
    if (*c == ':' && isdigit(c[1]) && isdigit(c[2])) {
        _s = 10* (c[1] - '0') + c[2]-'0';
        if (_s > 59) return 0;
        c += 3;
        rc=2;
    }
finale:
    if (_eon(c, rest)) {
        *h=_h;
        *m=_m;
        *s=_s;
        return rc;
    }
    return 0;
}

static uint8_t umatch(const char *str, const char *pat, const char **rest)
{
    while (*str && *pat) if (*str++ != *pat++) return 0;
    if (*pat) return 0;
    return _eow(str, rest);
}

static uint8_t cmatch(const char *str, const char *pat, const char **rest)
{
    uint32_t z1, z2;
    const char *se, *pe;
    while(*str && *pat) {
        z1 = get_unichar(str, &se);
        z2 = get_unichar(pat, &pe);
        if (zx_isspace(z2)) {
            if (!zx_isspace(z1)) return 0;
            str=se;
            pat=pe;
            while (*str) {
                z1=get_unichar(str,&se);
                if (!zx_isspace(z1)) break;
                str=se;
            }
            continue;
        }
        if (!cmpCharPat(z1, z2)) return 0;
        str=se;
        pat=pe;
    }
    if (*pat) return 0;
    return _eow(str, rest);

}

static const struct unitDefs *getUnit(poldit *buffer,const char *str, const char **rest)
{
    uint32_t znak;
    const char *d;
    int i;
    while (*str) {
        znak=get_unichar(str, &d);
        if (!zx_isspace(znak)) break;
        str=d;
    }
    if (!strncmp(str,"F[",2)) return NULL;
    for (i=0;unitDefs[i].unit; i++) {
        if (unitDefs[i].flags & FLG_BASEIGNORE) continue;
        if (!(buffer->flags & unitDefs[i].flags & UNIT_ALL)) continue;
        if (!(buffer->flags & FLG_COLL) && (unitDefs[i].flags & FLG_COLL)) continue;
        if (umatch(str, unitDefs[i].unit, rest)) {
            return &unitDefs[i];
        }
    }
    return NULL;
}

struct numberPart {
    const char *nbeg;
    int16_t nlen;
    int16_t subtyp;
    const struct unitDefs *unit;
};

static int getOctet(const char *str, const char **rest)
{
    if (!*str || !isdigit(*str)) return -1;
    int n = strtol(str, (char **) &str, 10);
    if (n < 0 || n > 255) return -1;
    *rest = str;
    return n;
}

static int getIPv4(poldit *buffer, const char *str, const char **rest)
{
    int i;
    for (i=0;i<4;i++) {
        if (i && *str++ != '.') return 0;
        if (getOctet(str, &str) < 0) return 0;
    }
    *rest=str;
    return 1;
}

static uint8_t getnumber(poldit *buffer, const char *str, const char **rest, struct numberPart *np)
{
    const char *cs = str, *cdn;
    int slen;
    int styp = SDT_INT;
    if (*str == '-') str++;
    if (!isdigit(*str)) return 0;
    while(*str && isdigit(*str)) str++;
    if ((*str == '.' || *str==',') && isdigit(str[1])) {
        str+=2;
        while(*str && isdigit(*str)) str++;
        styp = SDT_FXD;
    }
    cdn = cs;
    slen=str-cs;
    const struct unitDefs *ud = getUnit(buffer,str, rest);
    if (!ud) {
        if (!_eon(str, rest)) return 0;
    }
    np->nbeg=cdn;
    np->nlen=slen;
    np->subtyp=styp;
    np->unit=ud;
    return 1;
}

#define NPF_FIRST 1
#define NPF_LAST 2
#define NPF_ONLY 4
#define NPF_POS 8
#define NPF_NAR 16

static const struct numPfx {
    const char *pat;
    const char *repl;
    uint16_t flags;
} numPfx[]={
    {"od około", NULL, NPF_FIRST},
    {"od ok.", "od około", NPF_FIRST},
    {"od", NULL,NPF_FIRST},
    {"do około", NULL,NPF_FIRST | NPF_LAST},
    {"do ok.", "do około", NPF_FIRST | NPF_LAST},
    {"do", NULL,NPF_FIRST | NPF_LAST},
    {"z", NULL,NPF_LAST},
    {"ze", NULL,NPF_LAST},
    {NULL, 0}};


static uint8_t isNumber(poldit *buffer,const char *str, const char **rest,struct numberForm *nf)
{
    struct numberPart npart;
    memset((void *)&npart, 0, sizeof(npart));
    const char *sbeg = str;
    if (getIPv4(buffer, str, rest)) {
        nf->num1 = sbeg;
        nf->num1len = str-sbeg;
        return nf->type = DT_IPV4;
    }

    const char *pfx=NULL, *ifx=NULL;
    int pfxl=0, ifxl=0;
    uint16_t flags = 0;
    int i;
    for (i=0;numPfx[i].pat;i++) {
        if (cmatch(str,numPfx[i].pat,&sbeg)) {
            if (numPfx[i].repl) {
                pfx=numPfx[i].repl;
                pfxl=strlen(pfx);
            } else {
                pfx=str;
                pfxl=sbeg-str;
            }
            str=sbeg;
            flags = numPfx[i].flags;
            break;
        }
    }

    if (!getnumber(buffer, str, &sbeg, &npart)) return 0;
    nf->starter = pfx;
    nf->starterlen = pfxl;
    nf->subtyp1 = npart.subtyp;
    nf->num1 = npart.nbeg;
    nf->num1len=npart.nlen;
    nf->unit = npart.unit;
    nf->type = DT_NUM;
    if (npart.unit || (flags & NPF_LAST) || !pfx) {
        *rest = sbeg;
        nf->mode = (flags & NPF_NAR) ? 2 : pfx ? 1 : 0;
        return DT_NUM;
    }

    str = sbeg;
    const char *cinn = unblank(sbeg);
    for (i=0;numPfx[i].pat;i++) {
        if (!(numPfx[i].flags & NPF_LAST)) continue;
        if (cmatch(str,numPfx[i].pat,&sbeg)) {
            if (numPfx[i].repl) {
                ifx=numPfx[i].repl;
                ifxl=strlen(ifx);
            } else {
                ifx=str;
                ifxl=sbeg-str;
            }
            str=sbeg;
            flags = numPfx[i].flags;
            break;
        }
    }
    memset((void *)&npart, 0, sizeof(npart));
    int gu = getNextNumberType(buffer, str, NULL);
    if ((gu & NTYP_NPLAIN) || !getnumber(buffer, str, &sbeg, &npart)) {
        *rest = cinn;
        nf->mode = pfx ? 1 : 0;
        return DT_NUM;
    }
    nf->inner = ifx;
    nf->innerlen = ifxl;
    nf->subtyp2 = npart.subtyp;
    nf->num2 = npart.nbeg;
    nf->num2len=npart.nlen;
    nf->unit = npart.unit;
    nf->type = DT_NUM;
    *rest = sbeg;
    nf->mode = pfx ? 1 : 2;
    return DT_NUM;

}

/*
 *  GENERATOR
 */
static int getPolyFormN(int i, int female)
{
    if (i == 1) return female ? 1 : 3;
    return 2;
}

static int getPolyForm(int i)
{
    if (i == 1) return 0;
    i%=100;
    if (!i) return 2;
    if (i>19) i%=10;
    if (i>=2 && i<=4) return 1;
    return 2;

}

static int getNumForm(const char *csn, size_t len, uint8_t mode, uint8_t female)
{
    int i;
    for (i=0;i<len;i++) if (csn[i] == '.' || csn[i] == ',') return 3;
    i=strtol(csn,NULL, 10);
    if (i < 0) i=-i;
    if (mode) return getPolyFormN(i, female);
    return getPolyForm(i);

}

static const char *const _tysiace[] = {"tysiąc","tysiące","tysięcy","tysiąca"};
    
static const char * const dg_uni[]={
    "zero","jeden","dwa","trzy","cztery","pięć","sześć","siedem",
    "osiem","dziewięć","dziesięć","jedenaście","dwanaście","trzynaście",
    "czternaście","piętnaście","szesnaście","siedemnaście","osiemnaście",
    "dziewiętnaście"};
static const char  * const dg_dec[]={
    "dwadzieścia","trzydzieści","czterdzieści","pięćdziesiąt",
    "sześćdziesiąt", "siedemdziesiąt","osiemdziesiąt","dziewięćdziesiąt"};
static const char  * const dg_hun[]={
    "sto","dwieście","trzysta","czterysta","pięćset","sześćset",
    "siedemset","osiemset","dziewięćset"};

static const char  * const dg_eun[]={
    "zero","pierwsz","drug","trzec","czwart",
    "piąt","szóst","siódm","ósm","dziewiąt","dziesiąt",
    "jedenast","dwunast","trzynast","czternast","piętnast",
    "szesnast","siedemnast","osiemnast","dziewiętnast"};

static const char  * const dg_ede[]={"dwudziest","trzydziest","czterdziest",
    "pięćdziesiąt","sześćdziesiąt","siedemdziesiąt","osiemdziesiąt",
    "dziewięćdziesiąt"};
static const char  * const dg_ndec[]={
    "dwudziest","trzydziest","czterdziest","pięćdziesięci",
    "sześćdziesięci", "siedemdziesięci","osiemdziesięci","dziewięćdziesięci"};
static const char *const dg_subdeci[]={
    "dziesiąta","dziesiąte","dziesiątych",
    "setna","setne","setnych",
    "tysięczna", "tysięczne", "tysięcznych"};

static const char *const dg_subdecis[]={
    "dziesiątej","dziesiątych","dziesiątych",
    "setnej","setnych","setnych",
    "tysięcznej", "tysięcznych", "tysięcznych"};

static const char *namedMon[] ={"stycznia","lutego","marca","kwietnia",
    "maja","czerwca","lipca","sierpnia",
    "września","października","listopada","grudnia" };

static const char *namedMonNom[] ={"styczeń","luty","marzec","kwiecień",
    "maj","czerwiec","lipiec","sierpień",
    "wrzesień","październik","listopad","grudzień" };

static const char *romanMon[] ={"I","II","III","IV","V","VI","VII",
        "VIII", "IX","X","XI","XII" };

/*
 * 0 - mianownik
 * 1 - dopełniacz
 * 2 - celownik
 * 3 - biernik
 * 4 - narzędnik
 * 5 - miejscownik
 */

static const char  * const dg_ehu[]={
    "setn","dwusetn","trzysetn","czterysetn","pięćsetn",
    "sześćsetn", "siedemsetn", "osiemsetn", "dziewięćsetn"};

static const char * const posfin[6][6]={
    {"y","ego","emu","y","ym","ym"},
    {"a","ą","ej","ą","ą","ą"},
    {"e","ego","emu","e","ym","ym"},

    // drugi, trzeci
    {"i","iego","iemu","i","im","im"},
    {"a","ą","iej","ą","ą","ą"},
    {"ie","iego","iemu","ie","im","im"},
    };

static const char *dg_milg[] = {
    "jedno","dwu","trzy","cztero","pięcio","sześcio","siedmio","ośmio","dziewięcio",
    "dziesięcio","jedenasto","dwunasto","trzynasto","czternasto","piętnasto",
    "szesnasto","siedemnasto","osiemnasto","dziewiętnasto"};
static const char  * const dg_mild[]={"dwudziesto","trzydziesto","czterdziesto",
    "pięćdziesięcio","sześćdziesięcio","siedemdziesięcio","osiemdziesięcio",
    "dziewięćdziesięcio"};
static const char *dg_milh[]={"stu","dwustu","trzystu","czterystu","pięcset",
    "sześcset","siedemset","osiemset","dziewięćset"};

static const char *const dg_xhu[9][2]={
    {"stu","stoma"},
    {"dwustu", "dwustoma"},
    {"trzystu", "trzystoma"},
    {"czterystu", "czterystoma"},
    {"pięciuset","pięciuset"},
    {"sześciuset","sześciuset"},
    {"siedmiuset","siedmiuset"},
    {"ośmiuset","ośmiuset"},
    {"dziewięciuset","dziewięciuset"}};


static const char *const dg_cntmils[][6]={
    {"tysiąc","tysiąca","tysięcu","tysiąc","tysiącem","tysiącu"},
    {"tysiące","tysięcy","tysiącom","tysiące","tysiącami","tysiącach"}};

static const char * const dg_cntfin[6]=
    {"","u","u","","oma","u"};

static const char *const dg_cntone[3][6]={
    {"jeden","jednego","jednemu","jednego","jednym","jednym"},
    {"jedna","jednej","jednej","jedną","jedną","jednej"},
    {"jedno","jednego","jednemu","jedno","jednym","jednym"}};

static const char *const dg_cntsmal[][6]={
    {"","dwóch","dwóm","","dwoma","dwóch"},
    {"","trzech","trzem","","trzema","trzech"},
    {"","czterech","czterem","","czterema","czterech"},
    {"","pięciu","pięciu","","pięcioma","pięciu"},
    {"","sześciu","sześciu","","sześcioma","sześciu"},
    {"","siedmiu","siedmiu","","siedmioma","siedmiu"},
    {"","ośmiu","ośmiu","","ośmioma","ośmiu"},
    {"","dziewięciu","dziewięciu","","dziewięcioma","dziewięciu"},
    {"","dziesięciu","dziesięciu","","dziesięcioma","dziesięciu"}};

                        
// pełny triplet dla wyrażeń porządkowych z kompletną odmianą
static size_t getNumPosTripletG(poldit *buffer, int n, int genre, int declination)
{
    int len = 0;
    int m;
    if ( n == 0) {
         pushbufs("zero");
         return len;
     }
     if (n >= 100) {
         m = n / 100;
         n = n % 100;
         if (!n) {
             pushbufs(dg_ehu[m-1]);
             pushbuf(posfin[genre][declination]);
             return len;
         }
         pushbufs(dg_hun[m-1]);
     }
     if (n >= 20) {
         m=n / 10;
         n = n % 10;
         pushbufs(dg_ede[m-2]);
         pushbuf(posfin[genre][declination]);
    }
    if (n) {
        pushbufs(dg_eun[n]);
        int nn = genre;
        if (n == 2 || n == 3) nn += 3;
        pushbuf(posfin[nn][declination]);
    }
    return len;
}

// triplet dla wartości porządkwych (n-ty)

static size_t getNumPosTriplet(poldit *buffer,int n, int female, uint8_t was, uint8_t mode)
{
    int genre = female ? 1 : 0;
    int decl = (mode == 2) ? 1 : (mode == 1) ? 2 : 0;
    return getNumPosTripletG(buffer, n, genre, decl);
}

static size_t getNumPosG(poldit *buffer, int n, int genre, int declin)
{
    int m;
    size_t len=0;
    if (n >= 1000000) {
        pushbufs("dużo");
        return len;
    }

    if (n >= 1000) {
        m = n / 1000;
        n %= 1000;
        if (n) {
            if (m != 1) len += getNumTriplet(buffer,m, 0, 0, 0);
            int fm = getPolyForm(m);
            pushbufs(_tysiace[fm]);
        }
        else {
            //dg_milg
            int was = 0,mm;
            if (m != 1) {
                if (m >= 100) {
                    mm= m /100;
                    m = m % 100;
                    pushbufs(dg_milh[mm-1]);
                }
                
                if (m >= 20) {
                    mm = m / 10;
                    m = m % 10;
                    pushbufs(dg_mild[mm-2]);
                }
                if (m) {
                    pushbufs(dg_milg[m-1]);
                }
                was = 1;
            }
            if (was) pushbuf("tysięczn");
            else pushbufs("tysięczn");
            pushbuf(posfin[0][declin]);
            return len;
        }
    }
    len += getNumPosTripletG(buffer, n, genre, declin);
    return len;
}



// triplet dla wyliczeń (od-do n)
// dwieście dwustu dwustu dwieście dwustoma dwustu
//


static size_t getNumCntTripletG(poldit *buffer, int n, int genre, int declin)
{
    size_t len = 0;
    if (n == 0) {
        pushbufs("zero");
        return len;
    }

    if (n == 1) {
        pushbufs(dg_cntone[genre][declin]);
        return len;
    }
    int m;
    if (n >= 100) {
        m = n / 100;
        n %= 100;
        if (declin == 0 || declin ==3) pushbufs(dg_hun[m-1]);
        else {
            int xdc = (n == 0 && declin == 4) ? 1 : 0;
            pushbufs(dg_xhu[m-1][xdc]);
        }
        if(!n) return len;

    }
    if (n >= 20) {
        m = n / 10;
        n = n % 10;
        if (declin == 0 || declin == 3) {
            pushbufs(dg_dec[m-2]);
        }
        else {
            pushbufs(dg_ndec[m-2]);
            pushbuf(dg_cntfin[declin]);
        }

    }
    if (!n) return len;
    if (n == 1) {
        pushbufs("jeden");
        return len;
    }
    if (declin == 0 || declin == 3) {
        if (n == 2 && genre == 1) {
            pushbufs("dwie");
        }
        else {
            pushbufs(dg_uni[n]);
        }
        return len;
    }
    if (n < 10) {
        pushbufs(dg_cntsmal[n-2][declin]);
    }
    else {
        pushbufs(dg_eun[n]);
        pushbuf(dg_cntfin[declin]);
    }
    return len;
}


static size_t getNumCntG(poldit *buffer, int n, int genre, int decl)
{
    size_t len = 0;
    if (!n) {
        pushbufs("zero");
        return len;
    }
    if (n >= 1000) {
        int m = (n / 1000) % 1000;
        n = n % 1000;
        //printf("MN %d %d\n", m, n);
        if (m == 1) {
            //printf("<%s>\n", dg_cntmils[0][decl]);
            pushbufs(dg_cntmils[0][decl]);
        }
        else {
            //printf("<%s>\n", dg_cntmils[1][decl]);
            len += getNumCntTripletG(buffer, m, 0, decl);
            pushbufs(dg_cntmils[1][decl]);
        }
    }
    if (n) len += getNumCntTripletG(buffer,n, genre, decl);
    return len;
}
/*
static size_t _getNumCntTriplet(poldit *buffer, int n, int female)
{
    return getNumCntTripletG(buffer, n, female?1:0, 1);

}
*/
// podstawowy triplet

static size_t getNumTriplet(poldit *buffer,int n, int female, uint8_t uzero, uint8_t prezero)
{
    size_t len=0;
    if (uzero && n == 0) {
        pushbufs("zero");
        return len;
    }
    if (female && n == 1) {
        pushbufs("jedna");
        return len;
    }
    //sif (prezero == 2 && n < 100) pushbufs("zero");
    if (prezero && n < 10) pushbufs("zero");
    if (n >=100) {
        pushbufs(dg_hun[n/100 - 1]);
        n %= 100;
    }
    if (n >= 20) {
        pushbufs(dg_dec[n/10 -2]);
        n %= 10;
    }
    if (n) {
        if (female && n == 2) pushbufs("dwie");
        else pushbufs(dg_uni[n]);
    }
    return len;
}


static size_t getNumString(poldit *buffer,int n, int female, int mode)
{
    size_t len=0;
    if (n < 0) {
        pushbufs("minus");
        n=-n;
    }
    if (n >= 1000000) {
        char buf[32];
        sprintf(buf,"%d",n);
        len += spellFmt(buffer,buf,strlen(buf));
        return len;
    }
    if (mode) {
        len += getNumCntG(buffer, n, female?1:0, 1);
        return len;
    }
    if (n >= 1000) {
        int m = n / 1000;
        if (m == 1) {
            pushbufs("tysiąc");
        }
        else {
        
            len += getNumTriplet(buffer, m, 0, 0, 0);
            int idx = getPolyForm(m);
            pushbufs(_tysiace[idx]);
        }
        n %= 1000;
        if (!n) return len;
    }
    len += getNumTriplet(buffer,n, female, 1, 0);
    return len;

}

static size_t stringifyIPv4(poldit *buffer, const char *num, int numlen)
{
    int pv=0, i, n;
    size_t len=0;
    for (i=0;i<4;i++) {
        if (i) num++;
        n=strtol(num, (char **)&num, 10);
        if (i) {
            int dot = 0;
            if (buffer->flags & FLG_COLL) {
                if (pv != 0) {
                    if (!(pv % 100) && n < 100) dot = 1;
                    else if (!(pv % 10) && n < 10) dot = 1;
                }
            }
            else dot = 1;
            if (dot) pushbufs("kropka");
        }
        len += getNumString(buffer, n, 0,0);
        pv=n;
    }
    return len;
}

static size_t _poltora(poldit *buffer,const char *num, int numlen, uint8_t female)
{
    uint8_t neg=0;
    if (*num == '-') {
        neg = 1;
        numlen --;
    }
    if (numlen != 3) return 0;
    if (*num != '1' && *num != '0') return 0;
    if (num[1] != '.' && num[1] !=',') return 0;
    if (num[2] != '5') return 0;
    size_t len = 0;
    if (neg) pushbufs("minus");
    if (*num == '0') pushbufs("pół");
    else if (female) pushbufs("półtorej");
    else pushbufs("półtora");
    return len;
}

//#define IS_COLL ((buffer->flags & (FLG_COLL | FLG_DIVD)) == FLG_COLL)
#define IS_COLL (buffer->flags & FLG_COLL)

static size_t stringifyNumber(poldit *buffer, int subt,
    const char *num, int numlen, uint8_t mode, uint8_t female, const struct unitDefs *unit)
{
    size_t len=0;
    const char *rnum;
    int nval;

    // wyjątek dla pół i półtora w trybie potocznym
    if (subt == SDT_FXD && IS_COLL) {
        len = _poltora(buffer, num, numlen, female);
        if (len) goto dunit; // bo nie libię jak mi się if na szerokość nie mieści
    }

    nval = strtol(num, (char **) &rnum, 10);
    if (rnum - num > 6 || (num[0] == '0' && isdigit(num[1]))) len += spellFmt(buffer, num, rnum-num);
    else len += getNumString(buffer, nval, female,mode);
    if (subt == SDT_FXD) {
        char snum[4];
        rnum++;
        int rlen=numlen- (rnum - num);
        if (rlen > 3) rlen = 3;
        strncpy(snum, rnum, rlen);
        if (rlen == 1 && IS_COLL && snum[0] == '5') {
            pushbufs("i pół");
        }
        else {
            char *cs=snum;
            nval=0;
            if (!(buffer->flags & FLG_DIVD)) {
                pushbufs(buffer->decipoint);
                while (*cs == '0') {
                    pushbufs("zero");
                    cs++;
                }

                if (*cs) {
                    nval = strtol(cs, NULL, 10);
                    len += getNumTriplet(buffer, nval, 0, 0, 0);
                }
            }
            else {
                pushbufs("i");
                // czytanie dzielnych dla ułamków dziesiętnych
                nval = strtol(cs, NULL, 10);
                int nform=getPolyForm(nval) + 3* (rlen - 1);
                if (mode) {
                    len +=//getNumCntTriplet(buffer, nval, 1);
                        getNumCntTripletG(buffer, nval, 1, 1);
                    pushbufs(dg_subdecis[nform]);
                } else {
                    len += getNumTriplet(buffer, nval, 1, 1, 0);
                    pushbufs(dg_subdeci[nform]);
                }
            }

        }
    }
dunit:
    if (unit) {
        int idx = getNumForm(num, numlen, mode, female);
        pushbufs(unit->vals[idx]);
    }
    return len;
}


static size_t stringifyHour(poldit *buffer,int hms, uint8_t mode)
{
    uint8_t h=(hms >> 16) & 255;
    uint8_t m=(hms >> 8) & 255;
    uint8_t s=hms & 255;
    static const char * const scs[]={"sekunda", "sekundy","sekund"};
    size_t len=0;
    len += getNumPosTriplet(buffer,h, 1, 0, mode);
    if (m != 255) {
        int col = (s == 255) ?!(buffer->flags & FLG_COLL) : 1;
        len += getNumTriplet(buffer,m, 1, col , col?1:0);
        if (s != 255) {
            pushbufs("i");
            int n = getPolyForm(s);
            len += getNumTriplet(buffer,s, 1, 1, 0);
            pushbufs(scs[n]);
        }
    }
    return len;
}

size_t spellPart(poldit *buffer, const char *str, int lstr)
{
    size_t len = 0;
    while (lstr > 0 && *str == '0') {
        pushbufs("zero");
        str++;
        lstr--;
    }
    if (!lstr) return len;
    int n =0 ;
    while (lstr > 0) {
        n = (n * 10) + (*str++ - '0');
        lstr--;
    }
    len += getNumTriplet(buffer, n, 0, 0, 0);
    return len;
}

static size_t spellFmt(poldit *buffer, const char *str, int lstr)
{
    size_t len = 0;
    uint32_t znak;
    const char *d;
    int was = 0;
    while (lstr > 0) {
        znak = get_unichar(str, &d);
        if (zx_isspace(znak) || zx_isdash(znak)) {
            lstr -= d-str;
            str = d;
            continue;
        }
        if (zx_isdigit(znak)) {
            int nlen;
            for (nlen=0;nlen < lstr && isdigit(str[nlen]);nlen++);
            //printf("To spell: %-*.*s %d\n", nlen, nlen, str,nlen);
            while (nlen > 0) {
                if (was && buffer->flags & FLG_SPELCOMMA) pushbuf(",");
                was = 1;
                if (nlen <= 3) {
                    len += spellPart(buffer, str, nlen);
                    str += nlen;
                    lstr -=nlen;
                    nlen = 0;
                    //pushbuf(">"); // to taka kontrola była
                    break;
                }
                len += spellPart(buffer, str, 2);
                str+= 2;
                nlen -= 2;
                lstr -= 2; // KURWA TEGO ZABRAKŁO 
            }
            continue;
        }
        const char *t=zx_ismap(znak);
        if (t || zx_isalpha(znak) || znak == '.') {
            if (was && buffer->flags & FLG_SPELCOMMA) pushbuf(",");
            was = (isdigit(*d) && znak == '+')? 0 : 1;
            if (znak == '.') pushbufs("kropka");
            else if (t) pushbufs(t);
            else pushbufl(str, d-str);
        }
        lstr -= d-str;
        str = d;
    }
    return len;
}
        


/*
static size_t spellFmt(poldit *buffer, const char *str, int lstr)
{
    size_t len = 0;
    
    if (*str == '+') {
        pushbufs("plus");
        str++;
        lstr--;
    }
    //printf("LSTR %d %s\n", lstr, str);
    int was=0;
    while (lstr > 0) {
        while(lstr > 0) {
            if (isdigit(*str)) break;
            str++;
            lstr--;
        }
        if (!lstr) break;
        int nlen;
        for (nlen=0;nlen < lstr && isdigit(str[nlen]);nlen++);
        //printf("To spell: %-*.*s %d\n", nlen, nlen, str,nlen);
        while (nlen > 0) {
            if (was && buffer->flags & FLG_SPELCOMMA) pushbuf(",");
            was = 1;
            if (nlen <= 3) {
                len += spellPart(buffer, str, nlen);
                str += nlen;
                lstr -=nlen;
                nlen = 0;
                //pushbuf(">"); // to taka kontrola była
                break;
            }
            len += spellPart(buffer, str, 2);
            str+= 2;
            nlen -= 2;
            lstr -= 2; // KURWA TEGO ZABRAKŁO 
            
        }

    }
    return len;

}
*/

size_t stringifyFmt(poldit *buffer, struct numberForm *nf)
{
    size_t len = 0;
    int mode = nf->mode & 7;
    int genre = (nf->mode >> 3) & 3;
    int declin = (nf->mode >> 5) & 7;
    //printf("MGD %d %d %d %-*.*s\n",
    //    mode, genre, declin, nf->num1len, nf->num1len, nf->num1);
    if (mode >= FMT_SPELL) {
        len += spellFmt(buffer, nf->num1, nf->num1len);
        return len;
    }
    int nval=strtol(nf->num1, NULL, 10);

    if (mode == FMT_NORM) {
        len += getNumString(buffer, nval, 0,0);
        return len;
    }
    if (mode == FMT_POS) {
        //if (nval >= 1000) len += getNumString(buffer, nval, 0,0);
        //else
        //len += getNumPosTripletG(buffer, nval, genre, declin);
        len += getNumPosG(buffer, nval, genre, declin);
        return len;
    }
    if (mode == FMT_CNT) {
        len += getNumCntG(buffer, nval, genre, declin);
        return len;
    }
    pushbufs("format");
    return len;
}

size_t stringifyDate(poldit *buffer, struct numberForm *nf)
{
    size_t len=0;
    int day = nf->intv1 & 31;
    int mon = (nf->intv1 >> 5) & MONTH_NONE;
    int yer = (nf->intv1 >> 9) & YEAR_NONE;
    uint8_t flags = (nf->intv1 >> 23) & 0x7f;
    

    if (nf->starter) {
        pushbufl(nf->starter, nf->starterlen);
    }
    len += getNumPosG(buffer, day, 0, (flags & MONFLG_DECLINE) ? 1 : 0);
    if (mon == MONTH_NONE && !nf->subtyp2) return len;
    if (mon != MONTH_NONE) {
        if (flags & MONFLG_NOMINAL) pushbufs(namedMonNom[mon-1]);
        else pushbufs(namedMon[mon-1]);
        if (yer != YEAR_NONE) {
            if (flags & MONFLG_DECLINE) {
                len += getNumPosG(buffer, yer, 0, 1);
            }
            else {
                char buf[8];
                sprintf(buf,"%d",yer);
                len += stringifyNumber(buffer, SDT_INT,buf, strlen(buf),0,0,NULL);
            }
            //len += getNumPosG(buffer, yer, 0, 1);
        }
    }
    if (!nf->subtyp2) return len;
    day = nf->intv2 & 31;
    mon = (nf->intv2 >> 5) & MONTH_NONE;
    yer = (nf->intv2 >> 9) & YEAR_NONE;
    flags = (nf->intv2 >> 23) & 0x7f;

    if (nf->inner) {
        pushbufl(nf->inner, nf->innerlen);
    }
    len += getNumPosTripletG(buffer, day, 0, (flags & MONFLG_DECLINE) ? 1 : 0);
    if (mon == MONTH_NONE) return len;
    pushbufs(namedMon[mon-1]);
    printf("DFUPA\n");
    if (yer != YEAR_NONE) {
        if (flags & MONFLG_DECLINE) {
            len += getNumPosG(buffer, yer, 0, 1);
        }
        else {
            char buf[8];
            sprintf(buf,"%d",yer);
            len += stringifyNumber(buffer, SDT_INT,buf, strlen(buf),0,0,NULL);
        }
    }
    return len;
}


// parser

static int32_t _hmscode(uint8_t h, uint8_t m, uint8_t s)
{
    return (h<<16) | (m << 8) | s;
}

static int32_t _datecode(uint8_t d, uint8_t m, uint16_t y, uint8_t flags)
{
    return (flags << 23) | (y << 9) | ((m & 15) << 5) | d;
}

#define HPF_FORCE 1
#define HPF_MDUAL 2
#define HPF_MFINAL 4
#define HPF_MSECOND 8
#define HPF_FORM2 0x10
#define HPF_FDUAL 0x20
#define HPF_IDUAL 0x40

static int getNextNumberType(poldit *buffer, const char *str, uint16_t *dateflags)
{
    const char *rest, *endit;
    uint8_t dflag;
    int nfirst;
    uint8_t h, m, s, day, mon;
    uint16_t yer;
//static uint8_t getDate(poldit *buffer,const char *str, const char **rest,
//    uint8_t *day, uint8_t *month, uint16_t *year, int flags, uint8_t *outflags)

    if (!isdigit(*str)) return NTYP_NONE;
    if (getIPv4(buffer, str, &rest)) return NTYP_IPV4;
    if (getHour(buffer, str, &rest, &h, &m, &s)) {
        if (m != 255) return NTYP_HOUR;
    }

    nfirst = strtol(str, (char **) &endit, 10);
    // mianowna?
    if (getUnit(buffer, endit, &rest)) return NTYP_UNIT;
    if ((*endit == '.' || *endit == ',') && isdigit(endit[1])) {
        const char *cc;
        for (cc=endit+1;*cc && isdigit(*cc);cc++);
        if (*cc && getUnit(buffer, cc, &rest)) return NTYP_UNIT;
    }
    if (nfirst <1 || nfirst > 31) return NTYP_PLAIN;
    if (!getDate(buffer, str, &rest,&day, &mon, &yer,
        MONFLG_ARABIC, &dflag)) return NTYP_PLAIN;
    if (mon == MONTH_NONE) return NTYP_PLAIN;
    if (dflag & MONFLG_NATURAL) return NTYP_DATE;
    return NTYP_DATE | NTYP_PLAIN;
}

static int getDitMonth(const char *str, const char **rest, uint8_t flags, uint8_t *oflags)
{
    int mon;
    if ((flags & MONFLG_ARABIC) && isdigit(*str)) {
        mon = strtol(str, (char **) &str, 10);
        if (mon < 1 || mon > 12) return 0;
        *rest=str;
        *oflags = MONFLG_ARABIC;
        return mon;
    }
    int i;
    for (i=0;i<12;i++) {
        if (umatch(str,romanMon[i],&str)) {
            *oflags = MONFLG_ROMAN;
            break;
        }
        if (flags & MONFLG_NATURAL) {
            if (cmatch(str,namedMon[i],&str)) {
                *oflags = MONFLG_NATURAL;
                break;
            }
            if ((flags & MONFLG_NOMINAL) && cmatch(str, namedMonNom[i],&str)) {
                *oflags = MONFLG_NATURAL | MONFLG_NOMINAL;
                break;
            }
        }
    }
    if (i >= 12) return 0;
    *rest = str;
    return i+1;
}

static uint8_t getISODate(poldit *buffer, const char *str, const char **rest, int *d, int *m, int *y)
{
    const char *c;
    int _d, _m, _y;
    _y=strtol(str, (char **)&c, 10);
    if (c-str != 4) return 0;
    str=c;
    if (*str++!='-') return 0;
    _m=strtol(str, (char **)&c, 10);
    if (_m < 1 || _m > 12) return 0;
    if (c-str != 2) return 0;
    str=c;
    if (*str++!='-') return 0;
    _d=strtol(str, (char **)&c, 10);
    if (_d < 1 || _d > 31) return 0;
    if (c-str != 2) return 0;
    str=c;
    if (*str == 'T' && (getNextNumberType(buffer, str+1, NULL) & NTYP_HOUR))  str++;
    *d=_d;
    *m=_m;
    *y=_y;
    *rest=str;
    return 1;
}
    

static uint8_t getDate(poldit *buffer,const char *str, const char **rest,
    uint8_t *day, uint8_t *month, uint16_t *year, int flags, uint8_t *outflags)
{
    if (!isdigit(*str)) return 0;
    int _day,_month = MONTH_NONE, _year=YEAR_NONE;
    const char *estr, *cc;
    char marker = 0;
    int m;
    uint8_t oflags = 0;

    if (getISODate(buffer, str, &str, &_day,&_month, &_year)) {
        *outflags = MONFLG_ISODATE;
        goto retval;
    }
        
    _day = strtol(str, (char **)&str, 10);
    if (_day > 31) return 0;
    //printf("DATE PART <%s>\n",str);
    
    if (!*str) goto retval;
        
    if (*str == '.' || *str == '-') {
        marker = *str;
        estr = str+1;
        m = getDitMonth(estr, &estr, MONFLG_ARABIC, &oflags);
        if (m) {
            str = estr;
            _month = m;
            goto getyear;
        }
    }
    uint32_t znak=get_unichar(str, &cc);
    if (zx_isspace(znak)) {
        while (*cc) {
            estr = cc;
            znak = get_unichar(estr, &cc);
            if (!zx_isspace(znak)) break;
        }
        if (!*cc) goto retval;
        m=getDitMonth(estr, &estr, MONFLG_NATURAL | flags, &oflags);
        if (!m) goto retval;
        marker = 0; // means 'space'
        _month = m;
        str = estr;
        goto getyear;
    }
    goto retval;

getyear:
    //printf("GY <%s>\n", str);
    if (!*str) goto retval;
    estr = str;
    if (marker == *str) {
        estr ++;
    }
    else {
        znak = get_unichar(estr, &cc);
        //if (!zx_isspace(znak)) goto retval;
        while (*cc) {
            if (!zx_isspace(znak)) break;
            estr = cc;
            znak = get_unichar(estr,&cc);
        }
    }
    //printf("SS <%s>\n", estr);
    if (!*estr || !isdigit(*estr)) goto retval;
    m=strtol(estr,(char **) &estr, 10);
    if (m >= 1970 && m <= 2080) {
        _year = m;
        str = estr;
    }
retval:
    //printf("RETVAL\n");
    *day = _day;
    *month = _month;
    *year = _year;
    *rest = str;
    *outflags = oflags;
    return 1;
}

#define DATEFLG_START 0x100
#define DATEFLG_FINAL 0x200

static const struct {
    const char *pat;
    uint16_t flags;
} _datepars[] = {
    {"od dnia", MONFLG_FORCE | MONFLG_DECLINE | DATEFLG_START},
    {"od", MONFLG_DECLINE | DATEFLG_START},
    {"do dnia", MONFLG_FORCE | MONFLG_DECLINE | DATEFLG_FINAL},
    {"do", MONFLG_DECLINE | DATEFLG_FINAL},
    {NULL, 0}};

static uint8_t isDate(poldit *buffer,const char *str, const char **rest, struct numberForm *nf)
{
    memset((void *)nf, 0,sizeof(*nf));
    uint8_t d1, m1;
    uint8_t d2, m2;
    uint16_t y1;
    uint16_t y2;
    const char *cc; // temporary
    const char *edate;  // date end
    uint8_t outflags;

    const char *sb=NULL,*ib=NULL;int sl=0,il=0;

    
    if (getDate(buffer, str, &cc, &d1, &m1, &y1, MONFLG_NOMINAL,&outflags)) {
        //printf("M = %d, Y=%d, F=%02x\n", m1, y1, outflags);
        if (m1 == MONTH_NONE) return 0;
        if (y1 == YEAR_NONE && !(outflags & MON_FORCE_DATE)) return 0;
        *rest = cc;
        nf->intv1 = _datecode(d1,m1,y1,outflags);
        nf->num1 = str;
        nf->num1len = cc - str;
        nf->subtyp1 = SDT_DATE;
        nf->mode = outflags;
        return nf->type = DT_DATE;
    }

    int i;uint16_t flags1=0,flags2=0;
    for (i=0;_datepars[i].pat;i++) if (cmatch(str,_datepars[i].pat,&cc)) {
        flags1 = _datepars[i].flags;
        break;
    }
    if (!_datepars[i].pat) return 0;
    sb=_datepars[i].pat;
    sl=strlen(sb);
    str = cc;

    if (!getDate(buffer, str, &cc, &d1, &m1, &y1, 0, &outflags)) return 0;
    flags1 |= outflags;
    const char *nm1 = str, *nm2=NULL;
    int nm1len = cc-str, nm2len=0;
    str = cc;
    edate = cc;
    str = unblank(str);
    //printf("I am here %x <%s>\n",flags1,str);
    if (!(flags1 & DATEFLG_START)) goto retsingle;
    //printf("Searching\n");
    for (i=0;_datepars[i].pat;i++) {
        if ((_datepars[i].flags & DATEFLG_FINAL) && cmatch(str,_datepars[i].pat,&str)) {
            flags2 = _datepars[i].flags;
            break;
        }
    }
    //printf("DU %x\n",flags2);
    if (!(ib=_datepars[i].pat)) goto retsingle;
    //printf("AIL=%s\n",ib);
    il = strlen(ib);
    if (!(getNextNumberType(buffer, str, NULL) & NTYP_DATE)) goto retsingle;
    if (!getDate(buffer, str, &cc, &d2, &m2, &y2, 0, &outflags)) goto retsingle;
    //printf("FAN\n");
    flags1 |= outflags | flags2;
    nm2 = str;
    nm2len = cc - str;
    edate = cc;
    //printf("NM2 %s\n",nm2);
retsingle:
    outflags |= flags1;
    if (!nm2) { // single date;
        //printf("MFDX %x M1 %d Y1 %d\n",flags1 & MON_FORCE_DATE,m1, y1);
        if (!(flags1 & MON_FORCE_DATE) && m1 == MONTH_NONE) return 0;
        if (y1 == YEAR_NONE && !(flags1 & MON_FORCE_DATE)) return 0;
    }
    else {
        if (m2 == MONTH_NONE) return 0;
        if (!(flags1 & MON_FORCE_DATE)) return 0;
    }
    *rest = edate;
    nf->intv1 = _datecode(d1,m1,y1,flags1);
    nf->num1 = nm1;
    nf->num1len = nm1len;
    nf->starter = sb;
    nf->starterlen = sl;
    nf->subtyp1 = SDT_DATE;
    nf->mode = flags1;
    //printf("RESRE <%s>\n",edate);
    if (nm2) { // dual
        //printf("Dual date\n");
        nf->intv2 = _datecode(d2,m2,y2,flags1);
        nf->num2 = nm2;
        nf->num2len = nm2len;
        nf->inner = ib;
        nf->innerlen = il;
        nf->subtyp2 = SDT_DATE;
    }
    return nf->type = DT_DATE;
}
    

static const struct hourPrefix {
    const char *pfx;
    uint16_t flags;
    const char *repl;
} _hourPfx[]={
    {"od godziny", HPF_FORCE | HPF_MDUAL},
    {"do godziny", HPF_FORCE | HPF_MFINAL | HPF_MSECOND},
    {"około godziny", HPF_FORCE | HPF_MFINAL | HPF_MSECOND},
    {"od godz.", HPF_FORCE | HPF_MDUAL,"od godziny"},
    {"do godz.", HPF_FORCE | HPF_MFINAL | HPF_MSECOND,"do godziny"},
    {"ok. godziny", HPF_FORCE | HPF_MFINAL | HPF_MSECOND,"około godziny"},
    {"ok. godz.", HPF_FORCE | HPF_MFINAL | HPF_MSECOND,"około godziny"},
    {"około godz.", HPF_FORCE | HPF_MFINAL | HPF_MSECOND,"około godziny"},
    {"po godzinie", HPF_FORCE|HPF_MFINAL},
    {"po godz.", HPF_FORCE|HPF_MFINAL,"po godzinie"},
    {"o godzinie", HPF_FORCE|HPF_MFINAL},
    {"od około", HPF_MDUAL},
    {"do około", HPF_MFINAL | HPF_MSECOND},
    {"od ok,", HPF_MDUAL,"od około"},
    {"do ok.", HPF_MFINAL | HPF_MSECOND,"do około"},
    {"od", HPF_MDUAL},
    {"do", HPF_MFINAL | HPF_MSECOND},
    {"około",HPF_MFINAL},
    {"ok.",HPF_MFINAL,"około"},
    {"o",HPF_MFINAL},
    {"po",HPF_MFINAL},
    {"przed godziną",HPF_MFINAL | HPF_FORCE | HPF_FORM2},
    {"przed",HPF_MFINAL| HPF_FORM2},
    {"na godzinę",HPF_MFINAL | HPF_FORCE | HPF_FORM2},
    {"na",HPF_MFINAL| HPF_FORM2},
    {"pomiędzy godziną", HPF_FDUAL | HPF_FORCE | HPF_FORM2},
    {"pomiędzy godzinami", HPF_FDUAL | HPF_FORCE | HPF_FORM2},
    {"pomiędzy",HPF_FDUAL | HPF_FORM2},
    {"między godziną", HPF_FDUAL | HPF_FORCE | HPF_FORM2},
    {"między godzinami", HPF_FDUAL | HPF_FORCE | HPF_FORM2},
    {"między",HPF_FDUAL | HPF_FORM2},
    {"a godziną",HPF_IDUAL | HPF_FORCE | HPF_FORM2},
    {"i godziną",HPF_IDUAL | HPF_FORCE | HPF_FORM2},
    {"a",HPF_IDUAL | HPF_FORM2},
    {"i",HPF_IDUAL | HPF_FORM2},

    {NULL, 0}};

static uint8_t isHour(poldit *buffer,const char *str, const char **rest, struct numberForm *nf)
{
    uint8_t h1, m1, s1, h2, m2, s2;
    const char *start=str, *curr,*sbeg,*sinn,*n1s, *n2s;
    int lbeg=0, linn=0, ln1s=0, ln2s=0;
    if (getHour(buffer,start, &curr, &h1,&m1,&s1))  {
        if (m1 == 255) return 0;
        nf->num1=start;
        nf->num1len = curr-start;
        *rest = curr;
        nf->starter = NULL;
        nf->intv1 = _hmscode(h1, m1, s1);
        nf->type=DT_HOUR;
        nf->subtyp1 = SDT_HOUR;
        return DT_HOUR;
    }
    int i;
    uint16_t flags;
    for (i=0;;i++) {
        if (!_hourPfx[i].pfx) return 0;
        if (_hourPfx[i].flags & HPF_IDUAL) continue;
        if (cmatch(start,_hourPfx[i].pfx,&curr)) {
            flags = _hourPfx[i].flags;
            if (_hourPfx[i].repl) {
                sbeg = _hourPfx[i].repl;
                lbeg = strlen(sbeg);
            }
            else {
                sbeg = start;
                lbeg = curr-sbeg;
            }
            start = curr;
            break;
        }
    }
    if (!*start) return 0;
    if (!isdigit(*start)) return 0;
    if (!getHour(buffer,start, &curr, &h1, &m1, &s1)) return 0;
    if (m1 != 255) flags |= HPF_FORCE;
    n1s=start;
    ln1s = curr-start;
    start = curr;



    nf->num1=n1s;
    nf->num1len = ln1s;
    nf->intv1 = _hmscode(h1, m1, s1);
    nf->starter = sbeg;
    nf->starterlen = lbeg;
    nf->mode = (flags & HPF_FORM2) ? 2 : 1;
    nf->subtyp1 = SDT_HOUR;
    if (flags & HPF_MFINAL) {
        if (m1 == 255 && !(flags & HPF_FORCE)) return 0;
        *rest = curr;
        return nf->type = DT_HOUR;
    }
    const char *crest = curr;
    curr = unblank(curr);
    for (i=0;;i++) {
        if (flags & HPF_FDUAL) {
            if (!_hourPfx[i].pfx) return nf->type = 0;
            if (!(_hourPfx[i].flags & HPF_IDUAL)) continue;
        }
        else {
            if (!_hourPfx[i].pfx) {
                if (m1 == 255 && !(flags & HPF_FORCE)) {
                    return nf->type = 0;
                }
                *rest = crest;
                return nf->type = DT_HOUR;
            }
            if (!(_hourPfx[i].flags & HPF_MSECOND)) continue;
        }
        if (cmatch(start,_hourPfx[i].pfx,&curr)) {
            flags |= _hourPfx[i].flags;
            if (_hourPfx[i].repl) {
                sinn = _hourPfx[i].repl;
                linn = strlen(sbeg);
            }
            else {
                sinn = start;
                linn = curr-sinn;
            }
            start = curr;
            break;
        }
    }
    if (!(getNextNumberType(buffer, start, NULL) & NTYP_HOUR) || 
            !getHour(buffer, start, &curr, &h2, &m2, &s2)) {
        if (m1 ==255) return nf->type = 0;
        *rest = crest;
        return nf->type = DT_HOUR;
    }

    if (m1 == 255 && m2 == 255 && !(flags & HPF_FORCE)) return 0;

    n2s=start;
    ln2s = curr-start;
    start = curr;

    nf->num2=n2s;
    nf->num2len = ln2s;
    *rest = curr;
    nf->intv2 = _hmscode(h2, m2, s2);
    //printf("INNER %d %s\n",linn,sinn);
    nf->inner = sinn;
    nf->innerlen = linn;
    nf->subtyp2 = SDT_HOUR;

    return nf->type = DT_HOUR;
}

static int chrpos(const char *s, char c)
{
    const char *d=strchr(s,c);
    if (!d) return -1;
    return d-s;
}


static uint8_t isFormat(poldit *buffer, const char *str, const char **rest, struct numberForm *nf)
{
    if (*str++ != 'F') return 0;
    if (*str++ !='[') return 0;
    int mode=chrpos("pcnsS", *str++);
    if (mode < 0) return 0;
    int genre=0, declin=0;
    if (mode < 2) {
        if ((genre = chrpos("mfn", *str++)) < 0) return 0;
        if (*str == 'm' && str[1] == 'c') {
            declin = 5;
            str += 2;
        }
        else {
            declin = chrpos("012345", *str);
            if (declin < 0) declin = chrpos("mdcbnM", *str);
            if (declin < 0) return 0;
            str++;
        }

    }
    if (*str++ != ':') return 0;
    const char *strbeg=str;
    //printf("%s\n", strbeg);
    //if (mode == FMT_SPELLALL && *str == '+') str++;
    //if (!isdigit(*str)) return 0;
    uint32_t znak;
    const char *d;
    while (*str) {
        znak=get_unichar(str, &d);
        if ((mode == FMT_SPELLALL && znak !=']') || zx_isdigit(znak)) {
            str=d;
        }
        else break;
    }

    if (*str++ != ']') return 0;
    memset((void *)nf, 0,sizeof(*nf));
    nf->num1=strbeg;
    nf->num1len = d-strbeg;
    nf->subtyp1 = SDT_FMT;
    *rest = str;
    nf->mode = mode | (genre << 3) | (declin << 5);
    return nf->type = DT_FMT;
}


static uint8_t getNumber(poldit *buffer, const char *str, const char **rest, struct numberForm *nf)
{
    memset((void *)nf, 0,sizeof(*nf));
    if (isFormat(buffer, str, rest, nf)) return 1;
    if (isDate(buffer, str, rest, nf)) return 1;
    if (isHour(buffer, str, rest, nf)) return 1;
    if (isNumber(buffer, str, rest, nf)) return 1;
    return 0;

}

static size_t nextToken(poldit *buffer, const char *str, const char **rest)
{
    size_t len=0;
    const char *strcurr=str,*strnext;
    uint32_t znak = get_unichar(strcurr, &strnext);
    if (zx_isalnum(znak)) {
        strcurr=strnext;
        while (*strcurr) {
            znak = get_unichar(strcurr, &strnext);
            if (!zx_isalnum(znak)) break;
            strcurr=strnext;
        }
    }
    while (*strcurr) {
        if (*strcurr == '-' && isdigit(strcurr[1])) break;
        znak = get_unichar(strcurr, &strnext);
        if (zx_isalnum(znak))  break;
        if (znak == '\\') {
            if (strcurr != str) pushbufl(str,(int)(strcurr-str));
            str=strnext;
        }
        else {
            const char *t=zx_ismap(znak);
            if (t) {
                if (strcurr != str) pushbufl(str,(int)(strcurr-str));
                pushbufs(t);
                str=strnext;
            }
        }
        strcurr=strnext;
    }
    *rest =strcurr;
    if (strcurr != str) pushbufl(str, (int)(strcurr-str));
    return len;
}

size_t poldit_Convert(poldit *buffer,const char *c)
{
    struct numberForm nf;
    size_t len = 0;
    //printf("Converting %s\n",c);
    while (*c) {
        if (!getNumber(buffer,c, &c, &nf)) {
            //printf("Tokenizer %s\n",c);
            len += nextToken(buffer, c,&c);
            continue;
        }
        switch(nf.type) {
            case DT_DATE:
            len += stringifyDate(buffer,&nf);
            break;
            
            case DT_FMT:
            len+=stringifyFmt(buffer,&nf);
            break;
            
            case DT_HOUR:
            if (nf.starter) pushbufl(nf.starter, nf.starterlen);
            len += stringifyHour(buffer, nf.intv1,nf.mode);
            if (nf.subtyp2 == SDT_HOUR) {
                if (nf.inner) {
                    //pushbuf(" ");
                    pushbufl(nf.inner, nf.innerlen);
                }
                len += stringifyHour(buffer, nf.intv2,nf.mode);
            }
            break;

            case DT_NUM:
            if (nf.starter) pushbufl(nf.starter, nf.starterlen);
            len += stringifyNumber(buffer, nf.subtyp1, nf.num1, nf.num1len, nf.starter ? nf.mode: 0,
                nf.unit ? nf.unit->flags & FLG_FEMALE : 0, nf.subtyp2?NULL:nf.unit);
            if (nf.subtyp2) {
                if (nf.inner) {
                    pushbufl(nf.inner, nf.innerlen);
                }
                len += stringifyNumber(buffer, nf.subtyp2, nf.num2, nf.num2len, nf.mode,
                nf.unit ? nf.unit->flags & FLG_FEMALE : 0,
                nf.unit);
            }
            //pushbufs("numeric");
            break;

            case DT_IPV4:
            len += stringifyIPv4(buffer, nf.num1, nf.num1len);
            break;
        }

        //pushbuf(" ");
    }
    if (buffer->outbuf) *(buffer->outbuf) = 0;
    //printf("KLEN %d\n",(int)len);
    return len;

}


poldit *poldit_Init(poldit *buffer)
{
    if (!buffer) buffer=malloc(sizeof(poldit));
    strcpy(buffer->decipoint,"i");
    buffer->outbuf=NULL;
    buffer->mem = NULL;
    buffer->blank = 1;
    buffer->flags = unit_initial;
    buffer->memlen = 0;
    buffer->blksize = 0;
    return buffer;
}

void poldit_setColloquial(poldit *buffer, int yesno)
{
    if (yesno) buffer->flags |= FLG_COLL;
    else buffer->flags &= ~FLG_COLL;
}

int poldit_getColloquial(poldit *buffer)
{
    return (buffer->flags & FLG_COLL) ? 1 : 0;
}

uint8_t poldit_setDecipoint(poldit *buffer, const char *dp)
{
    if (strlen(dp) > 15) return 0;
    strcpy(buffer->decipoint,dp);
    return 1;
}

const char *poldit_getDecipoint(poldit *buffer)
{
    return buffer->decipoint;
}

void poldit_setDecibase(poldit *buffer, int yesno)
{
    if (yesno) buffer->flags |= FLG_DIVD;
    else buffer->flags &= ~FLG_DIVD;
}

int poldit_getDecibase(poldit *buffer)
{
    return (buffer->flags & FLG_DIVD) ? 1 : 0;
}

void poldit_setNatural(poldit *buffer, int yesno)
{
    if (yesno) buffer->flags |= FLG_NATURAL;
    else buffer->flags &= ~FLG_NATURAL;
}

int poldit_getNatural(poldit *buffer)
{
    return (buffer->flags % FLG_NATURAL) ? 1 : 0;
}

void poldit_setSepspell(poldit *buffer, int yesno)
{
    if (yesno) buffer->flags |= FLG_SPELCOMMA;
    else buffer->flags &= ~FLG_SPELCOMMA;
}

int poldit_getSepspell(poldit *buffer)
{
    return (buffer->flags & FLG_SPELCOMMA) ? 1 : 0;
}

int poldit_getSelUnits(poldit *buffer, char *units)
{
    int i,rc;
    for (i=rc=0;unitsel[i];i++) {
        if (buffer->flags & (UNIT_MIN << i)) {
            if (units) *units++ = unitsel[i];
            rc ++;
        }
    }
    if (units) *units = 0;
    return rc;
}

void poldit_selUnits(poldit *buffer, const char *usel)
{
    int neg = 0;
    if (usel[0] == '+') usel++;
    else if (usel[0] == '-') {
        usel++;
        neg = 1;
    }
        
    else buffer -> flags &= ~UNIT_ALL;
    for (;*usel;usel++) {
        if (*usel == 'A') {
            buffer -> flags |= UNIT_ALL;
            return;
        }
        if (*usel == 'B') {
            buffer -> flags &= ~UNIT_ALL;
            return;
        } 
        int n = chrpos(unitsel, *usel);
        if (n >= 0) {
            if (neg)
                buffer->flags &= ~(UNIT_MIN << n);
            else
                buffer->flags |= UNIT_MIN << n;
        }
    }
}


// Dodaje statyczny bufor do bufora poldit.
// Ustawia bufor w pozycji startowej
// Ustawia tryb pracy bufora na rzeczywistą konwersję

void poldit_setBuffer(poldit *buffer, char *buf)
{
    if (buffer->mem && buffer->memlen) free(buffer->mem);
    buffer->outbuf = buffer->mem = buf;
    buffer->blank = 1;
    *buf=0;
    buffer->memlen = 0;
}

// Ustawia bufor w pozycji startowej
// Ustawia tryb pracy bufora na rzeczywistą konwersję
// Nie sprawdza czy pamięć była przydzielona!

void poldit_enableBuffer(poldit *buffer)
{
    buffer->outbuf = buffer->mem;
    buffer->blank = 1;
    *buffer->mem=0;
}

// Ustawia bufor w pozycji startowej
// Ustawia tryb pracy bufora na pomiar długości wynikowego napisu

void poldit_resetBuffer(poldit *buffer)
{
    buffer->outbuf = NULL;
    buffer->blank = 1;
}

// jeśli pamięć bufora nie była przydzielona lub była mniejsza niż len
//   - zwalnia pamięć bufora jeśli była zallokowana
//   - przydziela obszar pamięci wystarczający do pomieszczenia napisu
//     o długości len
// Ustawia bufor w pozycji startowej
// Ustawia tryb pracy bufora na rzeczywistą konwersję

void poldit_allocBuffer(poldit *buffer, size_t len)
{
    len += 1;
    if (buffer->memlen < len || !buffer->mem) {
        if (buffer->memlen && buffer->mem) free(buffer->mem);
        if (buffer->blksize) {
            int n = (len + buffer->blksize-1)/buffer->blksize;
            len = n * buffer->blksize;
        }
        buffer->mem = (char *)malloc(buffer->memlen = len);
    }
    buffer->blank = 1;
    buffer->outbuf = buffer->mem;
    *buffer->mem=0;
}

// ustawia wielkość przydzielanego bloku pamięci

void poldit_setAllocSize(poldit *buffer, size_t len)
{
    buffer->blksize = len;
}

// zwalnia pamięć bufora jeśli była przydzielona

void poldit_freeBuffer(poldit *buffer)
{
    if (buffer->mem && buffer->memlen) free(buffer->mem);
    buffer->mem = buffer->outbuf = NULL;
    buffer->memlen = 0;
}

const char *poldit_getUnitTypes(void)
{
    return uudescr;
}


int poldit_unitType(int unitPos, char utype, char *buffer)
{
    int n;uint32_t flags;
    if (utype == 'A') {
        flags =UNIT_ALL;
    }
    else {
        n = chrpos(unitsel, utype);
        if (n < 0) return -1;
        flags = UNIT_MIN << n;
    }
    for (;unitDefs[unitPos].unit; unitPos++) {
        if (!(unitDefs[unitPos].flags & flags)) continue;
        if (unitDefs[unitPos].flags & (FLG_COLL | FLG_BASEIGNORE)) continue;
        if (unitDefs[unitPos].flags & FLG_BASEUNIT) break;
    }
    if (!unitDefs[unitPos].unit) return -1;
    sprintf(buffer,"%s %s", unitDefs[unitPos].unit, unitDefs[unitPos].vals[0]);
    return unitPos + 1;
}



