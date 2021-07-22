#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Created on Sat Jan 23 14:38:25 2021

@author: DingWB
"""

import os,sys
import tbmate
import pandas as pd
import numpy as np
from collections import defaultdict
from scipy import stats

# =============================================================================
# Example dataset can be found under: /mnt/isilon/zhoulab/labprojects/20201111_tbmate_test/CalFStatistic
# time python CalFStatistic.py grouped_tbk.txt test_cpgs.txt test
# It takes about 29 second to run this example in HPC.
# =============================================================================

#There are two parameter for this script, for example: CalFStatistic.py grouped_tbk.txt test_cpgs.txt
#infile1 is grouped tbk files list (without header), including two columns: tbk file and the corresponding group
infile1=os.path.abspath(sys.argv[1])
#infile1="grouped_tbk.txt"
#infile2 is a subset of tbk idx file including interested cpgs (without group), including chr, start, end and offset.
infile2=os.path.abspath(sys.argv[2])
#infile2="test_cpgs.txt"
#outdir is the output directory
outdir=os.path.abspath(sys.argv[3])
    
def anova(x):
    #x is offset for one cpg site.
    grouped_value=[]
    for group in group_dict:
        v=df_cpg.loc[x,group_dict[group]].dropna().tolist()
        #v is a list of methylation values for given cpg in all tbk files.
        if len(v)>0:
            #In some groups, all the values are NaN, should be excluded.
            grouped_value.append(v)
    #grouped_value is a list with length = number of group, each value is a list of the methylation value in different tbk files.
    if len(grouped_value)>1:
        r=stats.f_oneway(*grouped_value)
    else:
        r=[np.nan,np.nan]
    return list(r)
    
if __name__=="__main__":
    if not os.path.exists(outdir):
        os.mkdir(outdir)
    
    df_group=pd.read_csv(infile1,sep='\t',header=None,names=['tbk_file','group'])
    df_cpg=pd.read_csv(infile2,sep='\t',header=None,names=['chr','start','end','idx'])
    group_dict=defaultdict(list)
    for tbk_file,group in df_group.loc[:,['tbk_file','group']].values:
        fi=open(tbk_file,'rb')
        sname=os.path.basename(tbk_file).replace('.tbk','')
        df_cpg[sname]=df_cpg.idx.apply(lambda x:tbmate.read_idx_from_tbk(fi,tbk_file,line_num=x,fmt='f')[0])
        fi.close()
        group_dict[group].append(sname)
    
    df_cpg.replace({-1:np.nan},inplace=True)
    df_cpg.to_csv(os.path.join(outdir,"methylation.txt.gz"),sep='\t',index=False)
    data=df_cpg.loc[:,['chr','start','end','idx']]
    df_cpg=df_cpg.drop(['chr','start','end'],axis=1).set_index('idx')
    
    #Perform one-way ANOVA
    anova_r=data.idx.apply(anova)
    data['AnovaStatistic']=[r[0] for r in anova_r]
    data['AnovaP']=[r[1] for r in anova_r]
    data.to_csv(os.path.join(outdir,"Anova.txt"),sep='\t',index=False)