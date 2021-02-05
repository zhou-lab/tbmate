dtypes <- c(
    'INT1'=1, 'INT2'=2, 'INT32'=3, 'FLOAT'=4, 'DOUBLE'=5, 'STRINGD'=6, 'STRINGF'=7,
    'ONES'=30, 'FLOAT_INT'=31, 'FLOAT_FLOAT'=32)
dtype_size <- c(
    'INT1'=1, 'INT2'=1, 'INT32'=4, 'FLOAT'=4, 'DOUBLE'=8, 'STRINGD'=8, 'STRINGF'=8,
    'ONES'=4,  'FLOAT_INT'=8,  'FLOAT_FLOAT'=8)
dtype_type <- list(
    'INT1'='integer', 'INT2'='integer', 'INT32'='integer', 'FLOAT'='numeric',
    'DOUBLE'='numeric', 'STRINGD'='character', 'STRINGF'='character',
    'ONES'='numeric', 'FLOAT_INT'=c('numeric','integer'), 'FLOAT_FLOAT'=c('numeric','numeric'))

MAX_DOUBLE16 <- bitwShiftL(1,15)-2 # 32766
HDR_ID         <- 3
HDR_VERSION    <- 4
HDR_DATA_TYPE  <- 8
HDR_MAX_OFFSET <- 8
HDR_EXTRA      <- 8169
HDR_TOTALBYTES <- 8192

tbk_hdr <- function(tbk_file) {
    id <- rawToChar(readBin(tbk_file, raw(), 3, 1))
    stopifnot(id == "tbk")
    tbk_version <- readBin(tbk_file, integer(), 1, 4)
    dtype <- readBin(tbk_file, integer(), 1, 8)
    num <- readBin(tbk_file, integer(), 1, 8)
    msg <- rawToChar(readBin(tbk_file, raw(), HDR_EXTRA, 1))
    out <- structure(list(
        tbk_version = tbk_version,
        dtype = names(dtypes)[dtypes==bitwAnd(dtype, 0xff)],
        smax = bitwShiftR(dtype,8),
        num = num,
        msg = msg
    ), class='tbk')
}

tbk_hdr_write <- function(out_file, data, dtype = "FLOAT", msg = "", tbk_version = 1) {
    writeBin(charToRaw('tbk'), out_file, 1)
    writeBin(as.integer(tbk_version), out_file, 4)

    dt <- as.integer(dtypes[dtype])
    if (dtype == "STRINGF") dt <- bitwOr(dt, bitwShiftL(max(sapply(data, nchar)),8))
    writeBin(dt, out_file, 8);
    writeBin(as.integer(length(data)), out_file, 8)
    msgRaw <- charToRaw(msg)
    if (length(msgRaw) > HDR_EXTRA) {
        msgRaw <- msgRaw[1:HDR_EXTRA]
        warning(sprintf("msg longer than %d, truncated.", HDR_EXTRA))
    }
    if (length(msgRaw) < HDR_EXTRA) {
        msgRaw <- c(msgRaw, rep(as.raw('0'), HDR_EXTRA - length(msgRaw)))
    }
    writeBin(msgRaw, out_file, 1)
}

#' retrieve tbk headers
#'
#' @param tbk_fnames the tbk file names
#' @return a list object of class tbk
#' @export
tbk_hdrs <- function(tbk_fnames) {
    lapply(tbk_fnames, function(tbk_fname) {
        tbk_file <- file(tbk_fname, "rb")
        on.exit(close(tbk_file))
        hdr <- tbk_hdr(tbk_file)
        hdr
    })
}

