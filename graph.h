#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <unordered_map>
#include <list>
#include <string>
#include <vector>
#include <cstdlib> // For rand() and srand()
using namespace std;

// Class representing a directed graph using an adjacency list
class Graph {
    unordered_map<string, list<string>> adjList; // Adjacency list: maps nodes to their list of neighbors

public:
    // Adds a directed edge from node 'u' to node 'v'
    void add_edge(const string& u, const string& v) {
        adjList[u].push_back(v); // Append 'v' to the neighbor list of 'u'
    }

    // Ensures a node exists in the graph, even if it has no edges
    void ensure_node_exists(const string& node) {
        if (adjList.find(node) == adjList.end()) {
            adjList[node] = {}; // Initialize the node with an empty neighbor list
        }
    }

    // Prints the adjacency list of the graph
    void print() const {
        cout << "Adjacency List:" << endl;
        for (const auto& pair : adjList) { // Iterate over all nodes
            cout << pair.first << " -> ";
            for (const auto& neighbor : pair.second) { // Iterate over the neighbors of the current node
                cout << neighbor << ", ";
            }
            cout << endl;
        }
    }

    // Returns a constant reference to the adjacency list
    const unordered_map<string, list<string>>& get_adj_list() const {
        return adjList; // Provides read-only access to the graph's structure
    }
};

// Function to populate a graph with nodes, cycles, random edges, and isolated nodes
void graphData(Graph& g) {
    // Define parameters for the graph
    const int total_nodes = 100000; // Total number of nodes in the graph
    int total_cycles = 0;          // Counter for the number of cycles created
    int total_edges = 0;           // Counter for the number of random edges added
    int total_isolated = 0;        // Counter for the number of isolated nodes

    vector<string> nodes; // List of node names
    for (int i = 1; i <= total_nodes; ++i) {
        // Generate node names like "Node1", "Node2", ..., "NodeN"
        nodes.push_back("Node" + to_string(i));
        g.ensure_node_exists("Node" + to_string(i)); // Add the node to the graph
    }

    // Add cycles to the graph
    for (int i = 0; i < 1000; ++i) { // Create 1000 cycles
        total_cycles += 1; // Increment cycle count
        int cycle_size = 5; // Each cycle will have 5 nodes
        vector<string> cycle_nodes;

        // Randomly select nodes to form the cycle
        for (int j = 0; j < cycle_size; ++j) {
            cycle_nodes.push_back(nodes[rand() % total_nodes]);
        }

        // Add directed edges to form the cycle
        for (int j = 0; j < cycle_size; ++j) {
            g.add_edge(cycle_nodes[j], cycle_nodes[(j + 1) % cycle_size]);
        }
    }

    // Add random edges to make the graph more realistic
    for (int i = 0; i < 10000; ++i) { // Add 10,000 random edges
        total_edges += 1; // Increment edge count
        string node1 = nodes[rand() % total_nodes]; // Select a random source node
        string node2 = nodes[rand() % total_nodes]; // Select a random destination node
        if (node1 != node2) { // Avoid self-loops
            g.add_edge(node1, node2);
        }
    }

    // Ensure some nodes remain isolated
    for (int i = 0; i < 10000; ++i) { //
        total_isolated += 1; // Increment isolated node count
        string isolated_node = nodes[rand() % total_nodes]; // Select a random node
        g.ensure_node_exists(isolated_node); // Ensure the node exists but has no edges
    }

    // Print summary of the generated graph
    cout << "Graph generated with " << total_nodes << " nodes, " << total_cycles << " cycles, " << total_edges << " random edges, and " << total_isolated << " isolated nodes." << endl << endl;
}

#endif // GRAPH_H