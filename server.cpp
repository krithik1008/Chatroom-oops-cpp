#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#define MAX_LEN 200
#define NUM_COLORS 6
#define NUM_CLIENTS 5
using namespace std;

string def_col="\033[0m";
string colors[]={"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m","\033[36m"};
mutex cout_mtx,clients_mtx;

class server
{
	private:
	struct sockaddr_in socket_address;
	static int server_socket;
	public:
	server()
	{
		socket_address.sin_family=AF_INET;
		socket_address.sin_port=htons(10000);
		socket_address.sin_addr.s_addr=INADDR_ANY;
		bzero(&socket_address.sin_zero,0);
	}
	static void update_server_socket(int val){
		server_socket=val;
	}
	static int update_server_socket(){
		return server_socket;
	}
	void establish_server()
	{
		server_socket=(socket(AF_INET,SOCK_STREAM,0));
		if(server_socket==-1){
			perror("socket: ");
			exit(-1);
		}
		if((bind(server_socket,(struct sockaddr *)&socket_address,sizeof(struct sockaddr_in)))==-1){
			perror("bind error: ");
			exit(-1);
		}
		if((listen(server_socket,8))==-1){
			perror("listen error: ");
			exit(-1);
		}
	}
	// For synchronisation of cout statements
	static void shared_print(string str, bool endLine=true)
	{	
		lock_guard<mutex> guard(cout_mtx);
		cout<<str;
		if(endLine)
				cout<<endl;
	}
};
int server::server_socket=0;
int seed=0;

class terminal
{
	private:
	int id;
	string name;
	int socket;
	public:
	thread th;
	~terminal()
	{
		close(socket);
	}
	void update_id(int id){
		this->id=id;
	}
	void update_name(string name){
		this->name=name;
	}
	void update_socket(int socket){
		this->socket=socket;
	}
	int get_id(){
		return id;
	}
	string get_name(){
		return name;
	}
	int get_socket(){
		return socket;
	}
};

terminal* clients[NUM_CLIENTS];

string color(int code);
void set_name(int id, char name[]);
int broadcast_message(string message, int sender_id);
int broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);

int main()
{
	for(int i=0;i<NUM_CLIENTS;i++)
		clients[i]=new terminal;

	server *new_server=new server;
	new_server->establish_server();	

	struct sockaddr_in client;
	int client_socket;
	unsigned int len=sizeof(sockaddr_in);

	cout<<colors[NUM_COLORS-1]<<"\n\t  ====== Welcome to the chat-room ======   "<<endl<<def_col;

	while(1)
	{
		if((client_socket=accept(server::update_server_socket(),(struct sockaddr *)&client,&len))==-1)
		{
			perror("accept error: ");
			exit(-1);
		}
		seed++;
		thread t(handle_client,client_socket,seed);
		lock_guard<mutex> guard(clients_mtx);
		clients[seed]->update_id(seed);
		clients[seed]->update_name(string("Anonymous"));
		clients[seed]->update_socket(client_socket);
		clients[seed]->th=((move(t)));
	}

	for(int i=0; i<NUM_CLIENTS; i++)
	{
		if(clients[i]->th.joinable())
			clients[i]->th.join();
	}

	close(server::update_server_socket());
	return 0;
}
void set_name(int id, char name[])
{
	for(int i=0; i<NUM_CLIENTS; i++)
	{
		if(clients[i]->get_id()==id)
			clients[i]->update_name(string(name));
	}	
}

string color(int code){
	return colors[code%NUM_COLORS];
}
// Broadcast message to all clients except the sender
int broadcast_message(string message, int sender_id)
{
	char temp[MAX_LEN];
	strcpy(temp,message.c_str());
	for(int i=0; i<NUM_CLIENTS; i++)
	{
		if(clients[i]->get_id()!=sender_id)
		{
			send(clients[i]->get_socket(),temp,sizeof(temp),0);
		}
	}		
}

// Broadcast a number to all clients except the sender
int broadcast_message(int num, int sender_id)
{
	for(int i=0; i<NUM_CLIENTS; i++)
	{
		if(clients[i]->get_id()!=sender_id)
		{
			send(clients[i]->get_socket(),&num,sizeof(num),0);
		}
	}		
}

void end_connection(int id)
{
	for(int i=0; i<NUM_CLIENTS; i++)
	{
		if(clients[i]->get_id()==id)	
		{
			lock_guard<mutex> guard(clients_mtx);
			clients[i]->th.detach();
			delete clients[i];
			break;
		}
	}				
}

void handle_client(int client_socket, int id)
{
	char name[MAX_LEN],str[MAX_LEN];
	recv(client_socket,name,sizeof(name),0);
	set_name(id,name);	

	// Display welcome message
	string welcome_message=string(name)+string(" has joined");
	broadcast_message("#NULL",id);	
	broadcast_message(id,id);								
	broadcast_message(welcome_message,id);	
	server::shared_print(color(id)+welcome_message+def_col);
	
	while(1)
	{
		int bytes_received=recv(client_socket,str,sizeof(str),0);
		if(bytes_received<=0)
			return;
		if(strcmp(str,"#exit")==0)
		{
			// Display leaving message
			string message=string(name)+string(" has left");		
			broadcast_message("#NULL",id);			
			broadcast_message(id,id);						
			broadcast_message(message,id);
			server::shared_print(color(id)+message+def_col);
			end_connection(id);							
			return;
		}
		broadcast_message(string(name),id);					
		broadcast_message(id,id);		
		broadcast_message(string(str),id);
		server::shared_print(color(id)+name+" : "+def_col+str);		
	}	
}
