/** \file Table.h
 * Definition of a table data structure, which stores multi-element records in a variety of layouts.
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_TABLE_H
#define OPEN_SEA_TABLE_H

#include <open-sea/Util.h>

#include <memory>
#include <algorithm>
#include <functional>

//TODO use page allocator
//TODO make sure copying deals well with objects
namespace open_sea::data {
    /**
     * \addtogroup Data
     *
     * @{
     */

    /** \class Table
     * Stores records associated with keys.
     *
     * \tparam K Key type
     * \tparam R Record type
     */
    template<typename K, typename R>
    class Table {
        public:
            //! Key type
            typedef K key_t;
            //! Record type
            typedef R record_t;
            //! Record pointer type (struct of pointers to members of R)
            typedef typename R::Ptr record_ptr_t;

            /**
             * \brief Add record under the provided key
             *
             * Copies the values of the provided record and associates them with the key.
             * Does nothing when the key already has a record associated with it.
             *
             * \param key Key to associate with the record
             * \param record Record to add
             * \return `true` iff the structure was modified (i.e. the record was added)
             */
            virtual bool add(const key_t &key, const record_t &record) = 0;

            /**
             * \brief Remove record associated with the provided key
             *
             * Does nothing when no record is associated with the key.
             * Done by copying the last record over the one to be removed and decrementing size.
             * Potentially reshuffles the data, and therefore invalidates any references to it.
             *
             * \param key Key to remove
             * \return `true` iff the structure was modified (i.e. the key was present and associated record removed)
             */
            virtual bool remove(const key_t &key) = 0;

            /**
             * Get copy of record under the provided key
             *
             * \param key Key to look up
             * \return Instance of R whose members are copies of values associated with the provided key
             *
             * \throws std::out_of_range When no record is associated with the provided key
             */
            virtual record_t get_copy(const key_t &key) = 0;

            /**
             * Get read-write reference to the record under the provided key
             *
             * \param key Key to look up
             * \return Instance of R::Ptr (struct of pointers to members of R) whose members point to the values
             *  associated with the provided key
             *
             * \throws std::out_of_range When no record is associated with the provided key
             */
            virtual record_ptr_t get_reference(const key_t &key) = 0;

            /**
             * Get read-write reference to the first record
             *
             * \return Instance of R::Ptr (struct of pointers to members of R) whose members point to the values
             *  associated with the first record
             */
            virtual record_ptr_t get_reference() = 0;

            /**
             * Increment reference to point to the next record
             *
             * \param ref Reference to increment
             */
            virtual void increment_reference(record_ptr_t &ref) = 0;

            //! Get number of records
            virtual unsigned int size() = 0;
    };

    /** \class TableAoS
     * Table that stores its records as an array of structs
     *
     * \tparam K Key type
     * \tparam R Record type
     */
    template<typename K, typename R>
    class TableAoS : public Table<K, R> {
        public:
            //! Key type
            typedef K key_t;
            //! Record type
            typedef R record_t;
            //! Record pointer type (struct of pointers to members of R)
            typedef typename R::Ptr record_ptr_t;

        private:
            //! Map of keys to indices to the data
            std::unordered_map<K, unsigned int> map{};
            //! Start of the data
            record_t *data = nullptr;
            //! Number of records stored
            unsigned int n = 0;
            //! Allocated space in terms of number of records that fit in it
            unsigned int capacity = 0;

            //! Allocator
            std::allocator<record_t> allocator;

        public:
            //! Construct the table and defer allocation to first insertion
            TableAoS() = default;

            /**
             * Construct the table and allocate space for `count` records
             *
             * \param count Number of records to allocate space for
             */
            TableAoS(unsigned int count) { allocate(count); }

            bool add(const key_t &key, const record_t &record) override;
            bool remove(const key_t &key) override;
            record_t get_copy(const key_t &key) override;
            record_ptr_t get_reference(const key_t &key) override;
            record_ptr_t get_reference() override;
            void increment_reference(record_ptr_t &ref) override { util::invoke_n<record_t::count, IncrementHelper>(ref); }
            unsigned int size() override { return n; }

        private:
            //! Helper functor to fill Nth member of R::Ptr with a pointer to the appropriate value of record i
            template <unsigned int N>
            struct GetRefHelper {
                void operator()(record_t *arr, const unsigned int i, record_ptr_t &result) {
                    // Get the appropriate value's address
                    auto ptr = &(std::invoke(util::get_pointer_to_member<record_t, N>(), arr[i]));

                    // Set result's member to the address
                    std::invoke(util::get_pointer_to_member<record_ptr_t, N>(), result) = ptr;
                }
            };

            //! Helper functor to increment Nth member of R::Ptr
            template <unsigned int N>
            struct IncrementHelper {
                void operator()(record_ptr_t &ref) {
                    // Increment member by sizeof(R) bytes (next element, same offset)
                    // Note: cast to byte ptr -> increment by size of R -> cast back to member type ptr
                    typedef typename util::GetMemberType<record_ptr_t, N>::type member_type;
                    auto member_p = util::get_pointer_to_member<record_ptr_t, N>();
                    std::invoke(member_p, ref) = (member_type) (((unsigned char *) std::invoke(member_p, ref)) + sizeof(record_t));
                }
            };

            void allocate(unsigned int size);
    };

    /**
     * Allocate space for the data, potentially copying over data from previous space if needed
     *
     * \tparam K Key type
     * \tparam R Record type
     * \param size Number of records to allocate space for
     */
    template<typename K, typename R>
    void TableAoS<K, R>::allocate(unsigned int size) {
        // Make sure data will fit into new space
        assert(size > n);

        // Allocate space
        record_t *target = allocator.allocate(size);

        // Copy data into new space
        if (n > 0) {
            std::copy_n(data, n, target);
        }

        // Deallocate old data
        if (data && capacity > 0) {
            allocator.deallocate(data, capacity);
        }

        // Update state
        data = target;
        capacity = size;
    }

    template<typename K, typename R>
    bool TableAoS<K, R>::add(const key_t &key, const record_t &record) {
        // Check the key is not present yet
        try {
            map.at(key);
            return false;
        } catch (std::out_of_range &e) {}

        // Check there is enough space
        if (capacity <= n) {
            // At capacity -> reallocate double capacity (but at least 1)
            allocate((capacity == 0) ? 1 : capacity * 2);
        }

        // Set row to the record
        data[n] = record;

        // Update state
        map[key] = n;
        n++;

        return true;
    }

    template<typename K, typename R>
    bool TableAoS<K, R>::remove(const key_t &key) {
        // Check the key is present
        try {
            // Get the index to delete and of last item
            unsigned int index = map.at(key);
            unsigned int last = n - 1;

            // Only copy over when not last
            if (index != last) {
                // Move last into deleted
                data[index] = data[last];

                // Update key-index map
                for (auto i = map.begin(); i != map.end(); i++) {
                    if (i->second == last) {
                        i->second = index;
                        break;
                    }
                }
            }

            // Decrement size and erase the removed key
            n--;
            map.erase(key);

            return true;
        } catch (std::out_of_range &e) {
            // Not present -> nothing to remove
            return false;
        }
    }

    template<typename K, typename R>
    typename TableAoS<K, R>::record_t TableAoS<K, R>::get_copy(const key_t &key) {
        // Check the key is present
        try {
            // Return copy of the record associated with the key
            return data[map.at(key)];
        } catch (std::out_of_range &e) {
            // Not present -> error
            throw std::out_of_range("No record found for the provided key.");
        }
    }

    template<typename K, typename R>
    typename TableAoS<K, R>::record_ptr_t TableAoS<K, R>::get_reference(const key_t &key) {
        // Check the key is present
        try {
            // Get the index and prepare result
            unsigned int index = map.at(key);
            record_ptr_t result;

            // Set result to point to the correct entries
            util::invoke_n<record_t::count, GetRefHelper>(data, index, result);

            return result;
        } catch (std::out_of_range &e) {
            // Not present -> error
            throw std::out_of_range("No record found for the provided key.");
        }
    }

    template<typename K, typename R>
    typename TableAoS<K, R>::record_ptr_t TableAoS<K, R>::get_reference() {
        // Prepare result
        record_ptr_t result;

        // Set result to point to array starts
        util::invoke_n<record_t::count, GetRefHelper>(data, 0u, result);

        return result;
    }


    /** \class TableSoA
     * Table that stores its records as a struct of arrays
     *
     * \tparam K Key type
     * \tparam R Record type
     */
    template<typename K, typename R>
    class TableSoA : public Table<K, R> {
        public:
            //! Key type
            typedef K key_t;
            //! Record type
            typedef R record_t;
            //! Record pointer type (struct of pointers to members of R)
            typedef typename R::Ptr record_ptr_t;

        private:
            //! Map of keys to indices to the data
            std::unordered_map<key_t, unsigned int> map{};
            //! Start of the data
            void *data = nullptr;
            //! Starts of data arrays
            void *arrays[record_t::count] {nullptr};
            //! Number of records stored
            unsigned int n = 0;
            //! Allocated space in terms of number of records that fit in it
            unsigned int capacity = 0;

            //! Allocator
            std::allocator<unsigned char> allocator;

        public:
            //! Construct the table and defer allocation to first insertion
            TableSoA() = default;

            /**
             * Construct the table and allocate space for `count` records
             *
             * \param count Number of records to allocate space for
             */
            TableSoA(unsigned int count) { allocate(count); }

            bool add(const key_t &key, const record_t &record) override;
            bool remove(const key_t &key) override;
            record_t get_copy(const key_t &key) override;
            record_ptr_t get_reference(const key_t &key) override;
            record_ptr_t get_reference() override;
            void increment_reference(record_ptr_t &ref) override { util::invoke_n<record_t::count, IncrementHelper>(ref); }
            unsigned int size() override { return n; }

        private:
            //! Helper functor to compute start of Nth data array based on the previous one (N != 0) and the capacity
            template <unsigned int N>
            struct AllocatePtrHelper {
                void operator()(void **target_arrays, const unsigned int size) {
                    auto previous = (typename util::GetMemberType<record_t, N - 1>::type *) target_arrays[N - 1];
                    previous += size;
                    target_arrays[N] = (void*) previous;
                }
            };

            //! Specialization of the \ref AllocatePtrHelper functor to do nothing when N is 0 (handled separately)
            template <>
            struct AllocatePtrHelper<0> { void operator()(void */*target_arrays*/[], const unsigned int /*size*/) {} };

            //! Helper functor to copy Nth data array after reallocation
            template <unsigned int N>
            struct AllocateCopyHelper {
                void operator()(void **src_arrays, void **dest_arrays, const unsigned int count) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto src = static_cast<member_type *>(src_arrays[N]);
                    auto dest = static_cast<member_type *>(dest_arrays[N]);
                    std::copy_n(src, count, dest);
                }
            };

            //! Helper functor to append the Nth member of the provided record to the appropriate data array
            template <unsigned int N>
            struct AddHelper {
                void operator()(void **arr, const unsigned int count, const record_t &record) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto start = static_cast<member_type *>(arr[N]);
                    start[count] = std::invoke(util::get_pointer_to_member<record_t, N>(), record);
                }
            };

            //! Helper functor to copy Nth member of the last record to the provided index
            template <unsigned int N>
            struct RemoveHelper {
                void operator()(void **arr, const unsigned int index, const unsigned int last) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto start = static_cast<member_type *>(arr[N]);
                    start[index] = start[last];
                }
            };

            //! Helper functor to fill Nth member of R with a copy of the appropriate value of record i
            template <unsigned int N>
            struct GetCopyHelper {
                void operator()(void **arr, const unsigned int i, record_t &result) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto start = static_cast<member_type *>(arr[N]);
                    std::invoke(util::get_pointer_to_member<record_t, N>(), result) = start[i];
                }
            };

            //! Helper functor to fill Nth member of R::Ptr with a pointer to the appropriate value of record i
            template <unsigned int N>
            struct GetRefHelper {
                void operator()(void **arr, const unsigned int i, record_ptr_t &result) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto start = static_cast<member_type *>(arr[N]);
                    std::invoke(util::get_pointer_to_member<record_ptr_t, N>(), result) = start + i;
                }
            };

            //! Helper functor to increment Nth member of R::Ptr
            template <unsigned int N>
            struct IncrementHelper {
                void operator()(record_ptr_t &ref) {
                    // Increment member by 1 (next element of its array)
                    auto member_p = util::get_pointer_to_member<record_ptr_t, N>();
                    std::invoke(member_p, ref)++;
                }
            };

            void allocate(unsigned int size);
    };

    /**
     * Allocate space for the data, potentially copying over data from previous space if needed
     *
     * \tparam K Key type
     * \tparam R Record type
     * \param size Number of records to allocate space for
     */
    template<typename K, typename R>
    void TableSoA<K, R>::allocate(unsigned int size) {
        // Make sure data will fit into new space
        assert(size > n);

        // Compute size of new space in B
        unsigned byte_count = size * sizeof(record_t);

        // Allocate space
        void *target = allocator.allocate(byte_count);

        // Compute array start pointers
        void *target_arrays[record_t::count];
        target_arrays[0] = target;
        util::invoke_n<record_t::count, AllocatePtrHelper>(target_arrays, size);

        // Copy data into new space
        if (n > 0) {
            util::invoke_n<record_t::count, AllocateCopyHelper>(arrays, target_arrays, n);
        }

        // Deallocate old data
        if (data && capacity > 0) {
            allocator.deallocate(static_cast<unsigned char *>(data), capacity * sizeof(record_t));
        }

        // Update state
        data = target;
        std::copy(std::begin(target_arrays), std::end(target_arrays), std::begin(arrays));
        capacity = size;
    }

    template<typename K, typename R>
    bool TableSoA<K, R>::add(const key_t &key, const R &record) {
        // Check the key is not present yet
        try {
            map.at(key);
            return false;
        } catch (std::out_of_range &e) {}

        // Check there is enough space
        if (capacity <= n) {
            // At capacity -> reallocate double capacity (but at least 1)
            allocate((capacity == 0) ? 1 : capacity * 2);
        }

        // Set row to the record
        util::invoke_n<record_t::count, AddHelper>(arrays, n, record);

        // Update state
        map[key] = n;
        n++;

        return true;
    }

    template<typename K, typename R>
    bool TableSoA<K, R>::remove(const key_t &key) {
        // Check the key is present
        try {
            // Get the index to delete and of last item
            unsigned int index = map.at(key);
            unsigned int last = n - 1;

            // Only copy over when not last
            if (index != last) {
                // Move last into deleted
                util::invoke_n<record_t::count, RemoveHelper>(arrays, index, last);

                // Update key-index map
                for (auto i = map.begin(); i != map.end(); i++) {
                    if (i->second == last) {
                        i->second = index;
                        break;
                    }
                }
            }

            // Decrement size and erase the removed key
            n--;
            map.erase(key);

            return true;
        } catch (std::out_of_range &e) {
            // Not present -> nothing to remove
            return false;
        }
    }

    template<typename K, typename R>
    typename TableSoA<K, R>::record_t TableSoA<K, R>::get_copy(const key_t &key) {
        // Check the key is present
        try {
            // Get the index and prepare result
            unsigned int index = map.at(key);
            record_t result;

            // Move last into deleted
            util::invoke_n<record_t::count, GetCopyHelper>(arrays, index, result);

            return result;
        } catch (std::out_of_range &e) {
            // Not present -> error
            throw std::out_of_range("No record found for the provided key.");
        }
    }

    template<typename K, typename R>
    typename TableSoA<K, R>::record_ptr_t TableSoA<K, R>::get_reference(const key_t &key) {
        // Check the key is present
        try {
            // Get the index and prepare result
            unsigned int index = map.at(key);
            record_ptr_t result;

            // Set result to point to the correct entries
            util::invoke_n<record_t::count, GetRefHelper>(arrays, index, result);

            return result;
        } catch (std::out_of_range &e) {
            // Not present -> error
            throw std::out_of_range("No record found for the provided key.");
        }
    }

    template<typename K, typename R>
    typename TableSoA<K, R>::record_ptr_t TableSoA<K, R>::get_reference() {
        // Prepare result
        record_ptr_t result;

        // Set result to point to array starts
        util::invoke_n<record_t::count, GetRefHelper>(arrays, 0u, result);

        return result;
    }

    /**
     * @}
     */
}

// Generate member type struct and member pointer get function for member N of type T and identifier I of struct S,
//  as well as the pointer variant for sub-struct S:Ptr
//TODO use sub-struct S::AoS for base data? to keep data at the same level
#define SOA_MEMBER(S, N, T, I) \
template<>  \
struct open_sea::util::GetMemberType<S, N> { typedef T type; }; \
\
template<>  \
typename open_sea::util::GetMemberPointerType<S, N>::type open_sea::util::get_pointer_to_member<S, N>() {   \
    return &S::I;   \
}   \
template<>  \
struct open_sea::util::GetMemberType<S::Ptr, N> { typedef T* type; }; \
\
template<>  \
typename open_sea::util::GetMemberPointerType<S::Ptr, N>::type open_sea::util::get_pointer_to_member<S::Ptr, N>() {   \
    return &S::Ptr::I;   \
}

/* Example usage:
 *
 * struct Data {
 *     static constexpr unsigned int count = 2;
 *     struct Ptr {
 *         int *a;
 *         float *b;
 *     };
 *
 *     int a;
 *     float b;
 * };
 * SOA_MEMBER(Data, 0, int, a)
 * SOA_MEMBER(Data, 1, float, b)
 */

#endif //OPEN_SEA_TABLE_H
