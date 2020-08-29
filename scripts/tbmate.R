dtypes <- c('NA', 'INT2', 'INT32', 'FLOAT', 'DOUBLE', 'ONES')
dtype_size <- c('INT2'=1, 'INT32'=4, 'FLOAT'=4, 'DOUBLE'=8, 'ONES'=2)
dtype_type <- c('INT2'='integer', 'INT32'='integer', 'FLOAT'='numeric', 'DOUBLE'='numeric', 'ONES'='numeric')
MAX_DOUBLE16 <- bitwShiftL(1,15)-2 # 32766
HDR_ID         <- 3
HDR_VERSION    <- 4
HDR_DATA_TYPE  <- 4
HDR_MAX_OFFSET <- 8
HDR_EXTRA      <- 493
HDR_TOTALBYTES <- 512

tbk_hdr <- function(tbk_file) {
    id <- rawToChar(readBin(tbk_file, raw(), 3, 1))
    stopifnot(id == "tbk")
    tbk_version <- readBin(tbk_file, integer(), 1, 4)
    dtype <- dtypes[readBin(tbk_file, integer(), 1, 4)+1]
    num <- readBin(tbk_file, integer(), 1, 8)
    msg <- rawToChar(readBin(tbk_file, raw(), HDR_EXTRA, 1))
    out <- structure(list(
        tbk_version = tbk_version,
        dtype = dtype,
        num = num,
        msg = msg
    ), class='tbk')
}

tbk_hdrs <- function(tbk_fnames) {
    lapply(tbk_fnames, function(tbk_fname) {
        tbk_file <- file(tbk_fname, "rb")
        on.exit(close(tbk_file))
        hdr <- tbk_hdr(tbk_file)
        hdr
    })
}

tbk_data_bulk <- function(tbk_fnames, idx_addr, hdr) {
    data <- do.call(cbind, lapply(tbk_fnames, function(tbk_fname) {
        in_file <- file(tbk_fname,'rb')
        on.exit(close(in_file))
        hdr <- tbk_hdr(in_file)
        readBin(in_file, dtype_type[hdr$dtype], hdr$num, dtype_size[hdr$dtype])[idx_addr+1]
    }))
    rownames(data) <- names(idx_addr)
    data
}

