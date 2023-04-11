#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdbool.h>
#include <ctype.h>
#include <signal.h>
#include <set>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <vector>
#include "duckchat.h"
#include "raw.h"

using namespace std;
using std::string;
using std::vector;
using std::set;

vector<string>joined_channel;
char * active_channel=new char[CHANNEL_MAX];


int client_login(struct addrinfo *server_info, int sockfd, char *username){
    struct request_login login_pak;
    bzero(&login_pak,sizeof(login_pak));
    login_pak.req_type=htonl(REQ_LOGIN);
    strcpy(login_pak.req_username,username);
    int login_status;
    if((login_status=sendto(sockfd,&login_pak,sizeof(login_pak),0,server_info->ai_addr,server_info->ai_addrlen))<0){
        printf("Can't login\n");
        return -1;
    }
    return 0;
}

int client_join(struct addrinfo *server_info, int sockfd,char *channel_name){
    if(strlen(channel_name)>CHANNEL_MAX){
        printf("Oversized channel name\n");
        return -1;
    }
    int joined=0;
    vector<string>::iterator iter;
    for(iter=joined_channel.begin();iter!=joined_channel.end();iter++){
        if(strcmp(channel_name,(*iter).c_str())==0){
            joined=1;
            break;
        }
    }
    if(joined){
        bzero(active_channel,CHANNEL_MAX);
        strcpy(active_channel,channel_name);
    }else{
        string ch=string(channel_name);
        joined_channel.push_back(ch);
    }   

    struct request_join join;
    bzero(&join,sizeof(join));
    join.req_type=htonl(REQ_JOIN);
    strcpy(join.req_channel,channel_name);
    int join_status;
    if((join_status=sendto(sockfd,&join,sizeof(join),0,server_info->ai_addr,server_info->ai_addrlen))<0){
        printf("Can't join channel\n");
        return -1;
    }
    bzero(active_channel,CHANNEL_MAX);
    strcpy(active_channel,channel_name);
    return 0;
}

int client_leave(struct addrinfo *server_info, int sockfd,char *channel_name){
    if(strlen(channel_name)>CHANNEL_MAX){
        printf("Oversized channel name\n");
        return -1;
    }
    int joined=0;
    vector<string>::iterator iter;
    for(iter=joined_channel.begin();iter!=joined_channel.end();iter++){
        if(strcmp(channel_name,(*iter).c_str())==0){
            joined=1;
            break;
        }
    }
    if(joined){
        joined_channel.erase(iter);
        if(strcmp(channel_name,active_channel)==0){
            bzero(active_channel,CHANNEL_MAX);
            strcpy(active_channel,"Common");
        }
        struct request_leave leave;
        bzero(&leave, sizeof(leave));
        leave.req_type=htonl(REQ_LEAVE);
        strcpy(leave.req_channel,channel_name);

        int leave_status;
        if((leave_status=sendto(sockfd,&leave,sizeof(leave),0,server_info->ai_addr,server_info->ai_addrlen))<0){
            printf("Failed to leave channel\n");
            return -1;
        }
    }else{
        printf("\nChannel not joined\n");
        return -1;
    }   
    return 0;
}

int client_list(struct addrinfo *server_info, int sockfd){
    struct request_list list;
    bzero(&list,sizeof(list));
    list.req_type=htonl(REQ_LIST);
    
    int list_status;
    if((list_status=sendto(sockfd,&list,sizeof(list),0,server_info->ai_addr,server_info->ai_addrlen))<0){
            printf("Failed to list channel\n");
            return -1;
        }
    return 0;

}
                    
int client_who(struct addrinfo *server_info, int sockfd,char *channel_name){
    if(strlen(channel_name)>CHANNEL_MAX){
        printf("Oversized channel name\n");
        return -1;
    }
    
    struct request_who who;
    bzero(&who, sizeof(who));
    who.req_type=htonl(REQ_WHO);
    strcpy(who.req_channel,channel_name);

    int who_status;
    if((who_status=sendto(sockfd,&who,sizeof(who),0,server_info->ai_addr,server_info->ai_addrlen))<0){
        printf("Failed to get names\n");
        return -1;
    }
    
    return 0;
}
                 
int client_switch(struct addrinfo *server_info, int sockfd,char *channel_name){
    if(strlen(channel_name)>CHANNEL_MAX){
        printf("Oversized channel name\n");
        return -1;
    }
    int joined=0;
    for(unsigned int i=0;i<joined_channel.size();i++){
        if(strcmp(channel_name,joined_channel[i].c_str())==0){
            joined=1;
        }
    }
    if(joined){
        int join_status;
        if((join_status=client_join(server_info,sockfd,channel_name))<0){
            printf("Failed to switch channel\n");
            return -1;
        }
        bzero(active_channel,CHANNEL_MAX);
        strcpy(active_channel,channel_name);
        
    }else{
        printf("\nChannel not joined\n");
        return -1;
    }   
    return 0;
    // int join_status;
    // if((join_status=client_join(server_info,sockfd,channel_name))<0){
    //     printf("Failed to switch channel\n");
    //     return -1;
    // }
    // bzero(active_channel,sizeof(*active_channel));
    // strcpy(active_channel,channel_name);
    // return 0;

}

