#ifndef __SERVER_STUB_H__
#define __SERVER_STUB_H__

#include <memory>
#include <vector>
#include<map>

#include "ServerSocket.h"
#include "Messages.h"

class ServerStub {
private:
	std::unique_ptr<ServerSocket> socket;
public:
	std::map<int,ServerSocket> peer_sockets;
	ServerStub();
	void Init(std::unique_ptr<ServerSocket> socket);
	CustomerRequest ReceiveRequest();
	int ReceiveId();
	int SendAdminId(ServerSocket* socket_ptr, int unique_id);
	int SendLaptop(LaptopInfo info);
	int ReturnRecord(CustomerRecord record);
	int SendReplicationRequest(ServerSocket* socket_ptr, ReplicationRecord record);
	ReplicationRecord ReceiveReplicationRequest();
	int SendReplicationAck(int rep_ack=1);
	int ReceiveReplicationAck(ServerSocket* socket_ptr);
	int SendLeaderId(int unique_id);
};

#endif // end of #ifndef __SERVER_STUB_H__
