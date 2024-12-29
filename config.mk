EXEC    = csti
VERSION = 0.1

# paths
SRC_DIR   = src
BUILD_DIR = build

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700L -DVERSION=\"${VERSION}\"
CFLAGS   = -std=c99 -pedantic -Wall -O3 -march=native ${CPPFLAGS}

CC = cc
