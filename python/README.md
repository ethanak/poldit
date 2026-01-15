# Moduł poldit Python3

## Moduł poldit

Moduł składa się z:

* Niskopoziomowego modułu polditLL
* Bardziej rozbudowanego modułu poldit, umożliwiającego również
stosowanie wyrażeń regularnych do dokładniejszej konwersji

Moduł **polditLL** to po prostu interfejs do biblioteki poldit. Nie jest
przewidziany do używania w aplikacjach (chociaż nic nie stoi na przeszkodzie
aby go tak traktować), a raczej do tworzenia bardziej specjalizowanych
(lub bardziej uniwersalnych) modułów na nim bazujących.

Moduł **poldit** jest dość prostym, ale mającym dużo większe możliwości
narzędziem. Pozwala na przetwarzanie stringów zarówno na etapie wstępnej
obróbki, jak i korekty stringu będącego wynikiem działania funkcji Stringify.

Przetwarzanie dzieli się na cztery etapy.

* W etapie pierwszym po prostu zachodzi prosta substytucja (re.sub).
* W etapie drugim każdy element to lista zawierająca:
    * wyrażenie regularne zawierające nazwane grupy
    * słownik nazwa:substytucja dla każdej nazwanej grupy
    * opcjonalny trzeci element (flagi)
* Etap trzeci to wywołanie funkcji Stringify
* Etap czwarty to prosta zamiana słownikowa

W etapie drugim substytucje są normalnymi stringami (nie są
one traktowane jak wyrażenia ragularne). Jeśli konieczne jest
umieszczenie zawartości grupy w substytucji, należy użyć
wyrażenia **<G>** (patrz przykład).

Pole flag dla etapu drugiego może zawierać:

* **i** - oznacza, że wielkość liber nie będzie brana pod uwagę
* **b** - oznacza, że wyrażenie zostanie otoczone ```r'\b('``` ... ```r')\b'``` (wymuszenie granicy słowa)
* **w** - oznacza, że wyrażenie zostanie otoczone ```r'\b('``` ... ```r')(?=\W|$)'``` (podobnie jak wyżej, ale nie jest dokładnie tym samym)
* **a** - oznacza, że wyrażenie zostanie otoczone ```r'\b('``` ... ```r')(?=[^.\w]|$)``` (do stosowania w skrótowcach, które mogą ale nie muszą końəczyć się kropką)

W pozostałych etapach brane jest pod uwagę wyłącznie **i**.

## Instalacja

**UWAGA**

Sposób instalacji podano dla Linuksa!

Instalacja jest typowa. Należy zainstalować moduły wymagane przez
instalator (setuptools) oraz deweloperski Python3.
Zalecane jest rozszerzenie build, czyli:

```sh
pip3 install build
```
Następnie:

```sh
cd module
python3 -m build --no-isolation
pip3 install .
```

Jeśli wszystko jest prawidłowo skonfigurowane, instalacja
powinna przejść bez problemu.

**UWAGA**

W niektórych dystrybucjach, jeśli nie korzystamy z virtualenv, może być
konieczne dodanie do wywołania **pip3** parametru **--break-system-packages**
nawet, jeśli instalacja nie jest globalna a użytkownika.


## Przykład zastosowania - plik **demo/kolej.py**.

### Przetwarzanie wstępne - lista wyrażeń:

```python
[
    [r'\[\[(.*?)\]\]',r'F[S:\1]'],
    ]
```
Lista zawiera tylko jeden element. Zamienia on fragmenty typu
```[[tekst]]`` na ``F[SLtekst]```. Ma na celu uproszczenie wprowadzania
fragmentów do literowania - przykładowo łatwiejsze do zapamiętania:

```Pociąg numer [[IC 23144]] "Kowalski"```

zostanie zamieniony na rozumiane przez bibliotekę:

```Pociąg F[S:IC 23144] "Kowalski"```

W efekcie wynikowy napis będzie wyglądał tak:

```Pociąg I C dwadzieścia trzy sto czterdzieści cztery "Kowalski"```

### Druga faza przetwarzania:

Wyrażenia:

```python
[r'((w sektorze)|(na (torze|stanowisku))|((na|przy) peronie)) (?P<a>(\d+|%d|{}))',
        {'a':'F[pmn:<G>]'}],
    [r'((z (toru|peronu))|(ze stanowiska))\s+(?P<a>(\d+|%d|{}))',
        {'a':'F[pmd:<G>]'}],
    [r'na (tor|peron) (?P<a>(\d+|%d|{}))',
        {'a':'F[pmb:<G>]'}],
    [r'na stanowisko (?P<a>(\d+|%d|{}))',
        {'a':'F[pnb:<G>]'}],
    [r'numer\S+( od)? (?P<a>\d+) do (?P<b>\d+)',
        {'a':'F[n:<G>]','b':'F[n:<G>]'}],
```

służą do prawidłowej odmiany liczb w wyrażeniach typu "wjeżdża na tor N przy peronie M"
oraz do prawidłowej wymowy numerów wagonów.

Wyrażenia:

```python
    [r'\S+a ((?P<a>gł\.?)|(?P<b>zach\.?)|(?P<c>wsch\.?)|(?P<d>centr?\.?))',
        {'a':'Główna','b':'Zachodnia','c':'Wschodnia','d':"Centralna"},'ia'],
    [r'\S+e ((?P<a>gł\.?)|(?P<b>zach\.?)|(?P<c>wsch\.?)|(?P<d>centr?\.?))',
        {'a':'Główne','b':'Zachodnie','c':'Wschodnie','d':"Centralne"},'ia'],
    [r'((?P<a>gł\.?)|(?P<b>zach\.?)|(?P<c>wsch\.?)|(?P<d>centr?\.?))',
        {'a':'Główny','b':'Zachodni','c':'Wschodni','d':"Centralny"},'ia'],
```

to rozwinięcia skrótowych "Warszawa Wsch." czy "Szczecin Gł" na pełne
określenia "Warszawa wschodnia" i "Szczecin główny".

### Korekta słownika

Korekta zachodzi już na stringu będącym wynikiem Stringify.
Wyrażenia:

```python
[
    ['IC','intersjity'],
    ['KS',"koleje śląskie"],
]
```

zamieniają po prostu słowa na rozwinięcia.

## Przykłady rzeczywistej aplikacji

### demo/meteo.py

Aplikacja pobiera aktualny stan pogody z serwisu openmeteo
i wypisuje na wyjściu napis, który może być przekazany bezpośrednio
do syntezatora.

Przykłady wywołania (dla Linuksa):

```sh
#dla espeak
python3 demo/meteo.py | espeak -vpl

#dla RHVoice
python3 demo/meteo.py | RHVoice-test -r 130 -p alicja

#dla speech-dispatchera
spd-say -l pl -t female1 -r 30 "`python3 demo/meteo.py`"
#lub
python3 demo/meteo.py | spd-say -l pl -t female1 -r 30 -e >/dev/null
```

### demo/meteo2.py

Aplikacja pobiera aktualny stan pogody z serwisu openmeteo
i przekazuje komunikat do speech-dispatchera. Wymaga skonfigurowanego
speech-dispatchera oraz modułu python3-speechd.

Przykład ustawień dla RHVoice:

```python

    spd_data = {
        'module': 'rhvoice',
        'voice': 'alicja',
        'pitch': -10,
        'rate': 30
    }
```
