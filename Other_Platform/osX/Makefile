
all: 
	g++ -m32 -O2 -Wall -DLINUX ./PPCS_Client.cpp -L '../../../Lib/osX/x86' -lPPCS_API -o PPCS_Client -lpthread
	g++ -m32 -O2 -Wall -DLINUX ./PPCS_Device.cpp -L '../../../Lib/osX/x86' -lPPCS_API -o PPCS_Device -lpthread

clean:
	rm -rf *.o *~ *.bak PPCS_Client PPCS_Device
