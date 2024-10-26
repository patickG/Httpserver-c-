bin=httpserver 
cc=g++
LD_FLAG=-std=c++11 -lpthread
src=main.cc

.PHONY:clean
all:httpserver httpclient
$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAG)

#httpclient:client.cc
#	g++ -o $@ $^

.PHONY:clean
clean:
	rm -f $(bin)
