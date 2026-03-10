#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <queue>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <cmath>

// ============================================================
//  DATA STRUCTURES
// ============================================================

struct Edge {
    std::string to;
    double      distance;   // km
    double      time;       // minutes (base)
    std::string line;       // "purple" | "green"
};

struct Station {
    std::string id;
    std::string name;
    std::string line;
    bool        isTransfer;
};

// ---- Min-Heap node ----
struct HeapNode {
    std::string node;
    double      cost;
    bool operator>(const HeapNode& o) const { return cost > o.cost; }
};

// ---- Result of one shortest path ----
struct PathStep {
    std::string stationId;
    Edge        edgeToNext;   // edge used to reach NEXT station (empty for last)
    bool        hasEdge;
};

struct RouteResult {
    std::vector<PathStep> path;
    double totalDist;
    double totalTime;   // after traffic multiplier
    int    transfers;
    int    numStations;
};

// ============================================================
//  WEIGHTED GRAPH  (Adjacency List)
// ============================================================
class Graph {
public:
    std::unordered_map<std::string, std::vector<Edge>>    adj;
    std::unordered_map<std::string, Station>              stations;

    void addStation(const std::string& id, const std::string& name,
                    const std::string& line, bool isTransfer = false) {
        if (adj.find(id) == adj.end()) adj[id] = {};
        stations[id] = { id, name, line, isTransfer };
    }

    void addEdge(const std::string& u, const std::string& v,
                 double dist, double time, const std::string& line) {
        adj[u].push_back({ v, dist, time, line });
        adj[v].push_back({ u, dist, time, line });
    }

    // ----------------------------------------------------------
    //  DIJKSTRA'S ALGORITHM
    //  Uses std::priority_queue as a Min-Heap (via greater<>).
    //  Time:  O((V + E) log V)
    //  Space: O(V)
    // ----------------------------------------------------------
    RouteResult dijkstra(const std::string& start, const std::string& end,
                         double trafficMul = 1.0,
                         const std::unordered_set<std::string>& blockedEdges = {},
                         const std::unordered_set<std::string>& blockedNodes = {}) const {

        std::unordered_map<std::string, double> dist;
        std::unordered_map<std::string, std::string> prevNode;
        std::unordered_map<std::string, Edge>         prevEdge;
        std::unordered_set<std::string> visited;

        // Init: all distances = infinity
        for (auto& [id, _] : adj) {
            dist[id]     = std::numeric_limits<double>::infinity();
            prevNode[id] = "";
        }
        dist[start] = 0.0;

        // Min-heap: {cost, nodeId}
        std::priority_queue<HeapNode, std::vector<HeapNode>, std::greater<HeapNode>> pq;
        pq.push({ start, 0.0 });

        while (!pq.empty()) {
            auto [u, cost] = pq.top(); pq.pop();

            if (visited.count(u)) continue;
            if (blockedNodes.count(u) && u != start && u != end) continue;
            visited.insert(u);

            if (u == end) break;  // early exit

            for (auto& edge : adj.at(u)) {
                if (blockedNodes.count(edge.to)) continue;

                // blocked edge key: "u->v"
                std::string ekey = u + "->" + edge.to;
                if (blockedEdges.count(ekey)) continue;

                double weight   = edge.time * trafficMul;
                double newCost  = dist[u] + weight;

                if (newCost < dist[edge.to]) {
                    dist[edge.to]     = newCost;
                    prevNode[edge.to] = u;
                    prevEdge[edge.to] = edge;
                    pq.push({ edge.to, newCost });
                }
            }
        }

        if (dist[end] == std::numeric_limits<double>::infinity())
            return {};  // no path

        // ---- Reconstruct path ----
        std::vector<PathStep> path;
        std::string cur = end;
        while (!cur.empty()) {
            PathStep step;
            step.stationId = cur;
            if (prevNode[cur].empty()) {
                step.hasEdge = false;
            } else {
                step.edgeToNext = prevEdge[cur];
                step.hasEdge    = true;
            }
            path.push_back(step);
            cur = prevNode[cur];
        }
        std::reverse(path.begin(), path.end());

        // ---- Compute totals ----
        double totalDist = 0, totalTime = 0;
        int transfers = 0;
        std::string currentLine = "";
        for (auto& step : path) {
            if (step.hasEdge) {
                totalDist += step.edgeToNext.distance;
                totalTime += step.edgeToNext.time * trafficMul;
                if (!currentLine.empty() && step.edgeToNext.line != currentLine)
                    transfers++;
                currentLine = step.edgeToNext.line;
            }
        }

        RouteResult result;
        result.path        = path;
        result.totalDist   = ::round(totalDist * 10.0) / 10.0;
        result.totalTime   = ::round(totalTime);
        result.transfers   = transfers;
        result.numStations = (int)path.size();
        return result;
    }

