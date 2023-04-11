#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <stdio.h>
#include <string>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <queue>
#include <netdb.h>
#include <sys/fcntl.h>
#include <fstream>
#include <sstream>
#include <random>
#include <climits>
#include <errno.h>
#include <ctime>
#include "duckchat.h"

using namespace std;
using std::string;
using std::set;



class User;
class Channel;
class Server;

class User{
public:
    char *username;
    unsigned long user_ip;
    unsigned short user_port;
    struct sockaddr_in *user_addr;
    set<Channel *> user_channels;
    char *active_channel;

    User(char *name,struct sockaddr_in *addr){
        user_addr=new struct sockaddr_in;
        username=new char[USERNAME_MAX];
        active_channel=new char[CHANNEL_MAX];

        strcpy(username,name);
        *(user_addr) = *addr;
        user_ip=(addr->sin_addr).s_addr;
        user_port=addr->sin_port;
    };

    ~User();
};

class Channel{
public:
    char *channel_name;
    set<User *> channel_users;
    set<Server *> tree;

    Channel(char *name){
        channel_name=new char[CHANNEL_MAX];
        strcpy(channel_name,name);
    };

    ~Channel();
};

class Server{
public:

    unsigned long server_ip;
    unsigned short server_port;
    char sip[UNIX_PATH_MAX];
    char sport[6];
    struct sockaddr_in *server_addr;
    int server_socket;
    // set<Channel *>server_channels;

    Server(struct sockaddr_in *addr,int socket,char *ip,char *port){
        server_addr=new struct sockaddr_in;
        
        server_socket=socket;
        *(server_addr) = *addr;
        server_ip=(addr->sin_addr).s_addr;
        server_port=addr->sin_port;
        strcpy(sip,ip);
        strcpy(sport,port);
    };

    ~Server();
};

set<long long> uids;
set<User *> users;
set<Channel *> channels;
set<Server *> adj_servers;
Server *this_server;

unsigned int sigtime=60;
int soft_join_status=0;

void d(int n){
    cout<<"d"<<n<<endl;
}

void pa(){
    set<Server *>::iterator it;
    cout<<"join :: "<<this_server->server_port<<" : ";
    for(it=adj_servers.begin();it!=adj_servers.end();it++){
        cout<<(*it)->server_port<<" ";
    }
    cout<<endl;
}

void pt(char *ch){
    set<Channel *>::iterator iter;
    for(iter=channels.begin();iter!=channels.end();iter++){
        if(strcmp((*iter)->channel_name,ch)==0){
            set<Server *>::iterator it;
            cout<<"say :: "<<this_server->server_port<<" : ";
            for(it=(*iter)->tree.begin();it!=(*iter)->tree.end();it++){
                cout<<(*it)->server_port<<" ";
            }
            cout<<endl;
            break;
        }
    }
    
}

long long get_rand(){
    long long id=0ll;
    int fd=open("/dev/urandom", O_RDONLY);
    read(fd,&id,8);
    close(fd);
    return id;
}

// int getsockfd(Server *server){
    
//         struct addrinfo hints, *server_info, *result;
//         hints.ai_family=AF_INET;
//         hints.ai_socktype=SOCK_DGRAM;
//         hints.ai_protocol=0;

//         int info_status;
//         int sockfd;

//         if((info_status=getaddrinfo(server->sip,server->sport,&hints,&result))!=0){
//             printf("Can't get adj server address info\n");
//             exit(EXIT_FAILURE);
//         }

//         for(server_info=result;server_info!=NULL;server_info=server_info->ai_next){
//             if((sockfd=socket(server_info->ai_family,server_info->ai_socktype,server_info->ai_protocol))>=0){
//                 break;
            
//             }
//         }
   
//         if(server_info==NULL){
//             printf("Unable to get adj socket\n");
//             exit(EXIT_FAILURE);
//         }

//         return sockfd;
// }

void soft_join(int signum){
    soft_join_status=1;
    (void)signum;

    set<Channel *>::iterator iter;
    for(iter=channels.begin();iter!=channels.end();iter++){
        set<Server *>::iterator it;
        for(it=(*iter)->tree.begin();it!=(*iter)->tree.end();it++){
            struct request_s2s_join s2s_join;
            s2s_join.req_type=htonl(REQ_S2S_JOIN);
            strcpy(s2s_join.req_channel,(*iter)->channel_name);

            int send_status;
            if((send_status=sendto((*it)->server_socket,&s2s_join,sizeof(s2s_join),0,(struct sockaddr *)((*it)->server_addr),sizeof((*(*it)->server_addr))))<0){
                printf("Can't soft join\n");
                
            }
            
        }
    }
    soft_join_status=0;
    cout<<"soft join sending completed"<<endl;

    signal(SIGALRM,&soft_join);
    alarm(sigtime);

    
}

