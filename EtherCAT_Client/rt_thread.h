#ifndef __RT_THREAD__
#define __RT_THREAD__

#include <utils.h>
#include <pipes.h>
#include <thread_util.h>

#include <repl_cmd.pb.h>
#include <ecat_pdo.pb.h>

#define POOL_SIZE   4096
#define MAX_PB_SIZE 4096
#define MAX_WRK     64

extern bool verbose;
using namespace std;
using namespace iit::advr;

static int read_pb_from(pipe_base &ddp, uint8_t *buff, ::google::protobuf::Message *pb_msg, std::string who)
{
    uint32_t msg_size;
    int32_t  nbytes;

    nbytes = ddp.ddp_read( msg_size );
    if ( nbytes > 0 ) {
        //pb_msg.Clear();
        nbytes += ddp.ddp_read ( buff, msg_size );
        pb_msg->ParseFromArray(buff, msg_size);
        //DPRINTF("[%s]\tread %d bytes from %s\n", who.c_str(), nbytes, ddp.get_pipe_path().c_str());
        if ( verbose ) {
            DPRINTF("[%s] read msg :\n%s\n", who.c_str(), pb_msg->DebugString().c_str());
        }
    }
    return nbytes;
}

static int write_pb_to(pipe_base &ddp, uint8_t *buff, ::google::protobuf::Message *pb_msg, std::string who)
{
    uint32_t msg_size;
    int32_t  nbytes;
    
    msg_size = pb_msg->ByteSize();
    if ( msg_size > MAX_PB_SIZE ) {
        return -1;
    }
    pb_msg->SerializeToArray( (void*)buff, msg_size);
    //pb_msg.SerializeToString(pb_str);
    //msg_size = pb_str.length();
    
    nbytes = ddp.ddp_write ( ( void* )&msg_size, sizeof( msg_size ) );
    if ( nbytes > 0 ) { 
        nbytes += ddp.ddp_write ( (void*)buff, msg_size );
        DPRINTF("[%s]\twrite %d bytes to %s\n", who.c_str(), nbytes, ddp.get_pipe_path().c_str());
        if ( verbose ) {
            DPRINTF("[%s] write msg :\n%s\n", who.c_str(), pb_msg->DebugString().c_str());
        }
    }

    return nbytes;
}

////////////////////////////////////////////////////
// 
// 
//
////////////////////////////////////////////////////
class RT_motor_thread : public Thread_hook {

private:
    
    iit::ecat::stat_t   s_loop;
    uint64_t            start_time, tNow, tPre;
    uint64_t            loop_cnt;
    
    std::string         rd_pipe, wr_pipe;
    XDDP_pipe           rd_xddp, wr_xddp;
    
    std::vector<std::tuple<int, std::string, std::string>> rd_wr_motor_pdo;
    std::map<int, IDDP_pipe>    rd_iddp, wr_iddp;
    
    iit::advr::Cmd_reply                    pb_cmd_reply;
    std::map<int, iit::advr::Ec_slave_pdo>  pb_rx_pdos;
    std::map<int, iit::advr::Ec_slave_pdo>  pb_tx_pdos;
    uint8_t                                 pb_buf[MAX_PB_SIZE];   
    YAML::Node slaves_info;
    int STATUS,id_start;
    
    Repl_cmd  pb_cmd;
    
    void  set_motor_pdo(int id, float pos)
    {
        static struct timespec ts;
        auto pb = &pb_tx_pdos[id];
        
        clock_gettime(CLOCK_MONOTONIC, &ts);
        // Header
        pb->mutable_header()->mutable_stamp()->set_sec(ts.tv_sec);
        pb->mutable_header()->mutable_stamp()->set_nsec(ts.tv_nsec);
        // Type
        pb->set_type(iit::advr::Ec_slave_pdo::TX_XT_MOTOR);
        // 
        pb->mutable_motor_xt_tx_pdo()->set_pos_ref(pos);
        pb->mutable_motor_xt_tx_pdo()->set_tor_ref(0.0);
        pb->mutable_motor_xt_tx_pdo()->set_vel_ref(0.0);
        pb->mutable_motor_xt_tx_pdo()->set_gain_0 (0.01);
        pb->mutable_motor_xt_tx_pdo()->set_gain_1 (0.01);
        pb->mutable_motor_xt_tx_pdo()->set_gain_2 (0.01);
        pb->mutable_motor_xt_tx_pdo()->set_gain_3 (0.01);
        pb->mutable_motor_xt_tx_pdo()->set_gain_4 (0.01);
        pb->mutable_motor_xt_tx_pdo()->set_fault_ack(0);
        pb->mutable_motor_xt_tx_pdo()->set_ts(uint32_t(iit::ecat::get_time_ns()/1000));        
        
    }
    
