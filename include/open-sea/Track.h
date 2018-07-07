/** \file Track.h
 * Track: tree-stack hybrid
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_TRACK_H
#define OPEN_SEA_TRACK_H

#import <vector>
#import <string>
#include <sstream>

namespace open_sea::data {

    /** \class Track
     * \brief Tree-stack hybrid
     * Hybrid data structure that can be built up as a stack, but retains "memory" of past state.
     * Can be used to reconstruct the stack's evolution.
     * The tree has an implied root that represents an empty stack, therefore what is stored is actually a forest of
     *  subtrees of this implied root.
     *
     * \tparam T Value type (stack elements)
     */
    template<class T>
    class Track {
        public:
            //! Type of the node content
            typedef T value_type;
            typedef unsigned size_type;

            //! Tree node
            struct Node {
                //! Value representing invalid index (for example no next sibling)
                static constexpr unsigned INVALID = static_cast<const unsigned int>(~0);
                //! Parent (\c INVALID means implied root)
                unsigned parent;
                //! Next sibling (\c INVALID means none)
                unsigned next = INVALID;
                //! First child (\c INVALID means none)
                unsigned firstChild = INVALID;
                //! Node content
                value_type content;

                Node(unsigned int parent, T content) : parent(parent), content(content) {}
            };
        protected:
            //! Tree data
            std::vector<Node> store;
            //! Index of current node
            unsigned current;
            //! Index of last child of current node
            unsigned lastChild;
            //! Length of path from root to current
            size_type stack_size;
            //! Number of nodes in store (number of tree nodes without implied root)
            // Index of the next element to be inserted into the store
            size_type tree_size;
        private:
            std::string toIndentedStringRec(unsigned index, unsigned depth);
        public:
            Track();

            void push(value_type content);
            void pop();
            //! Get reference to the top element of the stack
            // Reference to allow a way to change contents
            value_type& top() { return store[current].content; }
            void clear();

            size_type stackSize() { return stack_size; }
            size_type treeSize() { return tree_size; }

            std::string toIndentedString();
    };

    /**
     * \brief Construct an empty track
     * Construct a track representing an empty stack.
     *
     * \tparam T Value type
     */
    // The tree contains only the implied root
    template<class T>
    Track<T>::Track() {
        current = Node::INVALID;
        lastChild = Node::INVALID;
        stack_size = 0;
        tree_size = 0;
    }

    /**
     * \brief Push element onto the stack
     * Push an element onto the stack.
     * This creates a new node in the tree as a next child of the current node and fills it with the provided content.
     *
     * \tparam T Value type
     * \param content Element
     */
    template<class T>
    void Track<T>::push(T content) {
        // Add as first child if the current node is valid and has no valid first child
        if (current != Node::INVALID && store[current].firstChild == Node::INVALID)
            store[current].firstChild = tree_size;

        // Add as next child if the last child is valid
        if (lastChild != Node::INVALID)
            store[lastChild].next = tree_size;

        // Add the actual node
        store.emplace_back(current, content);

        // Update state
        current = tree_size;
        lastChild = Node::INVALID;  // Newly created node has no children
        tree_size++;
        stack_size++;
    }

    /**
     * \brief Pop element from the stack
     * Pop element from the top of the stack.
     * This moves the current and next child pointers one level up in the tree.
     * Does nothing when the stack is empty.
     *
     * \tparam T Value type
     */
    template<class T>
    void Track<T>::pop() {
        // Skip if current is invalid
        if (current == Node::INVALID)
            return;

        // Point next child to current, current to parent, and decrement stack size
        lastChild = current;
        current = store[current].parent;
        stack_size--;
    }

    /**
     * \brief Clear track
     * Clear the tree and stack.
     * This returns the track into a state same as right after construction.
     *
     * \tparam T Value type
     */
    template<class T>
    void Track<T>::clear() {
        store.clear();
        current = Node::INVALID;
        lastChild = Node::INVALID;
        stack_size = 0;
        tree_size = 0;
    }

    /**
     * \brief Get contents of the node at the index and its children, indented starting at the depth
     * Recursively get the contents of the node at the specified index and its children as an indented string.
     * The indentation starts at the specified depth, increases by 1 every time a child is descended into, and is done
     *  with TABs.
     *
     * \tparam T Value type
     * \param index Node index
     * \param depth Starting indentation depth
     * \return Indented string
     */
    // Recursive helper for Track::toIndentedString()
    template<class T>
    std::string Track<T>::toIndentedStringRec(unsigned index, unsigned depth) {
        // Prepare result
        std::ostringstream stream;

        // Terminate on invalid
        if (index == Node::INVALID)
            return stream.str();

        // Print indentation
        for (int j = 0; j < depth; j++) {
            stream << '\t';
        }

        // Print content and newline
        stream << store[index].content << '\n';

        // Recurse to children
        unsigned child = store[index].firstChild;
        while (child != Node::INVALID) {
            stream << toIndentedStringRec(child, depth + 1);
            child = store[child].next;
        }

        // Return as string
        return stream.str();
    }

    /**
     * \brief Get track contents as an indented string
     * Get track contents with each node on a separate line and indented (with TABs) by depth.
     * The children of the implied root start with no indentation.
     *
     * \tparam T Value type
     * \return Indented string
     */
    template<class T>
    std::string Track<T>::toIndentedString() { return toIndentedStringRec(0, 0); }
}

#endif //OPEN_SEA_TRACK_H
