#include "http_server.h"
#include "json_builder.h"
#include "graph.h"
#include "metro_data.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>

// ---- Serialize a RouteResult to JSON ----
Json routeToJson(const RouteResult& r, const Graph& g) {
    if (r.path.empty()) { Json j; return j; }

    auto pathArr = Json::array();
    for (auto& step : r.path) {
        const auto& st = g.stations.at(step.stationId);
        auto node = Json::object();
        node.set("id",         Json(step.stationId));
        node.set("name",       Json(st.name));
        node.set("line",       Json(st.line));
        node.set("isTransfer", Json(st.isTransfer));

        if (step.hasEdge) {
            auto edge = Json::object();
            edge.set("to",       Json(step.edgeToNext.to));
            edge.set("distance", Json(step.edgeToNext.distance));
            edge.set("time",     Json(step.edgeToNext.time));
            edge.set("line",     Json(step.edgeToNext.line));
            node.set("edge", edge);
        }
        pathArr.push(node);
    }

    auto obj = Json::object();
    obj.set("path",        pathArr);
    obj.set("totalDist",   Json(r.totalDist));
    obj.set("totalTime",   Json((int)r.totalTime));
    obj.set("transfers",   Json(r.transfers));
    obj.set("numStations", Json(r.numStations));
    return obj;
}

int main() {
    Graph metro;
    buildMetroNetwork(metro);

    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout <<   "║    BANGALORE METRO — Route Optimizer     ║\n";
    std::cout <<   "║    C++ Backend + Web Frontend            ║\n";
    std::cout <<   "╚══════════════════════════════════════════╝\n\n";
    std::cout << "✓ Metro graph: " << metro.stations.size() << " stations loaded\n";

    HttpServer svr;

    // GET /api/stations
    svr.get("/api/stations", [&](const HttpRequest&, HttpResponse& res) {
        std::vector<Station> sorted;
        for (auto& [id, st] : metro.stations) sorted.push_back(st);
        std::sort(sorted.begin(), sorted.end(),
            [](const Station& a, const Station& b){ return a.name < b.name; });

        auto arr = Json::array();
        for (auto& st : sorted) {
            auto obj = Json::object();
            obj.set("id",         Json(st.id));
            obj.set("name",       Json(st.name));
            obj.set("line",       Json(st.line));
            obj.set("isTransfer", Json(st.isTransfer));
            arr.push(obj);
        }
        res.body = arr.dump();
        std::cout << "  GET /api/stations → " << sorted.size() << " items\n";
    });

    // POST /api/route
    svr.post("/api/route", [&](const HttpRequest& req, HttpResponse& res) {
        try {
            auto body    = parseJson(req.body);
            std::string src = body.count("start")   ? body.at("start")   : "";
            std::string dst = body.count("end")     ? body.at("end")     : "";
            double traffic  = body.count("traffic") ? std::stod(body.at("traffic")) : 1.0;
            int    k        = body.count("k")       ? std::stoi(body.at("k"))       : 3;

            if (src.empty() || dst.empty()) {
                res.status = 400;
                res.body = R"({"error":"start and end required"})"; return;
            }
            if (!metro.stations.count(src) || !metro.stations.count(dst)) {
                res.status = 404;
                res.body = R"({"error":"station not found"})"; return;
            }
            if (src == dst) {
                res.status = 400;
                res.body = R"({"error":"start and end must differ"})"; return;
            }
            traffic = std::max(1.0, std::min(traffic, 5.0));
            k       = std::max(1,   std::min(k, 5));

            std::cout << "  POST /api/route  "
                      << metro.stations[src].name << " → "
                      << metro.stations[dst].name
                      << "  ×" << traffic << "  k=" << k << "\n";

            auto routes = metro.kShortestPaths(src, dst, k, traffic);

            auto routesArr = Json::array();
            for (auto& r : routes) routesArr.push(routeToJson(r, metro));

            auto response = Json::object();
            response.set("routes",     routesArr);
            response.set("routeCount", Json((int)routes.size()));
            response.set("trafficMul", Json(traffic));
            response.set("from",       Json(metro.stations[src].name));
            response.set("to",         Json(metro.stations[dst].name));
            res.body = response.dump();

            if (!routes.empty())
                std::cout << "       → " << routes.size() << " route(s)  best: "
                          << routes[0].totalDist << "km, "
                          << (int)routes[0].totalTime << "min\n";
        } catch (const std::exception& e) {
            res.status = 500;
            res.body = std::string(R"({"error":")") + e.what() + "\"}";
        }
    });

    std::cout << "\n🚇  http://localhost:8080  (open in browser)\n";
    std::cout << "    GET  /api/stations\n";
    std::cout << "    POST /api/route\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

    svr.listen(8080);
    return 0;
}
