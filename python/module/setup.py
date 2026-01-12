from setuptools import setup, Extension

module1 = Extension('polditLL',
                    sources = ['polditp.c'],
                    libraries = ['poldit'])

setup (name = 'poldit',
       version = '0.2',
       description = 'poldit Python interface',
       ext_modules = [module1],
       packages = ["poldit"])
