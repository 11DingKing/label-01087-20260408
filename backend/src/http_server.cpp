#include "../include/http_server.h"
#include <sstream>
#include <iomanip>
#include <set>
#include <atomic>

#include "../include/httplib.h"

static std::set<httplib::ws::WebSocket*> g_ws_clients;
static std::mutex g_ws_mutex;
static std::atomic<bool> g_server_running{true};

HttpServer::HttpServer(ParkingLot* parkingLot, int port)
    : parkingLot_(parkingLot), port_(port), running_(false), nextClientId_(0) {
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    running_ = true;
    g_server_running = true;
    broadcastThread_ = std::thread(&HttpServer::broadcastLoop, this);
    serverThread_ = std::thread(&HttpServer::runServer, this);
    LOG_INFO("HTTP 服务器已启动，端口: " + std::to_string(port_));
}

void HttpServer::stop() {
    running_ = false;
    g_server_running = false;
    eventCV_.notify_all();
    
    {
        std::lock_guard<std::mutex> lock(g_ws_mutex);
        for (auto ws : g_ws_clients) {
            try {
                ws->close();
            } catch (...) {}
        }
        g_ws_clients.clear();
    }
    
    if (broadcastThread_.joinable()) {
        broadcastThread_.join();
    }
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    LOG_INFO("HTTP 服务器已停止");
}

void HttpServer::broadcastEvent(const WebSocketEvent& event) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    eventQueue_.push(event);
    eventCV_.notify_one();
}

void HttpServer::broadcastLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(eventMutex_);
        eventCV_.wait(lock, [this] { return !eventQueue_.empty() || !running_; });
        
        while (!eventQueue_.empty() && running_) {
            WebSocketEvent event = eventQueue_.front();
            eventQueue_.pop();
            lock.unlock();
            
            std::string json = event.toJson();
            LOG_INFO("广播事件: " + json);
            
            std::lock_guard<std::mutex> ws_lock(g_ws_mutex);
            for (auto it = g_ws_clients.begin(); it != g_ws_clients.end();) {
                auto ws = *it;
                try {
                    if (ws->is_open()) {
                        ws->send(json);
                        ++it;
                    } else {
                        it = g_ws_clients.erase(it);
                    }
                } catch (...) {
                    it = g_ws_clients.erase(it);
                }
            }
            
            lock.lock();
        }
    }
}

std::string HttpServer::getStatusJson() {
    std::stringstream ss;
    ss << "{";
    ss << "\"name\":\"" << parkingLot_->getName() << "\",";
    ss << "\"totalSpots\":" << parkingLot_->getTotalSpots() << ",";
    ss << "\"availableSpots\":" << parkingLot_->getAvailableCount() << ",";
    ss << "\"occupiedSpots\":" << parkingLot_->getOccupiedCount() << ",";
    ss << "\"waitingCount\":" << parkingLot_->getWaitingCount() << ",";
    ss << "\"floorCount\":" << parkingLot_->getFloorCount() << ",";
    
    ss << "\"floors\":[";
    for (int i = 0; i < parkingLot_->getFloorCount(); ++i) {
        Floor* floor = parkingLot_->getFloor(i);
        if (i > 0) ss << ",";
        ss << "{";
        ss << "\"floorId\":" << floor->getFloorId() << ",";
        ss << "\"zoneName\":\"" << floor->getZoneName() << "\",";
        ss << "\"totalSpots\":" << floor->getTotalSpots() << ",";
        ss << "\"availableSpots\":" << floor->getAvailableCount() << ",";
        ss << "\"rows\":" << floor->getRows() << ",";
        ss << "\"cols\":" << floor->getCols() << ",";
        
        ss << "\"spots\":[";
        auto& spots = floor->getSpots();
        for (int r = 0; r < floor->getRows(); ++r) {
            for (int c = 0; c < floor->getCols(); ++c) {
                if (r > 0 || c > 0) ss << ",";
                auto& spot = spots[r][c];
                ss << "{";
                ss << "\"id\":\"" << spot.getSpotId() << "\",";
                ss << "\"status\":" << static_cast<int>(spot.getStatus()) << ",";
                ss << "\"row\":" << r << ",";
                ss << "\"col\":" << c;
                if (spot.getStatus() == SpotStatus::OCCUPIED && spot.getVehicle()) {
                    ss << ",\"plate\":\"" << spot.getVehicle()->getLicensePlate() << "\"";
                }
                ss << "}";
            }
        }
        ss << "]";
        ss << "}";
    }
    ss << "]";
    ss << "}";
    return ss.str();
}