int s2s_join_broadcast(request_s2s_join s2s_join){
    //get join info
    //struct request_join *join_req=(struct request_join *)input;
    //struct request_s2s_join s2s_join;
    //s2s_join.req_type=htonl(REQ_S2S_JOIN);
    //strcpy(s2s_join.req_channel,join_req->req_channel);
    //find channel
    set<Channel *>::iterator it;
    for(it=channels.begin();it!=channels.end();it++){
        if(strcmp(s2s_join.req_channel,(*it)->channel_name)==0){
            break;
        }
    }
    //send join req to adj servers
    set<Server *>::iterator iter;
    for(iter=adj_servers.begin();iter!=adj_servers.end();iter++){
        int send_status;
        if((send_status=sendto((*iter)->server_socket,&s2s_join,sizeof(s2s_join),0,(struct sockaddr *)((*iter)->server_addr),sizeof((*(*iter)->server_addr))))<0){
            printf("Can't send adj join\n");
            return -1;
        }else{
            //add channel to adj server
            //check duplicate
      
            // (*iter)->server_channels.insert(*it);
 
        }
    }
    
    //add adj server to tree
    set<Server *>::iterator itt;
    for(itt=adj_servers.begin();itt!=adj_servers.end();itt++){
        (*it)->tree.insert(*itt);
    }
    //add this server to tree
    // this_server->server_channels.insert(*it);
    (*it)->tree.insert(this_server);
    //pa();
    //cout<<"broadcasting joining completed"<<endl;
    return 0;
}



int s2s_leave_send(char *req_channel,sockaddr_in *addr){
    //struct request_leave *leave_req=(struct request_leave *)input;
    struct request_s2s_leave s2s_leave;
    s2s_leave.req_type=htonl(REQ_S2S_LEAVE);
    strcpy(s2s_leave.req_channel,req_channel);

    set<Channel *>::iterator iter;
    for(iter=channels.begin();iter!=channels.end();iter++){
        if(strcmp(req_channel,(*iter)->channel_name)==0){
            set<Server *>::iterator it;
            for(it=(*iter)->tree.begin();it!=(*iter)->tree.end();it++){
                if( ((*it)->server_ip==(addr->sin_addr).s_addr) && ((*it)->server_port==addr->sin_port)){
                    int send_status;
                    if((send_status=sendto((*it)->server_socket,&s2s_leave,sizeof(s2s_leave),0,(struct sockaddr *)((*it)->server_addr),sizeof((*(*it)->server_addr))))<0){
                        printf("Can't send adj leave\n");
                        return -1;
                    }
                    //delete server from channel tree
                    //(*iter)->tree.erase(it);
                    // (*it)->server_channels.erase(iter);
                    break;
                }
            }
        }
    }

    //cout<<"sending s2s leave completed"<<endl;
    return 0;
}

int s2s_say_send(char *input, sockaddr_in *client_addr){
    struct request_say *say_req=(struct request_say *)input;
    struct request_s2s_say s2s_say;
    s2s_say.req_type=htonl(REQ_S2S_SAY);
    strcpy(s2s_say.req_channel,say_req->req_channel);
    strcpy(s2s_say.req_text,say_req->req_text);
    //get username
    set<User *>::iterator iter;
    for(iter=users.begin();iter!=users.end();iter++){
        if(((*iter)->user_ip==(client_addr->sin_addr).s_addr)and((*iter)->user_port==client_addr->sin_port)){
            strcpy(s2s_say.req_username,(*iter)->username);
            break;
        }
    }
    //get unique id
    long long id=get_rand();
    uids.insert(id);
    s2s_say.unique_id=id;
    //forward to all server subs to channel
    set<Channel *>::iterator itt;
    for(itt=channels.begin();itt!=channels.end();itt++){
        if(strcmp((*itt)->channel_name,say_req->req_channel)==0){
            set<Server *>::iterator it;
            for(it=(*itt)->tree.begin();it!=(*itt)->tree.end();it++){
                if((*it)->server_ip==this_server->server_ip && (*it)->server_port==this_server->server_port){
                    ;
                }else{
                    int send_status;
                    
                    if((send_status=sendto((*it)->server_socket,&s2s_say,sizeof(s2s_say),0,(struct sockaddr *)((*it)->server_addr),sizeof((*(*it)->server_addr))))<0){
                        printf("Can't send adj say\n");
                        return -1;
                    }else{
                        cout<<"say :: "<<this_server->server_port<<"->"<<(*it)->server_port<<" "<<endl;
                    }
                }
                
            }
        }
    }
    //pt(say_req->req_channel);
    //cout<<"s2s say sending completed"<<endl;
    return 0;
}


