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

#include <assert.h>

#include <yaml-cpp/yaml.h>

#include "cxxopts.hpp"
#include <test_common.h>

#include "rt_thread.h"
#include "zmq_thread.h"
#include <random>

/*
 * Use protobuf msg iit::advr::Repl_cmd
 * - set type ad iit::advr::CmdType::CTRL_CMD
 * - set header
 * 
 */
#define SIG_TEST 

std::random_device rd;     // only used once to initialise (seed) engine
std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
//std::uniform_int_distribution<int> uni(0,10); // guaranteed unbiased
std::uniform_real_distribution<double> uni(0,10);

static int loop_guard = 1;
bool verbose=0;

static const std::string rt2nrt_pipe( "RT2NRT" );
static const std::string nrt2rt_pipe( "NRT2RT" );



ThreadsMap threads;
    
static void test_sighandler(int signum) {
    
    std::cout << "Handling signal " << signum << std::endl;
    loop_guard = 0;
}




////////////////////////////////////////////////////
//
// Main
//
////////////////////////////////////////////////////

int main ( int argc, char * argv[] ) try {

    cxxopts::Options options(argv[0], " - wizardry setup");
    int num_wrk;
    int rt_th_period_us;
    
    try
    {
        options.add_options()
            ("v, verbose", "verbose", cxxopts::value<bool>()->implicit_value("true"));
        options.add_options()
            ("p, rt_th_period_us", "rt_th_period_us", cxxopts::value<int>()->default_value("250000"));
        options.parse(argc, argv);

        rt_th_period_us = options["rt_th_period_us"].as<int>();
        verbose = options["verbose"].as<bool>();
        verbose=0;
    }
    catch (const cxxopts::OptionException& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }

#ifdef SIG_TEST
    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    main_common (&argc, (char*const**)&argv, 0);
#else
    main_common (&argc, (char*const**)&argv, test_sighandler);
#endif
    
    
    std::string th_name = "ZMQ_thread";
    threads[th_name] = new zmq_thread(th_name, rt2nrt_pipe, nrt2rt_pipe);
    
    
    
    th_name = "RT_thread";
    //const std::vector<std::tuple<int, std::string, std::string>> rdwr {std::make_tuple(1, rd_pdo_pipe, wr_pdo_pipe)};
    threads[th_name] = new RT_motor_thread(th_name,nrt2rt_pipe, rt2nrt_pipe,rt_th_period_us);
        
    /*
     * The barrier is opened when COUNT waiters arrived.
     */
    pthread_barrier_init(&threads_barrier, NULL, threads.size()+1 );    

    threads["RT_thread"]->create(true);
    threads["ZMQ_thread"]->create(false);

    std::cout << "... wait on barrier" << std::endl;
    //sleep(3);
    pthread_barrier_wait(&threads_barrier);

#ifdef SIG_TEST
    #ifdef __COBALT__
        // here I want to catch CTRL-C 
        __real_sigwait(&set, &sig);
    #else
        sigwait(&set, &sig);  
    #endif
#else
    while ( loop_guard ) {
        sleep(1);
    }
#endif

    threads["ZMQ_thread"]->stop();
    threads["RT_thread"]->stop();
    
    for ( auto const &t : threads) {
        //t.second->stop();
        t.second->join();
        delete t.second;
    }
    
    std::cout << "Exit main" << std::endl;

    return 0;

} catch ( std::exception& e ) {

    std::cout << "Main catch .... " <<  e.what() << std::endl;

}


// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
