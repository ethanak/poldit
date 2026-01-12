#!/usr/bin/env python3

import sys, os, re

class genCmap(object):
    def __init__(self, file):
        self.file = file
        self.lines=[]
    def run(self):
        for line in open('cmap.in'):
            line = line.split('//')[0].strip()
            if line == '':
                continue
            cc, cl = line.split(None, 1)
            self.lines.append('{%d,"%s"}' % (ord(cc), cl))
        self.lines.append('{0, NULL}')
        print("static const struct chardic charDic[] = {", file=self.file)
        print(',\n'.join(self.lines), file=self.file)
        print('};', file=self.file)


class genTran(object):
    def __init__(self, file):
        self.file = file
        self.chrs=[]

    @staticmethod
    def cz(zch):
        if zch == 0:
            return['0',0,0]
    
        if zch <=32 or (zch >= 0x7f and zch <=0xa0):
            return ['ZX_SPACE',32,32]
        ch=chr(zch)
        if ch == '-':
            return ['ZX_PSA | ZX_NSB',zch]
        if ch >= '0' and ch <='9':
            return ['ZX_DIGIT',zch]
        if not ch.isalpha():
            if ch in "([{\"":
                return ['ZX_PSA', zch]
            return ['ZX_NSB',zch]
        if ch in 'ßŉ':
            return ['ZX_LOWER', zch]
        if ch in 'İ':
            return ['ZX_UPPER', zch]
        elif ch.isupper():
            return ['ZX_UPPER', ord(ch.lower())]
        else:
            return ['ZX_LOWER',zch]

    def run(self):
        for zch in range(0x180):
            c=self.cz(zch)
            self.chrs.append("%s | %d" % (c[0], c[1]))
        print("static const uint16_t charCode[]={", file=self.file)
        print(",\n".join(self.chrs), file=self.file)
        print("};", file=self.file);

class genUnit(object):
    pfxs={
        'p': ['p','piko'],
        'u': [['u','µ'],'mikro'],
        'n': ['n','nano'],
        'm': ['m','mili'],
        'c': ['c','centy'],
        'd': ['d','decy'],
        'a': ['da','deka'],
        'A': ['dk','deka'],
        'h': ['h','hekto'],
        'k': ['k','kilo'],
        'M': ['M','mega'],
        'G': ['G','giga'],
        'T': ['T', 'tera'],
        'P': ['P', 'peta']
    }
    
    pfxi={
        'k': [['Ki','K'], 'kilo'],
        'M': ['Mi','mibi'],
        'G': ['Gi','gibi'],
        'T': ['Ti','tebi']
    }
    
    def __init__(self, file):
        self.uutyps = {}
        self.outlist=[]
        self.file = file

    def expandItem(self,unit, pfx, line, flags, idec):
        if pfx in '+-':
            afx = ''
            efx = ''
        else:
            afx,efx=self.__class__.pfxs[pfx]
    
        def exline():
            nonlocal pfx, afx, efx, line, unit
            fg=flags
            if afx == '':
                fg += '|FLG_BASEUNIT'
            if pfx == '+':
                fg += '|FLG_BASEIGNORE'
            lout=','.join('"%s"' % (efx+x) for x in line)
            if isinstance(afx, str):
                afx=[afx]
            for a in afx:
                u = a+unit
                #rec='{"%s",%s,{%s}}' % (u, flags, lout)
                
                rac=(u, fg, lout)
                self.outlist.append(rac)
        exline()
        if not idec:
            return
        pfa = self.__class__.pfxi.get(pfx)
        if pfa is None:
            return
        afx=pfa[0]
        efx=pfa[1]
        exline()

    def expandUnit(self, unit, pix, line, flags, idec):
        if ',' in unit:
            for u in unit.split(','):
                self.expandUnit(u,pix,line, flags, idec)
            return
        for pfx in pix:
            if isinstance(pfx, list):
                continue
            self.expandItem(unit, pfx, line,flags,idec)

    def addtyp(self, line):
        mark = line[0]
        line = line[1:].lstrip(" \t-")
        self.uutyps[mark] = line
        
    def run(self):
        lines=open('units.in').readlines()
        for line in lines:
            line=line.strip()
            if line == '' or line.startswith('#'):
                continue
            if line.startswith('_typ'):
                self.addtyp(line[4:].strip())
                continue
            line = line.split()
            if len(line) != 7:
                print("CO TO", line)
                exit(1)
            female = False
            alter = False
            utyp = line.pop(0)
            flags = []
            for u in utyp:
                flags.append('UNIT_' + u)#self.__class__.uutyps[u])
            
            unit = line.pop(0)
            r=re.match(r'([:?]+)(.+)$',unit)
            if r is not None:
                alter = '?' in r.group(1)
                female = ':' in r.group(1)
                unit = r.group(2)
            if alter:
                flags.append('FLG_COLL')
            if female:
                flags.append('FLG_FEMALE')
            flags = '0' if len(flags) == 0 else '|'.join(flags)
            pix = line.pop(0)
            for n, a in enumerate(line):
                if n > 0 and a.endswith('*'):
                    a=a.replace('*',' '.join(line[0].split()[1:]))
                line[n] = a.replace('-',' ')
            if '+' in pix:
                pass
                #pix = pix.replace('+','')
            elif '-' not in pix:
                pix = '-' + pix
            idec = False
            if 'i' in pix:
                pix=pix.replace('i','')
                idec = True
            self.expandUnit(unit, pix, line, flags, idec)

        uukeys=''.join(sorted(self.uutyps.keys()))
        print("static const char *unitsel=\"%s\";\n\n" % uukeys, file=self.file)
        uval = 0
        ubit = 0x100
        ulis = []
        for u in uukeys:
            print("#define UNIT_%s 0x%x" % (u, ubit), file = self.file)
            if not self.uutyps[u].endswith('*'):
                uval |= ubit
            ulis.append("%s - %s\\n" % (u,self.uutyps[u]))
            ubit <<= 1
        ulis.append("A - wszystkie jednostki\\n")
        ulis.append("B - Nie używaj jednostek\\n")
        ulis = '\\\n        '.join(ulis)
        print("static int unit_initial = 0x%x;" % uval, file = self.file);
        print("static const char *uudescr = \"        %s\";" % ulis, file = self.file)

        #print("#define UNIT_ALL 0x%x" % ((ubit - 1 ) & 0xffffff00))
        #print("#define UNIT_ALL 0x%x" % ((ubit - 1 ) & 0xffffff00))
        self.outlist.sort(key=lambda x:('COLL' not in x[1], -len(x[0]), x[2] ))
        self.outlist=list('{"%s",%s,{%s}}' % x for x in self.outlist)
        self.outlist.append('{NULL,0}')
        print("static const struct unitDefs unitDefs[]={", file = self.file)
        print(",\n".join(self.outlist), file=self.file)
        print("};",file=self.file)
        
f=open("poldit_data.h", 'w')
print("""
/*
 * Plik generowany automatycznie - nie edytuj!
 * Autogenerated - do not edit!
 */

#ifndef POLDIT_DATA_H
#define POLDIT_DATA_H
""", file = f)
genCmap(f).run()    
genTran(f).run()
genUnit(f).run()
print("#endif",file=f);
