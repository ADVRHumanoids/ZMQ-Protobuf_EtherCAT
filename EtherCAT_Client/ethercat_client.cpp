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

using namespace iit::advr;
using namespace zmq;
using namespace std;


int main(int argc, char *argv[])
{
    using namespace google::protobuf::io;
   
    std::string m_cmd="MASTER_CMD";
    uint32_t idx(0);
    
   
    while(1) {
        
        zmq::context_t context{1};
        zmq::socket_t publisher{context, ZMQ_REQ};
        
        publisher.connect("tcp://localhost:5555");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        Ecat_Master_cmd *ecat_master_cmd= new Ecat_Master_cmd();
        Header *ecat_header_cmd =new Header();
        Repl_cmd pb_msg;
        std::string pb_msg_serialized;
        multipart_t multipart;


        if(idx==0)
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
        }

        sleep(8);
        
        if(idx==1)
        {
            pb_msg.set_type(CmdType::ECAT_MASTER_CMD);
            ecat_master_cmd->set_type(Ecat_Master_cmd::START_MASTER);
            pb_msg.set_allocated_ecat_master_cmd(ecat_master_cmd);
            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
        }
        
        //         std::cout << zmq_strerror (errno) << std::endl;

        ++idx;
        
        publisher.disconnect("tcp://localhost:5555");
        publisher.close();
        if (idx == 2) {
        break;
        }

        
    }

    //zmq_close (ethercat_client);
    //zmq_ctx_destroy (context);
    
   // std::cout << "done pub" << std::endl;
    return 0;
}