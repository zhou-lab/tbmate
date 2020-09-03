library(Rsamtools)
## setwd(/home/zhouw3/repo/tbmate/test)

betas <- readRDS('HM450/betas_GEOLoadSeriesMatrix.rds')
source('../scripts/tbmate.R')
tbk_pack(betas, out_dir = 'HM450/tbk', idx_fname = 'indices/HM450.idx.gz')
betas <- tbk_data('HM450/tbk' ,probes='cg00000236')
betas <- tbk_data('HM450/tbk' ,probes='cg00102731')







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


