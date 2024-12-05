#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <unordered_map>
#include <list>
#include <string>
using namespace std;

class Graph {
    unordered_map<string, list<string>> adjList;

public:
    // Add a directed edge between u and v
    void add_edge(const string& u, const string& v) {
        adjList[u].push_back(v);
    }

    // Ensure nodes exist
    void ensure_node_exists(const string& node) {
        if (adjList.find(node) == adjList.end()) {
            adjList[node] = {}; // Add node with an empty list
        }
    }

    // Print the adjacency list
    void print() const {
        cout << "Adjacency List:" << endl;
        for (const auto& pair : adjList) {
            cout << pair.first << " -> ";
            for (const auto& neighbor : pair.second) {
                cout << neighbor << ", ";
            }
            cout << endl;
        }
    }

    const unordered_map<string, list<string>>& get_adj_list() const {
        return adjList;
    }
};



void graphData(Graph& g) {
    // Create nodes
    const int total_nodes = 100000;
    int total_cycles = 0;
    int total_edges = 0;
    int total_isolated = 0;

    vector<string> nodes;
    for (int i = 1; i <= total_nodes; ++i) {
        nodes.push_back("Node" + to_string(i));
        g.ensure_node_exists("Node" + to_string(i));
    }

    // Add cycles with varying depths
    for (int i = 0; i < 1000; ++i) { // Create cycles
        total_cycles += 1;
        int cycle_size = 5;
        vector<string> cycle_nodes;
        for (int j = 0; j < cycle_size; ++j) {
            cycle_nodes.push_back(nodes[rand() % total_nodes]);
        }
        for (int j = 0; j < cycle_size; ++j) {
            g.add_edge(cycle_nodes[j], cycle_nodes[(j + 1) % cycle_size]);
        }
    }

    // Add random edges to create a realistic graph structure
    for (int i = 0; i < 10000; ++i) { // Add random edges
        total_edges += 1;
        string node1 = nodes[rand() % total_nodes];
        string node2 = nodes[rand() % total_nodes];
        if (node1 != node2) {
            g.add_edge(node1, node2);
        }
    }

    // Leave some nodes isolated (no edges)
    for (int i = 0; i < 10000; ++i) { // Ensure isolated nodes
        total_isolated += 1;
        string isolated_node = nodes[rand() % total_nodes];
        // Ensure it has no edges
        g.ensure_node_exists(isolated_node);
    }

    cout << "Graph generated with " << total_nodes << " nodes, " << total_cycles << " cycles, " << total_edges << " random edges, and " << total_isolated << " isolated nodes." << endl << endl;
}

#endif