int client_exit(struct addrinfo *server_info, int sockfd){
    struct request_logout logout_pak;
    bzero(&logout_pak,sizeof(logout_pak));
    logout_pak.req_type=htonl(REQ_LOGOUT);
    int logout_status;
    if((logout_status=sendto(sockfd,&logout_pak,sizeof(logout_pak),0,server_info->ai_addr,server_info->ai_addrlen))<0){
        printf("Can't logout\n");
        return -1;
    }
    
    return 0;

}

int client_say(struct addrinfo *server_info, int sockfd,char *channel_name,char *message){
    if(strlen(channel_name)>CHANNEL_MAX){
        printf("Oversized channel name\n");
        return -1;
    }
    if(strlen(message)>SAY_MAX){
        printf("Oversized message");
        return -1;
    }
    if(strcmp(active_channel,channel_name)!=0){
        printf("Not active channel");
        return -1;
    }
    struct request_say say_pak;
    bzero(&say_pak,sizeof(say_pak));
    say_pak.req_type=htonl(REQ_SAY);
    strcpy(say_pak.req_channel,channel_name);
    strcpy(say_pak.req_text,message);
    
    int say_status;
    if((say_status=sendto(sockfd,&say_pak,sizeof(say_pak),0,server_info->ai_addr,server_info->ai_addrlen))<0){
        printf("Can't say message\n");
        return -1;
    }
    return 0;
}

int client_keepalive(struct addrinfo *server_info, int sockfd){
    struct request_keep_alive keepalive_pak;
    bzero(&keepalive_pak,sizeof(keepalive_pak));
    keepalive_pak.req_type=htonl(REQ_KEEP_ALIVE);
    int keepalive_status;
    if((keepalive_status=sendto(sockfd,&keepalive_pak,sizeof(keepalive_pak),0,server_info->ai_addr,server_info->ai_addrlen))<0){
        printf("Can't keep alive\n");
        return -1;
    }
    
    return 0;
}

int reply_who(char *reply){
    //cout<<"Got who reply"<<endl;
    struct text_who *reply_who=(struct text_who *)reply;
    printf("Listing all the users on %s:\n",reply_who->txt_channel);
    for(unsigned int i=0; i<ntohl(reply_who->txt_nusernames);i++){
        printf("%s\n",reply_who->txt_users[i].us_username);
    }
    return 0;
}

int reply_say(char *reply){
    
    struct text_say *reply_say=(struct text_say *)reply;
    
    printf("[%s][%s]:%s\n",reply_say->txt_channel,reply_say->txt_username,reply_say->txt_text);
    return 0;
}

int reply_list(char *reply){
    //cout<<"Got list reply"<<endl;
    struct text_list *reply_list=(struct text_list *)reply;
    printf("Listing all the channels:\n");
    for(unsigned int i=0; i<ntohl(reply_list->txt_nchannels);i++){
        printf("%s\n",reply_list->txt_channels[i].ch_channel);
    }
    return 0;
}

int reply_error(char *reply){
    struct text_error *reply_error=(struct text_error *)reply;
    printf("Error:%s\n",reply_error->txt_error);
    return 0;
}