    // ----------------------------------------------------------
    //  YEN'S K-SHORTEST PATHS
    //  Finds top-K distinct shortest paths.
    //  Built on top of Dijkstra.
    //  Time: O(K * V * (V+E) log V)
    // ----------------------------------------------------------
    std::vector<RouteResult> kShortestPaths(const std::string& start,
                                             const std::string& end,
                                             int K = 3,
                                             double trafficMul = 1.0) const {
        std::vector<RouteResult> results;
        std::vector<RouteResult> candidates;

        auto first = dijkstra(start, end, trafficMul);
        if (first.path.empty()) return {};
        results.push_back(first);

        for (int k = 1; k < K; k++) {
            const auto& prevPath = results[k - 1].path;

            for (int i = 0; i < (int)prevPath.size() - 1; i++) {
                const std::string& spurNode = prevPath[i].stationId;

                // Root path: stations 0..i
                std::vector<std::string> rootPath;
                for (int j = 0; j <= i; j++)
                    rootPath.push_back(prevPath[j].stationId);

                // Block edges used by previous k-paths that share this root
                std::unordered_set<std::string> blockedEdges;
                for (auto& r : results) {
                    std::vector<std::string> rIds;
                    for (auto& s : r.path) rIds.push_back(s.stationId);
                    bool shareRoot = true;
                    if ((int)rIds.size() < i + 1) { shareRoot = false; }
                    else {
                        for (int j = 0; j <= i; j++)
                            if (rIds[j] != rootPath[j]) { shareRoot = false; break; }
                    }
                    if (shareRoot && i + 1 < (int)rIds.size()) {
                        blockedEdges.insert(rIds[i] + "->" + rIds[i + 1]);
                        blockedEdges.insert(rIds[i + 1] + "->" + rIds[i]);
                    }
                }

                // Block root-path nodes (except spurNode) to prevent cycles
                std::unordered_set<std::string> blockedNodes;
                for (int j = 0; j < i; j++)
                    blockedNodes.insert(rootPath[j]);

                auto spurResult = dijkstra(spurNode, end, trafficMul, blockedEdges, blockedNodes);
                if (spurResult.path.empty()) continue;

                // Total path = rootPath[0..i-1] + spur path
                RouteResult total;
                for (int j = 0; j < i; j++) {
                    PathStep ps;
                    ps.stationId = rootPath[j];
                    ps.hasEdge   = false;
                    total.path.push_back(ps);
                }
                for (auto& s : spurResult.path)
                    total.path.push_back(s);

                // Deduplicate
                std::vector<std::string> totalIds;
                for (auto& s : total.path) totalIds.push_back(s.stationId);
                bool dup = false;
                for (auto& c : candidates) {
                    std::vector<std::string> cIds;
                    for (auto& s : c.path) cIds.push_back(s.stationId);
                    if (cIds == totalIds) { dup = true; break; }
                }
                for (auto& r : results) {
                    std::vector<std::string> rIds;
                    for (auto& s : r.path) rIds.push_back(s.stationId);
                    if (rIds == totalIds) { dup = true; break; }
                }
                if (!dup) {
                    total.totalDist    = spurResult.totalDist;
                    total.totalTime    = spurResult.totalTime;
                    total.transfers    = spurResult.transfers;
                    total.numStations  = (int)total.path.size();
                    candidates.push_back(total);
                }
            }

            if (candidates.empty()) break;
            std::sort(candidates.begin(), candidates.end(),
                [](const RouteResult& a, const RouteResult& b) {
                    return a.totalTime < b.totalTime;
                });
            results.push_back(candidates.front());
            candidates.erase(candidates.begin());
        }

        return results;
    }
};