tbk_data_addr <- function(tbk_fnames, idx_addr, hdr) {
    data <- do.call(cbind, lapply(tbk_fnames, function(tbk_fname) {
        in_file <- file(tbk_fname,'rb')
        on.exit(close(in_file))
        hdr <- tbk_hdr(in_file)

        curr_offset <- 0
        betas1 <- sapply(idx_addr, function(offset) {
            if (offset != curr_offset) {
                seek(in_file, where=offset*dtype_size[hdr$dtype]+HDR_TOTALBYTES, origin='start')
                curr_offset <- offset
            }
            curr_offset <<- curr_offset + 1
            if (hdr$dtype == "ONES") {
                (as.numeric(readBin(in_file, 'integer', 1, 2, signed=FALSE)) - MAX_DOUBLE16) / MAX_DOUBLE16
            } else if (hdr$dtype == "INT2") { # FIXME
                readBin(in_file, "integer", 1, 4)
            } else if (hdr$dtype == "FLOAT") {
                readBin(in_file, 'numeric', 1, 4)
            } else if (hdr$dtype == "DOUBLE") {
                readBin(in_file, 'numeric', 1, 8)
            } else if (hdr$dtype == "INT32") {
                readBin(in_file, "integer", 1, 4)
            } else {
                stop("Unrecognized data type.\n")
            }
        })
    }))
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
#' @param name.use.base use basename for sample name
#' @param mx maximum number of probes to do random addressing
#' @return numeric matrix
#' @export
tbk_data <- function(tbk_fnames, idx_fname = NULL, probes = NULL, simplify = FALSE, name.use.base=TRUE, mx = 1000) {

    if (length(tbk_fnames) == 1 && dir.exists(tbk_fnames[1])) {
        tbk_fnames <- list.files(tbk_fnames, '.tbk$', full.names=TRUE)
    }
    
    if (is.null(idx_fname)) {
        if (file.exists(file.path(dirname(tbk_fnames[1]), 'idx.gz')) &&
                file.exists(file.path(dirname(tbk_fnames[1]), 'idx.gz.tbi'))) {
            idx_fname <- file.path(dirname(tbk_fnames[1]), 'idx.gz')
        } else {
            stop("Cannot locate index file. Provide through idx_fname.\n")
        }
    }
    
    idx_addr <- with(read.table(gzfile(idx_fname),
        header=F, stringsAsFactors=FALSE), setNames(V4,V1))
    
    if (length(probes) > 0) {
        idx_addr <- idx_addr[probes]
    }
    idx_addr <- sort(idx_addr)

    if (length(idx_addr) > mx) { # read whole data set if too many addr
        data <- tbk_data_bulk(tbk_fnames, idx_addr, hdr)
    } else {
        data <- tbk_data_addr(tbk_fnames, idx_addr, hdr)
    }
    
    if (ncol(data) == 1) data <- data[,1]
    else {
        if (name.use.base) {
            colnames(data) <- tools::file_path_sans_ext(basename(tbk_fnames))
        } else {
            colnames(data) <- tbk_fnames
        }
    }

    data
}

#' Pack data to tbk
#'
#' @param data
#' @param out_dir output directory, if given the out_fname is set to
#' out_dir/colnames(data).tbk
#' @param out_fname output tbk file name
#' @param dtype data type, INT2, INT32, FLOAT, DOUBLE, ONES
#' @param idx_fname index file name
#' @export
tbk_pack <- function(data, out_dir = NULL, out_fname = NULL, dtype="FLOAT", idx_fname = NULL, tbk_version = 1, msg="") {
    if (is.null(out_dir)) {
        if (is.null(out_fname)) {
            stop("Please provide out_fname.\n")
        } else {
            out_dir = dirname(out_fname);
        }
    }
        
    if (!dir.exists(out_dir)) dir.create(out_dir);
    if (is.null(idx_fname)) {
        if (file.exists(file.path(out_dir, 'idx.gz'))) {
            idx_fname = file.path(out_dir, 'idx.gz')
            message(paste0("Using index: ", idx_fname))
        } else {
            stop("Please provide idx_fname.\n")
        }
    } else if (!file.exists(file.path(out_dir, 'idx.gz'))) {
        file.symlink(
            tools::file_path_as_absolute(idx_fname),
            file.path(out_dir, 'idx.gz'))
        file.symlink(
            paste0(tools::file_path_as_absolute(idx_fname), '.tbi'),
            file.path(out_dir, 'idx.gz.tbi'))
    }
    
    idx_addr <- sort(with(read.table(gzfile(idx_fname), header=F, stringsAsFactors=FALSE), setNames(V4,V1)))
    
    ## if data is a single numeric vector
    if (is.null(dim(data))) {
        data <- as.matrix(data)
        colnames(data) <- out_fname
        if (is.null(out_fname)) stop("Please provide out_fname.\n");
    }

    tmp <- lapply(seq_len(ncol(data)), function(k) {
        fname <- colnames(data)[k]
        idx <- match(names(idx_addr), rownames(data))
        d1 <- ifelse(is.na(idx), -1, data[idx,k])

        out_file <- file(file.path(out_dir, paste0(fname, '.tbk')),'wb')
        on.exit(close(out_file))
        
        writeBin(charToRaw('tbk'), out_file, 1)
        writeBin(as.integer(tbk_version), out_file, 4)
        writeBin(as.integer(which(dtypes == dtype)-1), out_file, 4);
        writeBin(as.integer(length(d1)), out_file, 8)
        msgRaw <- charToRaw(msg)
        if (length(msgRaw) > HDR_EXTRA) {
            msgRaw <- msgRaw[1:HDR_EXTRA]
            warning(sprintf("msg longer than %d, truncated.", HDR_EXTRA))
        }
        if (length(msgRaw) < HDR_EXTRA) {
            msgRaw <- c(msgRaw, rep(as.raw('0'), HDR_EXTRA - length(msgRaw)))
        }
        writeBin(msgRaw, out_file, 1)

        if (dtype == "ONES") {
            writeBin(as.integer(round((d1 + 1.0) * MAX_DOUBLE16)), out_file, 2);
        } else if (dtype == "FLOAT") {
            writeBin(d1, out_file, 4)
        } else if (dtype == 'DOUBLE') {
            writeBin(d1, out_file, 8)
        } else if (dtype == "INT32") {
            writeBin(as.integer(d1), out_file, 4)
        } else if (dtype == "INT2") { # FIXME
            writeBin(as.integer(d1), out_file, 4)
        } else {
            stop("Unrecognized data type.\n")
        }
    })
}

test <- function() {
    source('/home/zhouw3/repo/tbmate/scripts/tbmate.R')
    
    betas <- readRDS('/mnt/isilon/zhou_lab/projects/20191212_GEO_datasets/GSE120878/betas_GEOLoadSeriesMatrix.rds')
    tbk_pack(betas, out_dir = '/home/zhouw3/repo/tbmate/test/HM450/tbk', idx_fname = '/mnt/isilon/zhou_lab/projects/20191221_references/InfiniumArray/HM450/HM450.idx.gz', msg = 'idx:/mnt/isilon/zhou_lab/projects/20191221_references/InfiniumArray/HM450/HM450.idx.gz')

    hdrs <- tbk_hdrs('/home/zhouw3/repo/tbmate/test/HM450/tbk/GSM3417643.tbk')
    bb <- tbk_data('/home/zhouw3/repo/tbmate/test/HM450/tbk/GSM3417643.tbk')
    bb <- tbk_data('/home/zhouw3/repo/tbmate/test/HM450/tbk/GSM3417643.tbk', probes='cg00059930')
    bb <- tbk_data(list.files('/home/zhouw3/repo/tbmate/test/HM450/tbk/', '.tbk$', full.names=T), probes=c('cg00059930','cg27647152','cg27648216'))
    head(bb[,1:4])

    tbk_hdrs('/home/zhouw3/repo/tbmate/test/EPIC/tbk/GSM2998063_201868590267_R01C01.tbk')
    bb <- tbk_data('/home/zhouw3/repo/tbmate/test/EPIC/tbk/')
}
    
