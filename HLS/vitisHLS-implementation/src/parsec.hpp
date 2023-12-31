/**
 * @file parsec.hpp
 *
 * @brief Contains macro definitions takes from the dedup kernel of the PARSEC benchmark suite.
 *
 * https://github.com/bamos/parsec-benchmark/tree/master
 * path:
 * 	pkgs/kernels/dedup
 *
 * @author Ole Oggesen
 * @bug No known bugs
 */

#ifndef PARSEC_H
#define PARSEC_H

#define CHECKBIT 123456 //implemented as an int in the header of the output file

#define COMPRESS_GZIP 0 //compress types supported by the PARSEC dedup benchmark
#define COMPRESS_BZIP2 1 //currently only COMPRESS_NONE supported
#define COMPRESS_NONE 2

#define TYPE_FINGERPRINT 0 //which type of data is printed to file
#define TYPE_COMPRESS 1

#endif //PARSEC_H
