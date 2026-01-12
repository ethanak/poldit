# POLDIT

Library for phonetic number conversion for the Polish language.

As it is interested only for Polish language speakers, further documentation
will be only in Polish

Biblioteka służąca do tworzenia fonetycznego tekstu z treści zawierającej
liczby (w tym daty, godziny i wartości mianowane). Utworzona z myślą o
tworzeniu komunikatów bezpośrednio dla syntezatora mowy (RHVoice, Ivona,
Vocalizer i innych) oraz do odczywaniu przez syntezatory dostarczonych
treści ze źródeł zewnętrznych (np. prognoza pogody). Rozpoznaje:

* liczby z zakresu -999999 do 999999
* liczby z częścią ułamkową (interpretacja do trzech miejsc po przecinku)
* godziny w zapisie HH:MM i HH:MM:SS
* daty (w tym daty w formacie ISO YYYY:mm:dd i YYYY:mm:ddT\<godzina>)
* adresy IPv4
* liczby mianowane (np. 20 km, 5V)
* zakresy od ... do ...

Odmiana liczebników jest względiona w popularnych przypadkach (np. "pięć
kilometrów", ale "od pięciu kilometrów).

Dołączony program **poldit** służy do zadeonstrowania możliwości, ale może
być użyty jako prosty konwerter.

## FUNKCJE:

```c
typedef struct poldit {
    char decipoint[16];
    char *outbuf;
    char *mem;
    size_t memlen;
    size_t blksize;
    unsigned int flags:31;
    unsigned int blank:1;    
} poldit;

poldit *poldit_Init(poldit *buffer)
size_t poldit_Convert(poldit *buffer,const char *str)
void poldit_allocBuffer(poldit *buffer, size_t len)
void poldit_setAllocSize(poldit *buffer, size_t len)
void poldit_resetBuffer(poldit *buffer)
void poldit_freeBuffer(poldit *buffer)
void poldit_setBuffer(poldit *buffer, char *buf)
void poldit_enableBuffer(poldit *buffer)
void poldit_setColloquial(poldit *buffer, int yesno)
void poldit_setDecibase(poldit *buffer, int yesno)
void poldit_setSepspell(poldit *buffer, int yesno)
void poldit_setNatural(poldit *buffer, int yesno)
uint8_t poldit_setDecipoint(poldit *buffer, const char *dp)
void poldit_selUnits(poldit *buffer, const char *usel)
const char *poldit_getUnitTypes(void)
int poldit_unitType(int unitPos, char utype, char *buffer)

```

**poldit \*poldit_Init(poldit \*buffer)**

Inicjalizuje bufor. Musi być wywołane raz przed wszystkimi innymi funkcjami.
Jeśli argument jest NULL, przydziela pamięć dla bufora. Zwraca adres bufora.

**size_t poldit_Convert(poldit \*buffer,const char \*str)**

Główna funkcja konwersji tekstu. Zwraca długość wynikowego tekstu
(bez kończącego \\0). Może pracować w dwóch trybach:
* tryb obliczania długości. W tym trybie nie jest wykonywana rzeczywista konwersja,
  tak więc pamięć bufora nie musi być zainicjalizowana.
* Tryb rzeczywisty. W tym trybie wykonywana jest rzeczywista konwersja, tak więc
  obszar pamięci przydzielony dla bufora musi zmieścić napis wynikowy. Wynik konwersji
  w polu mem bufora.

**void poldit_allocBuffer(poldit \*buffer, size_t len)**

Przydziela pamięć dla bufora. Jeśli pamięć była już przydzielona, a jej wielkość
pozwala na zmieszczenie napisu, nie zmienia przydzielonego obszaru. W przeciwnym
przypadku pamięć zostaje zwolniona (jeśli była przydzielona), i zostanie przydzielony
nowy obszar pamięci o odpowiedniej wielkości. Funkcja ustawia tryb rzeczywistej
konwersji oraz ustawia bufor na początek przetwarzania.

**void poldit_setAllocSize(poldit \*buffer, size_t len)**

Ustala wielkość przydzielanego bloku. Wielkość przydzielanego obszaru
będzie wielokrotnością len lub - w przypadku zera - będzie minimalną
wielkością potrzebną do zmieszczenia napisu.

**void poldit_resetBuffer(poldit \*buffer)**

Ustawia bufor na początek przetwarzania i przełącza na tryb obliczania
długości. Należy stosować razem z ```poldit_allocBuffer```.

**void poldit_freeBuffer(poldit \*buffer)**

Zwalnia pamięć przydzieloną dla bufora jeśli była przydzielona.

**void poldit_setBuffer(poldit \*buffer, char \*buf)**

Podłącza statyczny obszar pamięci do bufora. Musi on być wystarczający
do pomieszczenia napisu wynikowego. Należy stosować w wyjątkowych przypadkach,
jeśli znamy maksymalną długość wynikowego napisu!

**void poldit_enableBuffer(poldit \*buffer)**

Ustawia bufor w tryb rzeczywistej konwersji i ustawia początek przetwarzania.
Należy stosować razem z ```poldit_setBuffer```.

**void poldit_setColloquial(poldit \*buffer, int yesno)**

Ustawia tryb wymowy potocznej

**void poldit_setSepspell(poldit \*buffer, int yesno)**

Ustawia tryb wyraźnego oddzielania liter i liczb przy literowaniu