int s2s_join_req(char *input){
    struct request_s2s_join *join_req=(struct request_s2s_join *)input;
    //check if server is subs to channel
    int joined=0;
    int exist=0;
    set<Channel *>::iterator iter;
    for(iter=channels.begin();iter!=channels.end();iter++){
        if(strcmp((*iter)->channel_name,join_req->req_channel)==0){
            exist=1;
            set<Server *>::iterator itt;
            for(itt=(*iter)->tree.begin();itt!=(*iter)->tree.end();itt++){
                if(((*itt)->server_ip==this_server->server_ip) && ((*itt)->server_port==this_server->server_port)){
                    joined=1;
                    break;
                }
            }
            break;
        }
    }
    //create channel if not exist
    if(!exist){
        Channel *new_channel=new Channel(join_req->req_channel);
        channels.insert(new_channel);
        for(iter=channels.begin();iter!=channels.end();iter++){
            if(strcmp((*iter)->channel_name,join_req->req_channel)==0){
                break;
            }
        }    
    }
    //if not subs, forward to all adjs
    if(!joined){
        //send join req to adj servers
        set<Server *>::iterator ittt;
        for(ittt=adj_servers.begin();ittt!=adj_servers.end();ittt++){
            int send_status;
            if((send_status=sendto((*ittt)->server_socket,join_req,sizeof(*join_req),0,(struct sockaddr *)((*ittt)->server_addr),sizeof((*(*ittt)->server_addr))))<0){
                printf("Can't send adj join\n");
                
                return -1;
            }else{
                //add channel to adj server
                //check duplicate
      
                // (*iter)->server_channels.insert(*it);
                //cout<<this_server->server_port<<" send s2s join to "<<(*ittt)->server_port<<endl;
            }
        }

        //add adj server to tree
        set<Server *>::iterator itt;
        for(itt=adj_servers.begin();itt!=adj_servers.end();itt++){
            (*iter)->tree.insert(*itt);
        }
        //add this server to tree
        // this_server->server_channels.insert(*it);
        (*iter)->tree.insert(this_server);

    }
    //pa();
    //cout<<this_server->server_port<<" got s2s join req"<<endl;
    
    return 0;
}

int s2s_leave_req(char *input, sockaddr_in *addr){
    struct request_s2s_leave *leave_req=(struct request_s2s_leave *)input;
    //find channel
    set<Channel *>::iterator iter;
    for(iter=channels.begin();iter!=channels.end();iter++){
        if(strcmp((*iter)->channel_name,leave_req->req_channel)==0){
            set<Server *>::iterator itt;
            for(itt=(*iter)->tree.begin();itt!=(*iter)->tree.end();itt++){
                if(((*itt)->server_ip==(addr->sin_addr).s_addr) && ((*itt)->server_port==addr->sin_port)){
                    cout<<(*itt)->server_port<<"got deleted"<<endl;
                    (*iter)->tree.erase(itt);
                    break;
                }
            }
            break;
        }
    }
    //cout<<"s2s leave req completed"<<endl;
    return 0;
}

