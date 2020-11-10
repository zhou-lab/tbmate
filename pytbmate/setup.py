# -*- coding: utf-8 -*-
"""
Created on Sun Nov  8 14:41:17 2020

@author: DingWB
"""

try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

setup(
   name='tbmate',
   version='1.0',
   description='A Python API for tbmate',
   author='Wubin Ding & Wanding Zhou',
   author_email='ding_wu_bin@163.com',
   url="https://github.com/DingWB/pytbmate",
   packages=['tbmate'],
   install_requires=['pytabix==0.1','pandas'],
   scripts=['scripts/pytbmate']
)