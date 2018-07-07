/*
 * Track implementation
 */
#include <open-sea/Track.h>

#include <sstream>

namespace open_sea::data {
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
        current = 0;
        lastChild = 0;
        stack_size = 0;
        tree_size = 0;
    }

    template<class T>
    std::string Track<T>::toIndentedStringRec(unsigned i, unsigned d) {
        // Prepare result
        std::ostringstream stream;

        // Terminate on invalid
        if (i == Node::INVALID)
            return stream.str();

        // Print indentation
        for (int j = 0; j < d; j++) {
            stream << '\t';
        }

        // Print content and newline
        stream << store[i].content << '\n';

        // Recurse to children
        unsigned child = store[i].firstChild;
        while (child != Node::INVALID) {
            stream << toIndentedStringRec(child, d + 1);
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
