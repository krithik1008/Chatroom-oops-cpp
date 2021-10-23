#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include <mutex>
#define MAX_LEN 200					//max length of message
#define NUM_COLORS 6
using namespace std;

thread t_send, t_recv;				//sepeate threads for sending and receiving data (for concurrancy) 
string def_col="\033[0m";
string colors[]={"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};

class client							//class to hold client data members and functions
{
	private:
		struct sockaddr_in client_add;	//predefined struct containing address information of sockets
		char name[MAX_LEN];				//holds name of the client
		static int client_socket;		//holds socket descriptor of the client 
		static bool exit_flag;			//used to end client socket connection
	public:
		client()						//constructor to initialize required parameters of the client address
		{
			client_add.sin_family=AF_INET;		//specifying IP address family (IPv4)
			client_add.sin_port=htons(10000); 	// Port no. of server
			client_add.sin_addr.s_addr=INADDR_ANY;	//allows connecting to any available ip address
			bzero(&client_add.sin_zero,0);		//array buffer, usually set to 0
		}
		static int modify_client_socket(){		//overloaded static function to update client_socket
			return client_socket;
		}
		static void modify_client_socket(int v){//overloaded static function to return client_socket value
			client_socket=v;
		}
		static bool modify_exit_flag(){			//overloaded static function to update exit_flag
			return exit_flag;
		}
		static void modify_exit_flag(bool v){ 	//overloaded static function to return exit_flag value
			exit_flag=v;
		}
		//creates an IPv4 (AF_INET) TCP socket endpoint for the client (SOCK_STREAM => TCP)
		void connect_socket()
		{
			//returns socket descriptor if connection successful, else returns -1
			client::modify_client_socket(socket(AF_INET,SOCK_STREAM,0));
			if(client::modify_client_socket()==-1)
			{
				perror("socket: ");
				exit(-1);
			}
		}
		//connect is called by client to connect to the server at the specified port 
		void establish_connection()
		{
			if((connect(client::modify_client_socket(),(struct sockaddr *)&client_add,sizeof(struct sockaddr_in)))==-1)
			{
				perror("connect: ");
				exit(-1);
			}
		}
		void new_user()
		{
			cout<<"Enter your name : ";
			cin.getline(name,MAX_LEN);			//input the name of the client (user)
			//send client_socket and name of the client to server
			send(client::modify_client_socket(),name,sizeof(name),0);
			cout<<colors[NUM_COLORS-1]<<"\n\t  ====== Welcome to the chat-room ======   "<<endl<<def_col;
		}
		// Send message to all clients via the server
		static void send_message(int client_socket)
		{
			while(1) //we use infinite loops as sending and receiving must take place untill ctrl+c is pressed
			{
				cout<<colors[1]<<"You : "<<def_col;
				char str[MAX_LEN];
				cin.getline(str,MAX_LEN);				//getting message to be transmitted 
				send(client_socket,str,sizeof(str),0);	//send message to the server
				//To diconnect the client and exit chatroom
				if(strcmp(str,"#exit")==0)
				{
					client::modify_exit_flag(true);		//set exit_flag to true
					t_recv.detach();	
					close(client_socket);				//close the socket connection with server
					return;
				}	
			}		
		}

		// Receive message from the server
		static void recv_message(int client_socket)
		{
			char back_space=8;
			while(1)	//we use infinite loops as sending and receiving must take place untill ctrl+c is pressed
			{
				if(client::modify_exit_flag())	//if exit_flag is set to true, then return
					return;
				char name[MAX_LEN], str[MAX_LEN];
				int color_code;
				int bytes_received=recv(client_socket,name,sizeof(name),0);	//name of sender received from the server
				if(bytes_received<=0)			//if no data received then continue 
					continue;
				recv(client_socket,&color_code,sizeof(color_code),0);	//receive color of the sender from server
				recv(client_socket,str,sizeof(str),0);					//receive message of the sender from the server
				for(int i=0; i<6; i++)
					cout<<back_space;		
				//print the received message along with the name of the sender 				
				if(strcmp(name,"#NULL")!=0)	
					cout<<colors[color_code%NUM_COLORS]<<name<<" : "<<def_col<<str<<endl;
				else
					cout<<colors[color_code%NUM_COLORS]<<str<<endl;
				cout<<colors[1]<<"You : "<<def_col;
				fflush(stdout);
			}	
		}
};
int client::client_socket=0;
bool client::exit_flag=false;

// Handler for "Ctrl + C" to close the client chat
void catch_ctrl_c(int signal) 
{
	char str[MAX_LEN]="#exit";
	//transmit exit message if user exits chatroom
	send(client::modify_client_socket(),str,sizeof(str),0);
	t_send.detach();
	t_recv.detach();
	close(client::modify_client_socket());				//close the socket connection with server
	exit(signal);
}

int main()
{
	client *user=new client;			//creating object pointer of class client 
	user->connect_socket();
	user->establish_connection();
	signal(SIGINT, catch_ctrl_c);
	user->new_user();

	thread t1(client::send_message, client::modify_client_socket());	//passing send_message function as parameter to a thread
	thread t2(client::recv_message, client::modify_client_socket());	//passing recv_message function as parameter to a thread

	//assigning the above threads to the globally initialised threads
	t_send=move(t1);
	t_recv=move(t2);

	//join method allows one thread to wait for the completion of other threads
	if(t_send.joinable())
		t_send.join();
	if(t_recv.joinable())
		t_recv.join();
	return 0;
}



