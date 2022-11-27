#include "ClientStub.h"
#include <cstring>
#include <arpa/inet.h>

ClientStub::ClientStub() {}

int ClientStub::Init(std::string ip, int port) {
	return socket.Init(ip, port);
}

int ClientStub::SendId(int unique_id){
	char buffer[32];
	unique_id = htonl(unique_id);
	memcpy(buffer, &unique_id, sizeof(unique_id));
	return socket.Send(buffer, sizeof(unique_id), 0);
}

LaptopInfo ClientStub::OrderLaptop(CustomerRequest order) {
	LaptopInfo info;
	char buffer[32];
	int size;
	order.Marshal(buffer);
	size = order.Size();
	if (socket.Send(buffer, size, 0)) {
		size = info.Size();
		if (socket.Recv(buffer, size, 0)) {
			info.Unmarshal(buffer);
		}
	}
	return info;
}

CustomerRecord ClientStub::ReadRecord(CustomerRequest order){
	CustomerRecord record;
	char buffer[32];
	int size;
	order.Marshal(buffer);
	size = order.Size();
	if (socket.Send(buffer, size, 0)) {
		size = record.Size();
		if (socket.Recv(buffer, size, 0)) {
			record.Unmarshal(buffer);
		}
	}
	return record;

}
