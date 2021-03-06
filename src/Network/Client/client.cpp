/*
 * Copyright (c) 2020 Mutex Unlocked
 * Author: Friedrich Doku
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *   This program is distributed in the hope that it will be useful
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


#include <zephyr/pkgc.hpp>
#include <zephyr/pkg.hpp>
#include <zephyr/nodeclient.hpp>
#include <zephyr/nodeserver.hpp>
#include <zephyr/node.grpc.pb.h>
#include <zephyr/utils.hpp>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <ctime>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <netdb.h>
#include <sodium.h>
#include <sodiumwrap/sodiumtester.h>
#include <sodiumwrap/box_seal.h>
#include <sodiumwrap/keypair.h>
#include <sodiumwrap/allocator.h>
extern "C"{
    #include <sibe/ibe.h>
    #include <sibe/ibe_progs.h>
}

MyCout ccout;
CONF_CTX *cnfctx;
params_t params;
byte_string_t keyb;
params_t paramsb;
std::vector<std::vector<std::string>> config;
std::vector<std::string> msgtmp;
std::vector<std::string> outbox;

std::string mailboxaddress = "";

using namespace std;
using namespace node;
using namespace sodium;
using bytes = sodium::bytes;


// Mixers, Mailboxes, PKGS

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void CheckMailBox();
void getkeysfrominfo();
std::string attachtomixer(std::string msg);
int createciphertext(std::vector<std::string> mixerKeys, std::string encmsg);

int main(){

    if (sodium_init() == -1) {
        return 1;
    }
    string msg,email,key, params, filepath;
    // cout << "Enter message" << endl;
    // cin >> msg;
    // cout << "Enter recievers email" << endl;
    // cin >> email;
    // cout << "Enter config file path" << endl;
    // cin >> filepath;
    filepath= "/dems";

    // Load the configuration file
    //string filepath = "";
    vector<vector<std::string>> vec;
    vec = get_config_info(filepath);
    config = vec;
    cout << "PASSED HERE" << endl;
    cout << "Asking " << vec[2][0] << " for data" << endl;
    auto start = std::chrono::system_clock::now();
    auto x = getkeysfrompkg(vec[2][0], to_string(8080), email);
    cout << "GOT IT" << endl;
    // Get your private key
    
    
    IBE_init();
    cout << x[0] << endl;
    cout << x[1] << endl;
    std::string key_serial_tmp = std::move(x[0]);
    std::string param_serial_tmp = std::move(x[1]);
    deserialize_bytestring(key_serial_tmp, keyb);
    deserialize_params(param_serial_tmp, paramsb);

    // FILE * filePointer; 
    // filePointer = fopen("params.txt","w+");
    // params_out(filePointer, paramsb);
    // fclose(filePointer);

    cout << "SIZE OF FULL DATA IS " << sizeof(keyb) + sizeof(paramsb) << endl;
    // Encrypt message for user
    cout << "MADE IT HERE 2" << endl;

    std::string encdata = pkg_encrypt(email, paramsb, "LIfe is what we make of it.");
    cout << "ENCDATA START" << endl;
    cout << encdata << endl;;
    cout << "ENCDATA END" << endl;



    cout << "MADE IT HERE FINISHED" << endl;
    
    // Get mixer data from information node
    while(msgtmp.empty()){
        getkeysfrominfo();
    }
              
    //sleep(10);
    std::vector<std::string> mixerKeys;
    mixerKeys = msgtmp;
    cout << "CONVERTED !!!" << endl;
    createciphertext(mixerKeys, encdata);

    NodeClient mailreq(
    grpc::CreateChannel(/*mailboxaddress + */ "10.108.0.8:50051",
                            grpc::InsecureChannelCredentials()));

    std::cout << "-------------- GetMessages --------------" << std::endl;

    std::vector<std::string> mymsgs;
    while(true){
        //FIXME: handle failed encryption attemps
        //FIXME: Output add rounds
        mailreq.DumpMessages();
        mymsgs.clear();
        for(auto x : mailreq.data){
            mymsgs.push_back(x.data());
        }
        if(!mymsgs.empty()){
            for(auto x : mymsgs){
                msgtmp.push_back(x);
                auto d = x;
                   
                std::cout << "THIS IS WHAT I GOT " << std::endl;

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s\n";
                std::cout << d << std::endl;
                //TODO: Fix encrytion run a test on the local system first
                auto j = pkg_decrypt(d, keyb, paramsb);
                
            }
            break;
        }
    }

    return 0;
}

