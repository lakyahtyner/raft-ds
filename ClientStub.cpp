#include "ClientStub.h"
#include "Messages.h"

ClientStub::ClientStub() {}

int ClientStub::Init(std::string ip, int port) {
	return socket.Init(ip, port);	
}

LaptopInfo ClientStub::Order(Request rqst) {
	LaptopInfo info;
	char buffer[32];
	int size;
	rqst.Marshal(buffer);
	size = rqst.Size();
	
	if (rqst.GetOrderNumber() == 0){
		char ident_buffer[32];
		MarshalIdent(ident_buffer, 1);
		socket.Send(ident_buffer, sizeof(int), 0);
	}

	if (socket.Send(buffer, size, 0)) {
		size = info.Size();
		if (socket.Recv(buffer, size, 0)) {
			info.Unmarshal(buffer);
		} 
	}

	return info;
}

CustomerRecord ClientStub::ReadRecord(Request rqst) {
	CustomerRecord crcd;
	char buffer[32];
	int size;
	rqst.Marshal(buffer);
	size = rqst.Size();

	char ident_buffer[32];
	MarshalIdent(ident_buffer, 1);
	socket.Send(ident_buffer, sizeof(int), 0);
	if (socket.Send(buffer, size, 0)) {
		size = crcd.Size();
		if (socket.Recv(buffer, size, 0)) {
			crcd.Unmarshal(buffer);
		}
	}
	return crcd;
}

