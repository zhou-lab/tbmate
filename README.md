# tbmate: nimble disk storage and access for genomic data

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

tbmate is currently implemented in C and R. It can easily be extended to other programming languages.

## "Column-wise operation" by file selection.

Each sample is stored in a file. Data sets can be easily assembled by selecting files.

By array ID

![random access1](docs/clip1.gif)

By genomic coordinates

![random access2](docs/clip2.gif)

## Switch coordinate without copying data.

It is cumbersome to keep another copy of data after changing genome assembly. This can be even more storage consuming when one need to re-format array data to be co-analyzed by genome-wide data. tbmate can read a re-arranged address file so that data is used under the new coordinate system withot actual data re-storage. For example, one can use array data with a genomic coordinate so that it is used as a whole genome data. Here are the demo of how:

Array to genomic coordinates

![coordinate switch1](docs/clip3.gif)

Genomic coordinates to array

![coordinate switch2](docs/clip4.gif)

Data preprocessed under one genome assembly, analyzed under another

![coordinate switch3](docs/clip5.gif)

## Usage

### Pack into .tbk files

```
tbmate pack -s float input.bed output.tbk
```
input is a bed file that has the same row order as the index file.

Here are the function options:

```
Usage: tbmate pack [options] <in.bed> <out.tbk>

Options:
    -s        int2, int32, int, float, double, ones ([-1,1] up to 3e-5 precision)
    -x        optional output of an index file containing address for each record.
    -h        This help
```

### index file

The index file is a tabix-ed bed/bed.gz file. The fourth column contains the offset in the tbk file. There can be more columns for storing additional information (e.g., the array ID).

### tbk file

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

Currently tbmate supports the following data types
1. INT2: 2 bits
2. INT32: 4 bytes
3. FLOAT: 4 bytes
4. DOUBLE: 8 bytes
5. ONES: 2 bytes, low precision (3e-5) storage for [-1,1] real number.

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
