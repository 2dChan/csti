EXEC    = csti
VERSION = 0.1

# paths
SRC_DIR   = src
BUILD_DIR = build

# includes and libs
LIBS     = -ltls

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -DVERSION=\"${VERSION}\"
# CFLAGS   = -g -std=c99 -pedantic -Wall -O3 -march=native ${CPPFLAGS}
CFLAGS   = -std=c99 -pedantic -Wall -O3 -march=native ${CPPFLAGS}
LDFLAGS  = ${LIBS}

CC = cc
