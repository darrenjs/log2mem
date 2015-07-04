CXX=g++

# edit to specify where the log2mem build was installed
LOG2MEMDIR=/var/tmp/log2mem

hello:  hello.cc
	${CXX} -I$(LOG2MEMDIR)/include -o hello hello.cc   -L$(LOG2MEMDIR)/lib -llog2mem

clean:
	rm -f hello