#ifndef __zmq_THREAD__
#define __zmq_THREAD__

#include <utils.h>
#include <pipes.h>
#include <thread_util.h>

#include <repl_cmd.pb.h>
#include <ecat_pdo.pb.h>

#include "zmq.hpp"
#include <zmq_addon.hpp>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

char uri[]= "tcp://hhcm-mio.local:5555";
using namespace zmq;
using namespace std;
using namespace iit::advr;
////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////
class zmq_thread : public Thread_hook {

private:
    iit::ecat::stat_t   s_loop;
    uint64_t            start_time, tNow, tPre;
    uint64_t            loop_cnt;
    
    int                 wr_xddp, rd_xddp;
    std::string         rd_pipe_path, wr_pipe_path;
    
    iit::advr::Repl_cmd pb_repl_cmd;
    uint8_t             pb_buf[MAX_PB_SIZE];
    int timeout;    
    int STATUS;
    std::string m_cmd;
    std::string pb_msg_serialized;
    multipart_t multipart;
    
    Repl_cmd  pb_cmd;
    Cmd_reply pb_reply;
    
    int32_t write_to_RT(::google::protobuf::Message *pb_msg)
    {
        uint32_t msg_size;
        int32_t  nbytes;
        
        msg_size = pb_msg->ByteSize();
        if ( msg_size > MAX_PB_SIZE ) {
            return -1;
        }

        pb_msg->SerializeToArray( (void*)pb_buf, msg_size);
        
        nbytes = write ( wr_xddp, (void*)&msg_size, sizeof(msg_size) );
        if ( nbytes > 0 ) {
            nbytes += write ( wr_xddp, (void*)pb_buf, msg_size );
            //DPRINTF("[NRT]\t%lu loop \twrite %d bytes", loop_cnt, nbytes);
            if ( verbose ) {
                DPRINTF(" to %s\n", wr_pipe_path.c_str());
                DPRINTF("[NRT] write msg :\n%s\n", pb_msg->DebugString().c_str());
            }
            DPRINTF("\n");
        }
        return nbytes;
    }
    
    int32_t read_from_RT(::google::protobuf::Message *pb_msg)
    {
        uint32_t msg_size;
        int32_t  nbytes;

        nbytes = read ( rd_xddp, (void*)&msg_size, sizeof(msg_size) );
        if ( nbytes > 0 ) {
            pb_msg->Clear();
            nbytes += read ( rd_xddp, (void*)pb_buf, msg_size );
            pb_msg->ParseFromArray(pb_buf, msg_size);
            //DPRINTF("[NRT]\t%lu loop \tread %d bytes", loop_cnt, nbytes);
            if ( verbose ) {
                DPRINTF(" from %s\n", rd_pipe_path.c_str());
                DPRINTF("[NRT] read msg :\n%s", pb_msg->DebugString().c_str());
            }
            DPRINTF("\n");
        } 
        return nbytes;
    }

    
public:

    zmq_thread(std::string _name, std::string rd, std::string wr) :
    rd_pipe_path(pipe_prefix + "xddp/" + rd),
    wr_pipe_path(pipe_prefix + "xddp/" + wr)
    {
        name = _name;
        // non periodic
        period.period = {0,1};
        schedpolicy = SCHED_OTHER;
        priority = sched_get_priority_max ( schedpolicy ) / 2;
        stacksize = 0; // not set stak size !!!! YOU COULD BECAME CRAZY !!!!!!!!!!!!
    }

    ~zmq_thread()
    {
        close(wr_xddp);
        close(rd_xddp);
        iit::ecat::print_stat ( s_loop );
    }

    virtual void th_init ( void * )
    {
        rd_xddp = open ( rd_pipe_path.c_str(), O_RDONLY );
        wr_xddp = open ( wr_pipe_path.c_str(), O_WRONLY | O_NONBLOCK );
        
        if ( rd_xddp <= 0 || wr_xddp <= 0 ) {
            assert (0);
        }
        start_time = iit::ecat::get_time_ns();
        tNow, tPre = start_time;
        loop_cnt = 0;
        
        
        pthread_barrier_wait(&threads_barrier);
        /// ZMQ Setup
        timeout=10000; 
        STATUS=0;
    }
    
    virtual void th_loop ( void * )
    {
        tNow = iit::ecat::get_time_ns();
        s_loop ( tNow - tPre );
        tPre = tNow;
        
        loop_cnt++;
        zmq::context_t context{1};
        zmq::socket_t publisher{context, ZMQ_REQ};
        zmq::message_t update;
        
        int32_t nbytes_wr, nbytes_rd;
        
        //ZMQ Connection 
        publisher.setsockopt(ZMQ_RCVTIMEO,timeout);
        publisher.connect(uri);
        
        // ID's Getting STATUS
        if(STATUS==0)
        {
            m_cmd="MASTER_CMD";
            pb_cmd.set_type(CmdType::ECAT_MASTER_CMD);
            pb_cmd.mutable_ecat_master_cmd()->set_type(Ecat_Master_cmd::GET_SLAVES_DESCR);
            pb_cmd.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher,ZMQ_NOBLOCK);
        
            if(publisher.recv(&update))
            {
                STATUS=1;
                pb_reply.ParseFromArray(update.data(),update.size());
            }
        }
        // SEND ID's to RT Thread STATUS    
        
        if(STATUS==1)
        {
            ///////////////////////////////////////////////////////////////////////
            // 1 : write to RT
            nbytes_wr = write_to_RT(&pb_reply);
            ///////////////////////////////////////////////////////////////////////
            STATUS=2;
        }
        
        if(STATUS==2)
        {
            // 4 : read from RT -- BLOCKING
            nbytes_rd = read_from_RT(&pb_cmd);
            m_cmd="ESC_CMD";
            pb_cmd.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher,ZMQ_NOBLOCK);
            if(publisher.recv(&update))
            {
                pb_reply.ParseFromArray(update.data(),update.size());
                STATUS=1;
            }
        
        }
        
        //if ( nbytes_rd > 0 && nbytes_wr > 0 ) {
        //    DPRINTF("_-_-_-_-_-_-_-_-_-_-_-_-_-_-_\n");
        //}
        
        //ZMQ Disconnection 
        publisher.disconnect(uri);
    }


};


#endif
