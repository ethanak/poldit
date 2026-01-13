#!/usr/bin/env python3

import requests, json, poldit, os, time

class weather(object):
    #Ustaw własne wwspółrzędne geograficzne
    latitude  = 52.23
    longitude = 23.01
    
    cachefile = 'weather.json'

    #ustaw zależnie od syntezatora
    natural = True
    
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
        s.append("Temperatura %d °C, odczuwalna %d °C" % (
            js['temperature_2m'],js['apparent_temperature']))
        s.append("Ciśnienie %d hPa" % js['pressure_msl'])
        w1=int(js['wind_speed_10m'])
        w2=int(js['wind_gusts_10m'])
        wind = "Wiatr %d km/h" % w1
        if w2 > w1:
            wind = wind + ", w porywach %d km/h" % w2
        s.append(wind)
        s.append("Powłoka chmur %d %%" % js['cloud_cover'])
        s='. '.join(s)
        return self.fmter.Stringify(s)

w=weather()
spstr = w.getWString()
print(spstr)