tbk_read_chunk <- function(in_file, hdr, idx_addr, all_units = FALSE, config = config) {

    read_unit_default <- function() {
        cbind(
            sig=readBin(in_file, dtype_type[[hdr$dtype]], hdr$num, dtype_size[hdr$dtype]))
    }
    
    read_unit1 <- list(
        'INT1' = read_unit_default, # FIXME
        'INT2' = read_unit_default, # FIXME
        'ONES' = read_unit_default, # FIXME
        'FLOAT_FLOAT' = function() {
            d0 <- readBin(in_file, "raw", hdr$num*8, 1)
            d1 <- readBin(d0[bitwAnd(bitwShiftR(seq_along(d0)+3,2),1) == 1], 'numeric', hdr$num, 4)
            d2 <- readBin(d0[bitwAnd(bitwShiftR(seq_along(d0)+3,2),1) == 0], 'numeric', hdr$num, 4)
            d1[d2 > config$max_pval] <- NA
            if (all_units) {
                cbind(sig=d1, sig2=d2)
            } else {
                cbind(d1)
            }
        },
        'FLOAT_INT' = function() {
            d0 <- readBin(in_file, "raw", hdr$num*8, 1)
            d1 <- readBin(d0[bitwAnd(bitwShiftR(seq_along(d0)+3,2),1) == 1], 'numeric', hdr$num, 4)
            d2 <- readBin(d0[bitwAnd(bitwShiftR(seq_along(d0)+3,2),1) == 0], 'integer', hdr$num, 4)
            d1[d2 < config$min_coverage] <- NA
            if (all_units) {
                cbind(sig=d1, sig2=d2)
            } else {
                cbind(d1)
            }
        },
        'INT32' = read_unit_default, 'FLOAT' = read_unit_default, 'DOUBLE' = read_unit_default
    )

    data <- read_unit1[[hdr$dtype]]()
    data <- data[idx_addr+1,,drop=FALSE]
    rownames(data) <- names(idx_addr)
    data
}

tbk_data_bulk <- function(tbk_fnames, idx_addr, all_units = FALSE, config = config) {
    idx_addr <- sort(idx_addr)
    data <- lapply(tbk_fnames, function(tbk_fname) {
        in_file <- file(tbk_fname,'rb')
        on.exit(close(in_file))
        hdr <- tbk_hdr(in_file)
        idx_addr <- ifelse(idx_addr < 0, NA, idx_addr)
        tbk_read_chunk(in_file, hdr, idx_addr, all_units = all_units, config = config)
    })
    data
}

