#include "ServerStub.h"
#include <cstring>
#include <arpa/inet.h>

ServerStub::ServerStub() {}

void ServerStub::Init(std::unique_ptr<ServerSocket> socket) {
	this->socket = std::move(socket);
}


int ServerStub::SendReplicationAck(int rep_ack){
	char buffer[32];
	// int rep_ack = 1;
	int net_ack = htonl(rep_ack);
	memcpy(buffer,&net_ack, sizeof(net_ack));
	return socket->Send(buffer, sizeof(net_ack), 0);
}

int ServerStub::ReceiveReplicationAck(ServerSocket* socket_ptr){
	char buffer[32];
	int net_ack=-1;
	if(socket_ptr->Recv(buffer, sizeof(net_ack), 0)){
		memcpy(&net_ack, buffer, sizeof(net_ack));
	}


	int rep_ack = ntohl(net_ack);
	// printf("%d\n", rep_ack);
	return rep_ack;

}


int ServerStub::SendReplicationRequest(ServerSocket* socket_ptr, ReplicationRecord record){
	char buffer[32];
	record.Marshal(buffer);
	return socket_ptr->Send(buffer, record.Size(), 0);
}

ReplicationRecord ServerStub::ReceiveReplicationRequest(){
	char buffer[32];
	ReplicationRecord record;
	if (socket->Recv(buffer, record.Size(), 0)) {
		record.Unmarshal(buffer);
	}

	return record;


}

CustomerRequest ServerStub::ReceiveRequest() {
	char buffer[32];
	CustomerRequest order;
	if (socket->Recv(buffer, order.Size(), 0)) {
		order.Unmarshal(buffer);
	}
	return order;
}

int ServerStub::SendAdminId(ServerSocket* socket_ptr, int unique_id){
	char buffer[32];
	unique_id = htonl(unique_id);
	memcpy(buffer,&unique_id, sizeof(unique_id));
	return socket_ptr->Send(buffer, sizeof(unique_id), 0);
}

int ServerStub::ReceiveId(){

	char buffer[32];
	int unique_id = -1;
	int net_id = 0;
	if(socket->Recv(buffer, sizeof(net_id), 0)){
		memcpy(&net_id, buffer, sizeof(net_id));
	}
	unique_id = ntohl(net_id);

	return unique_id;
}

int ServerStub::SendLaptop(LaptopInfo info) {
	char buffer[32];
	info.Marshal(buffer);
	return socket->Send(buffer, info.Size(), 0);
}

int ServerStub::ReturnRecord(CustomerRecord record){
	char buffer[32];
	record.Marshal(buffer);
	return socket->Send(buffer, record.Size(), 0);
}

int ServerStub::SendLeaderId(int unique_id){
	char buffer[32];
	unique_id = htonl(unique_id);
	memcpy(buffer,&unique_id, sizeof(unique_id));
	return socket->Send(buffer, sizeof(unique_id), 0);
}
