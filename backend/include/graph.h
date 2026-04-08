#ifndef GRAPH_H
#define GRAPH_H

#include "common.h"

// 图节点
struct GraphNode {
    int id;
    std::string name;       // 节点名称（如车位ID或通道名）
    int x, y;               // 坐标
    bool isSpot;            // 是否是车位节点
    
    GraphNode() : id(-1), x(0), y(0), isSpot(false) {}
    GraphNode(int id, const std::string& name, int x, int y, bool isSpot = false)
        : id(id), name(name), x(x), y(y), isSpot(isSpot) {}
};

// 图边
struct GraphEdge {
    int from, to;
    double weight;
    
    GraphEdge() : from(-1), to(-1), weight(0) {}
    GraphEdge(int f, int t, double w) : from(f), to(t), weight(w) {}
};

// 停车场图结构（用于路径规划）
class ParkingGraph {
public:
    ParkingGraph() : nodeCount_(0) {}
    
    // 添加节点
    int addNode(const std::string& name, int x, int y, bool isSpot = false) {
        int id = nodeCount_++;
        nodes_.emplace_back(id, name, x, y, isSpot);
        adjList_.push_back(std::vector<std::pair<int, double>>());
        return id;
    }
    
    // 添加边（双向）
    void addEdge(int from, int to, double weight = 1.0) {
        if (from < 0 || from >= nodeCount_ || to < 0 || to >= nodeCount_) {
            return;
        }
        adjList_[from].push_back({to, weight});
        adjList_[to].push_back({from, weight});
        edges_.emplace_back(from, to, weight);
    }
    
    // Dijkstra最短路径算法
    std::vector<int> dijkstra(int start, int end) const {
        if (start < 0 || start >= nodeCount_ || end < 0 || end >= nodeCount_) {
            return {};
        }
        
        std::vector<double> dist(nodeCount_, std::numeric_limits<double>::infinity());
        std::vector<int> prev(nodeCount_, -1);
        std::vector<bool> visited(nodeCount_, false);
        
        // 优先队列：(距离, 节点ID)
        std::priority_queue<std::pair<double, int>,
                          std::vector<std::pair<double, int>>,
                          std::greater<std::pair<double, int>>> pq;
        
        dist[start] = 0;
        pq.push({0, start});
        
        while (!pq.empty()) {
            auto [d, u] = pq.top();
            pq.pop();
            
            if (visited[u]) continue;
            visited[u] = true;
            
            if (u == end) break;
            
            for (const auto& [v, w] : adjList_[u]) {
                if (!visited[v] && dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    prev[v] = u;
                    pq.push({dist[v], v});
                }
            }
        }
        
        // 回溯构建路径
        std::vector<int> path;
        if (prev[end] != -1 || start == end) {
            for (int at = end; at != -1; at = prev[at]) {
                path.push_back(at);
            }
            std::reverse(path.begin(), path.end());
        }
        
        return path;
    }
    
    // 获取路径的节点名称
    std::vector<std::string> getPathNames(const std::vector<int>& path) const {
        std::vector<std::string> names;
        for (int id : path) {
            if (id >= 0 && id < nodeCount_) {
                names.push_back(nodes_[id].name);
            }
        }
        return names;
    }
    
    // 计算路径总距离
    double getPathDistance(const std::vector<int>& path) const {
        double total = 0;
        for (size_t i = 1; i < path.size(); ++i) {
            int u = path[i-1], v = path[i];
            for (const auto& [neighbor, weight] : adjList_[u]) {
                if (neighbor == v) {
                    total += weight;
                    break;
                }
            }
        }
        return total;
    }
    
    // 查找最近的空闲车位节点
    int findNearestAvailableSpot(int start, const std::vector<int>& availableSpotNodes) const {
        if (availableSpotNodes.empty()) return -1;
        
        std::vector<double> dist(nodeCount_, std::numeric_limits<double>::infinity());
        std::vector<bool> visited(nodeCount_, false);
        std::priority_queue<std::pair<double, int>,
                          std::vector<std::pair<double, int>>,
                          std::greater<std::pair<double, int>>> pq;
        
        dist[start] = 0;
        pq.push({0, start});
        
        while (!pq.empty()) {
            auto [d, u] = pq.top();
            pq.pop();
            
            if (visited[u]) continue;
            visited[u] = true;
            
            // 检查是否是空闲车位
            if (std::find(availableSpotNodes.begin(), availableSpotNodes.end(), u) 
                != availableSpotNodes.end()) {
                return u;
            }
            
            for (const auto& [v, w] : adjList_[u]) {
                if (!visited[v] && dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    pq.push({dist[v], v});
                }
            }
        }
        
        return -1;
    }
    
    // Getters
    int getNodeCount() const { return nodeCount_; }
    const GraphNode& getNode(int id) const { return nodes_[id]; }
    const std::vector<GraphNode>& getNodes() const { return nodes_; }
    
    // 根据名称查找节点ID
    int findNodeByName(const std::string& name) const {
        for (const auto& node : nodes_) {
            if (node.name == name) return node.id;
        }
        return -1;
    }
    
private:
    int nodeCount_;
    std::vector<GraphNode> nodes_;
    std::vector<GraphEdge> edges_;
    std::vector<std::vector<std::pair<int, double>>> adjList_;  // 邻接表
};

#endif // GRAPH_H
