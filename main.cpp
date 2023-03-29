#include <iostream>

#include "config/config.h"
#include "server/webServer.h"

using namespace std;

int main() {
    string usr = "root";
    string passwd = "root";
    string dbname = "sj";

    Config *cfg = Config::singleton();
    WebServer server;

    server.init(cfg->m_server_port, usr, passwd, dbname, cfg->m_max_sql_num, 
    cfg->m_max_thread_num, cfg->m_close_log, cfg->m_async_log, cfg->m_linger, cfg->m_cgi,
    cfg->m_listen_trig_mode, cfg->m_conn_trig_mode, cfg->m_actor_mode, cfg->m_time_out);
    // cout<<"Server init successful"<<endl;

    server.log_init();
    // cout<<"Log init successful"<<endl;

    server.sql_pool_init();
    
    // cout<<"Sql Pool init successful"<<endl;

    server.thread_pool_init();
    // cout<<"Thread Pool init successful"<<endl;

    server.server_listen();
    cout<<"Server Listening..."<<endl;

    server.server_loop();
    // cout<<"Server exit..."<<endl;

    return 0;
}