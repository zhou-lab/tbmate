#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Created on Sun Nov  8 14:52:41 2020

@author: DingWB
"""
from .tbmate import pack_list,to_tbk,Reader1,Reader2,Query,Pack,read_idx_from_tbk
from .tbmate import read_one_site,QueryOneSiteInSamples,QueryMultiSitesInSamples
__all__=['tbmate']
__version__='1.0'
