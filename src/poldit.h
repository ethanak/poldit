#ifndef POLDIT_H
#define POLDIT_H

#include <stdint.h>

typedef struct poldit {
    char decipoint[16];
    char *outbuf;
    char *mem;
    size_t memlen;
    size_t blksize;
    unsigned int flags:31;
    unsigned int blank:1;
    
} poldit;


#ifdef __cplusplus
extern "C" {
#endif

extern size_t poldit_Convert(poldit *buffer,const char *c);
extern poldit *poldit_Init(poldit *buffer);
extern void poldit_setColloquial(poldit *buffer, int yesno);
extern uint8_t poldit_setDecipoint(poldit *buffer, const char *dp);
extern void poldit_setNatural(poldit *buffer, int yesno);
extern void poldit_setDecibase(poldit *buffer, int yesno);
extern void poldit_setSepspell(poldit *buffer, int yesno);
extern void poldit_selUnits(poldit *buffer, const char *usel);

extern const char *poldit_getDecipoint(poldit *buffer);
extern int poldit_getColloquial(poldit *buffer);
extern int poldit_getNatural(poldit *buffer);
extern int poldit_getDecibase(poldit *buffer);
extern int poldit_getSepspell(poldit *buffer);
extern int poldit_getSelUnits(poldit *buffer, char *units);

extern void poldit_setBuffer(poldit *buffer, char *buf);
extern void poldit_allocBuffer(poldit *buffer, size_t len);
extern void poldit_setAllocSize(poldit *buffer, size_t len);
extern void poldit_freeBuffer(poldit *buffer);
extern void poldit_resetBuffer(poldit *buffer);
extern const char *poldit_getUnitTypes(void);
extern int poldit_unitType(int unitPos, char utype, char *buffer);

#ifdef __cplusplus
}
#endif


#endif


