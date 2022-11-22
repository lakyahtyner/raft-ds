#include <iostream>  // remove this later
#include "ServerStub.h"
#include <string.h>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>

ServerStub::ServerStub() {}

void ServerStub::Init(std::unique_ptr<ServerSocket> socket) {
	this->in_socket = std::move(socket);
}

Request ServerStub::ReceiveRequest() {
	char buffer[32];
	
	Request rqst;
	if (in_socket->Recv(buffer, rqst.Size(), 0)) {
		rqst.Unmarshal(buffer);
	}
	
	return rqst;
}

int ServerStub::ShipLaptop(LaptopInfo info) {
	char buffer[32];
	info.Marshal(buffer);
	return in_socket->Send(buffer, info.Size(), 0);
}

int ServerStub::ReturnRecord(CustomerRecord crcd) {
	char buffer[32];
	crcd.Marshal(buffer);
	return in_socket->Send(buffer, crcd.Size(), 0);
}

int ServerStub::SendRepReq(ReplicationRequest rprqst, PeerServer peer, int i) {
	char buffer[32];
	
	rprqst.Marshal(buffer);
	return peer_sockets[i].Send(buffer, rprqst.Size(), 0);
}

ReplicationRequest ServerStub::ReceiveRepReq() {
	char buffer[32];
	
	ReplicationRequest rprqst;
	if (in_socket->Recv(buffer, rprqst.Size(), 0)) {
		rprqst.Unmarshal(buffer);
	}
	
	return rprqst;
}

int ServerStub::ReceiveIdleResp(PeerServer peer, int i) {
	char buffer[32];

	int resp = 0;
	if (peer_sockets[i].Recv(buffer, sizeof(int), 0)) {
		resp = UnmarshalIdent(buffer);
	}
	
	return resp;
}

int ServerStub::SendRepResponse() {
	char resp_buffer[32];

	MarshalIdent(resp_buffer, 1);
	
	return in_socket->Send(resp_buffer, sizeof(int), 0);
}

int ServerStub::ReceiveIdent(){
	char buffer[32];
	
	int ident = 0;
	if (in_socket->Recv(buffer, sizeof(int), 0)) {
		ident = UnmarshalIdent(buffer);
	}
	
	return ident;
}

int ServerStub::Connect(PeerServer peer, int i) {
	return peer_sockets[i].Connect(peer.ip, peer.port);
}

int ServerStub::SendIdent(int ident, int i) {
	char ident_buffer[32];
	int net_ident = htonl(2);

	MarshalIdent(ident_buffer, ident);
	return peer_sockets[i].Send(ident_buffer, sizeof(net_ident), 0);
}
