#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "repl_cmd.pb.h"


#include "zmq.hpp"
#include <zmq_addon.hpp>
#include <assert.h>
#define NUMOP 3

enum CMD_E { STOP_MASTER, START_MASTER, GET_SLAVES_DESCR,FOE_MASTER};


using namespace iit::advr;
using namespace zmq;
using namespace std;

void checkReply(Cmd_reply &pb_reply)
{
    if(pb_reply.type()==Cmd_reply::ACK)
    {
        if(pb_reply.cmd_type()==CmdType::ECAT_MASTER_CMD)
        {
            cout << "ECAT_MASTER_CMD " << endl;
            cout << pb_reply.msg() << endl;
        }
    }   
    else
       cout << "NACK" << endl;  
}

int main(int argc, char *argv[])
{
    using namespace google::protobuf::io;
   
    std::string m_cmd="MASTER_CMD";
    uint32_t idx(0);
    //char uri[]="tcp://localhost:5555";
    char uri[]= "tcp://advantech.local:5555";
   
    while(1) {
        
        zmq::context_t context{1};
        zmq::socket_t publisher{context, ZMQ_REQ};
        
        publisher.connect(uri);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        Ecat_Master_cmd *ecat_master_cmd= new Ecat_Master_cmd();
        FOE_Master *foe_master= new FOE_Master();
        string *path= new string();
        string *mcu= new string();
        
        Header *ecat_header_cmd =new Header();
        Repl_cmd pb_msg;
        Cmd_reply pb_reply;
        std::string pb_msg_serialized;
        multipart_t multipart;
        
        zmq::message_t update;


        if(idx==CMD_E::STOP_MASTER)
        {
            pb_msg.set_type(CmdType::ECAT_MASTER_CMD);

            ecat_master_cmd->set_type(Ecat_Master_cmd::STOP_MASTER);
            //         KeyValStr *kv=ecat_master_cmd->add_args();
            //         kv->set_name("id");
            //         kv->set_value("1");
            pb_msg.set_allocated_ecat_master_cmd(ecat_master_cmd);

            //         ecat_header_cmd->set_str_id("MASTER_CMD");
            //         pb_msg.set_allocated_header(ecat_header_cmd);


            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString((char *)update.data());
            checkReply(pb_reply);
        }

        
        if(idx==CMD_E::START_MASTER)
        {
            pb_msg.set_type(CmdType::ECAT_MASTER_CMD);
            ecat_master_cmd->set_type(Ecat_Master_cmd::START_MASTER);
            pb_msg.set_allocated_ecat_master_cmd(ecat_master_cmd);
            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString((char *)update.data());
            checkReply(pb_reply);
        }
        
        
        if(idx==CMD_E::GET_SLAVES_DESCR)
        {
            pb_msg.set_type(CmdType::ECAT_MASTER_CMD);
            ecat_master_cmd->set_type(Ecat_Master_cmd::GET_SLAVES_DESCR);
            pb_msg.set_allocated_ecat_master_cmd(ecat_master_cmd);
            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString((char *)update.data());
            checkReply(pb_reply);
        }
        
        if(idx==CMD_E::FOE_MASTER)
        {
            path[0]="/home/user/MultiDoF-superbuild/external/ec_master_test/examples/fw_update/fw_test_bug_friction_252/fw_test_bug_friction_252_after/";
            mcu[0]="c28";
            path[0]=path[0]+"cent_AC_c28.bin";
            
            cout << "PATH:" << path[0] << endl;
            cout << "MCU: " << mcu[0] << endl;
            
            
            pb_msg.set_type(CmdType::FOE_MASTER);
            foe_master->set_allocated_filename(path);
            foe_master->set_password(0xDAD0);
            foe_master->set_allocated_mcu_type(mcu);
            foe_master->set_slave_pos(1);
            pb_msg.set_allocated_foe_master(foe_master);
            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString((char *)update.data());
            checkReply(pb_reply);
        }
        
        //         std::cout << zmq_strerror (errno) << std::endl;

        ++idx;
        
        publisher.disconnect(uri);
        publisher.close();
        if (idx == NUMOP) {
        break;
        }

        
    }

    //zmq_close (ethercat_client);
    //zmq_ctx_destroy (context);
    
   // std::cout << "done pub" << std::endl;
    return 0;
}
