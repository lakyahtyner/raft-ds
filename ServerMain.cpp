#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>
#include <stdio.h>
#include <string.h>

#include "ServerSocket.h"
#include "ServerThread.h"

int main(int argc, char *argv[]) {
	int port;
	int engineer_cnt = 0;
	int uid;
	int leader_id;

	ServerSocket socket;
	LaptopFactory factory;
	std::unique_ptr<ServerSocket> new_socket;
	std::vector<std::thread> thread_vector;
	
	if (argc < 1) {
		std::cout << "not enough arguments" << std::endl;
		std::cout << argv[0] << "[port #] [# admins]" << std::endl;
		return 0;
	}
	uid = atoi(argv[1]);

	std::string line;
	std::ifstream myfile("./servers.txt");
	if (myfile.is_open()) {
		getline(myfile,line);
		leader_id = stoi(line);

		while (getline(myfile,line)) {
			struct PeerServer new_peer;
			new_peer.id = stoi(line.substr(0, 1));
			new_peer.ip = line.substr(2, 13);
			new_peer.port = stoi(line.substr(16, 6));
			new_peer.is_up = 1;
			if(new_peer.id != uid) {
				factory.AddPeer(new_peer);
				std::cout << new_peer.port << std::endl;
			} else {
				port = new_peer.port;
			}
		}
		myfile.close();
	} else {
		std::cout << "Unable to open file";	
		return 0;
	}

	std::thread prod_thread(&LaptopFactory::ProductionAdminThread,
			&factory, 0, uid);
	thread_vector.push_back(std::move(prod_thread));

	if(leader_id == uid) {
		std::thread send_hb_thread(&LaptopFactory::SendHeartbeatThread,
			&factory);
		thread_vector.push_back(std::move(send_hb_thread));
	} else {
		std::thread start_election_thread(&LaptopFactory::StartElectionThread,
			&factory);
		thread_vector.push_back(std::move(start_election_thread));
	}
	
	if (!socket.Init(port)) {
		std::cout << "Socket initialization failed" << std::endl;
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
