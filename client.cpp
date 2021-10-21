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
#define MAX_LEN 200
#define NUM_COLORS 6
using namespace std;

thread t_send, t_recv;
string def_col="\033[0m";
string colors[]={"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};

class client
{
	private:
		struct sockaddr_in client_add;
		char name[MAX_LEN];
		static int client_socket;
		static bool exit_flag;
	public:
		client()
		{
			client_add.sin_family=AF_INET;
			client_add.sin_port=htons(10000); // Port no. of server
			client_add.sin_addr.s_addr=INADDR_ANY;
			//client.sin_addr.s_addr=inet_addr("127.0.0.1"); // Provide IP address of server
			bzero(&client_add.sin_zero,0);
		}
		static int modify_client_socket(){
			return client_socket;
		}
		static void modify_client_socket(int val){
			client_socket=val;
		}
		static bool modify_exit_flag(){
			return exit_flag;
		}
		static void modify_exit_flag(bool val){
			exit_flag=val;
		}
		// Handler for "Ctrl + C"
		static void catch_ctrl_c(int signal) 
		{
			char str[MAX_LEN]="#exit";
			send(client::modify_client_socket(),str,sizeof(str),0);
			//exit_flag=true;
			t_send.detach();
			t_recv.detach();
			close(client::modify_client_socket());
			exit(signal);
		}
		void connect_socket()
		{
			client::modify_client_socket(socket(AF_INET,SOCK_STREAM,0));
			if(client::modify_client_socket()==-1)
			{
				perror("socket: ");
				exit(-1);
			}
		}
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
			cin.getline(name,MAX_LEN);
			send(client::modify_client_socket(),name,sizeof(name),0);

			cout<<colors[NUM_COLORS-1]<<"\n\t  ====== Welcome to the chat-room ======   "<<endl<<def_col;
		}
		// Send message to everyone
		static void send_message(int client_socket)
		{
			while(1)
			{
				cout<<colors[1]<<"You : "<<def_col;
				char str[MAX_LEN];
				cin.getline(str,MAX_LEN);
				send(client_socket,str,sizeof(str),0);
				if(strcmp(str,"#exit")==0)
				{
					client::modify_exit_flag(true);
					t_recv.detach();	
					close(client_socket);
					return;
				}	
			}		
		}

		// Receive message
		static void recv_message(int client_socket)
		{
			char back_space=8;
			while(1)
			{
				if(client::modify_exit_flag())
					return;
				char name[MAX_LEN], str[MAX_LEN];
				int color_code;
				int bytes_received=recv(client_socket,name,sizeof(name),0);
				if(bytes_received<=0)
					continue;
				recv(client_socket,&color_code,sizeof(color_code),0);
				recv(client_socket,str,sizeof(str),0);
				for(int i=0; i<6; i++)
					cout<<back_space;
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

int main()
{
	client *user=new client;
	user->connect_socket();
	user->establish_connection();
	signal(SIGINT, client::catch_ctrl_c);
	user->new_user();

	thread t1(client::send_message, client::modify_client_socket());
	thread t2(client::recv_message, client::modify_client_socket());

	t_send=move(t1);
	t_recv=move(t2);

	if(t_send.joinable())
		t_send.join();
	if(t_recv.joinable())
		t_recv.join();
	return 0;
}



