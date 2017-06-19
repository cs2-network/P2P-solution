
all: 
	g++ -O2 -m32 -Wall -DLINUX ./PPCS_Client.cpp ../../../Lib/Linux/x86/libPPCS_API.so -o PPCS_Client -s -lpthread
	g++ -O2 -m32 -Wall -DLINUX ./PPCS_Device.cpp ../../../Lib/Linux/x86/libPPCS_API.so -o PPCS_Device -s -lpthread

clean:
	rm -rf *.o *~ *.bak PPCS_Client PPCS_Device
