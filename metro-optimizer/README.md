# 🚇 Bangalore Metro Route Optimizer
### C++ Backend + Web Frontend | DSA Project

A full-stack shortest path navigator using **Dijkstra's Algorithm** with a **C++17 HTTP backend** and a polished web UI.

---

## 📁 Project Structure

```
metro-optimizer/
│
├── backend/
│   ├── main.cpp          ← HTTP server entry point
│   ├── graph.h           ← Core DSA: Graph, MinHeap, Dijkstra, Yen's K-Shortest
│   ├── metro_data.h      ← Bangalore metro network (stations + edges)
│   ├── http_server.h     ← Lightweight POSIX HTTP server (no external deps)
│   └── json_builder.h    ← Minimal JSON serializer (no external deps)
│
├── frontend/
│   └── index.html        ← Single-file web UI (HTML + CSS + JS)
│
├── build/
│   └── metro_server      ← Compiled binary (after make build)
│
├── Makefile              ← Build system
└── CMakeLists.txt        ← CMake alternative
```

---

## 🚀 Quick Start

### Requirements
- `g++` with C++17 support (`g++ --version`)
- `make`
- A web browser

### 1. Build
```bash
make build
```

### 2. Run
```bash
make run
# Server starts at http://localhost:8080
```

### 3. Open Browser
```
http://localhost:8080
```

---

## 🧠 DSA Concepts Implemented

### 1. Weighted Graph (Adjacency List)
```cpp
class Graph {
    unordered_map<string, vector<Edge>> adj;      // O(1) lookup
    unordered_map<string, Station>      stations;
};
```
Each edge stores: `{ to, distance_km, time_min, line }`

### 2. Min-Heap Priority Queue
```cpp
// Uses STL priority_queue with greater<> comparator
priority_queue<HeapNode, vector<HeapNode>, greater<HeapNode>> pq;
```
- `push()` → O(log V)
- `pop()`  → O(log V)

### 3. Dijkstra's Algorithm
```
Time:  O((V + E) log V)
Space: O(V)
```
- Relaxes edges using traffic multiplier as weight factor
- Early exit when destination is dequeued
- Path reconstruction via predecessor map

### 4. Yen's K-Shortest Paths
- Finds top-K distinct paths using repeated Dijkstra runs
- Temporarily removes used edges/nodes between iterations
- Returns sorted by total travel time

---

## 🌐 API Reference

### GET `/api/stations`
Returns all 46 stations sorted alphabetically.

```json
[
  { "id": "ban", "name": "Banashankari", "line": "green", "isTransfer": false },
  { "id": "mah", "name": "Majestic",     "line": "purple", "isTransfer": true },
  ...
]
```

### POST `/api/route`
Finds shortest paths between two stations.

**Request:**
```json
{
  "start":   "ws",    // station ID
  "end":     "con",   // station ID
  "traffic": 1.5,     // weight multiplier (1.0–3.0)
  "k":       3        // number of alternative routes
}
```

**Response:**
```json
{
  "routes": [
    {
      "path": [
        { "id": "ws", "name": "Whitefield", "line": "purple", "isTransfer": false,
          "edge": { "to": "hom", "distance": 1.8, "time": 4.0, "line": "purple" }},
        ...
      ],
      "totalDist":   40.1,
      "totalTime":   87,
      "transfers":   1,
      "numStations": 30
    }
  ],
  "routeCount": 1,
  "trafficMul": 1.5,
  "from": "Whitefield",
  "to":   "Gottigere"
}
```

---

## 🗺️ Metro Network

| Line   | Stations | Direction     | Transfer Points     |
|--------|----------|---------------|---------------------|
| Purple | 26       | East ↔ West   | MG Road, Cubbon Park, **Majestic** |
| Green  | 21       | North ↕ South | **Majestic**        |

**Majestic** is the main interchange between both lines.

---

## 🔧 Extending the Project

### Add a new station:
```cpp
// In metro_data.h
g.addStation("new_id", "New Station Name", "purple", false);
g.addEdge("prev_id", "new_id", 1.5 /*km*/, 3 /*min*/, "purple");
```

### Add a new metro line:
```cpp
g.addStation("s1", "Station 1", "blue", false);
g.addStation("s2", "Station 2", "blue", false);
g.addEdge("s1", "s2", 2.0, 4, "blue");
// Add a transfer connection to existing network:
g.addEdge("s1", "mah", 0.5, 1, "blue"); // connect at Majestic
```

---

## 📊 Complexity Summary

| Operation           | Time           | Space  |
|---------------------|----------------|--------|
| Build Graph         | O(V + E)       | O(V+E) |
| Dijkstra            | O((V+E) log V) | O(V)   |
| Yen's K-Shortest    | O(K·V·(V+E)logV) | O(KV) |
| HTTP Request Handle | O(1) routing   | O(req) |
