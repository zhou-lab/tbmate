# Try out TabixMate

## Viewing

```sh

wget ""
unzip test
cd test

# use tbmate_darwin_amd64 if on mac

# on native coordinate systems
./tbmate_linux_amd64 view -cd -g chr19:246460-346460 WGBS/tbk | less
./tbmate_linux_amd64 view -cd EPIC/tbk | less
./tbmate_linux_amd64 view -g cg00013374,cg00012123,cg00006867 -cd HM450/tbk | less


# convert hg38 to EPIC array and subset with probe ID
./tbmate_linux_amd64 view -cd -g cg00013374,cg00012123,cg00006867 -i indices/hg38_to_EPIC.idx.gz WGBS/tbk | less

# convert EPIC array to hg38 and subset with genomic range
./tbmate_linux_amd64 view -cd -g chr19:246460-346460 -i indices/EPIC_to_hg38.idx.gz WGBS/tbk | less

```

## Packing, index specification and switching

```sh
# note the index is provided as a relative path, it will remain valid as long as out.tbk is kept relatively
# same with respective to indices/hg38.idx.gz, one can also provide an absolute path
./tbmate_linux_amd64 pack -s float -m "indices/hg38.idx.gz" WGBS/raw/TCGA_BLCA_A13J_cpg.gz tmp.tbk

# view header
./tbmate_linux_amd64 header out.tbk

# change index location
./tbmate_linux_amd64 header -m "indices/hg38_to_EPIC.idx.gz" out.tbk

# confirm it's changed
./tbmate_linux_amd64 header out.tbk

# now using the new coordinates
./tbmate_linux_amd64 view -cd -g cg00013374,cg00012123,cg00006867 out.tbk | less

# switch back without modify the header
./tbmate_linux_amd64 view -cd -i "indices/hg38.idx.gz" -g chr19:246460-346460 out.tbk | less

```