int s2s_say_req(int sockfd,char *input,sockaddr_in *addr){
    //check uid, if loop exist send leave to sender + delete sender from tree, if not add id to uid
    struct request_s2s_say *say_req=(struct request_s2s_say *)input;
    int loop=0;

    set<Channel *>::iterator iter;
    for(iter=channels.begin();iter!=channels.end();iter++){
        if(strcmp((*iter)->channel_name,say_req->req_channel)==0){
            break;
        }
    }

    set<long long>::iterator it;
    for(it=uids.begin();it!=uids.end();it++){
        if((*it)==say_req->unique_id){
            loop=1;
            
            s2s_leave_send(say_req->req_channel,addr);
            //delete sender from tree
            
            set<Server *>::iterator itter;
            for(itter=(*iter)->tree.begin();itter!=(*iter)->tree.end();itter++){
            if(((*itter)->server_ip==(addr->sin_addr).s_addr) && ((*itter)->server_port==addr->sin_port)){
                (*iter)->tree.erase(itter);
                break;
                }
            }
            break;
             
        }else{
            //uids.insert(say_req->unique_id);
        }
    }

    uids.insert(say_req->unique_id);
  
    //check leaf, if can't forward say / no user, at most sender as adj server, send leave to sender + delete sender from tree
    //no user? one adj=sender?
    int leaf=0;
    if((*iter)->channel_users.size()==0 && (*iter)->tree.size()==1){
        leaf=1;
        //send leave to sender
       
        s2s_leave_send(say_req->req_channel,addr);
        //delete sender from tree
        
        set<Server *>::iterator itt;
        for(itt=(*iter)->tree.begin();itt!=(*iter)->tree.end();itt++){
            if(((*itt)->server_ip==(addr->sin_addr).s_addr) && ((*itt)->server_port==addr->sin_port)){
                (*iter)->tree.erase(itt);
                break;
            }
        }

    }
  
    //send say to channel users
    if(!loop){
        struct text_say say_pak;
        bzero(&say_pak,sizeof(say_pak));
        say_pak.txt_type=htonl(TXT_SAY);
        strcpy(say_pak.txt_username,say_req->req_username);
        strcpy(say_pak.txt_channel,say_req->req_channel);
        strcpy(say_pak.txt_text,say_req->req_text);
        set<Channel *>::iterator ittt;
        for(ittt=channels.begin();ittt!=channels.end();ittt++){
            if(strcmp((*ittt)->channel_name,say_req->req_channel)==0){
                set<User *>::iterator ittter;
                for(ittter=(*ittt)->channel_users.begin();ittter!=(*ittt)->channel_users.end();ittter++){
                    sendto(sockfd,&say_pak,sizeof(say_pak),0,(struct sockaddr *)((*ittter)->user_addr),sizeof(*((*ittter)->user_addr)));
                }
            }
        }

        set<Server *>::iterator iterr;
        for(iterr=(*iter)->tree.begin();iterr!=(*iter)->tree.end();iterr++){
            if(((*iterr)->server_ip==this_server->server_ip && (*iterr)->server_port==this_server->server_port) || ((*iterr)->server_ip==(addr->sin_addr).s_addr && (*iterr)->server_port==addr->sin_port)){
                ;
            }else{
            
                int send_status;
                if((send_status=sendto((*iterr)->server_socket,say_req,sizeof(*say_req),0,(struct sockaddr *)((*iterr)->server_addr),sizeof((*(*iterr)->server_addr))))<0){
                    printf("Can't send adj say\n");
                    return -1;
                }else{
                    cout<<"say :: "<<this_server->server_port<<"->"<<(*iterr)->server_port<<" "<<endl;
                }
            }
        }

    }
    //forward to all server subs to channel
    // if((*iter)->tree.size()==0){
    //     cout<<this_server->server_port<<"has no way to forward say"<<endl;
    // }
    
        
    //pt(say_req->req_channel);
    //cout<<this_server->server_port<<" got s2s say req"<<endl;
   
    return 0;
}


int server_login(char *input, struct sockaddr_in *client_sockaddr){
    struct request_login *login_req=(struct request_login *)input;
    // if(sizeof(login_req->req_username)>USERNAME_MAX){
    //     printf("Oversized username\n");
    //     return -1;
    // }
    User *user=new User(login_req->req_username,client_sockaddr);
    // bzero(user,sizeof(*user));
    
    users.insert(user);
    
    printf("login:new user created\n");
    
    return 0;

}

