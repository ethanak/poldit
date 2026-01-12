#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <poldit.h>

poldit mainBuffer;

void helpme(const char *arg0)
{
    printf("%s -h\n\
    pomoc\n\
%s -U <znak>\n\
    wypisuje rozpoznawane jednostki na podstawie znaku\n\
%s [-p] [-d] [-s] [-n] [-k <string>] [-u <string>] [tekst do konwersji]\n\
    Jeśli nie jest podany tekst, czytana jest linia z stdin\n\
    Znaczenie przełączników:\n\
     -p - tryb potoczny\n\
     -d - czytanie podstawy ułamków dziesiętnych\n\
     -s - wyraźne oddzielanie przy literowaniu\n\
     -n - naturalizacja fonetyczna\n\
     -k <string> - kropka dziesiętna (np. \"kropka\", domyślnie \"i\")\n\
     -u <string> - selektor rozpoznawanych jednostek:\n\
%s", arg0, arg0,arg0,poldit_getUnitTypes());
    exit(0);
}

void helpunit(char u)
{
    int i;
    char buf[80];
    for (i=0;;) {
        i=poldit_unitType(i, u,buf);
        if (i <0) break;
        printf("%s\n",buf);
    }
}

int main(int argc, char *argv[])
{
    char inbuf[1024];
    int opt;
    int qua=0;
    int dvd = 0;
    int spl = 0;
    int nat = 0;
    const char *un=NULL;
    const char *dp=NULL;
    for (opt=1;opt < argc;) {
        //if (argc <= opt) helpme(argv[0]);
        if (!strcmp(argv[opt],"-h")) {
            helpme(argv[0]);
        }
        else if (!strcmp(argv[opt],"-p")) {
            qua=1;
            opt++;
        }
        
        else if (!strcmp(argv[opt],"-n")) {
            nat=1;
            opt++;
        }
        else if (!strcmp(argv[opt],"-d")) {
            dvd=1;
            opt++;
        }
        else if (!strcmp(argv[opt],"-s")) {
            spl=1;
            opt++;
        }
        else if (!strcmp(argv[opt],"-k") && opt < argc-1) {
            dp=argv[opt+1];
            opt += 2;
        }
        else if (!strcmp(argv[opt],"-u") && opt < argc-1) {
            un=argv[opt+1];
            opt += 2;
        }
        else if (!strcmp(argv[opt],"-U") && opt < argc-1) {
            helpunit(argv[opt+1][0]);
            exit(0);
        }
        else break;
    }
    if (opt >= argc) {
        fgets(inbuf, 1024, stdin);
        char *c = strpbrk(inbuf,"\r\n");
        if (c) *c = 0;
    }
    else {
        int i;
        for (i=opt;i<argc;i++) {
            if (i==opt) strcpy(inbuf,argv[i]);
            else {
                strcat(inbuf," ");
                strcat(inbuf, argv[i]);
            }
        }
    }
    poldit_Init(&mainBuffer);
    poldit_setColloquial(&mainBuffer, qua);
    poldit_setDecibase(&mainBuffer, dvd);
    poldit_setSepspell(&mainBuffer, spl);
    poldit_setNatural(&mainBuffer, nat);
    if (dp) poldit_setDecipoint(&mainBuffer, dp);
    if (un) poldit_selUnits(&mainBuffer, un);
    size_t len=poldit_Convert(&mainBuffer, inbuf);
    poldit_allocBuffer(&mainBuffer, len);
    poldit_Convert(&mainBuffer, inbuf);
    printf("%s\n",mainBuffer.mem);
    return 0;
}