std::string HttpServer::getRecordsJson(int limit) {
    std::stringstream ss;
    ss << "[";
    
    auto& records = parkingLot_->getAllRecords();
    int count = 0;
    for (auto it = records.rbegin(); it != records.rend() && count < limit; ++it, ++count) {
        if (count > 0) ss << ",";
        ss << "{";
        ss << "\"recordId\":\"" << it->getRecordId() << "\",";
        ss << "\"plate\":\"" << it->getLicensePlate() << "\",";
        ss << "\"spotId\":\"" << it->getSpotId() << "\",";
        ss << "\"entryTime\":\"" << Utils::timeToStr(it->getEntryTime()) << "\",";
        ss << "\"exitTime\":\"" << Utils::timeToStr(it->getExitTime()) << "\",";
        ss << "\"fee\":" << std::fixed << std::setprecision(2) << it->getFee() << ",";
        ss << "\"duration\":" << std::fixed << std::setprecision(2) << it->getDuration();
        ss << "}";
    }
    ss << "]";
    return ss.str();
}

std::string HttpServer::getFloorJson(int floorIndex) {
    Floor* floor = parkingLot_->getFloor(floorIndex);
    if (!floor) {
        return "{\"error\":\"Floor not found\"}";
    }
    
    std::stringstream ss;
    ss << "{";
    ss << "\"floorId\":" << floor->getFloorId() << ",";
    ss << "\"zoneName\":\"" << floor->getZoneName() << "\",";
    ss << "\"totalSpots\":" << floor->getTotalSpots() << ",";
    ss << "\"availableSpots\":" << floor->getAvailableCount() << ",";
    ss << "\"rows\":" << floor->getRows() << ",";
    ss << "\"cols\":" << floor->getCols() << ",";
    
    ss << "\"spots\":[";
    auto& spots = floor->getSpots();
    for (int r = 0; r < floor->getRows(); ++r) {
        for (int c = 0; c < floor->getCols(); ++c) {
            if (r > 0 || c > 0) ss << ",";
            auto& spot = spots[r][c];
            ss << "{";
            ss << "\"id\":\"" << spot.getSpotId() << "\",";
            ss << "\"status\":" << static_cast<int>(spot.getStatus()) << ",";
            ss << "\"row\":" << r << ",";
            ss << "\"col\":" << c;
            if (spot.getStatus() == SpotStatus::OCCUPIED && spot.getVehicle()) {
                ss << ",\"plate\":\"" << spot.getVehicle()->getLicensePlate() << "\"";
            }
            ss << "}";
        }
    }
    ss << "]";
    ss << "}";
    return ss.str();
}

