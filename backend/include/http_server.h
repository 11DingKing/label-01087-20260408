#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "common.h"
#include "parking_lot.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>

struct WebSocketEvent {
    std::string type;
    std::string plate;
    std::string spotId;
    std::string timestamp;
    double fee;
    
    std::string toJson() const {
        std::stringstream ss;
        ss << "{";
        ss << "\"type\":\"" << type << "\",";
        ss << "\"plate\":\"" << plate << "\",";
        ss << "\"spotId\":\"" << spotId << "\",";
        ss << "\"timestamp\":\"" << timestamp << "\"";
        if (fee >= 0) {
            ss << ",\"fee\":" << std::fixed << std::setprecision(2) << fee;
        }
        ss << "}";
        return ss.str();
    }
};

class HttpServer {
public:
    HttpServer(ParkingLot* parkingLot, int port = 8080);
    ~HttpServer();
    
    void start();
    void stop();
    void broadcastEvent(const WebSocketEvent& event);
    
private:
    ParkingLot* parkingLot_;
    int port_;
    bool running_;
    std::thread serverThread_;
    
    std::mutex clientsMutex_;
    std::map<int, std::function<void(const std::string&)>> clients_;
    int nextClientId_;
    
    std::mutex eventMutex_;
    std::condition_variable eventCV_;
    std::queue<WebSocketEvent> eventQueue_;
    std::thread broadcastThread_;
    
    void runServer();
    void broadcastLoop();
    
    std::string getStatusJson();
    std::string getRecordsJson(int limit = 20);
    std::string getFloorJson(int floorIndex);
};

#endif // HTTP_SERVER_H
