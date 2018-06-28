/** \file Profiler.h
 * Runtime profiler
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_PROFILER_H
#define OPEN_SEA_PROFILER_H

#include <string>
#include <vector>

namespace open_sea::profiler {
    // Notes:
    //  - Result of profiling is a tree of runtime information nodes for code blocks.
    //  - Time in a node is start time while executing to save space (not needed after duration is know --> can be replaced).
    //  - Result is built into a vector of nodes, therefore the tree links are vector indices.
    //  - Root of result has no siblings and is at index 0.

    /**
     * \brief Node of the result tree
     * Node of the tree produced by the profiler.
     * Contains label and execution time of a block of code.
     * Structure of the tree represents relationships between blocks of code.
     * During execution, \c time is the start time of the execution.
     * After execution, \c time is the duration of the execution.
     * Links between nodes are indices into a vector where the tree is being built.
     * Root node has no siblings and is at index 0.
     */
    struct Node {
        // Tree data
        //! Parent (block of code that contains it) or -1 if root
        //TODO: maybe have root be recognised by being at index 0? compare complexity of approaches
        int parent;
        //! Next sibling (block of code that follows it)
        int next;   // only valid when > 1 (root is 0 and 1 cannot be the next)
        //! First child (first contained block)
        int child;  // only valid when > 0

        // Content
        //! Label
        std::string label;
        //! Execution duration in seconds (only when finished, start time while executing)
        double time;

        Node(int parent, const std::string &label);
    };

    void start();
    void finish();

    void push(const std::string &label);
    void pop();

    std::vector<Node> get_last();

    void show_text();
}

#endif //OPEN_SEA_PROFILER_H
