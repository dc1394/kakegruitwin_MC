PROG := kakeguruitwin_mc
SRCS :=	checkpoint.cpp goexit.cpp kakeguruitwin_mc.cpp SFMT.c

OBJS = checkpoint.o goexit.o kakeguruitwin_mc.o SFMT.o
DEPS = checkpoint.d goexit.d kakeguruitwin_mc.d SFMT.d

VPATH  = src/checkpoint src/kakeguruitwin_MC src/kakeguruitwin_MC/myrandom \
		 src/kakeguruitwin_MC/goexit src/SFMT-src-1.5.1
CC = clang
CFLAGS = -Wextra -O3 -pipe
CXX = clang++
CXXFLAGS = -Wextra -O3 -pipe -std=c++17
LDFLAGS = -ltbb

all: $(PROG) ;
#rm -f $(OBJS) $(DEPS)

-include $(DEPS)

$(PROG): $(OBJS)
		$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $@ $^
%.o: %.c
		$(CC) $(CFLAGS) -c -MMD -MP -msse2 -DHAVE_SSE2 -DSFMT_MEXP=19937 $<

%.o: %.cpp
		$(CXX) $(CXXFLAGS) -c -MMD -MP -msse2 -DHAVESSE2 -DSFMT_MEXP=19937 -D_CHECK_PARALELL_PERFORM $<

clean:
		rm -f $(PROG) $(OBJS) $(DEPS)