int server_join(char *input, struct sockaddr_in *client_sockaddr){
    //check if server subs to channel, yes: done no: broadcase to adj
    struct request_join *join_req=(struct request_join *)input;
    

    // if(sizeof(join_req->req_channel)>CHANNEL_MAX){
    //     printf("Oversized channel name\n");
    //     return -1;
    // }
    int joined=0;
    // int user_c=0;
    char *username=new char[USERNAME_MAX];
    //User *user_temp=(User *)malloc(sizeof(User));
    //bzero(user_temp,sizeof(*user_temp));
    bzero(username,USERNAME_MAX);

    int exist=0;
    set<Channel *>::iterator it;
    for(it=channels.begin();it!=channels.end();it++){
        if(strcmp((*it)->channel_name,join_req->req_channel)==0){
            exist=1;  
            // if(!joined){
            //     (channels[i]->channel_users).insert(*iter);
               
            // }
            break;
        }
    }

    

    set<User *>::iterator iter;
    for(iter=users.begin();iter!=users.end();iter++){
        if(((*iter)->user_ip==(client_sockaddr->sin_addr).s_addr)and((*iter)->user_port==client_sockaddr->sin_port)){
        
            
            
            strcpy(username,(*iter)->username);
            strcpy((*iter)->active_channel,join_req->req_channel);
            set<Channel *>::iterator itt;
            for(itt=(*iter)->user_channels.begin();itt!=(*iter)->user_channels.end();itt++){
                if(strcmp((*itt)->channel_name,join_req->req_channel)==0){
                    joined=1;
                }
            }
            if(!joined && exist){
                (*iter)->user_channels.insert(*it);
                (*it)->channel_users.insert(*iter);
            }else if(!joined &&!exist){
                Channel *new_channel=new Channel(join_req->req_channel);
                
                (*iter)->user_channels.insert(new_channel);
                new_channel->channel_users.insert(*iter);
                channels.insert(new_channel);
            }
            // user_c=1;
            break;
        }
    }
    // printf("join req user checked\n");
    // if(!user_c){
    //     printf("User doesn't exist\n");
    //     return -1;
    // }

    struct request_s2s_join s2s_join;
    s2s_join.req_type=htonl(REQ_S2S_JOIN);
    strcpy(s2s_join.req_channel,join_req->req_channel);

    int sjoined=0;
    set<Channel*>::iterator siter;
    for(siter=channels.begin();siter!=channels.end();siter++){
        if(strcmp((*siter)->channel_name,join_req->req_channel)==0){
            set<Server *>::iterator sitt;
            for(sitt=(*siter)->tree.begin();sitt!=(*siter)->tree.end();sitt++){
                if(((*sitt)->server_ip==this_server->server_ip) && ((*sitt)->server_port==this_server->server_port)){
                    sjoined=1;
                    break;
                }
            }
            break;
        }
    }

    if(!sjoined){
        s2s_join_broadcast(s2s_join);
    }

    //printf("join:user joined successed\n");
     
    
    return 0;

}

int server_leave(char *input, struct sockaddr_in *client_sockaddr){
    struct request_leave *leave_req=(struct request_leave *)input;
    // if(sizeof(leave_req->req_channel)>CHANNEL_MAX){
    //     printf("Oversized channel name\n");
    //     return -1;
    // }
    //int joined=0;
    char *username=new char[USERNAME_MAX];
    // User *user_temp=(User *)malloc(sizeof(User));
    // bzero(user_temp,sizeof(user_temp));
    
    //bzero(username,sizeof(*username));
    set<User *>::iterator iter;
    for(iter=users.begin();iter!=users.end();iter++){
        if(((*iter)->user_ip==(client_sockaddr->sin_addr).s_addr)and((*iter)->user_port==client_sockaddr->sin_port)){
            // user_temp=users[i];
            //cout<<"leave:user founded"<<endl;
            strcpy(username,(*iter)->username);
            
            if(strcmp((*iter)->active_channel,leave_req->req_channel)==0){
                bzero(((*iter)->active_channel),CHANNEL_MAX);
                strcpy((*iter)->active_channel,"Common");
            }
            set<Channel *>::iterator ittt;
            for(ittt=(*iter)->user_channels.begin();ittt!=(*iter)->user_channels.end();ittt++){
                if(strcmp((*ittt)->channel_name,leave_req->req_channel)==0){
                    //joined=1;

                    set<User *>::iterator it;
                    for(it=(*ittt)->channel_users.begin();it!=(*ittt)->channel_users.end();it++){
                
                        if(strcmp((*it)->username,username)==0){
                    
                            (*ittt)->channel_users.erase(it);
                   
                            break;
                        }
                    }
                    
                    (*iter)->user_channels.erase(ittt);
                    
                    break;
                }
            }
            // if(joined){
            //     set<string>::iterator it;
            //     for(it=(*iter)->user_channels.begin();it!=(*iter)->user_channels.end();it++){
            //         if(strcmp((*it).c_str(),leave_req->req_channel)==0){
            //             (*iter)->user_channels.erase(it);
            //             break;
            //         }
            //     }
                
            // }
            
            break;
        }
    }
    
    set<Channel *>::iterator itt;
    for(itt=channels.begin();itt!=channels.end();itt++){
        if((*itt)->channel_users.size()==0){
            channels.erase(itt);
            if(channels.begin()==channels.end()){
                break;
            }else{
                itt=channels.begin();   
            }
            
        }
    }
    
    //cout<<"leave:user leave successed"<<endl;
    
    return 0;
}

