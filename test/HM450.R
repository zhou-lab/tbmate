library(Rsamtools)
## setwd(/home/zhouw3/repo/tbmate/test)

betas <- readRDS('HM450/betas_GEOLoadSeriesMatrix.rds')
source('../scripts/tbmate.R')
tbk_pack(betas, out_dir = 'HM450/tbk', idx_fname = 'indices/HM450.idx.gz')
betas <- tbk_data('HM450/tbk' ,probes='cg00000236')
betas <- tbk_data('HM450/tbk')
betas <- tbk_data('HM450/tbk' ,probes='cg00102731')
betas <- tbk_data('/home/zhouw3/repo/tbmate/test/small/float_float.tbk')
source('../scripts/tbmate.R')

betas <- tbk_data('/home/zhouw3/repo/tbmate/test/small/float_int.tbk')
betas <- tbk_data('/home/zhouw3/repo/tbmate/test/small/float_int.tbk', all_units=TRUE)

betas <- tbk_data('/home/zhouw3/repo/tbmate/test/small/float_float.tbk')
betas <- tbk_data('/home/zhouw3/repo/tbmate/test/small/float_float.tbk', all_units=TRUE)

betas <- tbk_data(c(
    '/home/zhouw3/repo/tbmate/test/small/float_float.tbk',
    '/home/zhouw3/repo/tbmate/test/small/float_int.tbk',
    '/home/zhouw3/repo/tbmate/test/small/float.tbk') , all_units=TRUE)

betas <- tbk_data(c(
    '/home/zhouw3/repo/tbmate/test/small/float_float.tbk',
    '/home/zhouw3/repo/tbmate/test/small/float_int.tbk',
    '/home/zhouw3/repo/tbmate/test/small/float.tbk'))






hdrs <- tbk_hdrs('/home/zhouw3/repo/tbmate/test/HM450/tbk/GSM3417643.tbk')
bb <- tbk_data('/home/zhouw3/repo/tbmate/test/HM450/tbk/GSM3417643.tbk')
bb <- tbk_data('/home/zhouw3/repo/tbmate/test/HM450/tbk/GSM3417643.tbk', probes='cg00059930')
bb <- tbk_data(list.files('/home/zhouw3/repo/tbmate/test/HM450/tbk/', '.tbk$', full.names=T), probes=c('cg00059930','cg27647152','cg27648216'))
head(bb[,1:4])
source('/home/zhouw3/repo/tbmate/scripts/tbmate.R')
tbk_hdrs('/home/zhouw3/repo/tbmate/test/EPIC/tbk/GSM2998063_201868590267_R01C01.tbk')
bb <- tbk_data('/home/zhouw3/repo/tbmate/test/EPIC/tbk/')
head(tbk_data('/home/zhouw3/repo/tbmate/test/EPIC/tbk/', idx_fname = '~/references/InfiniumArray/EPIC/EPIC_to_hg38.idx.gz', chrm='chr19', beg=200000, end=300000))
head(b[,1:6])
b <- tbk_data('/home/zhouw3/repo/tbmate/test/EPIC/tbk/GSM2998063_201868590267_R01C01.tbk', idx_fname = '~/references/InfiniumArray/EPIC/EPIC_to_hg38.idx.gz')
b <- tbk_data('/home/zhouw3/repo/tbmate/test/EPIC/tbk/', idx_fname = '~/references/InfiniumArray/EPIC/EPIC_to_hg38.idx.gz', chrm='chr19', beg=200000, end=300000)
b <- tbk_data('/home/zhouw3/repo/tbmate/test/WGBS/tbk/', chrm='chr19', beg=200000, end=300000)
tbk_data('/home/zhouw3/repo/tbmate/test/WGBS/tbk/', idx_fname='~/references/hg38/annotation/cpg/hg38_to_EPIC_idx.gz', chrm='cg00004072')
tbk_data('/home/zhouw3/repo/tbmate/test/WGBS/tbk/', idx_fname='~/references/hg38/annotation/cpg/hg38_to_EPIC_idx.gz', probes='cg00004072')

library(sesame)
base_dir <- '/mnt/isilon/zhou_lab/projects/20191212_GEO_datasets/GSE122126/'
pfxs <- searchIDATprefixes(file.path(base_dir, 'IDATs'))
source('https://raw.githubusercontent.com/zhou-lab/tbmate/master/scripts/tbmate.R')

tmp <- mclapply(seq_along(pfxs), function(i) {
    sset <- readIDATpair(pfxs[i])
    cat(pfxs[i],sset@platform,'\n')
    betas <- getBetas(dyeBiasCorrTypeINorm(noob(sset)))
    pvals <- pval(sset)[names(betas)]
    tbk_pack(betas, data2 = pvals, out_dir = sprintf('%s/tbk_%s/', base_dir, sset@platform), out_fname = str_split(names(pfxs)[i],'_')[[1]][1], idx_fname = sprintf('/mnt/isilon/zhou_lab/projects/20191221_references/InfiniumArray/%s/%s.idx.gz', sset@platform, sset@platform), dtype='FLOAT_FLOAT')
} , mc.cores=15)

