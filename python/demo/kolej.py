#!/usr/bin/env python3

from poldit import Fonetyzer

data=[
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
    
    [r'\S+a ((?P<a>gł\.?)|(?P<b>zach\.?)|(?P<c>wsch\.?)|(?P<d>centr?\.?))',
        {'a':'Główna','b':'Zachodnia','c':'Wschodnia','d':"Centralna"},'ia'],
    [r'\S+e ((?P<a>gł\.?)|(?P<b>zach\.?)|(?P<c>wsch\.?)|(?P<d>centr?\.?))',
        {'a':'Główne','b':'Zachodnie','c':'Wschodnie','d':"Centralne"},'ia'],
    [r'((?P<a>gł\.?)|(?P<b>zach\.?)|(?P<c>wsch\.?)|(?P<d>centr?\.?))',
        {'a':'Główny','b':'Zachodni','c':'Wschodni','d':"Centralny"},'ia'],
]

predic = [
    [r'\[\[(.*?)\]\]',r'F[S:\1]'],
    ]

postdic= [
    ['IC','intersjity'],
    ['KS',"koleje śląskie"],
]

st1='Na torze 4 przy peronie 3 stoi pociąg do stacji Warszawa Gł.\\. Wagony o numerach 1 do 3 zatrzymują się w sektorze 3.'
st2='Opóźniony o 15 min pociąg IC [[IC 23144]] "Maruda", ze stacji Warszawa Cent. do stacji Katowice Wsch., wjedzie na tor 7 przy peronie 4.'

s=Fonetyzer(unit='T')
s.addPreList(predic)
s.addRegList(data)
s.addPostList(postdic)

print(s.Stringify(txt=st1,nat=1))
print(s.Stringify(txt=st2,nat=1))

