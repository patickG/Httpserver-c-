bin=httpserver 
cc=g++
LD_FLAG=-std=c++11 -lpthread
curr=$(shell pwd)
cgi=test_cgi
src=main.cc

ALL:$(bin) $(cgi)
.PHONY:ALL

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAG)

$(cgi):cgi/test_cgi.cc
	$(cc) -o $@ $^

.PHONY:clean
clean:
	rm -f $(bin) $(cgi)
	rm -rf output

.PHONY:output
output:
		mkdir -p output
		cp $(bin) output
		cp -rf wwwroot output
		cp $(cgi) output/wwwroot
