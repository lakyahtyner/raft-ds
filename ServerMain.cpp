#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "ServerSocket.h"
#include "ServerThread.h"

int main(int argc, char *argv[]) {
	int port;
	int engineer_cnt = 0;
	int uid;
	int num_peers;

	ServerSocket socket;
	LaptopFactory factory;
	std::unique_ptr<ServerSocket> new_socket;
	std::vector<std::thread> thread_vector;
	
	if (argc < 3) {
		std::cout << "not enough arguments" << std::endl;
		std::cout << argv[0] << "[port #] [# admins]" << std::endl;
		return 0;
	}
	port = atoi(argv[1]);
	uid = atoi(argv[2]);
	num_peers = atoi(argv[3]);

	for(int n = 4; n < 4 + (num_peers * 3); n += 3) {
		struct PeerServer new_peer;
		new_peer.id = atoi(argv[n]);
		new_peer.ip = argv[n+1];
		new_peer.port = atoi(argv[n+2]);
		new_peer.is_up = 1;
		factory.AddPeer(new_peer);
	}

	std::thread prod_thread(&LaptopFactory::ProductionAdminThread,
			&factory, 0, uid);
	thread_vector.push_back(std::move(prod_thread));

	if (!socket.Init(port)) {
		//std::cout << "Socket initialization failed" << std::endl;
		return 0;
	}

	while ((new_socket = socket.Accept())) {
		std::thread engineer_thread(&LaptopFactory::EngineerThread, 
				&factory, std::move(new_socket), 
				engineer_cnt++);
		thread_vector.push_back(std::move(engineer_thread));
	}
	return 0;
}
