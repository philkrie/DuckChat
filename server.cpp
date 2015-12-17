#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <iostream>
#include <time.h>
#include <string>
#include <map>
#include <set>
#include "def"
#include "duckchat.hpp"
#include "linked_list.hpp"
#include <arpa/inet.h>


using namespace std;

//Method for validating a user is logged in
int user_validate(struct user_node* list, char* sa_data){
    struct user_node* user_it = list;
            if(user_it->next_user == NULL){
                return 0;
            } 
            user_it = user_it->next_user; //Move past Server base user
            while(user_it != NULL){
                 if(strcmp(user_it->user_address.sa_data, sa_data)==0){
                    return 1;
                 }
                 user_it = user_it->next_user;
            }
            return 0;
}

//Random number generator
void seed_randgenerator(){
    FILE *file = fopen("/dev/random", "r");
    char seed = getc(file);
    seed = seed * getpid();
    srand(seed);
}

//Retrieves a random integer (needs to be 64 bit)
long long get_rand(){
    long long r = rand();
    return r;
}

string get_name(struct sockaddr* addr){
    sockaddr_in* addr_in = (sockaddr_in*) addr;
    string ip = inet_ntoa(addr_in->sin_addr);
    int port = ntohs(addr_in->sin_port);
    char port_str[6];
    sprintf(port_str, "%d", port);
    string key = ip + ":" +port_str;
    return key;
}

//Check if our id queue has a repeat to eliminate loops.
bool set_check(long long id, set<long long>* id_set){
    set<long long>::iterator set_it;
    set_it = id_set->find(id);
    if(set_it != id_set->end()){
        //WE HAVE FOUND A DUPLICATE
        return true;
    } else {
        id_set->insert(id);
        return false;
    }
}

