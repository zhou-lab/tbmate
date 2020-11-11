# pytbmate
Pytbmate is a Python API for [TabixMate](https://github.com/zhou-lab/tbmate): nimble storage of multifarious genomic data

## Dependencies:
- [tabix](http://www.htslib.org/doc/tabix.html)
- [tbmate](https://github.com/zhou-lab/tbmate)
- [pytabix](https://github.com/slowkow/pytabix)

## **Installation**
1. **Install tabix**:

You can download and install [tabix](http://www.htslib.org/doc/tabix.html). Or:
```
wget https://github.com/samtools/htslib/releases/download/1.11/htslib-1.11.tar.bz2
tar jxvf htslib-1.11.tar.bz2
cd htslib-1.11
./configure --prefix=/usr/local/htslib
make
make install

#Adding to your $PATH
export PATH=/usr/local/htslib/bin:$PATH
```

2. **Install tbmate**:
```
wget -O tbmate "https://github.com/zhou-lab/tbmate/releases/download/1.6.20200915/tbmate_linux_amd64"

# or mac
wget -O tbmate "https://github.com/zhou-lab/tbmate/releases/download/1.6.20200915/tbmate_darwin_amd64"

chmod a+x tbmate
# Moving tbmate to /usr/bin
mv tbmate /usr/bin
```

3. **Install pytbmate**:
```
pip install pytabix
pip install git+https://github.com/DingWB/pytbmate
```
OR
```
git clone https://github.com/DingWB/pytbmate
cd pytbmate
python setup.py install
```

## **Usage**
**1. Building tabix index.**
</br>
Download HM450 array manifest file and index it with tabix
```
wget ftp://webdata2:webdata2@ussd-ftp.illumina.com/downloads/ProductFiles/HumanMethylation450/HumanMethylation450_15017482_v1-2.csv
```
Prepare tabix index file
```
sed '1,8d' HumanMethylation450_15017482_v1-2.csv |cut -f 1 -d ","|grep -E "^cg|^ch|^rs" | sort -k1V |awk 'BEGIN {OFS="\t";} {print $0,1,2,NR-1}' | bgzip > hm450_idx.bed.gz
zcat hm450_idx.bed.gz |head
```
```
cg00000029	1	2	0
cg00000108	1	2	1
cg00000109	1	2	2
cg00000165	1	2	3
cg00000236	1	2	4
cg00000289	1	2	5
cg00000292	1	2	6
cg00000321	1	2	7
cg00000363	1	2	8
```
Index the bed.gz with tabix:
```
tabix -b 2 -e 3 -p bed hm450_idx.bed.gz 
```
Simple query with tabix:
```
tabix hm450_idx.bed.gz cg18478105:1-2
```

Similarly, a EPIC manifest file can be downloaded from [here](http://webdata.illumina.com.s3-website-us-east-1.amazonaws.com/downloads/productfiles/methylationEPIC/infinium-methylationepic-v5-manifest-file-csv.zip). To save time, we provided the index files and tabix index for EPIC and WGBS in [test dataset](https://).


**2. Packing data into .tbk files.**

- (1). Packing Hm450 array data.
```
cd Test/HM450/
```
Reading example.bed.gz into Python.
```
import tbmate
import pandas as pd

idx_file="hm450_idx.bed.gz"
data=pd.read_csv("example.bed.gz",sep='\t',header=None,names=['seqname','start','end','GSM3417545','GSM3417546',''GSM3417547'])
data.head()

      seqname  start  end  GSM3417545  GSM3417546  GSM3417547
0  cg00000029      1    2    0.622689    0.578341    0.418048
1  cg00000108      1    2    0.924396    0.903956    0.906324
2  cg00000109      1    2    0.741272    0.828271    0.873561
3  cg00000165      1    2    0.809239    0.465659    0.693626
4  cg00000236      1    2    0.785956    0.855497    0.804233

# Packing three columns into .tbk files:
tbmate.to_tbk(data=data,cols=['GSM3417545','GSM3417546','GSM3417547'],idx=idx_file,dtypes='float')
# Or you can pack a list/array into .tbk files:
tbmate.pack_list(L=data['GSM3417545'].tolist(),idx=idx_file,dtype='float',outfile="test.tbk")
```

- (2). Packing WGBS data.</br>
```
cd Test/WGBS/
```
Reading example .gz file  into python.
```
import tbmate
import pandas as pd
idx_file="idx.gz"
data=pd.read_csv("example/TCGA_BLCA_A13J_cpg.gz",sep='\t',header=None)
data.columns=['seqname','start','end','TCGA_BLCA_A13J','depth']
data.head()

  seqname  start    end  TCGA_BLCA_A13J depth
0    chr1  10468  10470         -1.0     .
1    chr1  10470  10472         -1.0     .
2    chr1  10483  10485         -1.0     .
3    chr1  10488  10490         -1.0     .
4    chr1  10492  10494         -1.0     .

# Packing the 4th columns into .tbk files:
tbmate.to_tbk(data=data,cols=['TCGA_BLCA_A13J'],idx=idx_file,dtypes='float',outdir="example")
```
The function to_tbk would first make sure the coordinate of idx file and dataframe are the same.</br>
If the coordinates of these two files were already the same, Then you could directly use pack_list function, which would be faster.
```
tbmate.pack_list(L=data['TCGA_BLCA_A13J'].tolist(),idx=idx_file,dtype='float',outfile="example/TCGA_BLCA_A13J.tbk")
```
```
ls -sh example/TCGA_BLCA_A13J*
```

```
256M example/TCGA_BLCA_A13J_cpg.gz  127M example/TCGA_BLCA_A13J.tbk
```

**3. Query .tbk files.**
```
cd Test/EPIC
```
Query probe cg27587195 from one .tbk file.
```
import tbmate
tbk_file="GSM2995280_201868590258_R01C01.tbk"
idx_file='idx.gz'
tbmate.Query(tbk_file=tbk_file,seqname="cg27587195",idx=idx_file,dtype='float')
```

Query probe cg27587195 from multiple .tbk file. If you have many sampels to query, QueryMultiSamples maybe useful.
```
import tbmate
tbk_files=[file for file in os.listdir("./") if file.endswith('.tbk')]
idx_file='idx.gz'
tbmate.QueryMultiSamples(tbk_files=tbk_files,seqname='cg27587195',idx=idx_file,dtype='float')
```
