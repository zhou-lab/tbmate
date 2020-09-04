# Try out TabixMate
## Download executables here
[https://github.com/zhou-lab/tbmate/releases](https://github.com/zhou-lab/tbmate/releases)

Download test data [here](https://www.dropbox.com/s/traeqvfqf3qtat4/test.zip?dl=1)

Or,
```sh
wget -O tbmate "https://github.com/zhou-lab/tbmate/releases/download/1.1.20200904/tbmate_linux_amd64"
## or mac
wget -O tbmate "https://github.com/zhou-lab/tbmate/releases/download/1.1.20200904/tbmate_darwin_amd64"

## unzip test data
unzip test.zip
cd test
```


## Viewing

```sh
cd test
# on native coordinate systems
./tbmate view -cd -g chr19:246460-346460 WGBS/tbk | less
./tbmate view -cd EPIC/tbk | less
./tbmate view -g cg00013374,cg00012123,cg00006867 -cd HM450/tbk | less


# convert hg38 to EPIC array and subset with probe ID
./tbmate view -cd -g cg00013374,cg00012123,cg00006867 -i indices/hg38_to_EPIC.idx.gz WGBS/tbk | less

# convert EPIC array to hg38 and subset with genomic range
./tbmate view -cd -g chr19:246460-346460 -i indices/EPIC_to_hg38.idx.gz WGBS/tbk | less

```

## Packing, index specification and switching

```sh
# note the index is provided as a relative path, it will remain valid as long as out.tbk is kept relatively
# same with respective to indices/hg38.idx.gz, one can also provide an absolute path
./tbmate pack -s float -m "indices/hg38.idx.gz" WGBS/raw/TCGA_BLCA_A13J_cpg.gz tmp.tbk

# view header
./tbmate header out.tbk

# change index location
./tbmate header -m "indices/hg38_to_EPIC.idx.gz" out.tbk

# confirm it's changed
./tbmate header out.tbk

# now using the new coordinates
./tbmate view -cd -g cg00013374,cg00012123,cg00006867 out.tbk | less

# switch back without modify the header
./tbmate view -cd -i "indices/hg38.idx.gz" -g chr19:246460-346460 out.tbk | less

```
