
.PHONY: test clean
test: test_stringd test_stringf test_float test_double test_int1 test_int2 test_int test_ones

test_stringd:
	../tbmate pack -s stringd small/string.bed small/string.tbk
	../tbmate header small/string.tbk
	../tbmate view -o small/view_string.out small/string.tbk
	diff small/view_string.out small/string.bed
	../tbmate view -ko small/view_string2.out small/string.tbk
	diff small/view_string2.out small/string.bed

test_stringf:
	../tbmate pack -s stringf small/string.bed small/string_fixed.tbk
	../tbmate header small/string_fixed.tbk
	../tbmate view -o small/view_string_fixed.out small/string_fixed.tbk
	diff small/view_string_fixed.out small/string.bed
	../tbmate view -ko small/view_string_fixed2.out small/string_fixed.tbk
	diff small/view_string_fixed2.out small/string.bed

test_float:
	../tbmate pack -s float small/float.bed small/float.tbk
	../tbmate header small/float.tbk
	../tbmate view -o small/view_float.out small/float.tbk
	paste small/view_float.out small/float.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'
	../tbmate view -ko small/view_float2.out small/float.tbk
	paste small/view_float2.out small/float.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'

test_double:
	../tbmate pack -s double small/double.bed small/double.tbk
	../tbmate header small/double.tbk
	../tbmate view -o small/view_double.out small/double.tbk
	paste small/view_double.out small/double.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'
	../tbmate view -ko small/view_double2.out small/double.tbk
	paste small/view_double2.out small/double.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'

test_int1:
	../tbmate pack -s int1 small/int1.bed small/int1.tbk
	../tbmate header small/int1.tbk
	../tbmate view -o small/view_int1.out small/int1.tbk
	diff small/view_int1.out small/int1.bed
	../tbmate view -ko small/view_int12.out small/int1.tbk
	diff small/view_int12.out small/int1.bed

test_int2:
	../tbmate pack -s int2 small/int2.bed small/int2.tbk
	../tbmate header small/int2.tbk
	../tbmate view -o small/view_int2.out small/int2.tbk
	diff small/view_int2.out small/int2.bed
	../tbmate view -ko small/view_int22.out small/int2.tbk
	diff small/view_int22.out small/int2.bed

test_int:
	../tbmate pack -s int small/integer.bed small/integer.tbk
	../tbmate header small/integer.tbk
	../tbmate view -o small/view_integer.out small/integer.tbk
	diff small/view_integer.out small/integer.bed
	../tbmate view -ko small/view_integer2.out small/integer.tbk
	diff small/view_integer2.out small/integer.bed

test_ones:
	../tbmate pack -s ones small/ones.bed small/ones.tbk
	../tbmate header small/ones.tbk
	../tbmate view -o small/view_ones.out small/ones.tbk
	paste small/view_ones.out small/ones.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'
	../tbmate view -ko small/view_ones2.out small/ones.tbk
	paste small/view_ones.out small/ones.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'
	diff small/view_ones.out small/view_ones2.out

clean:
	rm -f small/*.out
	rm -f small/*.tbk

test_HM450:
	Rscript HM450.R

clean_HM450:
	rm -f HM450/tbk/*.tbk

test_EPIC:
	parallel -j 10 'bedtools intersect -a /mnt/isilon/zhou_lab/projects/20191221_references/InfiniumArray/EPIC/EPIC.idx.gz -b {} -loj | cut -f1-3,8 | tbmate pack -s float -m /mnt/isilon/zhou_lab/projects/20191221_references/InfiniumArray/EPIC/EPIC.idx.gz - EPIC/tbk/{/.}.tbk' ::: EPIC/raw/*.txt
	tbmate view -i ~/references/InfiniumArray/EPIC/EPIC_to_hg38.idx.gz -g chr19 -cd EPIC/tbk/ >EPIC/chr19.out

clean_EPIC:
	rm -f EPIC/tbk/*.tbk

test_WGBS:
	parallel -j 10 'bedtools intersect -a /mnt/isilon/zhou_lab/projects/20191221_references/hg38/annotation/cpg/idx.gz -b {} -sorted -loj | cut -f1-3,8 | tbmate pack -m "/mnt/isilon/zhou_lab/projects/20191221_references/hg38/annotation/cpg/idx.gz" -s ones - WGBS/tbk/{/.}.tbk' ::: WGBS/raw/*.gz
	tbmate view -cd -g chr19:246460-346460 WGBS/tbk/TCGA_BLCA_A1AG_cpg.tbk WGBS/tbk/TCGA_BLCA_A20V_cpg.tbk >WGBS/1.out
	tbmate view -cd -g chr19:246460-346460 -p 3 WGBS/tbk/*.tbk >WGBS/2.out
	tbmate view -cdi indices/hg38_to_EPIC_idx.gz WGBS/tbk/TCGA_BLCA_A1AG_cpg.tbk WGBS/tbk/TCGA_BLCA_A20V_cpg.tbk >WGBS/3.out
	tbmate view -ac -i ~/references/hg38/annotation/cpg/hg38_to_EPIC_idx.gz tbk/*.tbk >WGBS/4.out
	tbmate view -c -g cg00001638,cg00013374,cg00012123,cg00006867,rs2468330,ch.9.2265413R,ch.9.2295001F -i ~/references/hg38/annotation/cpg/hg38_to_EPIC_idx.gz tbk/*.tbk >WGBS/5.out