int createciphertext(std::vector<std::string> mixerKeys, std::string encmsg){


    std::vector<std::pair<std::string,bytes>> mixers;
    for(auto x : mixerKeys){
        std::cout << "GOT THIS " <<  x << std::endl;
        mixers.push_back(getkeyfromtxt(x));
        // cout << x.first << endl;
        // cout << x.second << endl;
    }

    cout << "Works 1" << endl;

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(1,10);
    auto seed = dist6(rng);

    Shuffle<std::pair<std::string,bytes>> shu(mixers, (int) seed);
    mixers.clear();
    mixers = std::move(shu.vec);
    cout << "Works 2" << endl;

    //int N = mixers.size();

    box_seal<> sb{};
    std::vector<sodium::keypair<>> boxes;
    //for(auto x : mixers){
        sodium::keypair<> mix{};
	sodium::keypair<> mix1{};
	sodium::keypair<> mix2{};
        mix.public_key_ = mixers[0].second;
	mix1.public_key_ = mixers[1].second;
	mix2.public_key_ = mixers[2].second;

        boxes.push_back(mix);
	boxes.push_back(mix1);
	boxes.push_back(mix2);
	std::cout << "LOVE" << std::endl;
    //}
    
    //TODO: Create mailbox code and fix addresses
    //int num = rand() % config[1].size() -1;
    //auto x = config[1][num];
    //std::cout << "Talking to " << x << std::endl;
    mailboxaddress = "10.108.0.8";
    std::cout << "YOUR MAILBOX IS " << mailboxaddress << std::endl;

    std::string enctmp = encmsg;
    enctmp += mailboxaddress;
    enctmp += "CUTHERE";
    enctmp += std::to_string(mailboxaddress.size());

    cout << "Done Setting getting mixer keys and adding mailbox." << endl;
    
    bytes tmpenc{enctmp.cbegin(), enctmp.cend()};
    int i = 0;
    for(auto x : boxes){
        std::string tmpip = mixers[i].first;
        tmpenc =  sb.encrypt(tmpenc, x.public_key());
      

        std::string tt{tmpenc.cbegin(), tmpenc.cend()};
        tt += tmpip;   // Address
        tt += "CUTHERE";
        tt += std::to_string(tmpip.size()); // Size of ip address

        bytes tempenctmp{tt.cbegin(), tt.cend()};
        tmpenc = tempenctmp;
        i++;
    }

    std::string exitenc{tmpenc.cbegin(), tmpenc.cend()};

    attachtomixer(exitenc);
    return 0;
}

std::string attachtomixer(std::string msg){

    bytes power{msg.cbegin(), msg.cend()};
    

    string cut("CUTHERE");
    
    size_t found = msg.find("CUTHERE");
    if(found == std::string::npos){
        cout << "Failed to parse argument" << endl;
        exit(1);
    }
    cout << msg << endl;
    cout << "Works 4" << endl;
    cout << "FOUND " << found << endl;
    auto toread = msg.substr(found + cut.size());
    msg.erase(msg.begin() + found, msg.end());
    //toread.erase(toread.begin());

    cout << toread << endl;
    cout << msg << endl;
    cout << "Works 5" << endl;
    

    int toread_start;
    std::istringstream iss (toread);
    iss >> toread_start;
    auto ip = msg.substr(msg.size() - toread_start);
    cout << toread_start << endl;
    cout << ip << endl;
    msg.erase(msg.end() - toread_start - toread.size(), msg.end());
    cout << msg << endl;
    cout << "Works 6" << endl; 

    int toerase = cut.size() + toread.size() + ip.size();
    for(int i = 0; i < toerase; i++){
        power.pop_back();
    }

    std::string res{power.cbegin(), power.cend()};
    
    std::cout << "SENDING TO " << ip << std::endl;
    NodeClient mixreq(
    grpc::CreateChannel(ip + ":50051",
                          grpc::InsecureChannelCredentials()));

    std::cout << "-------------- GetMessages --------------" << std::endl;
    node::Msg tosend;
    tosend.set_data(res);
    mixreq.data.push_back(tosend);
    mixreq.PutMessages();

    return "";
}

//FIXME: Utilize other infonodes
//FIXME: There will be trouble sending data over the NAT with
// this implementation. Instead make the info node have sever
// and it will send a response back. 
void getkeysfrominfo(){

    int num = rand() % config[3].size() -1;
    auto x = config[3][num];
    std::cout << "Getting keys from " << x << std::endl;

    NodeClient inforeq(
    grpc::CreateChannel(config[3][num] + ":50051",
                          grpc::InsecureChannelCredentials()));

    std::cout << "-------------- GetMessages --------------" << std::endl;
    //node::Msg tosend;
    //tosend.set_data("NEED172.18.0.1");
    //inforeq.data.push_back(tosend);
    inforeq.DumpMessages();
    for(auto x : inforeq.data){
        msgtmp.push_back(x.data());
    }
}
