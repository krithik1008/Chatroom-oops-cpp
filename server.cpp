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
#define MAX_LEN 200				//max length of message
#define NUM_COLORS 6
#define NUM_CLIENTS 5			//max number of clients on the server
using namespace std;

string def_col="\033[0m";
string colors[]={"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m","\033[36m"};
mutex cout_mtx,clients_mtx;		//mutual exclusion to avoid some operations of threads interfering with each other

class server							//class to hold data members and functions required by the server
{
	private:
	struct sockaddr_in socket_address;	//predefined struct containing address information of sockets
	static int server_socket;			//holds socket descriptor(int) 		
	public:
	server()							//constructor to initialize the required server parameters
	{
		socket_address.sin_family=AF_INET;			//specifying IP address family (IPv4)
		socket_address.sin_port=htons(10000);		//assigning port number
		socket_address.sin_addr.s_addr=INADDR_ANY;	//allows binding to any available ip address 
		bzero(&socket_address.sin_zero,0);			//array buffer, usually set to 0
	}
	static void update_server_socket(int val){	//overloaded static function to update server_socket
		server_socket=val;
	}
	static int update_server_socket(){			//overloaded static function to return server_socket value
		return server_socket;
	}
	void establish_server()			
	{
		server_socket=(socket(AF_INET,SOCK_STREAM,0));	//creates a IPv4 TCP socket endpoint (SOCK_STREAM->TCP)
		if(server_socket==-1){
			perror("socket: ");
			exit(-1);
		}
		//bind attaches a port and ip address to an established socket
		if((bind(server_socket,(struct sockaddr *)&socket_address,sizeof(struct sockaddr_in)))==-1){
			perror("bind error: ");
			exit(-1);
		}
		//listen function waits for connection to be established be a client 
		if((listen(server_socket,8))==-1){	// here 8 clients can be handled in the queue by the server
			perror("listen error: ");
			exit(-1);
		}
	}
	// For synchronisation of cout statements so that messages from different clients are not jumbled
	static void shared_print(string str, bool endLine=true)
	{	
		lock_guard<mutex> guard(cout_mtx);		//locks cout_mtx mutex in this scope
		cout<<str;
		if(endLine)
				cout<<endl;
	}
};
int server::server_socket=0;
int seed=0;				//contains the number of clients connected to the server

class terminal			//class to hold details of each client connecting to the server
{
	private:
	int id;				//each client is assigned an id 
	string name;
	int socket;			//holds description of client socket
	public:
	thread th;			//each client operates on a seperate thread for simultaneous chatting
	//destructor to close the client socket connection 
	~terminal()	
	{
		close(socket);
	}
	//functions to get and update private data members for terminal class
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

terminal* clients[NUM_CLIENTS];		//creating an array of terminal object pointers for all new clients

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

	server *new_server=new server;		//creating object opinter of server class
	new_server->establish_server();		

	struct sockaddr_in client;			//temporary sockaddr_in struct to hold addresses of new clients
	int client_socket;					//hold socket descriptor of new clients 
	unsigned int len=sizeof(sockaddr_in);

	cout<<colors[NUM_COLORS-1]<<"\n\t  ====== Welcome to the chat-room ======   "<<endl<<def_col;

	while(1){
		//server accepting conncetion request of client using accept(), which returns client socket description
		if((client_socket=accept(server::update_server_socket(),(struct sockaddr *)&client,&len))==-1)
		{
			perror("accept error: ");
			exit(-1);
		}
		seed++;		//number of clients in server
		thread t(handle_client,client_socket,seed);			//creating a new thread and passing handle_client for the new user
		lock_guard<mutex> guard(clients_mtx);				//locks clients_mtx mutex in this scope
		//updating the details of the new client in the object pointer array
		clients[seed]->update_id(seed);
		clients[seed]->update_name(string("Anonymous"));
		clients[seed]->update_socket(client_socket);
		clients[seed]->th=((move(t)));
	}

	for(int i=0; i<NUM_CLIENTS; i++)
	{
		//join method allows one thread to wait for the completion of other threads
		if(clients[i]->th.joinable())
			clients[i]->th.join();
	}

	close(server::update_server_socket());		//end server socket connection
	return 0;
}
void set_name(int id, char name[])				//to set names to each client 
{
	for(int i=0; i<NUM_CLIENTS; i++)
	{
		if(clients[i]->get_id()==id)
			clients[i]->update_name(string(name));
	}	
}

string color(int code){							//choosing colors to display for each client 
	return colors[code%NUM_COLORS];
}
// Broadcast message to all clients except the sender (overloaded function)
int broadcast_message(string message, int sender_id)
{
	char temp[MAX_LEN];
	strcpy(temp,message.c_str());
	for(int i=0; i<NUM_CLIENTS; i++)
	{
		if(clients[i]->get_id()!=sender_id)
			send(clients[i]->get_socket(),temp,sizeof(temp),0);
	}		
}

// Broadcast a number to all clients except the sender  (overloaded function)
int broadcast_message(int num, int sender_id)
{
	for(int i=0; i<NUM_CLIENTS; i++)
	{
		if(clients[i]->get_id()!=sender_id)
			send(clients[i]->get_socket(),&num,sizeof(num),0);
	}		
}

void end_connection(int id)
{
	for(int i=0; i<NUM_CLIENTS; i++)
	{
		if(clients[i]->get_id()==id)	
		{
			lock_guard<mutex> guard(clients_mtx);		//locks clients_mtx mutex in this scope
			clients[i]->th.detach();	//detach thread of the particular client
			delete clients[i];			//delete the element of the object pointer array
			break;
		}
	}				
}

void handle_client(int client_socket, int id)
{
	char name[MAX_LEN],str[MAX_LEN];
	recv(client_socket,name,sizeof(name),0);		//receive name from the client 
	set_name(id,name);								//set client name 
	// Display welcome message
	string welcome_message=string(name)+string(" has joined");
	broadcast_message("#NULL",id);	
	broadcast_message(id,id);								
	broadcast_message(welcome_message,id);		//prints welcome message when new client joins
	server::shared_print(color(id)+welcome_message+def_col);
	
	while(1)						//we use infinite loops as connection must be live untill each client disconnects
	{
		//collect received data from the client
		int bytes_received=recv(client_socket,str,sizeof(str),0);
		if(bytes_received<=0)
			return;
		if(strcmp(str,"#exit")==0)		//if a client wants to leave the chatroom and send exit message
		{
			// Display leaving message
			string message=string(name)+string(" has left");		
			broadcast_message("#NULL",id);			
			broadcast_message(id,id);						
			broadcast_message(message,id);
			server::shared_print(color(id)+message+def_col);
			end_connection(id);			//end connection of the particular client				
			return;
		}
		//print the received messages from the various clients connected to the server
		broadcast_message(string(name),id);					
		broadcast_message(id,id);		
		broadcast_message(string(str),id);
		server::shared_print(color(id)+name+" : "+def_col+str);		
	}	
}
