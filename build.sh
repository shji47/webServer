g++ main.cpp ./common/util.cpp ./config/config.cpp ./log/log.cpp ./mysqlPool/mysqlPool.cpp ./server/webServer.cpp ./http/httpConnection.cpp ./threadPool/threadPool.cpp -o webserver -lpthread -lmysqlclient