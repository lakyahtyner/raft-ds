#include "ClientThread.h"
#include "Messages.h"

#include <iostream>

ClientThreadClass::ClientThreadClass() {}

void ClientThreadClass::
ThreadBody(std::string ip, int port, int id, int orders, int type) {
	customer_id = id;
	num_orders = orders;
	request_type = type;
	if (!stub.Init(ip, port)) {
		std::cout << "Thread " << customer_id << " failed to connect" << std::endl;
		return;
	}

	stub.SendId(-1);

	if (request_type == 1) { // harsh: build request_type functions.
			for (int i = 0; i < num_orders; i++) {
				CustomerRequest order;
				LaptopInfo laptop;
				order.SetOrder(customer_id, i, request_type);

				timer.Start();
				laptop = stub.OrderLaptop(order);
				timer.EndAndMerge();

				if (!laptop.IsValid()) {
					// std::cout << "Invalid laptop " << customer_id << std::endl;
					break;
				}
			}
		}
		else if(request_type == 2){

			CustomerRequest order;
			CustomerRecord record;
			order.SetOrder(customer_id, -1, request_type);

			timer.Start();
			record = stub.ReadRecord(order);
			timer.EndAndMerge();
			if (record.IsValid()){
				record.Print();
			}
		}

		else if(request_type == 3){
			int leader_id = -1;
			for (int i = 0; i < num_orders; i++) {
				CustomerRequest order;
				CustomerRecord record;
				order.SetOrder(i, -1, 2);

				timer.Start();
				record = stub.ReadRecord(order);
				leader_id = stub.ReceiveId();
				timer.EndAndMerge();

				if (record.IsValid()){
					record.Print();
				}
			}
			printf("Current Leader Id: %d\n", leader_id);
		}

		// else if(request_type == 4){
		//
		// 	printf("Need Leader\n");
		// 	CustomerRequest order;
		// 	// CustomerRecord record;
		// 	order.SetOrder(customer_id, -1, 3);
		//
		// 	timer.Start();
		// 	int leader_id = stub.ReceiveId();
		// 	timer.EndAndMerge();
		// 	printf("Current Leader Id: %d\n", leader_id);
		// }


		else{
			std::cout << "Undefined request type: "<< request_type << std::endl;
		}
}

ClientTimer ClientThreadClass::GetTimer() {
	return timer;
}
