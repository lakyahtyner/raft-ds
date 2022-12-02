#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>

#include "ServerSocket.h"
#include "ServerThread.h"

int LaptopFactory::unique_id;
// int LaptopFactory::leader_id = -1;
std::map<int,std::string> LaptopFactory::peer_ips;
std::map<int,int> LaptopFactory::peer_ports;
int LaptopFactory::n_peers;
std::map<int,bool> LaptopFactory::peer_isalive;


int main(int argc, char *argv[]) {
	int port;
	int engineer_cnt = 0;
	int num_experts=1;
	// int leader_id;

	ServerSocket socket;
	LaptopFactory factory;
	std::unique_ptr<ServerSocket> new_socket;
	std::vector<std::thread> thread_vector;

	if (argc < 2) {
		std::cout << "not enough arguments" << std::endl;
		std::cout << argv[0] << " [id #] " << std::endl;
		return 0;
	}

	LaptopFactory::unique_id = atoi(argv[1]);

	std::string line;
	std::ifstream myfile("./servers_init.txt");
	if (myfile.is_open()) {
		getline(myfile,line);
		// LaptopFactory::leader_id = stoi(line);

		while (getline(myfile,line)) {
			std::stringstream ss(line);

			std::string peer_id;
			ss >> peer_id;

			std::string peer_ip;
			ss >> peer_ip;

			int peer_port;
			ss >> peer_port;
			bool peer_isup = true;

			if(stoi(peer_id) != LaptopFactory::unique_id) {
				LaptopFactory::peer_ips.insert(std::pair<int, std::string>(stoi(peer_id), peer_ip));
				LaptopFactory::peer_ports.insert(std::pair<int,int>(stoi(peer_id), peer_port));
				LaptopFactory:: peer_isalive.insert(std::pair<int,bool>(stoi(peer_id), peer_isup));
				// factory.AddPeer(new_peer);
				// std::cout << peer_id << std::endl;
				std::cout << peer_ip << std::endl;
				// std::cout << peer_port << std::endl;
			}
			else {
				port = peer_port;
			}
		}
		myfile.close();
	}
	else {
		std::cout << "Unable to open file";
		return 0;
	}

	LaptopFactory::n_peers = LaptopFactory::peer_ips.size();

	for (int i = 0; i < num_experts; i++) {
		std::thread admin_thread(&LaptopFactory::AdminThread,
				&factory, engineer_cnt++);
		thread_vector.push_back(std::move(admin_thread));
	}


		std::thread hb_thread(&LaptopFactory::HeartbeatThread,&factory);
		thread_vector.push_back(std::move(hb_thread));

		std::thread election_thread(&LaptopFactory::ElectionThread, &factory);
		thread_vector.push_back(std::move(election_thread));


	// if(LaptopFactory::leader_id == LaptopFactory::unique_id) {
	// 	std::thread hb_thread(&LaptopFactory::HeartbeatThread,
	// 		&factory);
	// 	thread_vector.push_back(std::move(hb_thread));
	// }
	// else {
	// 	std::thread election_thread(&LaptopFactory::ElectionThread,
	// 		&factory);
	// 	thread_vector.push_back(std::move(election_thread));
	// }

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