tbk_read_unit <- function(in_file, idx_addr, hdr, all_units = FALSE, config = config) {
    
    read_unit1 <- list(
        'INT1' = function(idx_addr) {
            vapply(idx_addr, function(offset) {
                if (offset < 0) return(NA)
                if (offset != curr_offset) {
                    seek(in_file, where=offset*dtype_size[hdr$dtype]+HDR_TOTALBYTES, origin='start')
                    curr_offset <- offset
                }
                curr_offset <<- curr_offset + n
                readBin(in_file, "integer", 1, 1) # FIXME
            })
        },
        'INT2' = function(idx_addr) {
            vapply(idx_addr, function(offset) {
                if (offset < 0) return(NA)
                if (offset != curr_offset) {
                    seek(in_file, where=offset*dtype_size[hdr$dtype]+HDR_TOTALBYTES, origin='start')
                    curr_offset <- offset
                }
                curr_offset <<- curr_offset + 1
                readBin(in_file, "integer", 1, 1) # FIXME
            })
        },
        'FLOAT' = function(idx_addr) {
            vapply(idx_addr, function(offset) {
                if (offset < 0) return(NA)
                if (offset != curr_offset) {
                    seek(in_file, where=offset*dtype_size[hdr$dtype]+HDR_TOTALBYTES, origin='start')
                    curr_offset <- offset
                }
                curr_offset <<- curr_offset + 1
                readBin(in_file, "numeric", 1, 4)
            }, numeric(1))
        },
        'DOUBLE' = function(idx_addr) {
            vapply(idx_addr, function(offset) {
                if (offset < 0) return(NA)
                if (offset != curr_offset) {
                    seek(in_file, where=offset*dtype_size[hdr$dtype]+HDR_TOTALBYTES, origin='start')
                    curr_offset <- offset
                }
                curr_offset <<- curr_offset + 1
                readBin(in_file, "numeric", 1, 4)
            })
        },
        'INT32' = function(idx_addr) {
            vapply(idx_addr, function(offset) {
                if (offset < 0) return(NA)
                if (offset != curr_offset) {
                    seek(in_file, where=offset*dtype_size[hdr$dtype]+HDR_TOTALBYTES, origin='start')
                    curr_offset <- offset
                }
                curr_offset <<- curr_offset + 1
                readBin(in_file, "integer", 1, 4)
            })
        },
        'ONES' = function(idx_addr) {
            vapply(idx_addr, function(offset) {
                if (offset < 0) return(NA)
                if (offset != curr_offset) {
                    seek(in_file, where=offset*dtype_size[hdr$dtype]+HDR_TOTALBYTES, origin='start')
                    curr_offset <- offset
                }
                curr_offset <<- curr_offset + 1
                (as.numeric(readBin(in_file, 'integer', 1, 2, signed=FALSE)) - MAX_DOUBLE16) / MAX_DOUBLE16
            })
        },
        'FLOAT_INT' = function(idx_addr) {
            vapply(idx_addr, function(offset) {
                if (offset < 0) return(NA)
                if (offset != curr_offset) {
                    seek(in_file, where=offset*dtype_size[hdr$dtype]+HDR_TOTALBYTES, origin='start')
                    curr_offset <- offset
                }
                curr_offset <<- curr_offset + 1
                d1 = readBin(in_file, "numeric", 1, 4)
                d2 = readBin(in_file, "integer", 1, 4)
                d1[d2 < config$min_coverage] <- NA
                if (all_units) {
                    c(d1, d2)
                } else {
                    d1
                }
            })
        },
        'FLOAT_FLOAT' = function(idx_addr) {
            vapply(idx_addr, function(offset) {
                if (offset < 0) return(NA)
                if (offset != curr_offset) {
                    seek(in_file, where=offset*dtype_size[hdr$dtype]+HDR_TOTALBYTES, origin='start')
                    curr_offset <- offset
                }
                curr_offset <<- curr_offset + 1
                d1 = readBin(in_file, "numeric", 1, 4)
                d2 = readBin(in_file, "numeric", 1, 4)
                d1[d2 > config$max_pval] <- NA
                if (all_units) {
                    c(d1, d2)
                } else {
                    d1
                }
            })
        }
    )

    curr_offset <- 0
    read_unit1[[hdr$dtype]](idx_addr)        
}

tbk_data_addr <- function(tbk_fnames, idx_addr, all_units = FALSE, config = config) {
    idx_addr <- sort(idx_addr)
    data <- lapply(tbk_fnames, function(tbk_fname) {
        in_file <- file(tbk_fname,'rb')
        on.exit(close(in_file))
        hdr <- tbk_hdr(in_file)
        tbk_read_unit(in_file, idx_addr, hdr, all_units = FALSE, config = config)
    })
}

tbk_data0 <- function(tbk_fnames, idx_addr, all_units = FALSE, max_addr = 3000, max_source = 10^6, config = config) {

    ## read whole data set only if there are too many addresses but too small source data
    if (tbk_hdrs(tbk_fnames[1])[[1]]$num < max_source && length(idx_addr) < max_addr) {
        tbk_data_addr(tbk_fnames, idx_addr, all_units = all_units, config = config)
    } else {
        tbk_data_bulk(tbk_fnames, idx_addr, all_units = all_units, config = config)
    }
}

infer_idx <- function(tbk_fname) {
    if (file.exists(file.path(dirname(tbk_fname), 'idx.gz')) &&
            file.exists(file.path(dirname(tbk_fname), 'idx.gz.tbi'))) {
        idx_fname <- file.path(dirname(tbk_fname), 'idx.gz')
    } else {
        hdr <- tbk_hdrs(tbk_fname)[[1]]
        if (!is.null(hdr$msg) && file.exists(hdr$msg)) {
            idx_fname <- hdr$msg
        }
        stop("Cannot locate index file. Provide through idx_fname.\n")
    }
    idx_fname
}