int server_logout(struct sockaddr_in *client_sockaddr){
    char *username=new char[USERNAME_MAX];
    //bzero(username,sizeof(*username));
    set<User *>::iterator iter;
    for(iter=users.begin();iter!=users.end();iter++){
        if(((*iter)->user_ip==(client_sockaddr->sin_addr).s_addr)and((*iter)->user_port==client_sockaddr->sin_port)){
            strcpy(username,(*iter)->username);


            set<Channel *>::iterator itt;
            for(itt=(*iter)->user_channels.begin();itt!=(*iter)->user_channels.end();itt++){
       
                set<User *>::iterator it;
                for(it=(*itt)->channel_users.begin();it!=(*itt)->channel_users.end();it++){
                
                    if(strcmp((*it)->username,username)==0){
                    
                        (*itt)->channel_users.erase(it);
                
                        // if((*itt)->channel_users.size()==0){
                        //     channels.erase(itt);
                        //     itt=channels.begin();
                        // }
                        break;
                    }
                }
            }


            users.erase(iter);
            break;
        }
    }
    
    set<Channel *>::iterator ittt;
    for(ittt=channels.begin();ittt!=channels.end();ittt++){
        if((*ittt)->channel_users.size()==0){
            channels.erase(ittt);
            if(channels.begin()==channels.end()){
                break;
            }else{
                ittt=channels.begin();   
            }
        }
    }

    //for every channel contain user delete user
    // for(unsigned int i=0;i<channels.size();i++){
    //     set<User *>::iterator it;
    //     for(it=channels[i]->channel_users.begin();it!=channels[i]->channel_users.end();it++){
        
    //         if(strcmp((*it)->username,username)==0){
               
    //             channels[i]->channel_users.erase(it);
    //             break;
    //         }
    //     }
    // }
   
    // set<Channel *>::iterator ittt;
    // for(ittt=channels.begin();ittt!=channels.end();ittt++){
        
    // }
    //cout<<"logout:user logout successed"<<endl;
    
    return 0;
}

int server_say(int sockfd, char *input, struct sockaddr_in *client_sockaddr){
    struct request_say *say_req=(struct request_say *)input;
    struct text_say say_pak;
    bzero(&say_pak,sizeof(say_pak));
    say_pak.txt_type=htonl(TXT_SAY);
    set<User *>::iterator iter;
    for(iter=users.begin();iter!=users.end();iter++){
        if(((*iter)->user_ip==(client_sockaddr->sin_addr).s_addr)and((*iter)->user_port==client_sockaddr->sin_port)){
            strcpy(say_pak.txt_username,(*iter)->username);
            strcpy(say_pak.txt_channel,say_req->req_channel);
            strcpy(say_pak.txt_text,say_req->req_text);
            set<Channel *>::iterator itt;
            for(itt=channels.begin();itt!=channels.end();itt++){
                if(strcmp((*iter)->active_channel,(*itt)->channel_name)==0){
                    set<User *>::iterator it;
                    for(it=(*itt)->channel_users.begin();it!=(*itt)->channel_users.end();it++){

                        sendto(sockfd,&say_pak,sizeof(say_pak),0,(struct sockaddr *)((*it)->user_addr),sizeof(*((*it)->user_addr)));
                    }

                }
            }
        }
    }
    
    s2s_say_send(input, client_sockaddr);
    
    return 0;


}

int server_list(int sockfd, struct sockaddr_in *client_sockaddr){
    // struct text_list *list_pak=new text_list[(sizeof(text_list)+channels.size()*sizeof(channel_info))];
    size_t size=sizeof(text_list)+channels.size()*sizeof(channel_info);
    struct text_list *list_pak=(text_list *)malloc(size);
  
    bzero(list_pak,size);
    list_pak->txt_nchannels=htonl(channels.size());
    list_pak->txt_type=htonl(TXT_LIST);
    unsigned int i=0;
    set<Channel *>::iterator iter;
    for(iter=channels.begin();iter!=channels.end();iter++){
        strcpy(list_pak->txt_channels[i].ch_channel,(*iter)->channel_name);
        i++;
    }
    int send_status;
    if((send_status=sendto(sockfd,list_pak,size,0,(struct sockaddr *)client_sockaddr,sizeof(*client_sockaddr)))<0){
        printf("Can't send list\n");
        return -1;
    }
    return 0;
    

}