int main(int argc,char *argv[]){
    if (argc != 4){
        printf("Wrong usage\n");
        exit(EXIT_FAILURE);
    }
    char *server_domain=argv[1];
    char *port_num=argv[2];
    char *username=argv[3];
    if(strlen(username)>USERNAME_MAX){
        printf("Oversized username\n");
        exit(EXIT_FAILURE);
    }
    if(strlen(server_domain)>UNIX_PATH_MAX){
        printf("Oversized server name\n");
        exit(EXIT_FAILURE);
    }
    if(atoi(port_num)<0 or atoi(port_num)>65535){
        printf("Overranged port number\n");
        exit(EXIT_FAILURE);
    }
    // struct sockaddr_in server_addr;
    // memset(&server_addr,'\0',sizeof(server_addr));
    // server_addr.sin_family=AF_INET;
    // server_addr.sin_port=htonl(port_num);
    // server_addr.sin_addr.s_addr=INADDR_ANY;
    struct addrinfo hints, *result,*server_info;
    bzero(&hints,sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;

    int info_status;
    int sockfd;
    if((info_status=getaddrinfo(server_domain,port_num,&hints,&result))!=0){
        printf("Can't get server address info\n");
        exit(EXIT_FAILURE);
    }
    for(server_info=result;server_info!=NULL;server_info=server_info->ai_next){
        if((sockfd=socket(server_info->ai_family,server_info->ai_socktype,server_info->ai_protocol))>=0){
            break;
        }
    }
    // close(sockfd);
    if(server_info==NULL){
        printf("Unable to get socket\n");
        exit(EXIT_FAILURE);
    }
    // string common="Common"
    // joined_channel.insert(common);
    char co[7]="Common";
    int status;
    if ((status=client_login(server_info,sockfd,username))<0){
    	printf("Error login user\n");
    }
    if ((status=client_join(server_info,sockfd,co))<0){
    	printf("Error join user\n");
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    fd_set readfd_set;
    int select_status;
    char input[SAY_MAX];
    char reply[BUFSIZ];
    int recv_status;
    bzero(input,sizeof(input));
    raw_mode();
    while(1){
	
        FD_ZERO(&readfd_set);
        FD_SET(sockfd,&readfd_set);
        FD_SET(STDIN_FILENO,&readfd_set);
    
        if((select_status=select(sockfd+1,&readfd_set,NULL,NULL,NULL))<0){
            printf("Can't select fd\n");
            exit(EXIT_FAILURE);
        }
        if(select_status>0){
            if(FD_ISSET(STDIN_FILENO,&readfd_set)){
                
                char c=getchar();
                
                if(c!='\n'){
                    cout<<c;
                    if(strlen(input)<SAY_MAX){
                        input[strlen(input)]=c;
                        
                    }

                }else{
                    input[strlen(input)]='\0';
                    //printf("%s",input);
                    //strtok string buffer
                    
                    char *rest;
                    rest=input;
                    strtok_r(rest," ",&rest);
                    //char *temp=strchr(rest,'\n');
                    //if(temp){
                    //    *temp=0;
                    //}
                    
		
                    if(input[0]=='/'){
                        if(strncmp(input,"/join",5)==0 && strlen(rest)!=0){
                           
                            client_join(server_info,sockfd,rest);
                            bzero(input,sizeof(input));
                            cout<<endl;
                            cout<<"> ";
                            
                            
                        }else if(strncmp(input,"/leave",6)==0 && strlen(rest)!=0){
                          
                            client_leave(server_info,sockfd,rest);
                            bzero(input,sizeof(input));
                            cout<<endl;
                            cout<<"> ";
                            
                            
                        }else if(strncmp(input,"/list",5)==0 && strlen(input)==5){
                            client_list(server_info,sockfd);
                            bzero(input,sizeof(input));
                            cout<<endl;
                            cout<<"> ";
                        }else if(strncmp(input,"/who",4)==0 && strlen(rest)!=0){
                            client_who(server_info,sockfd,rest);
                            bzero(input,sizeof(input));
                            cout<<endl;
                            cout<<"> ";
                        }else if(strncmp(input,"/switch",7)==0 && strlen(rest)!=0){
                          
                            client_switch(server_info,sockfd,rest);
                            bzero(input,sizeof(input));
                            cout<<endl;
                            cout<<"> ";
                            
                            
                        }else if(strncmp(input,"/exit",5)==0 && strlen(input)==5){
                            client_exit(server_info,sockfd);
                            cout<<endl;
                            cooked_mode();
                            exit(EXIT_SUCCESS);
                        }else{
                            bzero(input,sizeof(input));
                            cout<<"\nInvalid command"<<endl;
                            cout<<"> ";
                        }
                    }else{
                        
                        client_say(server_info,sockfd,active_channel,input);
                        bzero(input,sizeof(input));
                        cout<<endl;
                        cout<<"> ";
                    }
                    
                }

            }else if(FD_ISSET(sockfd,&readfd_set)){
                bzero(reply,sizeof(reply));
                if((recv_status=recvfrom(sockfd,reply,sizeof(reply),0,server_info->ai_addr,&(server_info->ai_addrlen)))<0){
                    printf("Error while receving data from server\n");
                }
                if(recv_status>0){
                    // reply=ntohl(reply);
                    struct text *reply_pak=(struct text *)reply;

                    if(ntohl(reply_pak->txt_type)==TXT_WHO){
                        reply_who(reply);
                        //cout<<"> ";
                    }else if(ntohl(reply_pak->txt_type)==TXT_SAY){
                        // string back="";
                        // for(unsigned int i=0;i<SAY_MAX;i++){
                        //     back.append("\b");
        
                        // }
                        // cout<<back;
                        reply_say(reply);
                        //cout<<"> ";
                    }else if(ntohl(reply_pak->txt_type)==TXT_LIST){
                        reply_list(reply);
                        //cout<<"> ";
                    }else if(ntohl(reply_pak->txt_type)==TXT_ERROR){
                        reply_error(reply);
                        //cout<<"> ";
                    }
                }
                


            }

        }
        

    }
    

    return 0;
}
