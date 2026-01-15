#!/usr/bin/env python3

import requests, json, poldit, os, time, speechd

class weatherspk(object):
    #Ustaw własne wwspółrzędne geograficzne
    latitude  = 52.23
    longitude = 23.01
    
    cachefile = '/tmp/weather.json'

    #ustaw zależnie od syntezatora
    natural = True

    # możliwe parametry:
    # module - nazwa modułu wyjściowego
    # voicetype - typ głosu (np. 'MALE1')
    # voice - nazwa głosu (np. 'alicja')
    # name - nazwa klienta, domyślnie 'pogoda'
    # pitch - wysokość od -100 do 100 (domyślnie 0)
    # rate  - tempo od -100 do 100 (domyślnie 0)
    # vol - głośność od -100 do 100 (domyślnie 100)
    
    
    spd_data = {
        'module':'espeak-ng',
        'voicetype': 'male1',
        'pitch': 10,
        'rate': 20
    }
    
    def __init__(self):
        self._fmter = None

    @property
    def fmter(self):
        if self._fmter is None:
            self._fmter = poldit.Fonetyzer(nat=self.__class__.natural)
        return self._fmter
        
    def getNetWeather(self):
        url="https://api.open-meteo.com/v1/forecast?latitude=%.5f&longitude=%.5f" +\
        "&timeformat=iso8601&timezone=Europe/Warsaw" +\
        "&current=temperature_2m,apparent_temperature,wind_speed_10m,pressure_msl,cloud_cover" +\
        ",wind_gusts_10m,relative_humidity_2m"
        url = url % (self.__class__.latitude, self.__class__.longitude)
        r=requests.get(url)
        r.raise_for_status()
        open(self.__class__.cachefile,'wb').write(r.content)
        return json.loads(r.content)

    def getWeather(self):
        try:
            st=os.stat(self.__class__.cachefile)
            delta = time.time() - st.st_mtime
            if delta >= 900:
                raise Exception()
            return json.loads(open(self.__class__.cachefile,'rb').read())
        except:
            return self.getNetWeather()

    def getWString(self):
        js= self.getWeather()['current']

        s=[]
        s.append("Stan na godzinę %s" % js['time'][11:])
        t1=int(js['temperature_2m'])
        t2=int(js['apparent_temperature'])
        t="Temperatura %d °" % t1
        if (t1 != t2):
            t += ", odczuwalna %d °" % t2
        s.append(t)
        s.append("Ciśnienie %d hPa" % js['pressure_msl'])
        s.append("Wilgotność %d %%" % js['relative_humidity_2m'])
        w1=int(js['wind_speed_10m'])
        w2=int(js['wind_gusts_10m'])
        wind = "Wiatr %d km/h" % w1
        if w2 > w1:
            wind = wind + ", w porywach do %d km/h" % w2
        s.append(wind)
        s.append("Powłoka chmur %d %%" % js['cloud_cover'])
        s='. '.join(s)
        return self.fmter.Stringify(s)

    def _getv(self, param):
        p=self.__class__.spd_data.get(param, None)
        if p is None:
            return None
        if not isinstance(p, int):
            return None
        if p < -100 or p > 100:
            return None
        return p
            
    def sayWeather(self):
        try:
            spstr = self.getWString()
        except:
            spstr="błąd połączenia"
        spiker = speechd.Client(self.__class__.spd_data.get('name','pogoda'),autospawn=True)
        spiker.set_language('pl')
        modl = self.__class__.spd_data.get('module', None)
        if modl is not None:
            spiker.set_output_module(modl)
        vox = self.__class__.spd_data.get('voicetype', None)
        if (voc := self.__class__.spd_data.get('voicetype', None)) is not None:
            spiker.set_voice(vox)
        elif (vox := self.__class__.spd_data.get('voice', None)) is not None:
            spiker.set_synthesis_voice(vox)
        if (p := self._getv('pitch')) is not None:
            spiker.set_pitch(p)
        if (p := self._getv('rate')) is not None:
            spiker.set_rate(p)
        if (p := self._getv('vol')) is not None:
            spiker.set_volume(p)
        spiker.speak(spstr)
        spiker.close()
        
weatherspk().sayWeather()