int server_who(int sockfd,char *input,struct sockaddr_in *client_sockaddr){
    struct request_who *who_req=(struct request_who *)input;
    set<Channel *>::iterator iter;
    for(iter=channels.begin();iter!=channels.end();iter++){
        if(strcmp(who_req->req_channel,(*iter)->channel_name)==0){
            //struct text_who *who_pak=new text_who[(sizeof(text_who)+sizeof(user_info)*(((channels[i])->channel_users).size()))];
            size_t size=sizeof(text_who)+sizeof(user_info)*(((*iter)->channel_users).size());
            struct text_who *who_pak=(text_who *)malloc(size);
            bzero(who_pak,size);
            who_pak->txt_nusernames=htonl(((*iter)->channel_users).size());
            strcpy(who_pak->txt_channel,who_req->req_channel);
            who_pak->txt_type=htonl(TXT_WHO);
            set<User *>::iterator itt;
            unsigned int i=0;
            for(itt=(*iter)->channel_users.begin();itt!=(*iter)->channel_users.end();itt++){
                strcpy(who_pak->txt_users[i].us_username,(*itt)->username);
                i++;
            }

            int send_status;
            if((send_status=sendto(sockfd,who_pak,size,0,(struct sockaddr *)client_sockaddr,sizeof(*client_sockaddr)))<0){
                printf("Can't send who\n");
                return -1;
            }
        }
    }
    
    return 0;

}

int server_keepalive(){

    return 0;

}

int server_error(){
    
    return 0;

}

// template<typename T> 
// set<T *>::iterator get_iter(set<T *>& vec, char *key){
//     set<T *>::iterator it;
//     for(it=vec.begin();it!=vec.end();it++){
//         if(strcmp(key,(*it)->channel_name)==0){
//             break;
//         }
//     }
// }

int add_server(int argc, char*argv[]){
    for(int i=3;i<argc;i+=2){
        char *server_domain=argv[i];
        char *port_num=argv[i+1];

        //cout<<server_domain<<" : "<<port_num<<endl;
    
        if(strlen(server_domain)>UNIX_PATH_MAX){
            printf("Oversized adj server name\n");
            exit(EXIT_FAILURE);
        }

        struct addrinfo hints, *server_info, *result;
        hints.ai_family=AF_INET;
        hints.ai_socktype=SOCK_DGRAM;
        hints.ai_protocol=0;

        int info_status;
        int sockfd;

        if((info_status=getaddrinfo(server_domain,port_num,&hints,&result))!=0){
            printf("Didn't get adj server address info\n");
            exit(EXIT_FAILURE);
        }

        for(server_info=result;server_info!=NULL;server_info=server_info->ai_next){
            if((sockfd=socket(server_info->ai_family,server_info->ai_socktype,server_info->ai_protocol))>=0){
                break;
            
            }
        }
   
        if(server_info==NULL){
            printf("Unable to get adj socket\n");
            exit(EXIT_FAILURE);
        }

        Server *new_server = new Server((struct sockaddr_in *)server_info->ai_addr,sockfd,server_domain,port_num);
        
        adj_servers.insert(new_server);
        
    }
    return 0;
}



