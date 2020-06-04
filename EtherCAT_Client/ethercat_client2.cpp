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
#define NUMOP 5

enum CMD_E {CTRL_SET_HOME,CMD_START,CMD_TRJ,CMD_TRJ_QUEUE,CMD_STOP};


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
    pipe_prefix = std::string("/proc/xenomai/registry/rtipc/xddp/");
    double x[5]={0,1,2,3,4};
    double y[5]={0,-0.3,0,-0.3,0};
                                 
    while(1) {
        
        zmq::context_t context{1};
        zmq::socket_t publisher{context, ZMQ_REQ};
        string *trjtype=new string();
        Ctrl_cmd  *ctrl_cmd= new Ctrl_cmd();
        Trajectory_cmd  *trajectory_cmd= new Trajectory_cmd();
        Trj_queue_cmd *trj_queue_cmd= new Trj_queue_cmd();
        Trajectory_cmd_Smooth_par* smooth_par= new Trajectory_cmd_Smooth_par();
        Repl_cmd pb_msg;
        Cmd_reply pb_reply;
        std::string pb_msg_serialized;
        multipart_t multipart;

        
        zmq::message_t update;
        
        publisher.connect("tcp://localhost:5555");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        if(idx==CMD_E::CTRL_SET_HOME)
        {
            pb_msg.set_type(CmdType::CTRL_CMD);

            ctrl_cmd->set_type(Ctrl_cmd::CTRL_SET_HOME);
            ctrl_cmd->set_board_id(110);
            ctrl_cmd->set_value(-0.6);

            pb_msg.set_allocated_ctrl_cmd(ctrl_cmd);


            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            //publisher.recv(&update);
            //pb_reply.ParseFromString(update.to_string());
            //checkReply(pb_reply); 
        }
        
        if(idx==CMD_E::CMD_START)
        {
            pb_msg.set_type(CmdType::CTRL_CMD);

            ctrl_cmd->set_type(Ctrl_cmd::CTRL_CMD_START);
            ctrl_cmd->set_board_id(110);
            ctrl_cmd->set_value(0x3B);

            pb_msg.set_allocated_ctrl_cmd(ctrl_cmd);


            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            //publisher.recv(&update);
            //pb_reply.ParseFromString(update.to_string());
            //checkReply(pb_reply); 
        }
        
        if(idx==CMD_E::CMD_TRJ)
        {
            pb_msg.set_type(CmdType::TRJ_CMD);

            trajectory_cmd->set_type(Trajectory_cmd::SMOOTHER);
            trjtype[0]="smooth";
            trajectory_cmd->set_allocated_name(trjtype);
            trajectory_cmd->set_board_id(110);
            
            for(int i=0;i<5;i++)
            {
            smooth_par->add_x(x[i]);
            smooth_par->add_x(y[i]);
            }
            trajectory_cmd->set_allocated_smooth_par(smooth_par);

            pb_msg.set_allocated_trajectory_cmd(trajectory_cmd);

            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            //publisher.recv(&update);
            //pb_reply.ParseFromString(update.to_string());
            //checkReply(pb_reply); 
        }
        
        if(idx==CMD_E::CMD_TRJ_QUEUE)
        {
            pb_msg.set_type(CmdType::TRJ_QUEUE_CMD);

            trj_queue_cmd->set_type(Trj_queue_cmd::PUSH_QUE);
            trj_queue_cmd->add_trj_names("smooth");
            //trj_queue_cmd->set_trj_names(0,"smooth");
            
            pb_msg.set_allocated_trj_queue_cmd(trj_queue_cmd);

            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            //publisher.recv(&update);
            //pb_reply.ParseFromString(update.to_string());
            //checkReply(pb_reply); 
        }

        if(idx==CMD_E::CMD_STOP)
        {
            pb_msg.set_type(CmdType::CTRL_CMD);

            ctrl_cmd->set_type(Ctrl_cmd::CTRL_CMD_STOP);
            ctrl_cmd->set_board_id(110);
            ctrl_cmd->set_value(0x3B);

            pb_msg.set_allocated_ctrl_cmd(ctrl_cmd);


            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            //publisher.recv(&update);
            //pb_reply.ParseFromString(update.to_string());
            //checkReply(pb_reply); 
        }
        
        ++idx;
        
        publisher.disconnect("tcp://localhost:5555");
        publisher.close();
        
        
        if (idx == NUMOP) {
        break;
        }

        
    }

    return 0;
}