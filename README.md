# TabixMate: nimble storage of multifarious genomic data

[![Travis-CI Build Status](https://travis-ci.org/zhou-lab/tbmate.svg?branch=master)](https://travis-ci.org/zhou-lab/tbmate)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/zhou-lab/tbmate)

tbmate (TabixMate) works with [Tabix](http://www.htslib.org/doc/tabix.html) - the Swiss-knife tool for random accessing genomic data. However, there are 3 limitations of applying tabix to large genome-wide data sets.

1. Random access is on a per file basis. Splitting and merging data sets are clumsy.
2. Each tabix-ed file (bed, gff etc) must contain a set of genomic coordinates, which creates redundancy in storage.
3. Data is bound to a coordinate system. Changing coordinate system requires data copy and re-indexing.
4. Addressing is opaque.

tbmate is meant to solve/alleviate these challenges by de-coupling the data from the coordinate system.

1. Random accessing on any group of files packed under the same address set.
2. Coordinates are stored separately from data. Data is (semi-automatically) rebound to coordinates during IO
3. Data addresses can be re-arranged to allow accessing under a different coordinate system. Therefore, the coordinate system can be switched by swapping address file without copying data.
3. Addresses are stored in plain text. Disk offset can be easily calculated.
4. tbmate supports random disk output

tbmate is currently implemented in C, Python and R. It can easily be extended to other programming languages.

## Dependencies:
- [tabix](http://www.htslib.org/doc/tabix.html)

## **Installation**
1. **Install tabix**:

You can download and install tabix from [here](http://www.htslib.org/doc/tabix.html). Or directly install it with:
```
wget https://github.com/samtools/htslib/releases/download/1.11/htslib-1.11.tar.bz2
tar jxvf htslib-1.11.tar.bz2
cd htslib-1.11
./configure --prefix=/usr/local/htslib
make
make install

#Adding to your $PATH
export PATH=/usr/local/htslib/bin:$PATH

#To test whether tabix is installed
tabix -h
```

2. **Install tbmate**:
```
wget -O tbmate "https://github.com/zhou-lab/tbmate/releases/download/1.6.20200915/tbmate_linux_amd64"

# or mac
wget -O tbmate "https://github.com/zhou-lab/tbmate/releases/download/1.6.20200915/tbmate_darwin_amd64"

chmod a+x tbmate
```

#Or install from source code:
```
git clone https://github.com/zhou-lab/tbmate
cd tbmate
make

# Moving tbmate to /usr/bin
mv tbmate /usr/bin
```
R script (R API for tbmate) is inside the scripts folder.
Python API is under [pytbmate](https://github.com/zhou-lab/tbmate/pytbmate)

## Usage

**1. Building tabix index.**

For example, downloading HM450 array manifest file and index it with tabix
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

The index file is a tabix-ed bed/bed.gz file. The 4-th column contains the offset in the tbk file. There can be more columns for storing additional information (e.g., the array ID). The index file for data generated on the same coordinate system (the native address file) should just have a trivial enumerating 4-th column, i.e.,

````
chr1    10468   10470   0
chr1    10470   10472   1
chr1    10483   10485   2
chr1    10488   10490   3
chr1    10492   10494   4
chr1    10496   10498   5
````

or if it's array (the 2nd and 3rd columns are not used).

```
cg00000029      1       2       0
cg00000103      1       2       1
cg00000109      1       2       2
cg00000155      1       2       3
cg00000158      1       2       4
cg00000165      1       2       5
```

The cross-coordinate index will have a more scrambled addresses. For example, hg38_to_EPIC.idx.gz has

```
cg00000029   1   2   9719014    chr16:53434199-53434201
cg00000103   1   2   19704158   chr4:72604468-72604470
cg00000109   1   2   18796088   chr3:172198246-172198248
cg00000155   1   2   23714121   chr7:2550930-2550932
cg00000158   1   2   27254375   chr9:92248272-92248274
```

and EPIC_to_hg38.idx.gz has

```
chr1   10468   10470   -1   .
chr1   10470   10472   -1   .
chr1   10483   10485   -1   .
...
chr1    69590   69592   699401  cg21870274
...
```

Note most entries have -1 which indicate no Infinium EPIC array ID is spotted. `tbmate view -d` can optionally omit these in the display. All index files can be easily generated from the native address file.


### Pack into .tbk files

```
tbmate pack -s float input.bed output.tbk
```
input is a bed file that has the same row order as the index file.

Here are the function options:

```
Usage: tbmate pack [options] <in.bed> <out.tbk>

Options:
    -s        int1, int2, int32, int, float, double, stringf, stringd, ones ([-1,1] up to 3e-5 precision)
    -x        optional output of an index file containing address for each record.
    -m        optional message, it will also be used to locate index file.
    -h        This help

Note, in.bed is an index-ordered bed file. Column 4 will be made a .tbk file.
```

For example:</br>

- Packing HM450 array data into .tbk.
```
cd Test/HM450/
tbmate pack -s float -m "hm450_idx.bed.gz" example.bed.gz out.tbk  #Packing the 4th column into out.tbk
# Or using tbmate after pipe to pack the 5th column into tbk file.
zcat example.bed.gz | cut -f 1,2,3,5 |tbmate pack -m "hm450_idx.bed.gz" - GSM3417546.tbk
```
Please Note: the coordinate of example.bed.gz has been processed to be the same with hm450_idx.bed.gz.

- Packing WGBS data into .tbk.
```
cd Test/WGBS/
tbmate pack -s float -m "idx.gz" TCGA_BLCA_A13J_cpg.gz TCGA_BLCA_A13J.tbk
ls TCGA_BLCA_A13J* -sh
```
```
256M TCGA_BLCA_A13J_cpg.gz  127M TCGA_BLCA_A13J.tbk
```

### View .tbk files

```
tbmate view input.tbk [more_input.tbk [...]]
```

`-i index_file` can be provided to specify the index file. It is *crucial* to use the index file compatible with the .tbk file. Here are the options.

```
Usage: tbmate view [options] [.tbk [...]]

Options:
    -o        optional file output
    -i        index, a tabix-ed bed file. Column 4 is the .tbk offset.
              if not given search for idx.gz and idx.gz.tbi in the folder
              containing the first tbk file.
    -g        REGION
    -c        print column name
    -a        print all column in the index
    -d        using dot for negative values
    -p        precision used to print float[3]
    -u        show unaddressed (use -1)
    -R        file listing the regions
```

```
# view header
cd Test/WGBS/
tbmate header TCGA_BLCA_A13J.tbk

# view the whole dataset
tbmate view -c TCGA_BLCA_A13J.tbk |less

# query by a given genomic region (chr:start-end)
tbmate view -c -g chr16:132813-132831 TCGA_BLCA_A13J.tbk

# change index location
tbmate header -m "hg38_to_EPIC.idx.gz" TCGA_BLCA_A13J.tbk

# confirm it's changed
tbmate header TCGA_BLCA_A13J.tbk   # See "Message: hg38_to_EPIC.idx.gz"

# now using the new coordinates to query by EPIC probe ID.
tbmate view -cd -g cg00013374,cg00012123,cg00006867 TCGA_BLCA_A13J.tbk

# switch back without modify the header
tbmate view -cd -i "idx.gz" -g chr19:246460-346460 TCGA_BLCA_A13J.tbk | less
```

View or query from multiple .tbk files simultaneously
```
cd Test/EPIC
ls *.tbk
tbmate view -cd *.tbk |less #or export to a txt file with "> out.txt"
tbmate view -cd -g cg00013684,cg00029587,rs7746156 *.tbk  #That would be very useful to query a given probes from many .tbk files.
```

### The tbk files

tbk file is a binary file. The first three bytes have to be "tbk" and will be validated by tbmate. The first 512 bytes store the data header:

1. 3 bytes: "tbk"
2. 4 bytes: version, currently 1
3. 4 bytes: data type, enum {NA, INT2, INT32, FLOAT, DOUBLE, ONES}
4. 8 bytes: number of values in the data
5. 493 bytes: extra space, currently used for message

```
tbmate header input.tbk
```

The header subcommand will output the header inforamtion

## Demo
[https://github.com/zhou-lab/tbmate/blob/master/test/README.md](https://github.com/zhou-lab/tbmate/blob/master/test/README.md)

## "Column-wise operation" by file selection.

Each sample is stored in a file. Data sets can be easily assembled by selecting files.

Here is an example of Infinium EPIC array, stored and accessed by array ID

![random access1](docs/clip1.gif)

Here is another example of Whole Genome Bisulfite Sequencing Data, stored and accessed by genomic coordinates.

![random access2](docs/clip2.gif)

## Virtual coordinate switch without copying data.

It is cumbersome to keep another copy of data after changing genome assembly. This can be even more storage consuming when one need to re-format array data to be co-analyzed by genome-wide data. tbmate can read a re-arranged address file so that data is used under the new coordinate system withot actual data re-storage. For example, one can use array data with a genomic coordinate so that it is used as a whole genome data. 

Here is an example to demo projection of array (Infinium EPIC beadchip) to hg38 genomic coordinates. Note the change in index file with `-i`.

![coordinate switch1](docs/clip3.gif)

Here is another example of using WGBS data as if it's on the array platform. The index projects genomic coordinates to array probe ID:

![coordinate switch2](docs/clip4.gif)

Here is an example of projecting across genome assemblies (hg38 to hg19):

![coordinate switch3](docs/clip5.gif)

All index files can be uniquely specified by platform (genome assembly, array ID system etc.) and do not depend on data. Datasets can share one copy through symlinks.