#' Get data from tbk
#'
#' Assumption:
#' 1) All the tbks have the same index.
#' 2) If idx_fname is not given,  search for idx.gz file in the same folder
#' of the first sample.
#' 
#' @param fnames tbk_fnames
#' @param idx_fname index file name. If not given, use the
#' idx.gz in the same folder as tbk_fnames[1]. Assume all input is compatible with the
#' same index file.
#' @param probes probe names
#' @param simplify reduce matrix to vector if only one sample is queried
#' @param all_units retrieve all units for float.float and float.int
#' @param max_pval maximum sig2 for float.float
#' @param min_coverage minimum sig2 for float.int
#' @param name.use.base use basename for sample name
#' @param negative.as.na NA-mask negative value
#' @param max_addr random addressing if under max_addr
#' @param min_source random addressing if source size is under min_source
#' @return numeric matrix
#' @export
tbk_data <- function(
    tbk_fnames, idx_fname = NULL, probes = NULL, show.unaddressed = FALSE,
    chrm = NULL, beg = NULL, end = NULL, as.matrix = FALSE, negative.as.na = TRUE,
    simplify = FALSE, name.use.base=TRUE, max_addr = 3000, max_source = 10^6,
    max_pval = 0.05, min_coverage = 5, all_units = FALSE) {

    library(Rsamtools)

    ## given a folder
    if (length(tbk_fnames) == 1 && dir.exists(tbk_fnames[1])) {
        tbk_fnames <- list.files(tbk_fnames, '.tbk$', full.names=TRUE)
    }

    ## infer idx
    if (is.null(idx_fname)) { idx_fname <- infer_idx(tbk_fnames[1]); }

    ## subset probes
    if (is.null(chrm)) { # subset with probes
        idx_addr <- with(read.table(gzfile(idx_fname),
            header=F, stringsAsFactors=FALSE), setNames(V4,V1))
        if (length(probes) > 0) {
            idx_addr <- idx_addr[probes]
        }
    } else { # subset with genomic coordinates
        if (is.null(beg)) beg <- 1
        if (is.null(end)) end <- 2^28
        input_range <- GRanges(chrm, IRanges(beg, end))
        df <- as.data.frame(t(simplify2array(strsplit(scanTabix(
            idx_fname, param=input_range)[[1]], '\t'))), stringsAsFactors = FALSE)
        colnames(df)[1:4] <- c('seqnames','beg','end','offset')
        df$offset <- as.integer(df$offset)
        if (!show.unaddressed) df <- df[df$offset >= 0,]
        if (ncol(df) >= 5 && sum(df$beg==1) > 10) { # use probe names from 5th column
            idx_addr <- setNames(df$offset, df[,5])
        } else if (length(unique(df$seqnames[1:min(nrow(df),100)])) < 30) { # use chr:beg
            idx_addr <- setNames(df$offset, paste0(df$seqnames,":",df$beg))
        } else { # use probe names from rownames
            idx_addr <- setNames(df$offset, rownames(df))
        }
    }

    config <- list(max_pval = max_pval, min_coverage = min_coverage)

    data <- tbk_data0(tbk_fnames, idx_addr, max_addr=max_addr, max_source=max_source, all_units=all_units, config = config)

    ## add column names
    if (name.use.base) {
        names(data) <- tools::file_path_sans_ext(basename(tbk_fnames))
    } else {
        names(data) <- tbk_fnames
    }

    ## add row names
    if (!all_units) {
        snames <- names(data)
        data <- do.call(cbind, data)
        colnames(data) <- snames
    }

    if (negative.as.na) {
        data[data < 0] <- NA
    }
    ## if (!is.null(chrm)) {
    ##     if (as.matrix && ncol(df)>=5) {
    ##         rownames(data) <- df[rownames(data),5]
    ##         data <- as.matrix(data)
    ##     } else {
    ##         data <- cbind(df[rownames(data),], data)
    ##     }
    ## } else if (ncol(data) == 1 && simplify) {
    ##     data <- data[,1]
    ## }

    data
}