**void poldit_setDecibase(poldit \*buffer, int yesno)**

Przełącza na tryb czytania ułamków dziesiętnych (czyli 2.25 będzie czytane
jako "dwa i dwadzieścia pięć setnych")

**void poldit_setNatural(poldit \*buffer, int yesno)**

Usuwa tzw. "hiperpoprawność" występującą w niektórych syntezatorach
(np. RHVoice). Przykładowo: syntezator może wymawiać słowo "pięćdziesiąt"
ignorując reguły wymowy i wymawiać niepotrzebnie głoski "ę", "ć" i "dź".
Naturalizacja polega na zmianie tego słowa na prawidłowy fonetycznie
"pieńdziesiąt". Dotyczy to wyłącznie kilku zbitek liter i tylko w
generowanych liczebnikach.

**uint8_t poldit_setDecipoint(poldit \*buffer, const char \*dp)**

Ustawia sposób czytania kropki dziesiętnej. Domyślnie jest to "i"
(czyli 2.2 będzie czytane jako "dwa i dwa"), podanie np. "kropka"
spowoduje czytanie jako "dwa kropka dwa". Maksymalna długość
to 15 bajtów. Zwraca 1 jeśli udało się ustawić, 0 jeśli nie.

**void poldit_selUnits(poldit \*buffer, const char \*usel)**

Ustawia rozpoznawane typy jednostek (patrz **poldit_getUnitTypes**),
Argument **usel** może zawierać więcej niż jeden ty, przykładowo
jeśli **T** to typ 'czas', **E** to typ 'elektryka' po wywołaniu:
```c
poldit_selUnits(buffer, 'TE');
```
będą rozpoznawane jednoski takie jak 'µs' czy 'Ah', ale nie 'km/h'.
Głównym celem jest uniknięcie traktowania jako jednostki niektórych
specyficznych konstrukcji, np 'autobus linii 5 A' nie powinien być przełożony
na 'autobus linii 5 amper'.

**const char \*poldit_getUnitTypes(void)**

Zwraca statyczny napis, zawierający wykaz wszystkich wkompilowanych
typów jednostek.

**int poldit_unitType(int unitPos, char utype, char \*buffer)**

Parametr **unitPos** powinien być inicjalizowany wartością 0.
Parametr **utype** to znak odpowiadający typowi. Jeśli istnieje
jednostka podstawowa danego typu, do bufora **buffer** zostaje
wpisany opis danej jednostki, a funkcja zwróci następny indeks.
Jeśli takich już nie ma, funkcja zwraca -1. Przykładowo:

```c
    int i; char buffer[80];
    for (i=0;;) {
        i=poldit_unitType(i, 'E', buffer);
        if (i < 0) break;
        printf("%s\n", buffer);
    }
```
wypisze opisy wszystkich jednostek typu 'E'. Jeśli 'E' odpowiada elektryce, wynik może wyglądać tak:

ohm om\
Ah amperogodzina\
Hz herc\
Wh watogodzina\
A amper\
F farad\
H henr\
Ω om\
W wat\
V wolt


## TYPOWE WYWOŁANIA:

```c
poldit mainBuffer;
```

### Statyczny bufor:

```c
    char buf[1024];
    poldit_initBuffer(&mainBuffer);
    poldit_setBuffer(&mainBuffer,&buf);
    for (;;) {
        char *str = get_some_string();
        if (!str) break;
        poldit_enableBuffer(&mainBuffer);
        poldit_Stringify(&mainBuffer, str);
        printf("%s\n", mainBuffer.mem);
    }
```
### Dynamiczny bufor:

```c
    poldit_initBuffer(&mainBuffer);
    for (;;) {
        char *str = get_some_string();
        if (!str) break;
        poldit_resetBuffer(&mainBuffer);
        size_t len = poldit_Stringify(&mainBuffer, str);
        poldit_allocBuffer(&mainBuffer, len);
        poldit_Stringify(&mainBuffer, str);
        printf("%s\n", mainBuffer.mem);
    }
    poldit_freeBuffer(&mainBuffer);
```

## KONWERSJA

* Propgram rozpoznaje daty, godziny, adresy IPv4 oraz zwykłe liczby
* Program rozpoznaje również typowe konstrukcje (np. od ... do ...)
* W przypadku kiedy program błędnie odmienia liczby można poprzedzić ją znakiem '\' - przykładowo:
    * ```numery 1 do 3```  => numery jeden do trzech
    * ```numery 1 do \3``` => numery jeden do trzy

## FORMATOWANIE:

Ciąg formatujący ```F[<tryb>:<liczba>]```

Tryb:
* n - liczebnik
* p - liczebnik porządkowy
* c - liczebnik zbiorowy
* s - spelling (liczba)
* S - spelling (dowolny ciąg znaków).

Dla p i c należy podać również:

* rodzaj (m, f lub n)
* przypadek (0-5, jeden z m, d, c, b, n, M lub mc jako miejscownik)

Przykłady szablonów:

```
Pociąg ze Szczecina wjeżdża na tor F[pmb:%d] przy peronie F[pmM:%d]
Autobus do Pcimia wjeżdża na stanowisko F[pnb:%d]
Wagony numer %d do \%d zatrzymują się w sektorze F[pmM:%d]
telefon F[S:+%s %s]
```
