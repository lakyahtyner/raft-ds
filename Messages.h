#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <string>

struct MapOp{
	int opcode;
	int arg1;
	int arg2;
};

class ReplicationRecord{
private:
	int factory_id;
	int committed_index;
	int last_index;
	int opcode;
	int arg1;
	int arg2;

public:
	ReplicationRecord();
	void operator = (const ReplicationRecord &record) {
		factory_id = record.factory_id;
		committed_index = record.committed_index;
		last_index = record.last_index;
		opcode = record.opcode;
		arg1 = record.arg1;
		arg2 = record.arg2;
	}
	void SetRecord(int id, int cidx, int lidx, MapOp sobj);
	void Marshal(char* buffer);
	void Unmarshal(char* buffer);
	int GetFactoryId();
	int GetCommitedIndex();
	int GetLastIndex();
	void GetMapObj(MapOp &smr_obj);
	int Size();
	void Print();
};




class CustomerRecord{
private:
	int customer_id;
	int last_order;
public:
	CustomerRecord();
	void operator = (const CustomerRecord &record) {
		customer_id = record.customer_id;
		last_order = record.last_order;
	}
	void SetRecord(int cid, int order_num);
	int GetCustomerId();
	int GetLastOrder();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();

};

class CustomerRequest{
private:
	int customer_id;
	int order_number;
	int request_type; //changed to request_type

public:
	CustomerRequest();
	void operator = (const CustomerRequest &order) {
		customer_id = order.customer_id;
		order_number = order.order_number;
		request_type = order.request_type;
	}
	void SetOrder(int cid, int order_num, int type);
	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class LaptopInfo {
private:
	int customer_id;
	int order_number;
	int request_type; //change to request
	int engineer_id;
	int admin_id; //change to admin_id.

public:
	LaptopInfo();
	void operator = (const LaptopInfo &info) {
		customer_id = info.customer_id;
		order_number = info.order_number;
		request_type = info.request_type;
		engineer_id = info.engineer_id;
		admin_id = info.admin_id;
	}
	void SetInfo(int cid, int order_num, int type, int engid, int expid);
	void CopyOrder(CustomerRequest order);
	void SetEngineerId(int id);
	void SetAdminId(int id);

	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();
	int GetEngineerId();
	int GetAdminId();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class ConfigUpdate {
private:
	int type;
	int id;
	int ip_1;
	int ip_2;
	int ip_3;
	int ip_4;
	int port;
	
public:
	ConfigUpdate();
	void operator = (const ConfigUpdate &update) {
		type = update.type;
		id = update.id;
		ip_1 = update.ip_1;
		ip_2 = update.ip_2;
		ip_3 = update.ip_3;
		ip_4 = update.ip_4;
		port = update.port;
	}

	void SetUpdate(int type, int id, int ip_1, int ip_2, int ip_3, int ip_4, int port);
	int GetType();
	int GetId();
	std::string GetIp();
	int GetPort();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();
	void Print();

};


#endif // #ifndef __MESSAGES_H__
