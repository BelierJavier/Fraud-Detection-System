#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <set>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include "graph.h"

using namespace std;
using namespace std::chrono;

// Function to reconstruct a cycle path
vector<string> reconstructCycle(const string& start, const string& end, const unordered_map<string, string>& parent) {
    vector<string> cycle;
    string current = start;

    while (current != end) {
        cycle.push_back(current);
        current = parent.at(current);
    }
    cycle.push_back(end);
    cycle.push_back(start);

    if (cycle.size() > 2) {
        sort(cycle.begin(), cycle.end());
        return cycle;
    }
    return {};
}



// DFS function to detect cycles
void dfs(const string& node,
         const unordered_map<string, list<string>>& graph,
         unordered_set<string>& visited,
         unordered_set<string>& recStack,
         unordered_map<string, string>& parent,
         vector<vector<string>>& cycles) {

    visited.insert(node);
    recStack.insert(node);

    if (graph.find(node) == graph.end()) {
        recStack.erase(node);
        return;
    }

    for (const string& neighbor : graph.at(node)) {
        if (visited.find(neighbor) == visited.end()) {
            parent[neighbor] = node;
            dfs(neighbor, graph, visited, recStack, parent, cycles);
        } else if (recStack.find(neighbor) != recStack.end()) {
            auto cycle = reconstructCycle(node, neighbor, parent);
            if (!cycle.empty()) {
                cycles.push_back(cycle);
            }
        }
    }

    recStack.erase(node);
}



// Parallel DFS worker function
void parallel_dfs_worker(const vector<string>& nodes,
                          const unordered_map<string, list<string>>& graph,
                          unordered_set<string>& global_visited,
                          set<vector<string>>& global_cycles,
                          mutex& visited_mutex,
                          mutex& cycle_mutex) {

    unordered_set<string> local_visited;
    unordered_set<string> local_recStack;
    unordered_map<string, string> local_parent;
    vector<vector<string>> local_cycles;

    for (const auto& node : nodes) {
        {
            lock_guard<mutex> lock(visited_mutex);
            if (global_visited.find(node) != global_visited.end()) {
                continue; // Node already processed
            }
            global_visited.insert(node);
        }

        dfs(node, graph, local_visited, local_recStack, local_parent, local_cycles);
    }

    {
        lock_guard<mutex> lock(cycle_mutex);
        for (auto& cycle : local_cycles) {
            sort(cycle.begin(), cycle.end()); // Normalize the cycle
            global_cycles.insert(cycle);     // Add to global set
        }
    }
}



// Parallel cycle detection function
vector<vector<string>> parallel_fraudulent_cycles(const Graph& g, int num_threads = 4) {
    const auto& adjList = g.get_adj_list();
    vector<string> nodes;
    for (const auto& pair : adjList) {
        nodes.push_back(pair.first);
    }

    size_t chunk_size = nodes.size() / num_threads;
    vector<thread> threads;
    set<vector<string>> global_cycles;
    unordered_set<string> global_visited;
    mutex cycle_mutex;
    mutex visited_mutex;

    for (int t = 0; t < num_threads; ++t) {
        size_t start_idx = t * chunk_size;
        size_t end_idx = (t == num_threads - 1) ? nodes.size() : start_idx + chunk_size;
        vector<string> thread_nodes(nodes.begin() + start_idx, nodes.begin() + end_idx);

        threads.emplace_back(parallel_dfs_worker, ref(thread_nodes), ref(adjList),
                             ref(global_visited), ref(global_cycles), ref(visited_mutex), ref(cycle_mutex));
    }

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return vector<vector<string>>(global_cycles.begin(), global_cycles.end());
}



int main() {
    Graph g;

    // Generate the graph
    graphData(g);

    // Time standard cycle detection
    auto start_time = high_resolution_clock::now();
    vector<vector<string>> cycles;
    {
        // Regular DFS cycle detection logic
        unordered_set<string> visited;
        unordered_set<string> recStack;
        unordered_map<string, string> parent;
        const auto& adjList = g.get_adj_list();

        for (const auto& pair : adjList) {
            const string& node = pair.first;
            if (visited.find(node) == visited.end()) {
                dfs(node, adjList, visited, recStack, parent, cycles);
            }
        }
    }
    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time);

    cout << "Regular DFS Fradulent Cycles Detected: " << cycles.size() << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;

    // Time parallel cycle detection
    start_time = high_resolution_clock::now();
    vector<vector<string>> parallel_cycles = parallel_fraudulent_cycles(g, 4); // Using 4 threads
    end_time = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end_time - start_time);

    cout << "Parallel DFS Fradulent Cycles Detected: " << parallel_cycles.size() << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;

    return 0;
}
