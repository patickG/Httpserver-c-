bin=httpserver
cc=g++
LD_FLAG=-std=c++11 -lpthread
src=main.cc

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAG)

:PHONY:clean
clean:
	rm -f $(bin)
