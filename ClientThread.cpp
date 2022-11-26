#include "ClientThread.h"
#include "Messages.h"

#include <iostream>

ClientThreadClass::ClientThreadClass() {}

void ClientThreadClass::
ThreadBody(std::string ip, int port, int id, int requests, int type) {
	customer_id = id;
	num_requests = requests;
	request_type = type;
	if (!stub.Init(ip, port)) {
		//std::cout << "Thread " << customer_id << " failed to connect" << std::endl;
		return;
	}

	switch (request_type) {
		case 1:
			SendOrderRequest(customer_id, num_requests);
			break;
		case 2:
		case 3:
			SendReadRequest(customer_id, num_requests, request_type);
			break;
		default:
			// std::cout << "Undefined request type in client: "
			// 	<< request_type << std::endl;
			break;
	}
}

ClientTimer ClientThreadClass::GetTimer() {
	return timer;	
}

void ClientThreadClass::SendOrderRequest(int customer_id, int num_requests) {
	for (int i = 0; i < num_requests; i++) {
		Request request;
		LaptopInfo laptop;
		request.SetRequest(customer_id, i, 1);
		
		timer.Start();
		laptop = stub.Order(request);
		timer.EndAndMerge();

		if (!laptop.IsValid()) {
			// std::cout << "Invalid laptop " << customer_id << std::endl;
			break;	
		}
	}
}

void ClientThreadClass::SendReadRequest(int customer_id, int num_requests, int request_type) {
	for (int i = 0; i < num_requests; i++) {
		Request request;
		CustomerRecord crcd;

		if(request_type == 2) {
			request.SetRequest(customer_id, -1, 2);
		} else {
			request.SetRequest(i, -1, 2);
		}

		timer.Start();
		crcd = stub.ReadRecord(request);
		timer.EndAndMerge();

		if (crcd.IsValid()) {
			crcd.Print();
		}
	}
}

