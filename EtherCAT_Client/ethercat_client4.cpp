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
#define NUMOP 7

enum CMD_E { SLAVE_SDO_INFO, SLAVE_SDO_CMD_R1, SLAVE_SDO_CMD_W,SLAVE_SDO_CMD_R2,FLASH_CMD,SLAVE_SDO_CMD_R3};


using namespace iit::advr;
using namespace zmq;
using namespace std;

void checkReply(Cmd_reply &pb_reply)
{
    if(pb_reply.type()==Cmd_reply::ACK)
    {
        if(pb_reply.cmd_type()==CmdType::SLAVE_SDO_INFO)
        {
            cout << "SLAVE_SDO_INFO " << endl;
            cout << pb_reply.msg() << endl;
        }
        if(pb_reply.cmd_type()==CmdType::SLAVE_SDO_CMD)
        {
            cout << "SLAVE_SDO_CMD " << endl;
            cout << pb_reply.msg() << endl;
        }
        if(pb_reply.cmd_type()==CmdType::FLASH_CMD)
        {
            cout << "FLASH_CMD " << endl;
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
    //char uri[]="tcp://localhost:5555";
    char uri[]= "tcp://advantech.local:5555";
   
    while(1) {
        
        zmq::context_t context{1};
        zmq::socket_t publisher{context, ZMQ_REQ};
        
        publisher.connect(uri);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        KeyValStr *kvalstr_sdo= new KeyValStr();
        KeyValStr *kvalstr_sdo2= new KeyValStr();
        Slave_SDO_info *slave_SDO_info= new Slave_SDO_info();
        Slave_SDO_cmd *slave_SDO_cmd= new Slave_SDO_cmd();
        Flash_cmd *flash_cmd= new Flash_cmd();
        
        Repl_cmd pb_msg;
        Cmd_reply pb_reply;
        std::string pb_msg_serialized;
        multipart_t multipart;
        
        zmq::message_t update;


        if(idx==CMD_E::SLAVE_SDO_INFO)
        {
            pb_msg.set_type(CmdType::SLAVE_SDO_INFO);            
            slave_SDO_info->set_type(Slave_SDO_info::SDO_NAME);
            //slave_SDO_info->set_type(Slave_SDO_info::SDO_OBJD);
            slave_SDO_info->set_board_id(105);
            
            pb_msg.set_allocated_slave_sdo_info(slave_SDO_info);

            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
            checkReply(pb_reply);
        }
        
        if(idx==CMD_E::SLAVE_SDO_CMD_R1)
        {
            pb_msg.set_type(CmdType::SLAVE_SDO_CMD);            

            slave_SDO_cmd->set_board_id(105);
            slave_SDO_cmd->add_rd_sdo("Joint_robot_id");
            slave_SDO_cmd->add_rd_sdo("m3_fw_ver");
            slave_SDO_cmd->add_rd_sdo("c28_fw_ver");
            slave_SDO_cmd->add_rd_sdo("Min_pos");
            slave_SDO_cmd->add_rd_sdo("Max_pos");
            
            pb_msg.set_allocated_slave_sdo_cmd(slave_SDO_cmd);

            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
            checkReply(pb_reply);
        }
        
        if(idx==CMD_E::SLAVE_SDO_CMD_W)
        {
            pb_msg.set_type(CmdType::SLAVE_SDO_CMD); 
            slave_SDO_cmd->set_board_id(105);
            
            kvalstr_sdo=slave_SDO_cmd->add_wr_sdo();

            kvalstr_sdo->set_name("Min_pos");
            kvalstr_sdo->set_value("-1.0");
            
            kvalstr_sdo2=slave_SDO_cmd->add_wr_sdo();

            kvalstr_sdo2->set_name("Max_pos");
            kvalstr_sdo2->set_value("1.0");

            pb_msg.set_allocated_slave_sdo_cmd(slave_SDO_cmd);

            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
            checkReply(pb_reply);
        }
        
        if(idx==CMD_E::SLAVE_SDO_CMD_R2)
        {
            pb_msg.set_type(CmdType::SLAVE_SDO_CMD);            

            slave_SDO_cmd->set_board_id(105);
            slave_SDO_cmd->add_rd_sdo("Joint_robot_id");
            slave_SDO_cmd->add_rd_sdo("m3_fw_ver");
            slave_SDO_cmd->add_rd_sdo("c28_fw_ver");
            slave_SDO_cmd->add_rd_sdo("Min_pos");
            slave_SDO_cmd->add_rd_sdo("Max_pos");
            
            pb_msg.set_allocated_slave_sdo_cmd(slave_SDO_cmd);

            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
            checkReply(pb_reply);
        }
        
        if(idx==CMD_E::FLASH_CMD)
        {
            pb_msg.set_type(CmdType::FLASH_CMD);            

            flash_cmd->set_board_id(105);
            flash_cmd->set_type(Flash_cmd_Type::Flash_cmd_Type_LOAD_PARAMS_FROM_FLASH);

            
            pb_msg.set_allocated_flash_cmd(flash_cmd);

            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
            checkReply(pb_reply);
        }
 
 
        if(idx==CMD_E::SLAVE_SDO_CMD_R3)
        {
            pb_msg.set_type(CmdType::SLAVE_SDO_CMD);            

            slave_SDO_cmd->set_board_id(105);
            slave_SDO_cmd->add_rd_sdo("Joint_robot_id");
            slave_SDO_cmd->add_rd_sdo("m3_fw_ver");
            slave_SDO_cmd->add_rd_sdo("c28_fw_ver");
            slave_SDO_cmd->add_rd_sdo("Min_pos");
            slave_SDO_cmd->add_rd_sdo("Max_pos");
            
            pb_msg.set_allocated_slave_sdo_cmd(slave_SDO_cmd);

            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
            checkReply(pb_reply);
        }



        ++idx;
        
        publisher.disconnect(uri);
        publisher.close();
        if (idx == NUMOP) {
        break;
        }

        
    }

    return 0;
}