tbk_write_unit <- function(out_file, d1, d2, dtype) {

    if (dtype == "INT1") {
        d0 <- 0
        tmp <- lapply(seq_along(d1), function(i) {
            d0 <<- bitwOr(bitwShiftL(d0, i%%8), bitwAnd(d1, 0x1))
            if (i%%8 == 0) {
                writeBin(as.raw(d0), out_file, 1)
                d0 <<- 0
            }
        })
    } else if (dtype == "INT2") {
        d0 <- 0
        tmp <- lapply(seq_along(d1), function(i) {
            d0 <<- bitwOr(bitwShiftL(d0, 2*(i%%4)), bitwAnd(d1, 0x3))
            if (i%%4 == 0) {
                writeBin(as.raw(d0), out_file, 1)
                d0 <<- 0
            }
        })
    } else if (dtype == "INT32") {
        writeBin(as.integer(d1), out_file, 4)
    } else if (dtype == "FLOAT") {
        writeBin(d1, out_file, 4)
    } else if (dtype == 'DOUBLE') {
        writeBin(d1, out_file, 8)
    } else if (dtype == "STRINGF") {
        stop("String packing not supported in R yet. Please use the command line.")
    } else if (dtype == "STRINGD") {
        stop("String packing not supported in R yet. Please use the command line.")
    }  else if (dtype == "ONES") {
        writeBin(as.integer(round((d1 + 1.0) * MAX_DOUBLE16)), out_file, 2);
    }  else if (dtype == "FLOAT_FLOAT") {
        stopifnot(!is.null(d2))
        writeBin(do.call('c', lapply(seq_along(d1), function(i) {
            c(writeBin(d1[i], raw(), 4), writeBin(d2[i], raw(), 4))
        })), out_file, 1)
    }  else if (dtype == "FLOAT_INT") {
        stopifnot(!is.null(d2))
        writeBin(do.call('c', lapply(seq_along(d1), function(i) {
            c(writeBin(d1[i], raw(), 4), writeBin(as.integer(d2[i]), raw(), 4))
        })), out_file, 1)
    } else {
        stop("Unrecognized data type.\n")
    }
}

#' Pack data to tbk
#'
#' @param data
#' @param data2 2nd data if float.float or float.int
#' @param out_dir output directory, if given the out_fname is set to
#' out_dir/colnames(data).tbk
#' @param out_fname output tbk file name
#' @param dtype data type, INT1, INT2, INT32, FLOAT, DOUBLE, STRINGD, STRINGF
#' @param idx_fname index file name
#' @param link_idx whether or not a link is generated to the index
#' @export
tbk_pack <- function(data, data2 = NULL, out_dir = NULL, out_fname = NULL, dtype="FLOAT", idx_fname = NULL, tbk_version = 1, msg="", na.token = -1.0, link_idx = FALSE) {

    ## output dir
    if (is.null(out_dir)) {
        if (is.null(out_fname)) {
            stop("Please provide out_fname.\n")
        } else {
            out_dir = dirname(out_fname);
        }
    }
    if (!dir.exists(out_dir)) dir.create(out_dir);

    ## infer index
    if (is.null(idx_fname)) {
        if (file.exists(file.path(out_dir, 'idx.gz'))) {
            idx_fname = file.path(out_dir, 'idx.gz')
            message(paste0("Using index: ", idx_fname))
        } else {
            stop("Please provide idx_fname.\n")
        }
    } else if (link_idx && !file.exists(file.path(out_dir, 'idx.gz'))) { # set up idx.gz if nonexistent
        file.symlink(
            tools::file_path_as_absolute(idx_fname),
            file.path(out_dir, 'idx.gz'))
        file.symlink(
            paste0(tools::file_path_as_absolute(idx_fname), '.tbi'),
            file.path(out_dir, 'idx.gz.tbi'))
    }

    ## load index
    idx_addr <- sort(with(read.table(gzfile(idx_fname), header=F,
        stringsAsFactors=FALSE), setNames(V4,V1)))
    
    ## if data is a single numeric vector
    if (is.null(dim(data))) {
        data <- as.matrix(data)
        colnames(data) <- out_fname
        if (is.null(out_fname)) stop("Please provide out_fname.\n");
    }
    if (!is.null(data2) && is.null(dim(data2))) {
        data2 <- as.matrix(data2)
        colnames(data2) <- out_fname
        if (is.null(out_fname)) stop("Please provide out_fname.\n");
    }

    tmp <- lapply(seq_len(ncol(data)), function(k) {
        fname <- colnames(data)[k]
        idx <- match(names(idx_addr), rownames(data))
        d1 <- ifelse(is.na(idx), na.token, data[idx,k])
        d1[is.na(d1)] <- na.token
        if (is.null(data2)) {
            d2 <- NULL
        } else {
            d2 <- ifelse(is.na(idx), na.token, data2[idx,k])
            d2[is.na(d2)] <- na.token
        }

        out_fname1 <- file.path(out_dir, fname)
        if (!grepl('\\.tbk$', out_fname1)) out_fname1 <- paste0(out_fname1, '.tbk')
        out_file <- file(out_fname1, 'wb')
        on.exit(close(out_file))

        ## store idx_fname if msg is not given
        if (length(msg) == 0) {
            msg = idx_fname
        }
        if (nchar(msg) == 0) msg <- idx_fname
        tbk_hdr_write(out_file, d1, dtype = dtype, msg = msg, tbk_version = tbk_version)
        tbk_write_unit(out_file, d1, d2, dtype)
    })
}