std::string getDashboardHtml() {
    return R"HTML(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>停车场实时监控面板</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            color: #fff;
        }
        .container { max-width: 1400px; margin: 0 auto; padding: 20px; }
        .header {
            text-align: center;
            padding: 20px 0;
            margin-bottom: 30px;
        }
        .header h1 {
            font-size: 2.5em;
            background: linear-gradient(90deg, #00d4ff, #7b2cbf);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 10px;
        }
        .status-bar {
            display: flex;
            justify-content: center;
            gap: 30px;
            flex-wrap: wrap;
        }
        .status-item {
            background: rgba(255,255,255,0.1);
            padding: 15px 30px;
            border-radius: 10px;
            text-align: center;
            backdrop-filter: blur(10px);
        }
        .status-item .label { font-size: 0.9em; color: #aaa; margin-bottom: 5px; }
        .status-item .value { font-size: 2em; font-weight: bold; }
        .status-item.total .value { color: #00d4ff; }
        .status-item.available .value { color: #00ff88; }
        .status-item.occupied .value { color: #ff4757; }
        .status-item.waiting .value { color: #ffa502; }
        
        .floor-tabs {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
            justify-content: center;
            flex-wrap: wrap;
        }
        .floor-tab {
            padding: 12px 24px;
            background: rgba(255,255,255,0.1);
            border: none;
            border-radius: 8px;
            color: #fff;
            cursor: pointer;
            font-size: 1em;
            transition: all 0.3s;
        }
        .floor-tab:hover { background: rgba(255,255,255,0.2); }
        .floor-tab.active {
            background: linear-gradient(90deg, #00d4ff, #7b2cbf);
            box-shadow: 0 4px 15px rgba(0,212,255,0.4);
        }
        
        .floor-container {
            background: rgba(255,255,255,0.05);
            border-radius: 15px;
            padding: 30px;
            margin-bottom: 30px;
            backdrop-filter: blur(10px);
        }
        .floor-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
        }
        .floor-title { font-size: 1.5em; color: #00d4ff; }
        .floor-stats { display: flex; gap: 20px; }
        .floor-stat {
            padding: 8px 16px;
            border-radius: 5px;
            font-size: 0.9em;
        }
        .floor-stat.avail { background: rgba(0,255,136,0.2); color: #00ff88; }
        .floor-stat.occ { background: rgba(255,71,87,0.2); color: #ff4757; }
        
        .parking-grid {
            display: grid;
            gap: 10px;
            justify-content: center;
        }
        .spot {
            width: 80px;
            height: 60px;
            border-radius: 8px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            font-weight: bold;
            transition: all 0.3s;
            cursor: pointer;
            position: relative;
        }
        .spot:hover { transform: scale(1.05); }
        .spot.available {
            background: linear-gradient(135deg, #00ff88 0%, #00cc6a 100%);
            box-shadow: 0 4px 15px rgba(0,255,136,0.3);
        }
        .spot.occupied {
            background: linear-gradient(135deg, #ff4757 0%, #ff3838 100%);
            box-shadow: 0 4px 15px rgba(255,71,87,0.3);
        }
        .spot.reserved {
            background: linear-gradient(135deg, #ffa502 0%, #ff9f1a 100%);
            box-shadow: 0 4px 15px rgba(255,165,2,0.3);
        }
        .spot.maintenance {
            background: linear-gradient(135deg, #a55eea 0%, #8854d0 100%);
            box-shadow: 0 4px 15px rgba(165,94,234,0.3);
        }
        .spot .spot-id { font-size: 0.9em; }
        .spot .plate {
            font-size: 0.7em;
            margin-top: 2px;
            opacity: 0.9;
        }
        
        .legend {
            display: flex;
            justify-content: center;
            gap: 30px;
            margin-top: 20px;
            flex-wrap: wrap;
        }
        .legend-item {
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .legend-color {
            width: 20px;
            height: 20px;
            border-radius: 4px;
        }
        .legend-color.avail { background: #00ff88; }
        .legend-color.occ { background: #ff4757; }
        .legend-color.res { background: #ffa502; }
        .legend-color.maint { background: #a55eea; }
        
        .records-section {
            background: rgba(255,255,255,0.05);
            border-radius: 15px;
            padding: 30px;
            backdrop-filter: blur(10px);
        }
        .records-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
        }
        .records-title { font-size: 1.5em; color: #00d4ff; }
        .refresh-btn {
            padding: 8px 16px;
            background: linear-gradient(90deg, #00d4ff, #7b2cbf);
            border: none;
            border-radius: 5px;
            color: #fff;
            cursor: pointer;
            font-size: 0.9em;
        }
        .records-table {
            width: 100%;
            border-collapse: collapse;
        }
        .records-table th, .records-table td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        .records-table th {
            color: #00d4ff;
            font-weight: 600;
        }
        .records-table tr:hover {
            background: rgba(255,255,255,0.05);
        }
        
        .event-log {
            position: fixed;
            bottom: 20px;
            right: 20px;
            width: 350px;
            max-height: 300px;
            background: rgba(0,0,0,0.8);
            border-radius: 10px;
            padding: 15px;
            overflow-y: auto;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.1);
        }
        .event-log h4 {
            color: #00d4ff;
            margin-bottom: 10px;
            font-size: 0.9em;
        }
        .event-item {
            padding: 8px;
            margin-bottom: 5px;
            border-radius: 5px;
            font-size: 0.85em;
            animation: slideIn 0.3s ease;
        }
        @keyframes slideIn {
            from { opacity: 0; transform: translateX(20px); }
            to { opacity: 1; transform: translateX(0); }
        }
        .event-item.entry { background: rgba(0,255,136,0.2); border-left: 3px solid #00ff88; }
        .event-item.exit { background: rgba(255,71,87,0.2); border-left: 3px solid #ff4757; }
        .event-time { color: #888; font-size: 0.8em; margin-top: 2px; }
        
        .connection-status {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 0.85em;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .connection-status.connected {
            background: rgba(0,255,136,0.2);
            color: #00ff88;
        }
        .connection-status.disconnected {
            background: rgba(255,71,87,0.2);
            color: #ff4757;
        }
        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            animation: pulse 2s infinite;
        }
        .connected .status-dot { background: #00ff88; }
        .disconnected .status-dot { background: #ff4757; }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        
        @media (max-width: 768px) {
            .header h1 { font-size: 1.8em; }
            .status-bar { gap: 15px; }
            .status-item { padding: 10px 20px; }
            .spot { width: 60px; height: 45px; font-size: 0.8em; }
            .event-log { width: 300px; bottom: 10px; right: 10px; }
        }
    </style>
</head>
<body>
    <div class="connection-status disconnected" id="connectionStatus">
        <div class="status-dot"></div>
        <span>连接中...</span>
    </div>
    
    <div class="container">
        <div class="header">
            <h1>停车场实时监控面板</h1>
            <div class="status-bar">
                <div class="status-item total">
                    <div class="label">总车位</div>
                    <div class="value" id="totalSpots">0</div>
                </div>
                <div class="status-item available">
                    <div class="label">空闲车位</div>
                    <div class="value" id="availableSpots">0</div>
                </div>
                <div class="status-item occupied">
                    <div class="label">占用车位</div>
                    <div class="value" id="occupiedSpots">0</div>
                </div>
                <div class="status-item waiting">
                    <div class="label">等待车辆</div>
                    <div class="value" id="waitingCount">0</div>
                </div>
            </div>
        </div>
        
        <div class="floor-tabs" id="floorTabs"></div>
        
        <div class="floor-container" id="floorContainer">
            <div class="floor-header">
                <div class="floor-title" id="floorTitle">选择楼层</div>
                <div class="floor-stats">
                    <div class="floor-stat avail" id="floorAvail">空闲: 0</div>
                    <div class="floor-stat occ" id="floorOcc">占用: 0</div>
                </div>
            </div>
            <div class="parking-grid" id="parkingGrid"></div>
            <div class="legend">
                <div class="legend-item">
                    <div class="legend-color avail"></div>
                    <span>空闲</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color occ"></div>
                    <span>占用</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color res"></div>
                    <span>预约</span>
                </div>
                <div class="legend-item">
                    <div class="legend-color maint"></div>
                    <span>维护</span>
                </div>
            </div>
        </div>
        
        <div class="records-section">
            <div class="records-header">
                <div class="records-title">最近进出记录</div>
                <button class="refresh-btn" onclick="loadRecords()">刷新</button>
            </div>
            <table class="records-table">
                <thead>
                    <tr>
                        <th>记录号</th>
                        <th>车牌号</th>
                        <th>车位</th>
                        <th>入场时间</th>
                        <th>出场时间</th>
                        <th>停车时长</th>
                        <th>费用</th>
                    </tr>
                </thead>
                <tbody id="recordsBody"></tbody>
            </table>
        </div>
    </div>
    
    <div class="event-log" id="eventLog">
        <h4>实时事件</h4>
    </div>
    
    <script>
        const API_BASE = window.location.origin;
        let currentFloor = 0;
        let parkingData = null;
        let ws = null;
        
        async function fetchAPI(endpoint) {
            try {
                const response = await fetch(API_BASE + endpoint);
                return await response.json();
            } catch (error) {
                console.error('API 请求失败:', error);
                return null;
            }
        }
        
        async function loadStatus() {
            const data = await fetchAPI('/api/status');
            if (data) {
                parkingData = data;
                updateStatusDisplay(data);
                updateFloorTabs(data);
                renderFloor(currentFloor);
            }
        }
        
        function updateStatusDisplay(data) {
            document.getElementById('totalSpots').textContent = data.totalSpots;
            document.getElementById('availableSpots').textContent = data.availableSpots;
            document.getElementById('occupiedSpots').textContent = data.occupiedSpots;
            document.getElementById('waitingCount').textContent = data.waitingCount;
        }
        
        function updateFloorTabs(data) {
            const tabsContainer = document.getElementById('floorTabs');
            tabsContainer.innerHTML = '';
            
            data.floors.forEach((floor, index) => {
                const tab = document.createElement('button');
                tab.className = 'floor-tab' + (index === currentFloor ? ' active' : '');
                tab.textContent = floor.floorId + '层 (' + floor.zoneName + '区)';
                tab.onclick = function() {
                    currentFloor = index;
                    updateFloorTabs(data);
                    renderFloor(currentFloor);
                };
                tabsContainer.appendChild(tab);
            });
        }
        
        function renderFloor(floorIndex) {
            if (!parkingData || !parkingData.floors[floorIndex]) return;
            
            const floor = parkingData.floors[floorIndex];
            document.getElementById('floorTitle').textContent = floor.floorId + '层 - ' + floor.zoneName + '区';
            document.getElementById('floorAvail').textContent = '空闲: ' + floor.availableSpots;
            document.getElementById('floorOcc').textContent = '占用: ' + (floor.totalSpots - floor.availableSpots);
            
            const grid = document.getElementById('parkingGrid');
            grid.style.gridTemplateColumns = 'repeat(' + floor.cols + ', 80px)';
            grid.innerHTML = '';
            
            floor.spots.forEach(function(spot) {
                const spotDiv = document.createElement('div');
                let statusClass = 'available';
                if (spot.status === 1) statusClass = 'occupied';
                else if (spot.status === 2) statusClass = 'reserved';
                else if (spot.status === 3) statusClass = 'maintenance';
                
                spotDiv.className = 'spot ' + statusClass;
                let html = '<div class="spot-id">' + spot.id + '</div>';
                if (spot.plate) {
                    html += '<div class="plate">' + spot.plate + '</div>';
                }
                spotDiv.innerHTML = html;
                
                let title = '车位: ' + spot.id + '\n状态: ' + getStatusText(spot.status);
                if (spot.plate) {
                    title = '车位: ' + spot.id + '\n车牌: ' + spot.plate;
                }
                spotDiv.title = title;
                grid.appendChild(spotDiv);
            });
        }
        
        function getStatusText(status) {
            const texts = ['空闲', '占用', '预约', '维护'];
            return texts[status] || '未知';
        }
        
        async function loadRecords() {
            const records = await fetchAPI('/api/records?limit=20');
            if (records) {
                const tbody = document.getElementById('recordsBody');
                tbody.innerHTML = '';
                
                records.forEach(function(record) {
                    const tr = document.createElement('tr');
                    tr.innerHTML = 
                        '<td>' + record.recordId + '</td>' +
                        '<td>' + record.plate + '</td>' +
                        '<td>' + record.spotId + '</td>' +
                        '<td>' + record.entryTime + '</td>' +
                        '<td>' + record.exitTime + '</td>' +
                        '<td>' + record.duration.toFixed(2) + ' 小时</td>' +
                        '<td style="color: #ffa502;">¥' + record.fee.toFixed(2) + '</td>';
                    tbody.appendChild(tr);
                });
            }
        }
        
        function addEventLog(type, plate, spotId, timestamp, fee) {
            const logContainer = document.getElementById('eventLog');
            const eventDiv = document.createElement('div');
            eventDiv.className = 'event-item ' + type;
            
            let text = type === 'entry' 
                ? '车辆入场: ' + plate + ' -> ' + spotId
                : '车辆出场: ' + plate + ' <- ' + spotId;
            if (fee !== null && fee !== undefined) {
                text += ' (费用: ¥' + fee.toFixed(2) + ')';
            }
            
            eventDiv.innerHTML = 
                '<div>' + text + '</div>' +
                '<div class="event-time">' + timestamp + '</div>';
            
            if (logContainer.children.length > 1) {
                logContainer.insertBefore(eventDiv, logContainer.children[1]);
            } else {
                logContainer.appendChild(eventDiv);
            }
            
            while (logContainer.children.length > 11) {
                logContainer.removeChild(logContainer.lastChild);
            }
        }
        
        function connectWebSocket() {
            const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = wsProtocol + '//' + window.location.host + '/ws';
            
            console.log('尝试连接 WebSocket:', wsUrl);
            
            try {
                ws = new WebSocket(wsUrl);
                
                ws.onopen = function() {
                    console.log('WebSocket 连接成功');
                    document.getElementById('connectionStatus').className = 'connection-status connected';
                    document.getElementById('connectionStatus').querySelector('span').textContent = '实时连接';
                };
                
                ws.onmessage = function(event) {
                    try {
                        const data = JSON.parse(event.data);
                        console.log('收到 WebSocket 消息:', data);
                        
                        if (data.type === 'entry' || data.type === 'exit') {
                            addEventLog(data.type, data.plate, data.spotId, data.timestamp, data.fee);
                            loadStatus();
                            loadRecords();
                        }
                    } catch (e) {
                        console.error('解析 WebSocket 消息失败:', e);
                    }
                };
                
                ws.onclose = function() {
                    console.log('WebSocket 连接关闭，尝试重连...');
                    document.getElementById('connectionStatus').className = 'connection-status disconnected';
                    document.getElementById('connectionStatus').querySelector('span').textContent = '连接断开';
                    setTimeout(connectWebSocket, 3000);
                };
                
                ws.onerror = function(error) {
                    console.error('WebSocket 错误:', error);
                    document.getElementById('connectionStatus').className = 'connection-status disconnected';
                    document.getElementById('connectionStatus').querySelector('span').textContent = '连接错误';
                };
            } catch (e) {
                console.error('创建 WebSocket 失败:', e);
                setTimeout(connectWebSocket, 3000);
            }
        }
        
        async function init() {
            await loadStatus();
            await loadRecords();
            
            setInterval(loadStatus, 5000);
            
            connectWebSocket();
        }
        
        init();
    </script>
</body>
</html>
)HTML";
}

void handleWebSocketConnection(const httplib::Request& req, httplib::ws::WebSocket& ws) {
    LOG_INFO("WebSocket 客户端连接: " + req.remote_addr);
    
    {
        std::lock_guard<std::mutex> lock(g_ws_mutex);
        g_ws_clients.insert(&ws);
    }
    
    std::string msg;
    while (g_server_running && ws.is_open()) {
        auto result = ws.read(msg);
        if (result == httplib::ws::ReadResult::Fail) {
            break;
        }
        if (result == httplib::ws::ReadResult::Text || result == httplib::ws::ReadResult::Binary) {
            LOG_INFO("收到 WebSocket 消息: " + msg);
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(g_ws_mutex);
        g_ws_clients.erase(&ws);
    }
    LOG_INFO("WebSocket 客户端断开连接");
}

void HttpServer::runServer() {
    httplib::Server svr;
    
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });
    
    svr.Options("/.*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
    });
    
    svr.Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(getStatusJson(), "application/json");
    });
    
    svr.Get("/api/records", [this](const httplib::Request& req, httplib::Response& res) {
        int limit = 20;
        if (req.has_param("limit")) {
            limit = std::stoi(req.get_param_value("limit"));
        }
        res.set_content(getRecordsJson(limit), "application/json");
    });
    
    svr.Get("/api/floor/(\\d+)", [this](const httplib::Request& req, httplib::Response& res) {
        int floorIndex = std::stoi(req.matches[1]);
        res.set_content(getFloorJson(floorIndex), "application/json");
    });
    
    svr.WebSocket("/ws", handleWebSocketConnection);
    
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_redirect("/dashboard");
    });
    
    svr.Get("/dashboard", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(getDashboardHtml(), "text/html; charset=utf-8");
    });
    
    LOG_INFO("HTTP 服务器监听端口: " + std::to_string(port_));
    svr.listen("0.0.0.0", port_);
}
