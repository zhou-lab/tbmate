dts <- c('NA', 'INT2', 'INT32', 'FLOAT', 'DOUBLE', 'ONES')
MAX_DOUBLE16 <- bitwShiftL(1,15)-2 # 32766

#' Get data from tbk
#'
#' Assumption:
#' 1) All the tbks have the same index.
#' 2) If idx_fname is not given,  search for idx.gz file in the same folder
#' of the first sample.
#' 
#' @param fnames tbk_fnames
#' @param idx_fname index file name
#' @param probes probe names
#' @param simplify reduce matrix to vector if only one sample is queried
#' @return numeric matrix
#' @export
tbk_data <- function(tbk_fnames, idx_fname = NULL, probes = NULL, simplify = FALSE) {

    if (is.null(idx_fname)) {
        if (file.exists(file.path(dirname(idx_fname), 'idx.gz')) &&
                file.exists(file.path(dirname(idx_fname), 'idx.gz.tbi'))) {
            idx_fname <- file.path(dirname(idx_fname), 'idx.gz')
        } else {
            stop("Please provide index file.\n")
        }
    }
    
    idx_addr <- with(read.table(gzfile(idx_fname),
        header=F, stringsAsFactors=FALSE), setNames(V4,V1))
    
    if (length(probes) > 0) {
        idx_addr <- idx_addr[probes]
    }
    idx_addr <- sort(idx_addr)

    data <- do.call(cbind, lapply(tbk_fnames, function(tbk_fname) {
        in_file <- file(tbk_fname,'rb')
        on.exit(close(in_file))
        dt <- dts[readBin(in_file, 'integer', 1, 4)+1]

        curr_offset <- 0
        betas1 <- sapply(idx_addr, function(offset) {
            if (offset != curr_offset) {
                seek(in_file, where=offset, origin='start')
                curr_offset <- offset
            }
            curr_offset <<- curr_offset + 1
            if (dt == "ONES") {
                (as.numeric(readBin(in_file, 'integer', 1, 2, signed=FALSE)) - MAX_DOUBLE16) / MAX_DOUBLE16
            } else if (dt == "FLOAT") {
                readBin(in_file, 'numeric', 1, 4)
            } else if (dt == "DOUBLE") {
                readBin(in_file, 'numeric', 1, 8)
            } else if (dt == "INT32") {
                readBin(in_file, "integer", 1, 4)
            } else if (dt == "INT2") { # FIXME
                readBin(in_file, "integer", 1, 4)
            } else {
                stop("Unrecognized data type.\n")
            }
        })
    }))
    if (ncol(data) == 1) data <- data[,1]
    else colnames(data) <- tbk_fnames

    data
}

#' Pack data to tbk
#'
#' @param data
#' @param out_dir output directory, if given the out_fname is set to
#' out_dir/colnames(data).tbk
#' @param out_fname output tbk file name
#' @param datatype data type
#' @param idx_fname index file name
#' @export
tbk_pack <- function(data, out_dir = NULL, out_fname = NULL, datatype="FLOAT", idx_fname = NULL) {
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
        out_file <- file(file.path(out_dir, paste0(fname, '.tbk')),'wb')
        on.exit(close(out_file))
        writeBin(as.integer(which(dts == datatype)-1), out_file, 4);

        idx <- match(names(idx_addr), rownames(data))
        d1 <- ifelse(is.na(idx), -1, data[idx,k])
        if (datatype == "ONES") {
            writeBin(as.integer(round((d1 + 1.0) * MAX_DOUBLE16)), out_file, 2);
        } else if (datatype == "FLOAT") {
            writeBin(d1, out_file, 4)
        } else if (datatype == 'DOUBLE') {
            writeBin(d1, out_file, 8)
        } else if (datatype == "INT32") {
            writeBin(as.integer(d1), out_file, 4)
        } else if (datatype == "INT2") { # FIXME
            writeBin(as.integer(d1), out_file, 4)
        } else {
            stop("Unrecognized data type.\n")
        }
        ## curr_offset <- 0
        ## tmp <- lapply(seq_along(idx_addr), function(offset) {
        ##     ## idx_addr[i], write b1[i,1]
        ##     if (offset != curr_offset) {
        ##         seek(out_file, where = offset, origin='start')
        ##         curr_offset <- offset
        ##     }
        ##     curr_offset <<- curr_offset + 1
        ##     if (datatype == "ONES") {
        ##         writeBin(as.integer(round((data[offset,k] + 1.0) * MAX_DOUBLE16)), out_file, 2);
        ##     } else if (datatype == "FLOAT") {
        ##         writeBin(as.numeric(data[offset,k]), out_file, 'numeric', 1, 4)
        ##     } else if (datatype == "DOUBLE") {
        ##         writeBin(out_file, 'numeric', 1, 8)
        ##     } else if (datatype == "INT32") {
        ##         writeBin(out_file, "integer", 1, 4)
        ##     } else if (datatype == "INT2") { # FIXME
        ##         writeBin(out_file, "integer", 1, 4)
        ##     } else {
        ##         stop("Unrecognized data type.\n")
        ##     }
        ## })
        ## close(out_file)
    })
}

test <- function() {
    source('/home/zhouw3/repo/tbmate/scripts/tbmate.R')
    bb <- tbk_data(tbk_fnames = '/home/zhouw3/repo/tbmate/test/HM450/tmp.tbk', idx_fname = '/home/zhouw3/repo/tbmate/test/HM450/HM450_to_HM450.bed4i.gz')
    
    betas <- readRDS('/mnt/isilon/zhou_lab/projects/20191212_GEO_datasets/GSE120878/betas_GEOLoadSeriesMatrix.rds')
    tbk_pack(betas[,1:3], out_dir = '/mnt/isilon/zhou_lab/projects/20191212_GEO_datasets/GSE120878/HM450_tbk', idx_fname = '/home/zhouw3/repo/tbmate/test/HM450/HM450_to_HM450.bed4i.gz')
}
    
