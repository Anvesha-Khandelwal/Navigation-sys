#pragma once
#include "graph.h"

// ============================================================
//  BANGALORE METRO NETWORK DATA
//  Purple Line (East-West) + Green Line (North-South)
//  Transfer stations: Majestic, MG Road, Cubbon Park
// ============================================================

inline void buildMetroNetwork(Graph& g) {

    // -------------------------------------------------------
    //  PURPLE LINE — East to West
    // -------------------------------------------------------
    struct StationDef { const char* id; const char* name; bool transfer; };

    std::vector<StationDef> purple = {
        { "ws",  "Whitefield",          false },
        { "hom", "Hoodi",               false },
        { "kar", "Kadugodi",            false },
        { "vvn", "Varthur",             false },
        { "ibl", "ITPL",                false },
        { "bru", "Brookefield",         false },
        { "kk",  "Kundalahalli",        false },
        { "svy", "Seetharampalya",      false },
        { "nb",  "Nallurhalli",         false },
        { "bng", "Baiyappanahalli",     false },
        { "swm", "Swami Vivekananda",   false },
        { "ind", "Indiranagar",         false },
        { "hal", "Halasuru",            false },
        { "tri", "Trinity",             false },
        { "mg",  "MG Road",             true  },   // transfer
        { "cub", "Cubbon Park",         true  },   // transfer
        { "vis", "Vidhana Soudha",      false },
        { "crd", "Central College",     false },
        { "mah", "Majestic",            true  },   // MAJOR interchange
        { "cit", "City Railway",        false },
        { "mag", "Magadi Road",         false },
        { "hos", "Hosahalli",           false },
        { "vij", "Vijayanagar",         false },
        { "att", "Attiguppe",           false },
        { "drs", "Deepanjali Nagar",    false },
        { "mys", "Mysuru Road",         false },
    };

    for (auto& s : purple)
        g.addStation(s.id, s.name, "purple", s.transfer);

    // { from, to, distance_km, time_min }
    struct EdgeDef { const char* u; const char* v; double d; double t; };
    std::vector<EdgeDef> purpleEdges = {
        {"ws","hom",1.8,4}, {"hom","kar",1.5,3}, {"kar","vvn",2.0,4},
        {"vvn","ibl",1.6,3}, {"ibl","bru",1.4,3}, {"bru","kk",1.2,3},
        {"kk","svy",1.3,3}, {"svy","nb",1.1,2}, {"nb","bng",1.5,3},
        {"bng","swm",1.3,3}, {"swm","ind",1.8,4}, {"ind","hal",1.5,3},
        {"hal","tri",1.2,3}, {"tri","mg",1.0,2}, {"mg","cub",0.8,2},
        {"cub","vis",0.9,2}, {"vis","crd",0.7,2}, {"crd","mah",0.9,2},
        {"mah","cit",0.8,2}, {"cit","mag",1.2,3}, {"mag","hos",1.4,3},
        {"hos","vij",1.5,3}, {"vij","att",1.2,3}, {"att","drs",1.3,3},
        {"drs","mys",2.0,4}
    };
    for (auto& e : purpleEdges)
        g.addEdge(e.u, e.v, e.d, e.t, "purple");

    // -------------------------------------------------------
    //  GREEN LINE — North to South
    // -------------------------------------------------------
    std::vector<StationDef> green = {
        { "nag",  "Nagasandra",          false },
        { "das",  "Dasarahalli",         false },
        { "jal",  "Jalahalli",           false },
        { "pny",  "Peenya Industry",     false },
        { "pen",  "Peenya",              false },
        { "ysh",  "Yeshwanthpur",        false },
        { "san",  "Sandal Soap",         false },
        { "mal",  "Mahalakshmi",         false },
        { "raj",  "Rajajinagar",         false },
        // "mah" = Majestic already added above (shared interchange)
        { "kri",  "Krishna Raja",        false },
        { "ntn",  "National College",    false },
        { "lap",  "Lalbagh",             false },
        { "sou",  "South End Circle",    false },
        { "jai2", "Jayanagar",           false },
        { "rsh",  "Rashtreeya Vidya",    false },
        { "ban",  "Banashankari",        false },
        { "jpa",  "JP Nagar",            false },
        { "pon",  "Puttenahalli",        false },
        { "bom",  "Bommanahalli",        false },
        { "con",  "Gottigere",           false },
    };

    for (auto& s : green)
        if (g.stations.find(s.id) == g.stations.end())
            g.addStation(s.id, s.name, "green", s.transfer);

    std::vector<EdgeDef> greenEdges = {
        {"nag","das",2.1,4}, {"das","jal",1.8,4}, {"jal","pny",2.3,5},
        {"pny","pen",1.0,2}, {"pen","ysh",2.5,5}, {"ysh","san",1.6,3},
        {"san","mal",1.5,3}, {"mal","raj",1.8,4}, {"raj","mah",2.0,4},
        {"mah","kri",1.0,2}, {"kri","ntn",0.9,2}, {"ntn","lap",1.2,3},
        {"lap","sou",1.3,3}, {"sou","jai2",1.4,3}, {"jai2","rsh",1.5,3},
        {"rsh","ban",1.8,4}, {"ban","jpa",1.6,3}, {"jpa","pon",1.7,4},
        {"pon","bom",2.0,4}, {"bom","con",2.2,5}
    };
    for (auto& e : greenEdges)
        g.addEdge(e.u, e.v, e.d, e.t, "green");
}
