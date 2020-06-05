#include <iostream>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "repl_cmd.pb.h"


#include "zmq.hpp"
#include <zmq_addon.hpp>
#include <assert.h>
#define NUMOP 3
#define ITER_MAX 5

enum CMD_E {CMD_START,CMD_TOURQUE,CMD_STOP};


using namespace iit::advr;
using namespace zmq;
using namespace std;

string pipe_prefix, repl_in_path, repl_out_path;
int repl_in_fd, repl_out_fd;

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
   
    std::string m_cmd="ESC_CMD";
    uint32_t idx(0);
    double x[5]={0,1,2,3,4};
    double y[5]={0,-0.3,0,-0.3,0};
    //char uri[]="tcp://localhost:5555";
    char uri[]= "tcp://advantech.local:5555";
         
    int iter=0;
    double torque=10.0;
    
    while(1) {
        
        zmq::context_t context{1};
        zmq::socket_t publisher{context, ZMQ_REQ};
        string *trjtype=new string();
      
        
        Gains *gains=new Gains();
        Ctrl_cmd  *ctrl_cmd= new Ctrl_cmd();
        Repl_cmd pb_msg;
        
        
        Cmd_reply pb_reply;
        std::string pb_msg_serialized;
        multipart_t multipart;
        
        zmq::message_t update;
        
        publisher.connect(uri);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        if(idx==CMD_E::CMD_START)
        {
            pb_msg.set_type(CmdType::CTRL_CMD);

            gains->set_type(Gains_Type::Gains_Type_IMPEDANCE);
            gains->set_pos_kp(250);
            gains->set_pos_kd(5);
            gains->set_tor_kp(2);
            gains->set_tor_kd(0.01);
            gains->set_tor_ki(0.8);
            
            ctrl_cmd->set_allocated_gains(gains);
            ctrl_cmd->set_type(Ctrl_cmd::CTRL_CMD_START);
            ctrl_cmd->set_board_id(123);
            ctrl_cmd->set_value(0xD4);

            pb_msg.set_allocated_ctrl_cmd(ctrl_cmd);


            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
            checkReply(pb_reply); 
            
        }
        
        if(idx==CMD_E::CMD_TOURQUE)
        {
            
            pb_msg.set_type(CmdType::CTRL_CMD);

            ctrl_cmd->set_type(Ctrl_cmd::CTRL_SET_TORQUE);
            ctrl_cmd->set_board_id(123);
            if(torque==10.0)
                torque=-10.0;
            else
                torque=10.0;
            ctrl_cmd->set_value(torque);
            
            pb_msg.set_allocated_ctrl_cmd(ctrl_cmd);
            

            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
            checkReply(pb_reply); 
            sleep(2);
            ++iter;
            cout << "SET TORQUE ITER: " << iter << endl;
        }

        if(idx==CMD_E::CMD_STOP)
        {
            pb_msg.set_type(CmdType::CTRL_CMD);

            ctrl_cmd->set_type(Ctrl_cmd::CTRL_CMD_STOP);
            ctrl_cmd->set_board_id(123);

            pb_msg.set_allocated_ctrl_cmd(ctrl_cmd);


            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
            checkReply(pb_reply); 
        }
        
        if(idx==CMD_E::CMD_TOURQUE)
        {
            if(iter==ITER_MAX)
                ++idx;
        }
        else
            ++idx;
        
        publisher.disconnect(uri);
        publisher.close();
        
        
        if (idx == NUMOP) {
        break;
        }

        
    }

    return 0;
}