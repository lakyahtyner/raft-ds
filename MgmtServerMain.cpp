#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>

#include "ServerSocket.h"
#include "ServerThread.h"
#include "ServerStub.h"
#include "Messages.h"

int LaptopFactory::unique_id;
int LaptopFactory::leader_id;
std::map<int,std::string> LaptopFactory::peer_ips;
std::map<int,int> LaptopFactory::peer_ports;
int LaptopFactory::n_peers;
std::map<int,bool> LaptopFactory::peer_isalive;
ServerStub stub;

void addNode(){
	bool good_id = false;
	std::string id;
	while(!good_id) {
		std::cout << "Enter the id for the new node: ";
		std::cin >> id;

		if(LaptopFactory::peer_ips.count(stoi(id)) > 0) {
			std::cout << "This id already exists" << std::endl;
		} else {
			good_id = true;
		}
	}

	bool good_ip = false;
	std::string ip_1, ip_2, ip_3;
	while(!good_ip) {
		std::cout << "Enter segment 1 for ip for the new node: ";
		std::cin >> ip_1;

		std::cout << "Enter segment 2 for ip for the new node: ";
		std::cin >> ip_2;

		std::cout << "Enter segment 3 for ip for the new node: ";
		std::cin >> ip_3;

		std::string ip = ip_1 + "." + ip_2 + "." + ip_3;
		bool statement = stoi(ip_1) > 0 && stoi(ip_1) < 256 &&
							stoi(ip_2) > 0 && stoi(ip_2) < 256 &&
							stoi(ip_3) > 0 && stoi(ip_3) < 256;
		if(statement){
			for(const auto& peer_ip : LaptopFactory::peer_ips){
				if(peer_ip.second == ip) {
					std::cout << "This ip already exists" << std::endl;
				} else {
					good_ip = true;
				}
			}
		} else {
			std::cout << "This ip is out of range" << std::endl;
		}
	}

	bool good_port = false;
	std::string port;
	while(!good_port) {
		std::cout << "Enter the port for the new node: ";
		std::cin >> port;

		if(stoi(port) < 20000 || stoi(port) > 65535) {
			std::cout << "This port is invalid" << std::endl;
		} else {
			good_port = true;
		}
	}

	ConfigUpdate update;
	update.SetUpdate(1, stoi(id), stoi(ip_1), stoi(ip_2), stoi(ip_3), stoi(port));

	std::cout << "New Node Info: " << std::endl;
	update.Print();
	std::cout << std::endl;
	
	int confirm;
	std::cout << "Would you lke to add this node?" << std::endl;
	std::cout << "(1) Yes (2) No: ";
	std::cin >> confirm;
	
	if(confirm == 1) {	
		for (auto const& peer_info : LaptopFactory::peer_ips) {
			if(LaptopFactory::peer_isalive[peer_info.first]){
				stub.SendUpdateRequest(&stub.peer_sockets[peer_info.first], update);

				// LaptopFactory::peer_ips.insert(std::pair<int, std::string>(stoi(id), ip));
				// LaptopFactory::peer_ports.insert(std::pair<int,int>(stoi(id), stoi(port)));
				// LaptopFactory:: peer_isalive.insert(std::pair<int,bool>(stoi(id), true));
			}
		}
		std::cout << "Node deleted" << std::endl;
	}  else if (confirm == 2){
		std::cout << "Nothing deleted" << std::endl;
	} else {
		std::cout << "Ivalid Option" << std::endl;
	}
	

	std::cout << "Node successfully added" << std::endl;
}

void removeNode(){
	bool good_id = false;
	std::string id;
	while(!good_id) {
		std::cout << "Enter the id of the node to delete: ";
		std::cin >> id;

		if(LaptopFactory::peer_ips.count(stoi(id)) < 1) {
			std::cout << "This id does not exists" << std::endl;
		} else {
			good_id = true;
		}
	}

	int ip = -1;
	int confirm;
	std::cout << "Delete Node: " + id + "?" << std::endl;
	std::cout << "Are you sure? (1) Yes (2) No: ";
	std::cin >> confirm;

	if(confirm == 1) {
		ConfigUpdate update;
		update.SetUpdate(2, stoi(id), ip, ip, ip, -1);
		for (auto const& peer_info : LaptopFactory::peer_ips) {
			if(LaptopFactory::peer_isalive[peer_info.first]){
				stub.SendUpdateRequest(&stub.peer_sockets[peer_info.first], update);
				
				// LaptopFactory::peer_ips.erase(LaptopFactory::peer_ips.find(stoi(id)));
				// LaptopFactory::peer_ports.erase(LaptopFactory::peer_ports.find(stoi(id)));
				// LaptopFactory::peer_isalive.erase(LaptopFactory::peer_isalive.find(stoi(id)));
			}
			
		}
		std::cout << "Node successfully deleted" << std::endl;
	} else if (confirm == 2){
		std::cout << "Nothing Added" << std::endl;
	} else {
		std::cout << "Ivalid Option" << std::endl;
	}
}

int main(int argc, char *argv[]) {
	LaptopFactory factory;
	std::vector<std::thread> thread_vector;

	std::string line;
	std::ifstream myfile("./servers.txt");
	if (myfile.is_open()) {
		getline(myfile,line);
		LaptopFactory::leader_id = stoi(line);

		while (getline(myfile,line)) {
			std::stringstream ss(line);

			std::string peer_id;
			ss >> peer_id;

			std::string peer_ip;
			ss >> peer_ip;

			int peer_port;
			ss >> peer_port;
			bool peer_isup = true;

			LaptopFactory::peer_ips.insert(std::pair<int, std::string>(stoi(peer_id), peer_ip));
			LaptopFactory::peer_ports.insert(std::pair<int,int>(stoi(peer_id), peer_port));
			LaptopFactory::peer_isalive.insert(std::pair<int,bool>(stoi(peer_id), peer_isup));
			
			std::cout << peer_id << std::endl;
			std::cout << peer_ip << std::endl;
			std::cout << peer_port << std::endl;
			
		}
		myfile.close();
	} else {
		std::cout << "Unable to open file";
		return 0;
	}

	LaptopFactory::n_peers = LaptopFactory::peer_ips.size();

	for (auto const& peer_info : LaptopFactory::peer_ips) {
		int ident = peer_info.first;
		std::string ip = peer_info.second;
		ServerSocket temp_socket;
		
		stub.peer_sockets.insert(std::pair<int,ServerSocket>(ident, temp_socket));
		if (!stub.peer_sockets[ident].Init(ip, LaptopFactory::peer_ports[ident])) {
			std::cout << "Node " << ident << " failed to connect" << std::endl;
			LaptopFactory::peer_isalive[ident] = false;
		}  else {
			stub.SendAdminId(&stub.peer_sockets[ident], -2);
		}
	}

	int option = 0; 
	std::cout << "Welcome to the Raft Configuration Module " << std::endl;
	while(option != 3) {
		std::cout << "Select from the following options: " << std::endl;
		std::cout << "1: Add node to cluster" << std::endl;
		std::cout << "2: Remove Node from cluster" << std::endl;
		std::cout << "3: Exit Module" << std::endl;
		std::cout << "Option: ";
		std::cin >> option;

		switch (option) {
			case 1:
				addNode();
				break;
			case 2:
				removeNode();
				break;
			case 3:
				break;
			default:
				std::cout << std::endl;
				std::cout << "Incorrect input" << std::endl;
				break;
		}
		std::cout << "IOption: " << option << std::endl;
	}

	return 0;
}

