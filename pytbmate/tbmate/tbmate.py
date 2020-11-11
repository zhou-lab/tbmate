#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Created on Tue Sep  8 17:34:38 2020

@author: DingWB
"""
import sys
import os
import struct
import tabix
import gzip
import pandas as pd

version=1.0

dtype_fmt={
        'float':'f',
        'int':'i',
        'string':'s',
        'chr':'c',
        'double':'d'
        }

dtype_map={
        'int':1,
        'float':4,
        'double':5,
        'string':7,
        'chr':6
        }

dtype_map_rev={
        1:'int',
        2:'int',
        3:'int',
        4:'float',
        5:'double',
        6:'chr',
        7:'string'
        }

dtype_func={
        'float':float,
        'int':int,
        'string':str,
        'chr':str,
        'double':float
        }

def pack_header(idx,outfile,dtype):
    print(f"Packing and writing to {outfile}")
    idx_len=len(idx)
    fo=open(outfile,'wb')
    fo.write(struct.pack('3s',b'tbk')) #'tbk',byte=3*1=3
    fo.write(struct.pack('f',version)) #version:1; byte=4
    fo.write(struct.pack('q',dtype_map[dtype])) #dtype, bytes=8
#    fo.write(struct.pack('i',idx_len)) #idx_len, byte=4
    fo.write(struct.pack('q',-1)) #max data, byte=8

    if idx_len > 8169:
        idx=idx[:8169]

    fo.write(struct.pack(f'{idx_len}s',bytes(idx,'utf-8'))) #idx file, byte=idx_len

    if idx_len < 8169:
        for i in range(8169-idx_len):
            fo.write(struct.pack('x')) #fill to 500.
    #Total used bytes = 3+4+8+8+8169=8192
    return fo
# =============================================================================
def Pack(Input,idx='idx.gz',basename="out",cols_to_pack=[4],
        dtypes=['float']):
    """
    Input: Input, a file (.gz is supported), header must be contained in Input file.
        if there are more than one column, the 4th column will be written to .tbk.
    idx: index file, should be indexed with tabix.
    outfile: output .tbk file.
    cols_to_pack: The columns (1-based) to be packed.
    dtype: 'float', 'int', 'string', 'chr','double'. The length of dtypes
            must be the same with cols_to_packpack
    """
    #Starting to write data
    if Input.endswith('.gz'):
        fi=gzip.open(Input,mode='rb')
    else:
        fi=open(Input,'r',encoding='utf-8')

    line=fi.readline()
    if isinstance(line,bytes):
        line=line.decode('utf-8')
    cols=line.replace('\n','').split('\t')

    f_write_dict={}
    FMT={}
    DT_FUNC={}
    for i,dtype in zip(cols_to_pack,dtypes):
        FMT[i-1]=dtype_fmt[dtype]
        DT_FUNC[i-1]=dtype_func[dtype]
        col=cols[i-1]
        outfile=basename+'_'+col+'.tbk'
        f_write_dict[i-1]=pack_header(idx,outfile,dtype)

    line=fi.readline()
    if isinstance(line,bytes):
        line=line.decode('utf-8')

    while line:
        values=line.split('\t')
        for i in f_write_dict:
            fo=f_write_dict[i]
            v=values[i]
            dt_func=DT_FUNC[i]
            fo.write(struct.pack(FMT[i],dt_func(v)))
        line=fi.readline()
        if isinstance(line,bytes):
            line=line.decode('utf-8')

    for i in f_write_dict:
        fo=f_write_dict[i]
        num=int((fo.tell()-8192) / struct.calcsize(FMT[i]))
        fo.seek(15)
        fo.write(struct.pack('q',num))

    fi.close()
    for i in f_write_dict:
        fo=f_write_dict[i]
        fo.close()
# =============================================================================
def pack_list(L=None,idx='idx.gz',dtype='float',outfile="out.tbk"):
    """
    Packing a list or an array into .tbk file.
    L: A list or array.
    idx: tabix indexed index file.
    dtype: data type.
    outfile: output .tbk file name.
    """
    f=pack_header(idx,outfile,dtype)
    fmt=dtype_fmt[dtype]
    dt_func=dtype_func[dtype]
    for v in L:
        f.write(struct.pack(fmt,dt_func(v)))
    num=int((f.tell()-8192) / struct.calcsize(fmt))
    f.seek(15)
    f.write(struct.pack('q',num))
    f.close()
# =============================================================================
def to_tbk(data=None,cols=[],idx='idx.gz',outdir="./",
        dtypes=[],na=-1):
    """
    data: A pandas dataframe or a list or values ordered by the tabix indexed coordinate.
        The first three columns should be seqname,start,end, Nan will be filled with na.
    idx: index file, should be indexed with tabix.
    cols: A list of columns to pack.
    out_basename: Output basename. If the length of cols > 1, outfile will be out_basename_col.tbk
    dtypes: A string or a list of data type. 'float', 'int', 'string', 'chr','double'. dtypes should have the same length as cols.
          If dtypes is a string, then it will be expanded to a list with the same length with cols.
    na: The NaN values in data will be replace with na, should be an integer.
    """
    outdir=os.path.abspath(outdir)
    idx=os.path.abspath(idx)
#    header=None
#    with gzip.open(idx,'rb') as f:
#        line=f.readline()
#        line=line.decode('utf-8')
#    if line.split('\t')[0]=='seqname':
#        header=0
    df_idx=pd.read_csv(idx,sep='\t',header=None)
    df_idx.columns=['seqname', 'start', 'end', 'index']
    df_idx['ID']=df_idx.seqname.map(str)+'-'+df_idx.start.map(str)+'-'+df_idx.end.map(str)
    data['ID']=data.iloc[:,0].map(str)+'-'+data.iloc[:,1].map(str)+'-'+data.iloc[:,2].map(str)
    data.set_index('ID',inplace=True)
    df_idx.drop(['seqname','start','end'],axis=1,inplace=True)
    if type(dtypes)==str:
        dtypes=[dtypes]*len(cols)
    assert len(dtypes)==len(cols)
    for col,dtype in zip(cols,dtypes):
        print(col,dtype)
        df_idx[col]=df_idx.ID.map(data[col].to_dict())
        df_idx[col].fillna(na,inplace=True)
        pack_list(L=df_idx[col].tolist(),idx=idx,dtype=dtype,\
                  outfile=os.path.join(outdir,col+'.tbk'))
# =============================================================================
def Read(tbk_file,start,size,fmt):
    """
    Reading one line from tbk file.
    tbk_file: .tbk
    start: start index.
    size: bytes
    fmt: f,s,c...,values of dtype_fmt.
    """
    with open(tbk_file,'rb') as f:
        f.seek(start)
        r=f.read(size)
    return struct.unpack(fmt,r)[0]
# =============================================================================
def ReadBulk(tbk_file,start,end,size,fmt):
    """
    Readling multiple continious lines from .tbk.
    tbk_file: .tbk
    start: start index.
    end: end index.
    size: bytes
    fmt: f,s,c...,values of dtype_fmt.
    """
    R=[]
    f=open(tbk_file,'rb')
    f.seek(start)
    while start <= end:
        r=f.read(size)
        R.append(struct.unpack(fmt,r)[0])
        start+=size
    f.close()
    return R
# =============================================================================
def Header(tbk_file):
    identifier=Read(tbk_file,0,3,'3s') #'tbk',byte=3*1=3
    if identifier.decode('utf-8') != 'tbk':
        raise Exception("Input .tbk file is not standard tbk file.")
    ver=Read(tbk_file,3,4,'f') #version:1; byte=4
#    dtype=Read(tbk_file,7,4,'i') #dtype,byte=4
    dtype=Read(tbk_file,7,8,'q') #dtype,byte=8
#    idx_len=Read(tbk_file,11,4,'i') #idx_len, bytes=4
#    idx=Read(tbk_file,15,idx_len,f'{idx_len}s') #idx,bytes=idx_len
#    idx=idx.decode('utf-8')
    num=Read(tbk_file,15,8,'q') #maximum data length, byte=8
    idx=Read(tbk_file,23,8169,'8169s')
    idx=idx.decode('utf-8').replace('\x00','')
    return [ver,dtype,num,idx]
# =============================================================================
def read_one_site(tbk_file,line_num,fmt,base_idx=8192):
    """
    Query single line.
    """
    size=struct.calcsize(fmt)
    start=size*line_num+base_idx
    return Read(tbk_file,start,size,fmt)
# =============================================================================
def read_multi_site(tbk_file,n1,n2,fmt,base_idx=8192):
    """
    Query multiple lines.
    n1,n2: line number.
    """
    size=struct.calcsize(fmt)
    start=size*n1+base_idx
    end=size*n2+base_idx
    return ReadBulk(tbk_file,start,end,size,fmt)
# =============================================================================
def read_multi_samples(tbk_files=[],n=0,fmt='f',base_idx=8192):
    """
    Query multiple lines.
    n1,n2: line number.
    """
    size=struct.calcsize(fmt)
    start=size*n+base_idx
    R=[Read(tbk_file,start,size,fmt) for tbk_file in tbk_files]
    return R
# =============================================================================
def Query(tbk_file=None,seqname=None,start=1,end=2,
          idx=None,dtype=None,base_idx=8192):
    """
    The main function for querying.
    tbk_file: input a single .tbk file.
    seqname: Chromosome (or the sequence name for tabix).
    start: start position.
    end: end position.
    base_idx: Number of index that should be skipped.
    """
    if dtype is None:
        ver,dtype,num,idx1=Header(tbk_file)
        dtype=dtype_map_rev[dtype]
    if idx is None:
        idx=idx1
    fmt=dtype_fmt[dtype]
    tb = tabix.open(idx)
    records=tb.query(seqname,start,end)
    lineNum=[record[3] for record in records]
    if len(lineNum)==0:
        return None
    elif len(lineNum)==1:
        return read_one_site(tbk_file,int(lineNum[0]),fmt,base_idx)
    n1,n2=lineNum[0],lineNum[-1]
    return read_multi_site(tbk_file,n1,n2,fmt,base_idx)
# =============================================================================
def QueryMultiSamples(tbk_files=[],seqname=None,start=1,end=2,
          idx=None,dtype=None,base_idx=8192):
    """
    The function for querying multiple samples.
    tbk_file: input a file list.
    seqname: Chromosome (or the sequence name for tabix).
    start: start position.
    end: end position.
    idx: index file.
    dtype: dtype
    base_idx: Number of index that should be skipped.
    """
    if idx is None or dtype is None:
        raise Exception("Please provide idx and dtype")
    fmt=dtype_fmt[dtype]
    tb = tabix.open(idx)
    records=tb.query(seqname,start,end)
    lineNum=[record[3] for record in records]
    if len(lineNum)==0:
        return None
    if len(lineNum)==1:
        return [read_one_site(tbk_file,int(lineNum[0]),fmt,base_idx) for tbk_file in tbk_files]
    else:
        raise Exception("Not support querying of multiple position now.")
#    n1,n2=lineNum[0],lineNum[-1]
#
#    return querys(tbk_file,n1,n2,fmt,base_idx)
# =============================================================================
def View(tbk_file=None,idx=None,dtype=None,base_idx=8192):
    """
    Viewing all records in a given tbk file.
    tbk_file: input .tbk file.
    base_idx: Number of index that should be skipped.
    """
    if idx is None or dtype is None:
        ver,dtype,num,idx=Header(tbk_file)
        dtype=dtype_map_rev[dtype]
    fmt=dtype_fmt[dtype]
    fi=gzip.open(idx,mode='rb')
    f_tbk=open(tbk_file,'rb')
    line=fi.readline()
    line=line.decode('utf-8')
#    if line.split('\t')[0]=='seqname':
#        line=fi.readline()
#        line=line.decode('utf-8')
    size=struct.calcsize(fmt)
    name=os.path.basename(tbk_file).replace('.tbk','')
    sys.stdout.write(f"seqname\tstart\tend\t{name}\n")
    while line:
        values=line.split('\t')
        seqname, start, end, Index=values
        start=size*int(Index)+base_idx
        f_tbk.seek(start)
        r=f_tbk.read(size)
        v=struct.unpack(fmt,r)[0]
#        if v==-1:
#            v='NA'
        try:
            sys.stdout.write(f"{seqname}\t{start}\t{end}\t{v}\n")
            line=fi.readline()
            line=line.decode('utf-8')
        except:
            try:
                sys.stdout.close()
                fi.close()
                f_tbk.close()
                break
            except: #IOError
                pass
    fi.close()
    f_tbk.close()