int main(int argc,char *argv[]){
    if (argc < 5){
        printf("Wrong usage\n");
        exit(EXIT_FAILURE);
    }
    char *server_domain=argv[1];
    char *port_num=argv[2];
    
    if(strlen(server_domain)>UNIX_PATH_MAX){
        printf("Oversized server name\n");
        exit(EXIT_FAILURE);
    }
    // struct sockaddr_in server_addr;
    // bzero(&server_addr,sizeof(server_addr));
    // server_addr.sin_family=AF_INET;
    // server_addr.sin_port=htonl(port_num);
    // server_addr.sin_addr.s_addr=htol(INADDR_ANY);
    struct addrinfo hints, *server_info, *result;
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

    int bind_status;
    if((bind_status=bind(sockfd,server_info->ai_addr,server_info->ai_addrlen))<0){
        printf("Unable to bind\n");
        exit(EXIT_FAILURE);
    }
    add_server(argc,argv);
    this_server=new Server((struct sockaddr_in *)server_info->ai_addr,sockfd,server_domain,port_num);
    
    
    signal(SIGALRM,&soft_join);
    alarm(sigtime);
    
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    char input[BUFSIZ];
    struct sockaddr_in client_sockaddr;
    while(1){

        

        bzero(input,sizeof(input));
        int recv_status;
        socklen_t addr_len = sizeof(client_sockaddr);
        if((recv_status=recvfrom(sockfd,input,sizeof(input),0,(struct sockaddr *)&client_sockaddr,&addr_len))<0){
            //printf("Error while receving data from client\n");
            
        }
       
        if(recv_status>0){
            struct text *client_pak=(struct text *)input;
            if(ntohl(client_pak->txt_type)==REQ_LOGIN){
            	printf("Got login req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    server_login(input,&client_sockaddr);
                }
            }else if(ntohl(client_pak->txt_type)==REQ_JOIN){
            	//printf("Got join req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    server_join(input,&client_sockaddr);
                }
            }else if(ntohl(client_pak->txt_type)==REQ_LOGOUT){
            	printf("Got logout req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    server_logout(&client_sockaddr);
                }
            }else if(ntohl(client_pak->txt_type)==REQ_LEAVE){
            	printf("Got leave req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    server_leave(input,&client_sockaddr);
                }
            }else if(ntohl(client_pak->txt_type)==REQ_SAY){
            	//printf("Got say req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    server_say(sockfd,input,&client_sockaddr);
                }
            }else if(ntohl(client_pak->txt_type)==REQ_LIST){
            	printf("Got list req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    server_list(sockfd,&client_sockaddr);
                }
            }else if(ntohl(client_pak->txt_type)==REQ_WHO){
            	printf("Got who req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    server_who(sockfd,input,&client_sockaddr);
                }
            }else if(ntohl(client_pak->txt_type)==REQ_KEEP_ALIVE){
            	printf("Got keepalive req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    server_keepalive();
                }
            }else if(ntohl(client_pak->txt_type)==REQ_S2S_JOIN){
            	//printf("Got s2s join req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    s2s_join_req(input);
                }
            }else if(ntohl(client_pak->txt_type)==REQ_S2S_LEAVE){
            	//printf("Got s2s leave req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    s2s_leave_req(input,&client_sockaddr);
                }
            }else if(ntohl(client_pak->txt_type)==REQ_S2S_SAY){
            	//printf("Got s2s say req\n");
                if(soft_join_status){
                    cout<<"Can't process command while sending soft join, please try again"<<endl;
                }else{
                    s2s_say_req(sockfd,input,&client_sockaddr);
                }
            }
        }
        
        //server for loop
        // char sinput[BUFSIZ];
        // struct sockaddr_in saddr;
        // int srecv_status;
        // set<Server *>::iterator iter;
        // for(iter=adj_servers.begin();iter!=adj_servers.end();iter++){
          
        //     bzero(sinput,sizeof(sinput));
            

        //     int sbind_status;
        //     if((sbind_status=bind((*iter)->server_socket,(struct sockaddr *)((*iter)->server_addr),sizeof((*iter)->server_addr)))<0){
        //         //printf("Unable to bind\n");
        //     }
            
        //     bzero(&saddr,sizeof(saddr));
           
            
        //     socklen_t addr_len = sizeof(saddr);
        //     if((srecv_status=recvfrom((*iter)->server_socket,sinput,sizeof(sinput),0,(struct sockaddr *)&saddr,&addr_len))<0){
        //         //printf("Error while receving data from client\n");
        //     }
         
        //     if(srecv_status>0){
        //         struct text *spak=(struct text *)sinput;
        //         if(ntohl(spak->txt_type)==REQ_S2S_JOIN){
        //     	    printf("Got s2s join req\n");
        //             s2s_join_req(sinput);
        //         }else if(ntohl(spak->txt_type)==REQ_S2S_LEAVE){
        //     	    printf("Got s2s leave req\n");
        //             s2s_leave_req(sinput,&saddr);
        //         }else if(ntohl(spak->txt_type)==REQ_S2S_SAY){
        //     	    printf("Got s2s say req\n");
        //             s2s_say_req((*iter)->server_socket,sinput,&saddr);
        //         }
        //     }

            
        // }
        

    }

    return 0;
}
