#ifndef __SERVER_STUB_H__
#define __SERVER_STUB_H__

#include <memory>
#include <queue>

#include "ServerSocket.h"
#include "Messages.h"

struct PeerServer {
	int id;
	int port;
	std::string ip;
	int is_up;
};

class ServerStub {
private:
	std::unique_ptr<ServerSocket> in_socket;

public:
	std::vector<ServerSocket> peer_sockets;
	ServerStub();
	void Init(std::unique_ptr<ServerSocket> socket);
	Request ReceiveRequest();
	int ShipLaptop(LaptopInfo info);
	int ReturnRecord(CustomerRecord crcd);
	int SendRepReq(ReplicationRequest rprqst, PeerServer peer, int i);
	int SendRepResponse();
	int ReceiveIdleResp(PeerServer peer, int i);
	int ReceiveIdent();
	ReplicationRequest ReceiveRepReq();
	int Connect(PeerServer peer, int i);
};

#endif // end of #ifndef __SERVER_STUB_H__