#' Read from tabix-indexed bed file to list objects
#' 
#' @param paths paths to the bed files
#' @param chrm  chromosome name
#' @param beg   start coordinate of CpG
#' @param end   end coordinate of CpG 2^29 is the max taken
#' @param sample_names sample names, just use paths if not specified
#' @param BPPARAM how to parallelize
#' @return a list object with DNA methylation level and depth
#'
#' @import BiocParallel
#' @import dplyr
#'
#' @export
tabixRetrieve <- function(
    paths, chrm, beg = 1, end = 2^28,
    min_depth = 0, sample_names = NULL,
    BPPARAM = SerialParam()) {

    input_range <- GRanges(chrm, IRanges(beg, end))
    df_list <- bplapply(paths, function(path) {
        df <- as_tibble(t(simplify2array(strsplit(scanTabix(
            path, param=input_range)[[1]], '\t'))), stringsAsFactors = FALSE)
        
        colnames(df) <- c('chrm','beg','end','beta','depth')
        df$beg <- as.numeric(df$beg)
        df$beg <- df$beg + 1
        df$end <- as.numeric(df$end)
        ## in case the tabix is mal-formed
        df$beta[df$beta == '.' | df$beta == 'NA'] <- NA
        df$beta <- as.numeric(df$beta)
        df$depth <- as.integer(df$depth)
        df$depth[is.na(df$beta)] <- 0
        df
    })
    
    ## make sure the coordinates are the same
    same_coordinates <- sapply(seq_len(length(df_list)-1),
        function(i) identical(df_list[[i]][,1:3], df_list[[i+1]][,1:3]))
    stopifnot(all(same_coordinates))

    ## set sample names
    if (is.null(sample_names)) {
        sample_names <- paths
    }
    stopifnot(length(sample_names) == length(paths))
    names(df_list) <- sample_names

    df_list
}


## requires tbk and platform column in samples
tbk_data2 <- function(samples, probes=NULL, max_pval=0.2) {

    platform2idx = c(
        EPIC='~/references/InfiniumArray/EPIC/EPIC.idx.gz',
        HM450='~/references/InfiniumArray/HM450/HM450.idx.gz')

    platforms <- unique(samples$platform)
    betases <- lapply(platforms, function(x) {
        tbk_data(
            samples$tbk,
            idx_fname = platform2idx,
            probes = probes, max_pval = max_pval)
    })
    do.call(cbind_betas_onCommon, betases)
}
