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
        //DPRINTF("[%s]\twrite %d bytes to %s\n", who.c_str(), nbytes, ddp.get_pipe_path().c_str());
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
    uint64_t            loop_cnt,pipe_mechanism_timer,wait_start_motor;
    
    std::string         rd_pipe, wr_pipe;
    XDDP_pipe           rd_xddp, wr_xddp;
    
    std::vector<std::tuple<int, std::string, std::string>> rd_wr_motor_pdo;
    std::map<int, IDDP_pipe>    rd_iddp, wr_iddp;
    
    iit::advr::Cmd_reply                    pb_cmd_reply;
    iit::advr::Repl_cmd                     pb_repl_cmd;
    std::map<int, iit::advr::Ec_slave_pdo>  pb_rx_pdos;
    std::map<int, iit::advr::Ec_slave_pdo>  pb_tx_pdos;
    
    uint8_t                                 pb_buf[MAX_PB_SIZE]; 

    YAML::Node slaves_info;
    std::map<int,std::map<std::string,int>> slave_info_map;
    string pipe_name="NoNe@Motor_id_",rpipe_name,wpipe_name;
    int STATUS;
    
    
    Repl_cmd  pb_cmd;
    bool start_traj,start_motor;
    
    std::map<int,float> start_pos,ref;
    
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
    
    void  set_dummy_pdo(int id)
    {
        static struct timespec ts;
        auto pb = &pb_tx_pdos[id];
        
        clock_gettime(CLOCK_MONOTONIC, &ts);
        // Header
        pb->mutable_header()->mutable_stamp()->set_sec(ts.tv_sec);
        pb->mutable_header()->mutable_stamp()->set_nsec(ts.tv_nsec);
        // Type
        pb->set_type(iit::advr::Ec_slave_pdo::DUMMY);
        // 
        //pb->mutable_dummy_pdo()->set_vect_f(0,0);

      
        
    }
    
    void iddp_pipe_init ()
    {
        // bind rd iddp
        for ( const auto& [id, pipe_name, not_used] : rd_wr_motor_pdo ) {
            if(id==15)
            {
            rd_iddp[id] = IDDP_pipe();
            rd_iddp[id].bind(pipe_name, POOL_SIZE);
            }
        }
    
        // 
        for ( const auto& [id, pipe_name, not_used] : rd_wr_motor_pdo ) {
            if((id==15))
            {
            pb_rx_pdos[id] = iit::advr::Ec_slave_pdo();
            pb_tx_pdos[id] = iit::advr::Ec_slave_pdo();
            }
            
        }
        
        // connect wr iddp
        for ( const auto& [id, not_used, pipe_name] : rd_wr_motor_pdo ) {
            if(id==15)
            {
            wr_iddp[id] = IDDP_pipe();
            wr_iddp[id].connect(pipe_name);
            start_pos[id]=0.0;
            ref[id]=0.0;
            }
        }
        

        // sanity check
        DPRINTF("Sizes: [%ld] [%ld] [%ld] [%ld]", rd_iddp.size(),wr_iddp.size(),pb_tx_pdos.size(),pb_rx_pdos.size());
        //assert(rd_iddp.size() == wr_iddp.size() == pb_tx_pdos.size() == pb_rx_pdos.size());    
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
        pipe_mechanism_timer=0;
        wait_start_motor=0;
        
        pb_cmd_reply.Clear();
        pb_repl_cmd.Clear();
        
        // bind xddp pipes
        rd_xddp.bind(rd_pipe);
        wr_xddp.bind(wr_pipe);
        
        pthread_barrier_wait(&threads_barrier);
        
        
       STATUS=0;
    }
    
    virtual void th_loop ( void * )
    {
        int rd_bytes_from_nrt, rd_bytes_from_rt, wr_bytes_to_rt;
        int msg_size;
        
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
                        slave_info_map=slaves_info.as<std::map<int,std::map<std::string,int>>>();
                        rpipe_name="";
                        wpipe_name="";
                        for(auto [key,val]:slave_info_map)
                        {
                            for(auto [ikey,ival]:val)
                            {
                                if(ival==21)
                                {
                                    rpipe_name=pipe_name+to_string(key)+"_tx_pdo";
                                    wpipe_name=pipe_name+to_string(key)+"_rx_pdo";
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
         
            STATUS=2;
            iddp_pipe_init();
            pipe_mechanism_timer=0;
            start_traj=false;
            start_motor=false;
        }
        

        
        if(STATUS==2)
        {
        
            // *******************SENSE*********************************         
            
            for ( auto& [id, pipe] : rd_iddp ){
                    do {
                        rd_bytes_from_rt = read_pb_from(pipe, pb_buf, &pb_rx_pdos[id], name);
                        // at least check faults on multiple reads ...
                    } while (rd_bytes_from_rt > 0);
            }
            
            for ( auto& [id,not_used]: pb_rx_pdos){
                
                if(!start_traj)
                {
                    start_pos[id] = pb_rx_pdos[id].mutable_motor_xt_rx_pdo()->motor_pos();
                    ref[id] = start_pos[id];
                } 

                if(start_traj)
                {
                    if(id==15)
                    {
                        if ( ref[id] > start_pos[id] + 0.5) {
                            ds = -0.001;    
                        }
                        if ( ref[id] < start_pos[id] - 0.5) {
                            ds = 0.001;
                        }
                        ref[id] += ds; 
                    }
                }
                
                set_motor_pdo(id, ref[id]);
            }

            // *******************MOVE********************************* 

            for ( auto& [id, pipe] : wr_iddp ){ 
                write_pb_to(pipe, pb_buf, &pb_tx_pdos[id], name);
            }
        
            if(pipe_mechanism_timer>1000)
                start_motor=true;
            else
                pipe_mechanism_timer++;
             
        }

        //if ( rd_bytes_from_nrt > 0 ) 
        if(start_motor){
            pb_repl_cmd.set_type(CmdType::CTRL_CMD);
            pb_repl_cmd.mutable_ctrl_cmd()->set_type(Ctrl_cmd::CTRL_CMD_START);
            pb_repl_cmd.mutable_ctrl_cmd()->set_board_id(11);
            // 3 : write to NRT
            write_pb_to(wr_xddp, pb_buf, &pb_repl_cmd, name);
        }
         
        if(pb_cmd_reply.type()==Cmd_reply::ACK)
        {
            if(pb_cmd_reply.cmd_type()==CmdType::CTRL_CMD)
            {
                start_traj=true;
                start_motor=false;
            }
                
        }
    }
    
};

#endif