    void iddp_pipe_init ()
    {

       string pipe_name,not_used;
       int id;
      
       // bind rd iddp
       for (std::vector<std::tuple<int, std::string, std::string>>::const_iterator i = rd_wr_motor_pdo.begin(); i != rd_wr_motor_pdo.end(); ++i) {
            id=         get<0>(*i);
            pipe_name=  get<1>(*i);
            not_used=   get<2>(*i);
            rd_iddp[id] = IDDP_pipe();
            rd_iddp[id].bind(pipe_name, POOL_SIZE);
       }
        
        // 
        for (std::vector<std::tuple<int, std::string, std::string>>::const_iterator i = rd_wr_motor_pdo.begin(); i != rd_wr_motor_pdo.end(); ++i) {
            id=         get<0>(*i);
            pipe_name=  get<1>(*i);
            not_used=   get<2>(*i);
            pb_rx_pdos[id] = iit::advr::Ec_slave_pdo();
            pb_tx_pdos[id] = iit::advr::Ec_slave_pdo();
        }
        

        // connect wr iddp
        for (std::vector<std::tuple<int, std::string, std::string>>::const_iterator i = rd_wr_motor_pdo.begin(); i != rd_wr_motor_pdo.end(); ++i) {
            id=        get<0>(*i);
            not_used=  get<1>(*i);
            pipe_name= get<2>(*i);
            wr_iddp[id] = IDDP_pipe();
            wr_iddp[id].connect(pipe_name);
        }

        // sanity check
        assert(pb_tx_pdos.size() == pb_rx_pdos.size());     
    }
    
    
       
public:

    RT_motor_thread(std::string _name,
                    std::string rd,
                    std::string wr,
                    uint32_t th_period_us ) :
     rd_pipe(rd), wr_pipe(wr)
    {
        name = _name;
        // periodic
        struct timespec ts;
        iit::ecat::us2ts(&ts, th_period_us);
        // period.period is a timeval ... tv_usec 
        period.period = { ts.tv_sec, ts.tv_nsec / 1000 };
#ifdef __COBALT__
        schedpolicy = SCHED_FIFO;
#else
        schedpolicy = SCHED_OTHER;
#endif
        priority = sched_get_priority_max ( schedpolicy ) / 2;
        stacksize = 0; // not set stak size !!!! YOU COULD BECAME CRAZY !!!!!!!!!!!!
        
       
       string rd_name,wr_name;
       int id;
      
    }

    ~RT_motor_thread()
    { 
        iit::ecat::print_stat ( s_loop );
    }

    virtual void th_init ( void * )
    {
        start_time = iit::ecat::get_time_ns();
        tNow, tPre = start_time;
        loop_cnt = 0;
        
        // bind xddp pipes
        rd_xddp.bind(rd_pipe);
        wr_xddp.bind(wr_pipe);
        
        pthread_barrier_wait(&threads_barrier);
        
       string pipe_name,not_used;
       int id;  
       STATUS=0;
    }
    
