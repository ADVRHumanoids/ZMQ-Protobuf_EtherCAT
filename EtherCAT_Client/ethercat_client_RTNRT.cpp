#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <rtdm/ipc.h>


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

pthread_t rt1, nrt;
#define XDDP_PORT_LABEL  "xddp-demo"

char uri[]= "tcp://advantech.local:5555";

using namespace iit::advr;
using namespace zmq;
using namespace std;


static void fail(const char *reason)
{
        perror(reason);
        exit(EXIT_FAILURE);
}
static void *realtime_thread(void *arg)
{
        struct rtipc_port_label plabel;
        struct sockaddr_ipc saddr;
        char buf[128];
        int ret, s;
        /*
         * Get a datagram socket to bind to the RT endpoint. Each
         * endpoint is represented by a port number within the XDDP
         * protocol namespace.
         */
        s = socket(AF_RTIPC, SOCK_DGRAM, IPCPROTO_XDDP);
        if (s < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
        }
        /*
         * Set a port label. This name will be registered when
         * binding, in addition to the port number (if given).
         */
        strcpy(plabel.label, XDDP_PORT_LABEL);
        ret = setsockopt(s, SOL_XDDP, XDDP_LABEL,
                         &plabel, sizeof(plabel));
        if (ret)
                fail("setsockopt");
        /*
         * Bind the socket to the port, to setup a proxy to channel
         * traffic to/from the Linux domain. Assign that port a label,
         * so that peers may use a descriptive information to locate
         * it. For instance, the pseudo-device matching our RT
         * endpoint will appear as
         * /proc/xenomai/registry/rtipc/xddp/<XDDP_PORT_LABEL> in the
         * Linux domain, once the socket is bound.
         *
         * saddr.sipc_port specifies the port number to use. If -1 is
         * passed, the XDDP driver will auto-select an idle port.
         */
        memset(&saddr, 0, sizeof(saddr));
        saddr.sipc_family = AF_RTIPC;
        saddr.sipc_port = -1;
        ret = bind(s, (struct sockaddr *)&saddr, sizeof(saddr));
        if (ret)
                fail("bind");
        for (;;) {
                /* Get packets relayed by the regular thread */
                ret = recvfrom(s, buf, sizeof(buf), 0, NULL, 0);
                if (ret <= 0)
                        fail("recvfrom");
                printf("%s: \"%.*s\" relayed by peer\n", __FUNCTION__, ret, buf);
        }
        return NULL;
}

static void *regular_thread(void *arg)
{
        char *devname;
        int fd, ret;
        if (asprintf(&devname, "/proc/xenomai/registry/rtipc/xddp/%s",XDDP_PORT_LABEL) < 0)
                fail("asprintf");
        fd = open(devname, O_WRONLY | O_NONBLOCK);
        free(devname);
        
        std::string m_cmd="MASTER_CMD";
        Ecat_Master_cmd *ecat_master_cmd= new Ecat_Master_cmd();
        Repl_cmd pb_msg;
        std::string pb_msg_serialized;
        multipart_t multipart;
        zmq::message_t update;
        Cmd_reply pb_reply;
        
        if (fd < 0)
                fail("open");
        
        for (;;) {
            
            zmq::context_t context{1};
            zmq::socket_t publisher{context, ZMQ_REQ};
        
            publisher.connect(uri);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            pb_msg.set_type(CmdType::ECAT_MASTER_CMD);
            ecat_master_cmd->set_type(Ecat_Master_cmd::GET_SLAVES_DESCR);
            pb_msg.set_allocated_ecat_master_cmd(ecat_master_cmd);
            pb_msg.SerializeToString(&pb_msg_serialized);
            multipart.push(message_t(pb_msg_serialized.c_str(), pb_msg_serialized.length()));
            multipart.push(message_t(m_cmd.c_str(), m_cmd.length()));
            multipart.send(publisher);
            
            publisher.recv(&update);
            pb_reply.ParseFromString(update.to_string());
       
            
            /* Relay the message to realtime_thread1. */
            ret = write(fd, pb_reply.msg().c_str(), pb_reply.msg().size());
            if (ret <= 0)
                    fail("write");
        }
        return NULL;
}
int main(int argc, char **argv)
{
        struct sched_param rtparam = { .sched_priority = 42 };
        pthread_attr_t rtattr, regattr;
        sigset_t set;
        int sig;
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        sigaddset(&set, SIGTERM);
        sigaddset(&set, SIGHUP);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        pthread_attr_init(&rtattr);
        pthread_attr_setdetachstate(&rtattr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setinheritsched(&rtattr, PTHREAD_EXPLICIT_SCHED);
#ifdef __COBALT__
        pthread_attr_setschedpolicy(&rtattr, SCHED_FIFO);
#else
        pthread_attr_setschedpolicy(&rtattr, SCHED_OTHER);
#endif
        pthread_attr_setschedparam(&rtattr, &rtparam);
        /* Both real-time threads have the same attribute set. */
        errno = pthread_create(&rt1, &rtattr, &realtime_thread, NULL);
        if (errno)
                fail("pthread_create");

        pthread_attr_init(&regattr);
        pthread_attr_setdetachstate(&regattr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setinheritsched(&regattr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&regattr, SCHED_OTHER);
        errno = pthread_create(&nrt, &regattr, &regular_thread, NULL);
        if (errno)
                fail("pthread_create");
        sigwait(&set, &sig);
        pthread_cancel(rt1);
        pthread_cancel(nrt);
        pthread_join(rt1, NULL);
        pthread_join(nrt, NULL);
        return 0;
}