int main(int argc, char *argv[]) {
    (void) argc;
    int sd; //Variable for the socket
    struct sockaddr_in server; //Structure that specifies an address to connect the socket
    int rc;
    string server_name;

    typedef map<string, string> server_channels;

    map<string, server_channels> servers; //Including yourself!
    map<string, struct sockaddr_in> servers_addr; //Servers that you are connected to

    set<long long> id_set;

    if(argc <= 3){
        cout << "This program can also take more than two arguments" << endl;
    }

    if(argc%2 != 1){
        cout << "This program takes at least two arguments: host address,"
        "and port number for the server. After this, every pair of host address"
        "and port number is a different server that this one connects to\n" << endl;
        exit(0);
    }

    //Address family that is used to designate the type of address you communicate with. 
    //In this case, protocol v4 addresses
    server.sin_family = AF_INET; 
    //Server is bound to all available interfaces.
    if(strcmp(argv[1], "localhost")==0){
        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server_name = "127.0.0.1";
    } else {
        server.sin_addr.s_addr = inet_addr(argv[1]);
        server_name = argv[1];
    }
    
    //htonl converts longs from host to network byte order. htons does same but for shorts
    server.sin_port = htons(atoi(argv[2]));

    server_name = server_name + ":" + argv[2];

    //Assign socket to sd descriptor. Socket takes type (AF_INET),
    //type of comm (SOCK_DGRAM), and protocol to use (0).
    sd = socket(AF_INET, SOCK_DGRAM,0);

    //Sets sets local address and port of the socket
    bind(sd, (SA *) &server, sizeof(server));

    map<string, string> new_server_channels;
    servers["HOME"] = new_server_channels;
    if(argc > 3){
        for(int i = 3; i < argc; i += 2){
            string ip;
            if(strcmp(argv[i], "localhost")==0){
                ip = "127.0.0.1";
            } else {
                ip = argv[i];
            }       
            int port = atoi(argv[i+1]);
            char port_str[6];
            sprintf(port_str, "%d", port);
            string key = ip + ":" +port_str;
            map<string, string> new_server_channels;
            servers[key] = new_server_channels;
            struct hostent *hp;
            struct sockaddr_in server_address;
            hp = gethostbyname(argv[i]);
            server_address.sin_family = AF_INET;
            memcpy(&(server_address.sin_addr.s_addr), hp->h_addr, hp->h_length);
            server_address.sin_port = htons(port);
            servers_addr[key] = server_address;
        }
    }

    //Create channel list that tracks all current channels. Initialized with Common
    struct channel_node* channel_list = (struct channel_node*) malloc(sizeof(struct channel_node));
    strcpy(channel_list->channel_name, "Common");
    channel_list->next_channel = NULL; 
    //Give the channel its first user: the Server. Never gets deleted.
    channel_list->channels_users = (struct user_node*) malloc(sizeof(struct user_node));
    strcpy(channel_list->channels_users->user_name, "Server");
    channel_list->channels_users->next_user = NULL;
    
    //Create the first user
    struct user_node* user_list = (struct user_node*) malloc(sizeof(struct user_node));
    strcpy(user_list->user_name, "Server");
    user_list->next_user = NULL;

    //seed random number generator
    seed_randgenerator();

    for(;;){
        struct sockaddr sender = {0,0};
        socklen_t sendsize = sizeof(sender);

        //Our request message that we will recieve
        struct request* recieving = (struct request *)malloc(sizeof(struct request_say));
        rc = recvfrom(sd, recieving, sizeof(struct request_say), 
                       0, (struct sockaddr*)&sender, &sendsize);
        (void) rc;
        
        if(recieving->req_type == REQ_LOGIN){
            //A new user has joined the server and is "logging in"
            //Send them welcome message
            //Add them to the user list. If first user, initialize user list.
            //Parse message
            //printf("Received a REQ_LOGIN message\n");
            struct request_login* recieving_login = (struct request_login*) recieving;
            //printf("Received login request from user %s\n", recieving_login->req_username);
            //printf("Placing %s into Common channel\n", recieving_login->req_username);

            cout << server_name << " " << get_name(&sender) << " recv Request Login " << recieving_login->req_username << endl;

            //Add user to the user list
            push_user(user_list, recieving_login->req_username, sender);
            
             //Add user to channel "Common" and add user to the user channel
            if(channel_list->channels_users == NULL){
                channel_list->channels_users = (struct user_node*) malloc(sizeof(struct user_node));
                channel_list->channels_users->user_address = sender;
                channel_list->channels_users->next_user = NULL; 
                strcpy(channel_list->channels_users->user_name, recieving_login->req_username);
                channel_list->channels_users->users_channels = (struct channel_node*) malloc(sizeof(struct channel_node));
                strcpy(channel_list->channels_users->users_channels->channel_name,"Common");
                channel_list->channels_users->users_channels->next_channel = NULL;
            } else {
                push_user(channel_list->channels_users, recieving_login->req_username, sender);
            }
        
        } else if (recieving->req_type == REQ_S2S_JOIN){
            struct request_s2s_join* recieving_s2s_join = (struct request_s2s_join*) recieving;
            cout << server_name << " " << get_name(&sender) << " recv S2S Join " << recieving_s2s_join->req_channel << endl;

            map<string, string>::iterator channel_iter;
            //CHECK IF YOU ARE SUBSCRIBED TO THE CHANNEL ALREADY IF YES, DO NOTHING
            string key = string(recieving_s2s_join->req_channel);
            channel_iter = servers["HOME"].find(key);
            servers[get_name(&sender)][key] = key;
            if(channel_iter == servers["HOME"].end()){
            //OTHERWISE, SEND TO ALL ADJACENT SERVERS EXCEPT THE ONE THAT SENT IT TO YOU AND SUBSCRIBE TO IT. 
            //FURTHERMORE, NOTE THAT THE SERVER THAT SENT IT TO YOU IS SUBSCRIBED TO THAT CHANNEL AS WELL

                servers["HOME"][key] = key;
                
                struct request_s2s_join* message = (struct request_s2s_join*) malloc(sizeof(struct request_s2s_join));
                message->req_type = REQ_S2S_JOIN;
                strcpy(message->req_channel, recieving_s2s_join->req_channel);

                map<string, struct sockaddr_in>::iterator server_addr_iter;

                for(server_addr_iter = servers_addr.begin(); server_addr_iter != servers_addr.end(); server_addr_iter++){
                    struct sockaddr_in target = server_addr_iter->second;
                    struct sockaddr_in* sender_server = (struct sockaddr_in*) &sender;
                    if((sender_server->sin_addr.s_addr != target.sin_addr.s_addr)||
                        (sender_server->sin_port != target.sin_port)){
                    cout << server_name << " " << get_name((struct sockaddr *)&target) 
                    << " send S2S Join " << recieving_s2s_join->req_channel << endl;
                    sendto(sd, message, sizeof(struct request_s2s_join), 0, (SA *) &target, sizeof(target));
                    string channel = string(recieving_s2s_join->req_channel);
                    servers[get_name((struct sockaddr *)&target)][key] = key;
                    }
                }
            }

        } else if (recieving->req_type == REQ_S2S_SAY){
            struct request_s2s_say* recieving_s2s_say = (struct request_s2s_say*) recieving;
            cout << server_name << " " << get_name(&sender) << " recv S2S Say " << 
            recieving_s2s_say->req_channel << " " << recieving_s2s_say->req_username  <<
            " " << recieving_s2s_say->req_text << " " << recieving_s2s_say->id << endl;
            int servers_sent_to = 0;
            int clients_sent_to = 0;

            if(set_check(recieving_s2s_say->id, &id_set)){
                //WE HAVE FOUND A DUPLCATE
                //DISCARD THE MESSAGE
                //SEND A LEAVE REQUEST TO THE SENDER
                struct request_s2s_leave* message = (struct request_s2s_leave*) malloc(sizeof(struct request_s2s_leave));
                message->req_type = REQ_S2S_LEAVE;
                strcpy(message->req_channel, recieving_s2s_say->req_channel);
                cout << server_name << " " << get_name(&sender) << " send S2S Leave " << message->req_channel << endl;
                sendto(sd, message, sizeof(struct request_s2s_leave), 0, (SA *) &sender, sizeof(sender));
            } else {
                //BROADCAST A REQ_S2S_SAY MESSAGE TO ALL SERVERS SUBSCRIBED TO THIS CHANNEL
                struct sockaddr_in* sender_server = (struct sockaddr_in*) &sender;
                map<string, server_channels>::iterator server_iter;
                for(server_iter = servers.begin(); server_iter != servers.end(); server_iter++){
                    if(server_iter->first.compare(string("HOME")) != 0){
                        if((sender_server->sin_addr.s_addr != servers_addr[server_iter->first].sin_addr.s_addr)||
                        (sender_server->sin_port != servers_addr[server_iter->first].sin_port)){
                            map<string, string>::iterator channel_iter;
                            channel_iter = server_iter->second.find(string(recieving_s2s_say->req_channel));
                            if(channel_iter != server_iter->second.end()){
                                struct request_s2s_say* message = (struct request_s2s_say*) malloc(sizeof(struct request_s2s_say));
                                message->req_type = REQ_S2S_SAY;
                                strcpy(message->req_channel, recieving_s2s_say->req_channel);
                                strcpy(message->req_username, recieving_s2s_say->req_username);
                                strcpy(message->req_text, recieving_s2s_say->req_text);
                                message->id = recieving_s2s_say->id;
                                id_set.insert(message->id);
                                struct sockaddr_in target = servers_addr[server_iter->first];
                                cout << server_name << " " << get_name((struct sockaddr*)&target) << " send S2S Say " << recieving_s2s_say->req_text << endl;
                                sendto(sd, message, sizeof(struct request_s2s_say), 0, (SA *) &target, sizeof(target));
                                servers_sent_to++;
                            }
                        }
                    }
                }

                //SEND MESSAGE TO EVERY CLIENT SUBSCRIBED TO IT ON THIS SERVER
                struct user_node* user_it = user_list;
                            //Construct our message
                struct text_say message;
                message.txt_type = TXT_SAY;
                strcpy(message.txt_text, recieving_s2s_say->req_text);
                strcpy(message.txt_username, recieving_s2s_say->req_username);
                strcpy(message.txt_channel, recieving_s2s_say->req_channel);
                //Find the channel and send it to each of its users
                struct channel_node* channel_it = channel_list;
                while(channel_it != NULL){
                    if(strcmp(channel_it->channel_name, recieving_s2s_say->req_channel)==0){
                        user_it = channel_it->channels_users->next_user;
                        while(user_it != NULL){
                            sendto(sd, &message, sizeof(struct text_say), 
                                0, (SA *) &(user_it->user_address), sendsize);
                            clients_sent_to++;
                            //printf("Sent to %s\n", user_it->user_name);
                            cout << server_name << " " << get_name(&(user_it->user_address)) << 
                            " send Text Say " << message.txt_username << " " << message.txt_channel << " " << message.txt_text << endl;
                            if(user_it != NULL){
                                user_it = user_it->next_user;
                            }
                        }
                        break;
                    }
                    channel_it = channel_it->next_channel; 
                }


                //IF IT HAS NO CLIENTS, AND NO SERVERS TO FORWARD TO, LEAVE
                if(clients_sent_to == 0 && servers_sent_to == 0){
                    string key = string(recieving_s2s_say->req_channel);
                    servers["HOME"].erase(key);
                    struct request_s2s_leave* message = (struct request_s2s_leave*) malloc(sizeof(struct request_s2s_leave));
                    message->req_type = REQ_S2S_LEAVE;
                    strcpy(message->req_channel, recieving_s2s_say->req_channel);
                    cout << server_name << " " << get_name(&sender) << " send S2S Leave " << message->req_channel << endl;
                    sendto(sd, message, sizeof(struct request_s2s_leave), 0, (SA *) &sender, sizeof(sender));
                    //WE LEFT THE CHANNEL. WE CAN STILL KNOW WHO IS SUBBED TO WHICH CHANNELS
                }
            }

        } else if (recieving->req_type == REQ_S2S_LEAVE){
            //A NODE HAS DECIDED TO LEAVE THE COLLECTIVE. DESTROY ALL RECORDS OF ITS EXISTENCE FROM THIS CHANNEL
            struct request_s2s_leave* recieving_s2s_leave = (struct request_s2s_leave*) recieving;
            cout << server_name << " " << get_name(&sender) << " recv S2S Leave " << 
            recieving_s2s_leave->req_channel << endl;

            //WE MUST RESPOND BY ERADICATING ALL TRACE OF ITS EXISTENCE.
            servers[get_name(&sender)].erase(recieving_s2s_leave->req_channel);

        }

        else if(user_validate(user_list, sender.sa_data) == 0){
            //printf("This user has not yet logged in\n");
            struct text_error error;
            error.txt_type = TXT_ERROR;
            strcpy(error.txt_error, "You have not logged in. Please restart the client\n");
            cout << server_name << " " << get_name(&sender) << " send Text Error " << error.txt_error << endl;
            sendto(sd, &error, sizeof(struct text_error), 0, (SA *) &sender, sendsize);

        }

        else if(recieving->req_type == REQ_SAY){
            //We have recieved a message from a user on a channel.
            //We want to redirect this message to everyone who is a member of the channel
            //And with the username, which we determine serverside with a sock_addr comparison
            //printf("Received a REQ_SAY message\n");
            struct request_say* recieving_say = (struct request_say*) recieving;
            //printf("Received message '%s' on channel %s\n", recieving_say->req_text, recieving_say->req_channel);
            cout << server_name << " " << get_name(&sender) << " recv Request Say " 
            << recieving_say->req_channel << " " << recieving_say->req_text <<  endl;
            //Find the name of the user based on the address
            char sender_name[USERNAME_MAX];
            struct user_node* user_it = user_list;
            while(user_it != NULL){
                 if(strcmp(user_it->user_address.sa_data, sender.sa_data)==0){
                    strcpy(sender_name, user_it->user_name);
                    break;
                 }
                 user_it = user_it->next_user;
            }

            //Send an S2S text message to each server subscribed to the channel
            int random_id = get_rand();
            map<string, server_channels>::iterator server_iter;
            for(server_iter = servers.begin(); server_iter != servers.end(); server_iter++){
                if(server_iter->first.compare(string("HOME")) != 0){
                map<string, string>::iterator channel_iter;
                channel_iter = server_iter->second.find(string(recieving_say->req_channel));
                    if(channel_iter != server_iter->second.end()){
                        struct request_s2s_say* message = (struct request_s2s_say*) malloc(sizeof(struct request_s2s_say));
                        message->req_type = REQ_S2S_SAY;
                        strcpy(message->req_channel, recieving_say->req_channel);
                        strcpy(message->req_username, sender_name);
                        strcpy(message->req_text, recieving_say->req_text);
                        message->id = random_id;
                        id_set.insert(message->id);
                        struct sockaddr_in target = servers_addr[server_iter->first];
                        cout << server_name << " " << get_name((struct sockaddr*)&target) <<  " send S2S Say " << recieving_say->req_text << endl;
                        sendto(sd, message, sizeof(struct request_s2s_say), 0, (SA *) &target, sizeof(target));
                    }
                }
            }
            
            //Construct our message
            struct text_say message;
            message.txt_type = TXT_SAY;
            strcpy(message.txt_text, recieving_say->req_text);
            strcpy(message.txt_username, sender_name);
            strcpy(message.txt_channel, recieving_say->req_channel);
            //Find the channel and send it to each of its users
            struct channel_node* channel_it = channel_list;
            while(channel_it != NULL){
                if(strcmp(channel_it->channel_name, recieving_say->req_channel)==0){
                    user_it = channel_it->channels_users->next_user;
                    while(user_it != NULL){
                        sendto(sd, &message, sizeof(struct text_say), 
                            0, (SA *) &(user_it->user_address), sendsize);
                        //printf("Sent to %s\n", user_it->user_name);
                        cout << server_name << " " << get_name(&(user_it->user_address)) << 
                        " send Text Say " << message.txt_username << " " << message.txt_channel << " " << message.txt_text << endl;
                        if(user_it != NULL){
                            user_it = user_it->next_user;
                        }
                    }
                    break;
                }
                channel_it = channel_it->next_channel; 
            }
        }
        
        else if(recieving->req_type == REQ_LOGOUT){
            //Based on the senders sock_addr, remove them from the user list, and from all channel lists
            //printf("Received a REQ_LOGOUT message\n");
            cout << server_name << " " << get_name(&sender) << " recv Request Logout" << endl;

            char sender_name[USERNAME_MAX];
            struct user_node* it = user_list;
            while(it != NULL){
                 if(strcmp(it->user_address.sa_data, sender.sa_data)==0){
                    strcpy(sender_name, it->user_name);
                 }
                 it = it->next_user;
            }

            delete_user_from_user_list(&user_list, sender_name);
            delete_user_from_channel_list(channel_list, sender_name);

            struct text_say message;
            message.txt_type = TXT_SAY;
            strcpy(message.txt_text, "You have successfully logged out. Goodbye!");
            cout << server_name << " " << get_name(&sender) << 
            " Text Say You have successfully logged out. Goodbye!" << endl;
            sendto(sd, &message, sizeof(struct text_say), 0, (SA *) &sender, sendsize);
        }

        else if(recieving->req_type == REQ_JOIN){
            //User wants to join a channel. If the channel already exists, let them join it
            //If they are already in the channel, let them know that nothing has changed
            //If the channel doesn't exist, create a new channel, and let them know they have joined it
            //printf("Received a REQ_JOIN message\n");
            struct request_join* recieving_join = (struct request_join*) recieving;
            //printf("User is asking to join: %s\n", recieving_join->req_channel);

            cout << server_name << " " << get_name(&sender) << " recv Request Join " 
            << recieving_join->req_channel << endl;



            char sender_name[USERNAME_MAX];
            struct user_node* user_it = user_list;
            while(user_it != NULL){
                 if(strcmp(user_it->user_address.sa_data, sender.sa_data)==0){
                    strcpy(sender_name, user_it->user_name);
                 }
                 user_it = user_it->next_user;
            }

            if(strcmp("Common", recieving_join->req_channel)==0){
                map<string, string>::iterator iter;
                        string key = string(recieving_join->req_channel);
                        iter = servers["HOME"].find(key);
                        if(iter == servers["HOME"].end()){
                            servers["HOME"][key] = key;
                            struct request_s2s_join* message = (struct request_s2s_join*) malloc(sizeof(struct request_s2s_join));
                            message->req_type = REQ_S2S_JOIN;
                            strcpy(message->req_channel, recieving_join->req_channel);

                            map<string, struct sockaddr_in>::iterator server_addr_iter;

                            for(server_addr_iter = servers_addr.begin(); server_addr_iter != servers_addr.end(); server_addr_iter++){
                                struct sockaddr_in target = server_addr_iter->second;
                                cout << server_name << " " << get_name((struct sockaddr *)&target) 
                                << " send S2S Join " << recieving_join->req_channel << endl;
                                sendto(sd, message, sizeof(struct request_s2s_join), 0, (SA *) &target, sizeof(target));
                                string channel = string(recieving_join->req_channel);
                                servers[get_name((struct sockaddr *)&target)][channel] = channel;
                            }
                        }
            } else {
                struct channel_node* channel_it = channel_list;
                user_it = user_list;
                int channel_exists = 0;
                int subscribed = 0;
                while(channel_it != NULL){
                    if(strcmp(channel_it->channel_name, recieving_join->req_channel)==0){
                        channel_exists = 1;
                        break; //Found channel
                    }
                    channel_it = channel_it->next_channel;
                }
                if(channel_exists == 1){
                    cout << "Channel already exists. Checking if user is already subscribed";
                    user_it = channel_it->channels_users;
                    while(user_it != NULL){
                        if(strcmp(user_it->user_name, sender_name)==0){
                            subscribed = 1;
                            break;
                        }
                        user_it = user_it->next_user;
                    }
                    if(subscribed == 1){
                        //printf("User is already subscribed to this channel\n");
                    } else {
                        //printf("User isn't yet subscribed to this channel, adding\n");
                        push_user_to_channel_list(channel_list, recieving_join->req_channel, sender_name, 
                            sender);
                        push_channel_to_user_list(user_list, recieving_join->req_channel, 
                            sender_name, sender);
                        //Send a S2S JOIN to each adjacent server if home server is not yet subbed
                        map<string, string>::iterator iter;
                        string key = string(recieving_join->req_channel);
                        iter = servers["HOME"].find(key);
                        if(iter == servers["HOME"].end()){
                            servers["HOME"][key] = key;
                            struct request_s2s_join* message = (struct request_s2s_join*) malloc(sizeof(struct request_s2s_join));
                            message->req_type = REQ_S2S_JOIN;
                            strcpy(message->req_channel, recieving_join->req_channel);

                            map<string, struct sockaddr_in>::iterator server_addr_iter;

                            for(server_addr_iter = servers_addr.begin(); server_addr_iter != servers_addr.end(); server_addr_iter++){
                                struct sockaddr_in target = server_addr_iter->second;
                                cout << server_name << " " << get_name((struct sockaddr *)&target) 
                                << " send S2S Join " << recieving_join->req_channel << endl;
                                sendto(sd, message, sizeof(struct request_s2s_join), 0, (SA *) &target, sizeof(target));
                                string channel = string(recieving_join->req_channel);
                                servers[get_name((struct sockaddr *)&target)][channel] = channel;
                            }
                        }
                        

                    }

                } else {
                    //printf("Channel does not exist. Creating channel and adding user\n");
                    string key = string(recieving_join->req_channel);
                    servers["HOME"][key] = key;
                    push_channel(channel_list, recieving_join->req_channel, sender_name, sender);
                    push_channel_to_user_list(user_list, recieving_join->req_channel, 
                        sender_name, sender);

                    struct request_s2s_join* message = (struct request_s2s_join*) malloc(sizeof(struct request_s2s_join));
                        message->req_type = REQ_S2S_JOIN;
                        strcpy(message->req_channel, recieving_join->req_channel);

                        map<string, struct sockaddr_in>::iterator server_addr_iter;

                        for(server_addr_iter = servers_addr.begin(); server_addr_iter != servers_addr.end(); server_addr_iter++){
                            struct sockaddr_in target = server_addr_iter->second;
                            cout << server_name << " " << get_name((struct sockaddr *)&target) 
                            << " send S2S Join " << recieving_join->req_channel << endl;
                            sendto(sd, message, sizeof(struct request_s2s_join), 0, (SA *) &target, sizeof(target));
                            string channel = string(recieving_join->req_channel);
                            servers[get_name((struct sockaddr *)&target)][channel] = channel;
                        }
                }
            }
        }

        else if(recieving->req_type == REQ_LEAVE){
            //User wants to leave channel. Remove channel from users list and user from channels list
            //If this list becomes empty, delete the channel (unless it is common)
            //printf("Received a request_leave message\n");
            struct request_leave* recieving_leave = (struct request_leave*) recieving;
            //printf("Received request to leave channel %s\n", recieving_leave->req_channel);

            cout << server_name << " " << get_name(&sender) << " recv Request Leave " 
            << recieving_leave->req_channel << endl;

            if(strcmp(recieving_leave->req_channel, "Common")==0){
                //printf("You may not leave Common, it is the default channel\n");
                struct text_error error;
                error.txt_type = TXT_ERROR;
                strcpy(error.txt_error, "You may not delete Common. It is the default\n");
                cout << server_name << " " << get_name(&sender) << " send Text Error " 
                << error.txt_error << endl;
                sendto(sd, &error, sizeof(struct text_error), 0, (SA *) &sender, sendsize);
            }
            char sender_name[USERNAME_MAX];
            struct user_node* user_it = user_list;
            while(user_it != NULL){
                 if(strcmp(user_it->user_address.sa_data, sender.sa_data)==0){
                    strcpy(sender_name, user_it->user_name);
                    break;
                 }
                 user_it = user_it->next_user;
            }

            struct channel_node* channel_it = channel_list;
            while(channel_it != NULL){
                if(strcmp(channel_it->channel_name, recieving_leave->req_channel)==0){
                    break;
                }
                channel_it = channel_it->next_channel;
            }

            if(channel_it == NULL){
                //printf("This channel does not exist. Please try another channel\n");
            } else {
                delete_user_from_user_list(&(channel_it->channels_users), sender_name);
                delete_channel_from_channel_list(&(user_it->users_channels), recieving_leave->req_channel);

                if(channel_it->channels_users->next_user == NULL){
                    delete_channel_from_channel_list(&channel_list, recieving_leave->req_channel);
                }
            }

            
        }
        else if(recieving->req_type == REQ_LIST){
            //Send the user a list of all of the channels on the server
            //printf("Received a request_list message\n");

            cout << server_name << " " << get_name(&sender) << " recv Request List" << endl;

            struct channel_node* channel_it = channel_list;
            int num_channels = 0;
            while(channel_it != NULL){
                num_channels++;
                channel_it = channel_it->next_channel;
            }

            struct text_list* message = (struct text_list*) malloc(sizeof(struct text_list)
                + num_channels * sizeof(struct channel_info));
            message->txt_type = TXT_LIST;
            message->txt_nchannels = num_channels;

            channel_it = channel_list;
            int i = 0;
            while(channel_it != NULL){
                strcpy(message->txt_channels[i].ch_channel, channel_it->channel_name);
                cout << channel_it->channel_name << endl;
                channel_it = channel_it->next_channel;
                i++;
            }
            cout << server_name << " " << get_name(&sender) << " send Text List" << endl;
            sendto(sd, message, sizeof(struct text_list) + i*32, 0, (SA *) &sender, sendsize);
        }
        else if(recieving->req_type == REQ_WHO){
            //Send user list of all users on a channel
            //printf("Received a REQUEST_WHO message\n");
            struct request_who* recieving_who = (struct request_who*) recieving;
            //printf("Inquiring about users on %s\n", recieving_who->req_channel);

            cout << server_name << " " << get_name(&sender) << " recv Request Who " << recieving_who->req_channel << endl;

            int user_count = 0;
            struct channel_node* channel_it = channel_list;
            while(channel_it != NULL){
                if(strcmp(channel_it->channel_name, recieving_who->req_channel)==0){
                    struct user_node* user_it = channel_it->channels_users->next_user;
                    while(user_it != NULL){
                        user_count++;
                        user_it = user_it->next_user;
                    }
                    break;
                }
                channel_it = channel_it->next_channel;
            }

            if(channel_it == NULL){
                //printf("This channel does not exist. \n");
                struct text_error error;
                error.txt_type = TXT_ERROR;
                strcpy(error.txt_error, "No such channel\n");
                cout << server_name << " " << get_name(&sender) << " send Text Error " << error.txt_error << endl;
                sendto(sd, &error, sizeof(struct text_error), 0, (SA *) &sender, sendsize);
            } else {
                //Needs logic in case the channel isn't found
            
                struct text_who* message = (struct text_who*) malloc(sizeof(struct text_who) 
                    + user_count * sizeof(struct user_info));
                message->txt_type = TXT_WHO;
                message->txt_nusernames = user_count;
                strcpy(message->txt_channel, channel_it->channel_name);
                struct user_node* user_it= channel_it->channels_users->next_user;
                int i = 0;
                while(user_it != NULL){
                            strcpy(message->txt_users[i].us_username, user_it->user_name);
                            //printf("%s\n", message->txt_users[i].us_username);
                            user_it = user_it->next_user;
                            i++;
                        }
                cout << server_name << " " << get_name(&sender) << " send Text Who" << endl;
                sendto(sd, message, sizeof(struct text_who) + user_count * sizeof(struct user_info), 
                    0, (SA *) &sender, sendsize);
                free(message);
            }
        } 

        free(recieving);

        
         //Prints out our message
    }
}