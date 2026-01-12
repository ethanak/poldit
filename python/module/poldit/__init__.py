#!/usr/bin/env python3

import polditLL
try:
    import regex as re
except:
    import re

def getUnitsInClass(s):
    """ Zwraca listę opisów jednostek w jednoliterowej klasie 
"""
    i=0
    bs={}
    while True:
        rc = polditLL.unitInClass(i,s)
        if rc is None:
            break
        i = rc[0]
        st = rc[1].split(None,1)
        if st[1] not in bs:
            bs[st[1]] = []
        bs[st[1]].append(st[0])
    bo=[]
    for key in sorted(bs.keys()):
        bo.append('%s: %s' % (key, ', '.join(bs[key])))
    return bo

class Fonetyzer(polditLL.Poldit):
    cre = re.compile(r'\s+')
    def __init__(self,*args,**kwargs):
        polditLL.Poldit.__init__(self, *args, **kwargs)
        self.fdic = []
        self.idic = []
        self.pdic = []

    def addPostItem(self,pat, rep, fg=None):
        """Przyjmuje trójkę (wzorzec, zamiennik, flagi) stosowaną na końcu
przetwarzania. Wzorzec jest przetwarzany na eskejpowane wyrażenie
regularne, rozpoczynające się i kończące granicą słowa. Jeśli fg
jest stringiem i zawiera 'i', wyrażenie jest kompilowane z flagą
IGNORE. Służy do dodawania prostego słownika
(np. addPostItem('KS','koleje śląskie').
"""
        flags = 0
        if fg is not None and 'i' in fg:
            flags = re.I
        r=re.compile(r'\b('+ re.escape(pat)+r')\b', flags)
        self.idic.append([r,rep])

    def addPostList(self, lista):
        """Dodaje listę trójek do stosowania na końcu przetwarzania"""
        for item in lista:
            self.addPostItem(item[0], item[1], item[2] if len(item)> 2 else None)

    def addPreItem(self, pat, rep, fg = None):
        """Przyjmuje trójkę (regex, zamiennik, flagi) stosowaną na początku
przetwarzania. Jeśli fg jest stringiem i występuje tam 'i', regex
jest kompilowane z IGNORE. Wyrażenia są przykładane od pierwszego
dodanego."""
        flags = 0
        if fg is not None:
            if 'i' in fg:
                flags |= re.I
        self.pdic.append([re.compile(pat, flags), rep])

    def addPreList(self, lista):
        """Dodaje listę trójek do stosowania na początku"""
        for item in lista:
            self.addPreItem(item[0], item[1], item[2] if len(item)> 2 else None)
            
    
    def addRegItem(self, rgx, outs, fg=None):
        """Dodaje trójkę (regex, słownik, flagi) do głównego przetwarzania.
Wyrażenie powinno zawierać nazywane grupy. Słownik to lista
wyrażeń typu {nazwa: zamiennik}, gdzie 'nazwa' to nazwa grupy,
a zamiennik to tekst wstawiany w to miejsce. Zamiennik może
zawierać <G>, co oznacza wstawienie zawartości grupy.
"""
        s=fg if fg is not None else 'ib'
        rgx = self.__class__.cre.sub(r'\\s+',rgx)
        if 'b' in s:
            rgx = r'\b('+ rgx + r')\b'
        elif 'w' in s:
            rgx = r'\b('+ rgx + r')(?=\W|$)'
        elif 'a' in s:
            rgx = r'\b('+ rgx + r')(?=[^.\w]|$)'
        flags = 0
        if 'i' in s:
            flags = re.I
        self.fdic.append([re.compile(rgx, flags),outs])

    def addRegList(self, lista):
        """dodaje listę trójek do głównego przetwarzania
    """
        for item in lista:
            self.addRegItem(item[0], item[1], item[2] if len(item) > 2 else None)

    @staticmethod
    def _splitty(r,xs):
        if r is None:
            return None
        ranges = sorted(list([r.span(x),x] for x in xs.keys() if r.span(x)[0]>=0))
        outl = []
        pos = 0
        while len(ranges) > 0:
            ran = ranges.pop(0)
            if ran[0][0] > pos:
                outl.append(r.string[pos:ran[0][0]])
                pos = ran[0][0]
            ss=r.string[ran[0][0]:ran[0][1]]
            rep = xs.get(ran[1],'').replace('<G>',ss)
            outl.append(rep)
            pos=ran[0][1]
        if len(r.string) > pos:
            outl.append(r.string[pos:])
        outl=''.join(outl)
        return outl

        
    def Stringify(self, txt, *args, **kwargs):
        """Przetwarza wejściowy tekst w następujący sposób:
1) po kolei przykładane są wyrażenie z listy Pre
2) po kolei przykładane są wyrażenia z listy Reg
3) wywołana jest funkcja fonetyzacji
4) po kolei przykładahe są wyrażenia z listy Post
Może przyjmować dodatkowe nazwane argumenty stosowane wyłącznie
w bieżącym przetwarzaniu:
    col  = bool - ustaw tryb potoczny
    base = bool - czytanie ułamków dziesiętnych
    sep  = bool - wyraźna separacja przy literowaniu
    nat  = bool - naturalizacja
    unit = str  - selektor jednostek (patrz selUnit)
    dp   = str  - reprezentacja fonetyczna kropki dziesiętnej
"""
        for item in self.pdic:
            txt = item[0].sub(item[1],txt)
        #txt=re.sub(r'\[\[(.*?)\]\]',r'F[S:\1]',txt)
        for ustr in self.fdic:
            r=ustr[0].search(txt)
            while r is not None:
                txt = self._splitty(r, ustr[1])
                r=ustr[0].search(txt)
        
        txt=polditLL.Poldit.Stringify(self,txt, *args, **kwargs)
        
        for r in self.idic:
            txt=r[0].sub(r[1],txt)
        return txt

    def getUnitsInClass(self, s):
        """ Zwraca listę opisów jednostek w jednoliterowej klasie 
"""
        return getUnitsInClass(s)
        
            
        

if __name__ == '__main__':

    s=Fonetyzer()
    print(s.Stringify('50',nat=1))
