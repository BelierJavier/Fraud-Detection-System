#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <set>
#include <string>
#include <omp.h>
#include <algorithm>
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

    // Trace back the path from the start node to the end node using the parent map
    while (current != end) {
        cycle.push_back(current);
        current = parent.at(current);
    }
    cycle.push_back(end);
    cycle.push_back(start); // Close the cycle by adding the start node again

    // Normalize the cycle if it has more than two nodes by sorting it
    if (cycle.size() > 2) {
        sort(cycle.begin(), cycle.end());
        return cycle;
    }
    return {}; // Return an empty vector if it's not a valid cycle
}



// DFS function to detect cycles
void dfs(const string& node,
         const unordered_map<string, list<string>>& graph,
         unordered_set<string>& visited,
         unordered_set<string>& recStack,
         unordered_map<string, string>& parent,
         vector<vector<string>>& cycles) {

    visited.insert(node); // Mark the current node as visited
    recStack.insert(node); // Add the node to the recursion stack

    // If the node has no outgoing edges, return immediately
    if (graph.find(node) == graph.end()) {
        recStack.erase(node);
        return;
    }



    // Explore all neighbors of the current node
    for (const string& neighbor : graph.at(node)) {
        if (visited.find(neighbor) == visited.end()) {
            // If the neighbor is not visited, mark the current node as its parent and recurse
            parent[neighbor] = node;
            dfs(neighbor, graph, visited, recStack, parent, cycles);
        } else if (recStack.find(neighbor) != recStack.end()) {
            // If the neighbor is in the recursion stack, a cycle is detected
            auto cycle = reconstructCycle(node, neighbor, parent);
            if (!cycle.empty()) {
                cycles.push_back(cycle); // Add the cycle to the list of detected cycles
            }
        }
    }

    recStack.erase(node); // Remove the node from the recursion stack
}



// Parallel DFS worker function
void parallel_dfs_worker(const vector<string>& nodes,
                          const unordered_map<string, list<string>>& graph,
                          unordered_set<string>& global_visited,
                          set<vector<string>>& global_cycles,
                          mutex& visited_mutex,
                          mutex& cycle_mutex) {

    unordered_set<string> local_visited; // Local visited set for this thread
    unordered_set<string> local_recStack; // Local recursion stack
    unordered_map<string, string> local_parent; // Local parent map for reconstructing cycles
    vector<vector<string>> local_cycles; // Local cycles detected by this thread

    // Process each node in the assigned subset
    for (const auto& node : nodes) {
        {
            // Ensure global synchronization to avoid duplicate processing of nodes
            lock_guard<mutex> lock(visited_mutex);
            if (global_visited.find(node) != global_visited.end()) {
                continue; // Skip nodes already processed by another thread
            }
            global_visited.insert(node); // Mark the node as globally visited
        }

        // Perform DFS from the current node
        dfs(node, graph, local_visited, local_recStack, local_parent, local_cycles);
    }

    {
        // Normalize and merge the local cycles into the global set
        lock_guard<mutex> lock(cycle_mutex);
        for (auto& cycle : local_cycles) {
            sort(cycle.begin(), cycle.end()); // Normalize the cycle
            global_cycles.insert(cycle);     // Add the cycle to the global set
        }
    }
}



// Parallel cycle detection function
vector<vector<string>> threading_fraudulent_cycles(const Graph& g, int num_threads = 4) {
    const auto& adjList = g.get_adj_list(); // Get the adjacency list representation of the graph
    vector<string> nodes;
    for (const auto& pair : adjList) {
        nodes.push_back(pair.first); // Collect all nodes in the graph
    }

    size_t chunk_size = nodes.size() / num_threads; // Determine the size of each thread's workload
    vector<thread> threads;
    set<vector<string>> global_cycles; // Global set to store unique cycles
    unordered_set<string> global_visited; // Global visited set to prevent duplicate work
    mutex cycle_mutex; // Mutex for synchronizing access to global_cycles
    mutex visited_mutex; // Mutex for synchronizing access to global_visited

    // Launch threads to process chunks of nodes in parallel
    for (int t = 0; t < num_threads; ++t) {
        size_t start_idx = t * chunk_size;
        size_t end_idx = (t == num_threads - 1) ? nodes.size() : start_idx + chunk_size;
        vector<string> thread_nodes(nodes.begin() + start_idx, nodes.begin() + end_idx);

        // Each thread runs the parallel DFS worker function
        threads.emplace_back(parallel_dfs_worker, ref(thread_nodes), ref(adjList),
                             ref(global_visited), ref(global_cycles), ref(visited_mutex), ref(cycle_mutex));
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Convert the global set of cycles into a vector and return it
    return vector<vector<string>>(global_cycles.begin(), global_cycles.end());
}



// OpenMP Parallel DFS function
vector<vector<string>> openMP_dfs(const Graph& g, int num_threads) {
    const auto& adjList = g.get_adj_list();
    vector<string> nodes;
    for (const auto& pair : adjList) {
        nodes.push_back(pair.first);
    }

    vector<set<vector<string>>> thread_local_cycles(num_threads);  // Thread-local cycle storage
    vector<unordered_set<string>> thread_local_visited(num_threads); // Thread-local visited sets

    omp_set_num_threads(num_threads);

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        unordered_set<string> local_recStack;
        unordered_map<string, string> local_parent;

        #pragma omp for schedule(dynamic)
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (thread_local_visited[thread_id].find(nodes[i]) == thread_local_visited[thread_id].end()) {
                dfs(nodes[i], adjList, thread_local_visited[thread_id], local_recStack, local_parent, thread_local_cycles[thread_id]);
            }
        }
    }

    // Post-processing: Merge thread-local cycles into a global set
    set<vector<string>> global_cycles;
    for (const auto& thread_cycles : thread_local_cycles) {
        for (const auto& cycle : thread_cycles) {
            global_cycles.insert(cycle);  // Normalize and deduplicate automatically via `set`
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
        unordered_set<string> visited; // Tracks visited nodes
        unordered_set<string> recStack; // Tracks the recursion stack
        unordered_map<string, string> parent; // Tracks the parent of each node for cycle reconstruction
        const auto& adjList = g.get_adj_list(); // Get the adjacency list of the graph

        for (const auto& pair : adjList) {
            const string& node = pair.first;
            if (visited.find(node) == visited.end()) {
                dfs(node, adjList, visited, recStack, parent, cycles); // Perform DFS from unvisited nodes
            }
        }
    }
    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time);

    cout << "Sequential DFS Fradulent Cycles Detected: " << cycles.size() << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;

    // Threading parallel cycle detection
    start_time = high_resolution_clock::now();
    vector<vector<string>> parallel_cycles = threading_fraudulent_cycles(g, 4); // Using 4 threads
    end_time = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end_time - start_time);
    cout << "Parallel DFS Fradulent Cycles Detected: " << parallel_cycles.size() << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;

    // OpenMP parallel cycle detection
    start_time = high_resolution_clock::now();
    auto parallel_cycles = openMP_dfs(g, 4);
    end_time = high_resolution_clock::now();
    auto parallel_duration = duration_cast<milliseconds>(end_time - start_time);
    cout << "OpenMP DFS Cycles Detected: " << parallel_cycles.size() << endl;
    cout << "Time taken: " << parallel_duration.count() << " ms" << endl;

    return 0;
}