    virtual void th_loop ( void * )
    {
        int rd_bytes_from_nrt, rd_bytes_from_rt, wr_bytes_to_rt;
        int msg_size;
        static float start_pos, ref;
        static float ds = 0.001;
        
        tNow = iit::ecat::get_time_ns();
        s_loop ( tNow - tPre );
        tPre = tNow;
        
        loop_cnt++;
        
        // 2 : read from NRT
        rd_bytes_from_nrt = read_pb_from(rd_xddp, pb_buf, &pb_cmd_reply, name);

        if(STATUS==0)  // status check ids for motor 
        {
            if ( rd_bytes_from_nrt > 0 )
            {
                if(pb_cmd_reply.type()==Cmd_reply::ACK)
                {
                    if(pb_cmd_reply.cmd_type()==CmdType::ECAT_MASTER_CMD)
                    {
                        slaves_info = YAML::Load(pb_cmd_reply.msg().c_str());
                        
                        auto slave_info_map=slaves_info.as<std::map<int,std::map<std::string,int>>>();
                        string pipe_name="NoNe@Motor_id_";
                        string rpipe_name="";
                        string wpipe_name="";

                        for(auto [key,val]:slave_info_map)
                        {
                            for(auto [ikey,ival]:val)
                            {
                                if(ival==21)
                                {
                                    rpipe_name=pipe_name+to_string(key)+"_tx_pdo";
                                    wpipe_name=pipe_name+to_string(key)+"_rx_pdo";
                                    cout << "Creating reading pipe: " << rpipe_name << endl;
                                    cout << "Creating writing pipe: " << wpipe_name << endl;
                                    rd_wr_motor_pdo.push_back(std::make_tuple(key, rpipe_name, wpipe_name));
                                }
                            }
                        }
                        
                        STATUS=1;    
                    }
                } 
            }    
        }
        
        if(STATUS==1)
        {
          iddp_pipe_init();
          STATUS=2;
          id_start=0;
        }
        
        if(STATUS==2)
        {
            if(pb_cmd_reply.type()==Cmd_reply::ACK)
                    if(pb_cmd_reply.cmd_type()==CmdType::CTRL_CMD)
                        id_start=id_start+1;
                    
            if(id_start<rd_wr_motor_pdo.size())
            {
                int id= get<0>(rd_wr_motor_pdo[id_start]);
                pb_cmd.set_type(CmdType::CTRL_CMD);
                pb_cmd.mutable_ctrl_cmd()->set_type(Ctrl_cmd::CTRL_CMD_START);
                pb_cmd.mutable_ctrl_cmd()->set_board_id(id);
                //pb_cmd.mutable_ctrl_cmd()->set_value(0x3B);
                write_pb_to(wr_xddp, pb_buf, &pb_cmd, name);
            }
            else
            {
                STATUS=3;
                cout << "-------------ALL MOTORS STARTED------" << endl;
            }
 
        }
//         IDDP_pipe pipe;
//         int id;
//         
//         // *******************SENSE*********************************         
//         
//         //for ( auto& [id, pipe] : rd_iddp )
//         for ( auto const& item : rd_iddp){
//             id=item.first; 
//             pipe=item.second;
//             do {
//                 rd_bytes_from_rt = read_pb_from(pipe, pb_buf, &pb_rx_pdos[id], name);
//                 // at least check faults on multiple reads ...
//             } while (rd_bytes_from_rt > 0);
//         }
//         
//         if ( ! start_pos ) {
//             start_pos = pb_rx_pdos[1].mutable_motor_xt_rx_pdo()->motor_pos();
//             ref = start_pos;
//         }
// 
//         if ( ref > start_pos + 0.5) {
//             ds = -0.001;    
//         }
//         if ( ref < start_pos - 0.5) {
//             ds = 0.001;
//         }
//         ref += ds; 
//         
//         //std::map<int, iit::advr::Ec_slave_pdo> 
//         
//         //for ( auto& [id, pb] : pb_rx_pdos )
//         for ( auto const& item : pb_rx_pdos){
//             id=item.first; 
//             set_motor_pdo(id, ref );
//         }
// 
//         //std::map<int, IDDP_pipe>    rd_iddp, wr_iddp;
//         
//        
//         // *******************MOVE********************************* 
//         
// 
//         //for ( auto& [id, pipe] : wr_iddp ) 
//         for ( auto const& item : wr_iddp){
//             id=item.first; 
//             pipe=item.second;
//             write_pb_to(pipe, pb_buf, &pb_tx_pdos[id], name);
//         }
//             
//         if ( rd_bytes_from_nrt > 0 ) {
//             // 3 : write to NRT
//             //write_pb_to(wr_xddp, pb_buf, &pb_repl_cmd, name);
//         }
        
    }

    
};

#endif
