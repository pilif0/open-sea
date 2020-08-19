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
#include <unistd.h>
#include <stdlib.h>

//TODO if performance degrades due to removing, can add inverse key-index map to speed up last's key lookup

// Note: These currently don't support non-trivial data (e.g. shared_ptr) in records. This is because the space is
//  uninitialised after allocation and modifications aren't too careful about objects being moved around. For example,
//  with shared pointers this causes a segfault as the new pointer tries to interact with the uninitialised data in its
//  destination.
//  Placement new or allocator::construct might be a help here.
//TODO fix this
namespace open_sea::data {
    /**
     * \addtogroup Data
     *
     * @{
     */

    /** \struct opt_index
     * \brief Optional index
     *
     * Represents an optional index, which is either unset or set to a `size_t` value.
     * All but the maximum value of `size_t` are safe.
     * The maximum value of `size_t` is reserved to represent the unset state.
     */
    // Note: could also use 0 to represent unset but then every evaluation would need an extra operation to subtract 1
    struct opt_index {
        private:
            //! Value used to represent unset state
            static constexpr size_t none = std::numeric_limits<size_t>::max();
            //! Index value, or `none`
            size_t value = none;
        public:
            //! Construct unset optional index
            opt_index() = default;
            //! Construct set optional index
            explicit opt_index(size_t value) : value(value) {}

            //! `true` iff the index is set
            [[nodiscard]] bool is_set() const { return value != none; }
            //! Get index value (only safe when `is_set` is `true`
            [[nodiscard]] size_t get() const { return value; }
            //! Set the index value
            void set(size_t v) { value = v; }
            //! Set the index value to another optional index's value
            void set(opt_index v) { value = v.value; }
            //! Unset the index
            void unset() { value = none; }

            //! Optional indices are equal iff their values are
            friend bool operator==(const opt_index &lhs, const opt_index &rhs) { return lhs.value == rhs.value; }
            friend bool operator!=(const opt_index &lhs, const opt_index &rhs) { return !(rhs == lhs); }
    };

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
             * \return index of the record iff the structure was modified (i.e. the record was added), unset otherwise
             */
            virtual opt_index add(const key_t &key, const record_t &record) = 0;

            /**
             * \brief Add records under the provided keys
             *
             * Copies the values of the provided records and associates them with the keys.
             * Does nothing (at all) when any of the keys already has a record associated with it.
             * The records have to be provided as an array of record instances (AoS layout).
             *
             * \param keys Keys to associate with the records
             * \param records Records to add
             * \param count Number of records to add
             * \return `true` iff the structure was modified (i.e. the records were added)
             */
            virtual bool add(const key_t *keys, const record_t *records, size_t count) = 0;

            /**
             * \brief Add records under the provided keys
             *
             * Copies the values of the provided records and associates them with the keys.
             * Does nothing (at all) when any of the keys already has a record associated with it.
             * The records have to be provided as a record pointer type (SoA layout).
             *
             * \param keys Keys to associate with the records
             * \param records Records to add
             * \param count Number of records to add
             * \return `true` iff the structure was modified (i.e. the records were added)
             */
            virtual bool add(const key_t *keys, const record_ptr_t &records, size_t count) = 0;

            /**
             * \brief Remove record at the provided index
             *
             * Does nothing when no record is at the index.
             * Done by copying the last record over the one to be removed and decrementing size.
             * Potentially reshuffles the data, and therefore invalidates any references to it.
             *
             * \param idx Index of record to remove
             * \return index of the removed element (and where last element was moved) iff the structure was modified
             *          (i.e. the key was present and associated record removed)
             */
            virtual opt_index remove(opt_index idx) = 0;

            /**
             * \brief Remove record associated with the provided key
             *
             * Does nothing when no record is associated with the key.
             * Done by copying the last record over the one to be removed and decrementing size.
             * Potentially reshuffles the data, and therefore invalidates any references to it.
             *
             * \param key Key to remove
             * \return index of the removed element (and where last element was moved) iff the structure was modified
             *          (i.e. the key was present and associated record removed)
             */
            virtual opt_index remove(const key_t &key) = 0;

            /**
             * Get index of record under the provided key
             *
             * \param key Key to look up
             * \return Instance of `opt_index` with the index, or unset if no record associated with the provided key
             */
            virtual opt_index lookup(const key_t &key) = 0;

            /**
             * Get key of record at the provided index
             *
             * \param idx Index to look up
             * \return Instance of `key_t` with which the index is associated
             *
             * \throws std::out_of_range When no record is at the provided index
             */
            virtual key_t lookup(const opt_index &idx) = 0;

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
             * Get copy of record at the provided index
             *
             * \param i Index
             * \return Instance of R whose members are copies of values at the provided index
             *
             * \throws std::out_of_range When no record is at the provided index
             */
            virtual record_t get_copy(const opt_index &i) = 0;

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
             * Get read-write reference to the record at the provided index
             *
             * \param i Index
             * \return Instance of R::Ptr (struct of pointers to members of R) whose members point to the values
             *  at the provided index
             *
             * \throws std::out_of_range When no record is at the provided index
             */
            virtual record_ptr_t get_reference(const opt_index &i) = 0;

            /**
             * Get read-write references to the records under the provided keys
             * References are set to `nullptr` when no record is associated with that key
             *
             * \param keys Keys to look up
             * \param dest Array of size `count` with R:Ptr instances to fill with the references
             * \param count Number of records to look up
             */
            virtual void get_reference(const key_t *keys, record_ptr_t *dest, size_t count) = 0;

            /**
             * Get read-write reference to the first record
             * Undefined when empty
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
            virtual size_t size() = 0;

            //! Get key set
            virtual std::vector<key_t> keys() = 0;

            //! Get number of allocated records
            virtual size_t allocated() = 0;

            //! Get number of allocated pages
            virtual size_t pages() = 0;

            //! Get name of the table type (ie the storage scheme)
            virtual const char* type_name() = 0;

            virtual ~Table() = default;
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
            std::unordered_map<K, size_t> map{};
            //! Start of the data
            record_t *data = nullptr;
            //! Number of records stored
            size_t n = 0;
            //! Allocated space in terms of number of records that fit in it
            size_t capacity = 0;
            //! Number of pages allocated
            size_t pages_alloc = 0;

        public:
            //! Construct the table and defer allocation to first insertion
            TableAoS() = default;

            /**
             * Construct the table and allocate space for `count` records
             *
             * \param count Number of records to allocate space for
             */
            explicit TableAoS(size_t count) { allocate(count); }

            opt_index add(const key_t &key, const record_t &record) override;
            bool add(const key_t *keys, const record_t *records, size_t count) override;
            bool add(const key_t *keys, const record_ptr_t &records, size_t count) override;
            opt_index remove(const key_t &key) override;
            opt_index remove(opt_index idx) override;
            opt_index lookup(const key_t &key) override;
            key_t lookup(const opt_index &idx) override;
            record_t get_copy(const key_t &key) override;
            record_t get_copy(const opt_index &i) override;
            record_ptr_t get_reference(const key_t &key) override;
            record_ptr_t get_reference(const opt_index &i) override;
            record_ptr_t get_reference() override;
            void get_reference(const key_t *keys, record_ptr_t *dest, size_t count) override;
            void increment_reference(record_ptr_t &ref) override { util::invoke_n<record_t::count, IncrementHelper>(ref); }
            size_t size() override { return n; }
            std::vector<key_t> keys() override;
            size_t allocated() override { return capacity; }
            size_t pages() override { return pages_alloc; }
            const char* type_name() override { return "AoS"; }

            virtual ~TableAoS();

        private:
            //! Helper functor to fill the Nth member of the ith record with data being pointed to by the Nth member of
            //! the record pointer, incrementing the record pointer member.
            template <size_t N>
            struct AddsHelper {
                void operator()(record_t *arr, const size_t i, record_ptr_t &records) {
                    // From Nth member of records,
                    auto src = std::invoke(util::get_pointer_to_member<record_ptr_t, N>(), records);
                    // to Nth member of ith record, copy
                    std::invoke(util::get_pointer_to_member<record_t, N>(), arr[i]) = *src;
                    // and increment the pointer in records
                    std::invoke(util::get_pointer_to_member<record_ptr_t, N>(), records)++;
                }
            };

            //! Helper functor to fill Nth member of R::Ptr with a pointer to the appropriate value of record i
            template <size_t N>
            struct GetRefHelper {
                void operator()(record_t *arr, const size_t i, record_ptr_t &result) {
                    // Get the appropriate value's address
                    auto ptr = &(std::invoke(util::get_pointer_to_member<record_t, N>(), arr[i]));

                    // Set result's member to the address
                    std::invoke(util::get_pointer_to_member<record_ptr_t, N>(), result) = ptr;
                }
            };

            //! Helper functor to increment Nth member of R::Ptr
            template <size_t N>
            struct IncrementHelper {
                void operator()(record_ptr_t &ref) {
                    // Increment member by sizeof(R) bytes (next element, same offset)
                    // Note: cast to byte ptr -> increment by size of R -> cast back to member type ptr
                    typedef typename util::GetMemberType<record_ptr_t, N>::type member_type;
                    auto member_p = util::get_pointer_to_member<record_ptr_t, N>();
                    std::invoke(member_p, ref) = (member_type) (((unsigned char *) std::invoke(member_p, ref)) + sizeof(record_t));
                }
            };

            void allocate(size_t size);
    };

    /**
     * Allocate page-aligned space that fits at least the given amount of records.
     * Size of the allocated space is rounded up to the nearest 2^N pages.
     * If the table wasn't empty, the old data is copied over to the new space.
     *
     * \tparam K Key type
     * \tparam R Record type
     * \param size Number of records to allocate space for
     */
    // Note: rounding up to 2^N instead of just nearest page to ensure geometric progression and amortised performance
    template<typename K, typename R>
    void TableAoS<K, R>::allocate(size_t size) {
        // Make sure old data will fit into new space
        assert(size >= n);

        // Skip if already big enough or requested zero size (which any space fits)
        if (capacity >= size || size == 0) {
            return;
        }

        // Calculate target space size
        // The nearest greater power of 2 has 1 in the position of the last leading 0 and 0s everywhere else
        // For the rounded-up division see https://stackoverflow.com/q/2745074
        long pagesize = sysconf(_SC_PAGESIZE);
        unsigned long space_needed = size * sizeof(record_t);
        unsigned long pages_needed = space_needed / pagesize + (space_needed % pagesize != 0);
        size_t pages_target;
        if ((pages_needed & (pages_needed - 1)) == 0) {
            // Pages needed is a power of two -> use that
            // See http://www.graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2 for power of 2 test
            pages_target = pages_needed;
        } else {
            // Not a power of two -> find the nearest greater one
            // The nearest greater power of 2 has 1 in the position of the last leading 0 and 0s everywhere else
            int shift_by = ((sizeof(unsigned long) * 8) - __builtin_clzl(pages_needed));
            pages_target = 1 << shift_by;
        }
        size_t space_target = pages_target * pagesize;
        assert(space_target >= space_needed);

        // Allocate that much page-aligned space
        record_t *target = static_cast<record_t *>(aligned_alloc(pagesize, space_target));
        assert(target != nullptr);

        // Copy data into new space
        if (n > 0) {
            std::copy_n(data, n, target);
        }

        // Deallocate old data
        if (data && capacity > 0) {
            free(data);
        }

        // Update state
        data = target;
        capacity = space_target / sizeof(record_t);
        pages_alloc = pages_target;
    }

    template<typename K, typename R>
    opt_index TableAoS<K, R>::add(const key_t &key, const record_t &record) {
        // Check the key is not present yet
        try {
            map.at(key);
            return opt_index();
        } catch (std::out_of_range &e) {}

        // Check there is enough space
        if (capacity <= n) {
            // At capacity -> allocate enough space to add a record
            allocate(n + 1);
        }

        // Set row to the record
        data[n] = record;

        // Update map
        map[key] = n;

        // Return inserted index and increment size
        return opt_index(n++);
    }

    template<typename K, typename R>
    bool TableAoS<K, R>::add(const key_t *keys, const record_t *records, size_t count) {
        // Skip if count is zero
        if (count == 0) {
            return false;
        }

        // Check no key is present yet
        for (size_t i = 0; i < count; i++) {
            try {
                map.at(keys[i]);
                return false;
            } catch (std::out_of_range&) {}
        }

        // Check there is enough space
        if (capacity < (n + count)) {
            // Wouldn't fit -> allocate enough space to add all the records
            allocate(n + count);
        }

        // Copy all records to the end of the data
        std::copy_n(records, count, data+n);

        // Update key map
        for (size_t i = 0; i < count; i++) {
            map[keys[i]] = n;
            n++;
        }

        return true;
    }

    template<typename K, typename R>
    bool TableAoS<K, R>::add(const key_t *keys, const record_ptr_t &records, size_t count) {
        // Skip if count is zero
        if (count == 0) {
            return false;
        }

        // Check no key is present yet
        for (size_t i = 0; i < count; i++) {
            try {
                map.at(keys[i]);
                return false;
            } catch (std::out_of_range&) {}
        }

        // Check there is enough space
        if (capacity < (n + count)) {
            // Wouldn't fit -> allocate enough space to add all the records
            allocate(n + count);
        }

        // Process each record (copying records to modify it)
        record_ptr_t rs_copy = records;
        for (size_t i = 0; i < count; i++) {
            // Set row to the record
            util::invoke_n<record_t::count, AddsHelper>(data, n, rs_copy);

            // Update state
            map[keys[i]] = n;
            n++;
        }

        return true;
    }

    template<typename K, typename R>
    opt_index TableAoS<K, R>::remove(opt_index idx) {
        // Find the index's key in the map and remove using the key overload
        if (idx.is_set() && idx.get() < n) {
            for (auto i = map.begin(); i != map.end(); i++) {
                if (i->second == idx.get()) {
                    return remove(i->first);
                }
            }

            // We got here -> none found
            return opt_index();
        } else {
            // Unset or out of range
            return opt_index();
        }
    }

    template<typename K, typename R>
    opt_index TableAoS<K, R>::remove(const key_t &key) {
        // Check the key is present
        try {
            // Get the index to delete and of last item
            size_t index = map.at(key);
            size_t last = n - 1;

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

            return opt_index(index);
        } catch (std::out_of_range &e) {
            // Not present -> nothing to remove
            return opt_index();
        }
    }

    template<typename K, typename R>
    opt_index TableAoS<K, R>::lookup(const key_t &key) {
        // Find the key
        try {
            // Return index when found
            return opt_index(map.at(key));
        } catch (std::out_of_range &e) {
            // Not present -> unset
            return opt_index();
        }
    }

    template<typename K, typename R>
    typename TableAoS<K, R>::key_t TableAoS<K, R>::lookup(const opt_index &idx) {
        // Check the index is valid
        if (!idx.is_set()) {
            // Unset -> error
            throw std::out_of_range("Index is unset.");
        }
        if (idx.get() > n) {
            // Outside -> error
            throw std::out_of_range("Index is outside the table.");
        }

        // Find the key
        for (auto i = map.begin(); i != map.end(); i++) {
            if (i->second == idx.get()) {
                return i->first;
            }
        }

        // Not found -> error
        throw std::out_of_range("No record found for the provided key.");
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
    typename TableAoS<K, R>::record_t TableAoS<K, R>::get_copy(const opt_index &i) {
        // Check the index is set and within range
        if (!i.is_set()) {
            // Not set -> error
            throw std::invalid_argument("Index is not set.");
        } else if(i.get() < this->n) {
            // Return copy of the record at the index
            return data[i.get()];
        } else {
            // Not present -> error
            throw std::out_of_range("No record found for the provided index.");
        }
    }

    template<typename K, typename R>
    typename TableAoS<K, R>::record_ptr_t TableAoS<K, R>::get_reference(const key_t &key) {
        // Check the key is present
        try {
            // Get the index and prepare result
            size_t index = map.at(key);
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
    typename TableAoS<K, R>::record_ptr_t TableAoS<K, R>::get_reference(const opt_index &i) {
        // Check if the index is set and within range
        if (!i.is_set()) {
            // Not set -> error
            throw std::invalid_argument("Index is not set.");
        } else if (i.get() < this->n) {
            // Get the index and prepare result
            size_t index = i.get();
            record_ptr_t result;

            // Set result to point to the correct entries
            util::invoke_n<record_t::count, GetRefHelper>(data, index, result);

            return result;
        } else {
            // Not present -> error
            throw std::out_of_range("No record found for the provided index.");
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

    template<typename K, typename R>
    void TableAoS<K, R>::get_reference(const key_t *keys, record_ptr_t *dest, size_t count) {
        // Go through each key
        const key_t *k = keys;
        record_ptr_t *d = dest;
        for (size_t i = 0; i < count; i++, k++, d++) {
            // Set destination to the reference
            try {
                *d = get_reference(*k);
            } catch (std::out_of_range &e) {
                // Not present -> fill with nullptrs
                *d = {nullptr};
            }
        }
    }

    template<typename K, typename R>
    std::vector<typename TableAoS<K, R>::key_t> TableAoS<K, R>::keys() {
        std::vector<key_t> result;
        for (auto kv : map) {
            result.push_back(kv.first);
        }
        return result;
    }

    template<typename K, typename R>
    TableAoS<K, R>::~TableAoS() {
        // Deallocate data
        if (data && capacity > 0) {
            free(data);
        }
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
            std::unordered_map<key_t, size_t> map{};
            //! Start of the data
            void *data = nullptr;
            //! Starts of data arrays
            void *arrays[record_t::count] {nullptr};
            //! Number of records stored
            size_t n = 0;
            //! Allocated space in terms of number of records that fit in it
            size_t capacity = 0;
            //! Number of pages allocated
            size_t pages_alloc = 0;

        public:
            //! Construct the table and defer allocation to first insertion
            TableSoA() = default;

            /**
             * Construct the table and allocate space for `count` records
             *
             * \param count Number of records to allocate space for
             */
            explicit TableSoA(size_t count) { allocate(count); }

            opt_index add(const key_t &key, const record_t &record) override;
            bool add(const key_t *keys, const record_t *records, size_t count) override;
            bool add(const key_t *keys, const record_ptr_t &records, size_t count) override;
            opt_index remove(const key_t &key) override;
            opt_index remove(opt_index idx) override;
            opt_index lookup(const key_t &key) override;
            key_t lookup(const opt_index &idx) override;
            record_t get_copy(const key_t &key) override;
            record_t get_copy(const opt_index &i) override;
            record_ptr_t get_reference(const key_t &key) override;
            record_ptr_t get_reference(const opt_index &i) override;
            record_ptr_t get_reference() override;
            void get_reference(const key_t *keys, record_ptr_t *dest, size_t count) override;
            void increment_reference(record_ptr_t &ref) override { util::invoke_n<record_t::count, IncrementHelper>(ref); }
            size_t size() override { return n; }
            std::vector<key_t> keys() override;
            size_t allocated() override { return capacity; }
            size_t pages() override { return pages_alloc; }
            const char* type_name() override { return "SoA"; }

            virtual ~TableSoA();

        private:
            //! Helper functor to compute start of Nth data array based on the previous one (N != 0) and the capacity
            template <size_t N>
            struct AllocatePtrHelper {
                void operator()(void **target_arrays, const size_t size) {
                    auto previous = (typename util::GetMemberType<record_t, N - 1>::type *) target_arrays[N - 1];
                    previous += size;
                    target_arrays[N] = (void*) previous;
                }
            };

            //! Specialization of the \ref AllocatePtrHelper functor to do nothing when N is 0 (handled separately)
            template <>
            struct AllocatePtrHelper<0> { void operator()(void */*target_arrays*/[], const size_t /*size*/) {} };

            //! Helper functor to copy Nth data array after reallocation
            template <size_t N>
            struct AllocateCopyHelper {
                void operator()(void **src_arrays, void **dest_arrays, const size_t count) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto src = static_cast<member_type *>(src_arrays[N]);
                    auto dest = static_cast<member_type *>(dest_arrays[N]);
                    std::copy_n(src, count, dest);
                }
            };

            //! Helper functor to append the Nth member of the provided record to the appropriate data array
            template <size_t N>
            struct AddHelper {
                void operator()(void **arr, const size_t count, const record_t &record) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto start = static_cast<member_type *>(arr[N]);
                    start[count] = std::invoke(util::get_pointer_to_member<record_t, N>(), record);
                }
            };

            //! Helper functor to append the Nth members of the provided records to the appropriate data array
            template <size_t N>
            struct AddsPtrHelper {
                void operator()(void **arr, const size_t offset, const record_ptr_t &records, size_t count) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    // From the relevant member of records,
                    auto src = std::invoke(util::get_pointer_to_member<record_ptr_t, N>(), records);
                    // to just after the last entry in the relevant array,
                    auto dest = static_cast<member_type *>(arr[N]) + offset;
                    // copy count elements
                    std::copy_n(src, count, dest);
                }
            };

            //! Helper functor to copy Nth member of the last record to the provided index
            template <size_t N>
            struct RemoveHelper {
                void operator()(void **arr, const size_t index, const size_t last) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto start = static_cast<member_type *>(arr[N]);
                    start[index] = start[last];
                }
            };

            //! Helper functor to fill Nth member of R with a copy of the appropriate value of record i
            template <size_t N>
            struct GetCopyHelper {
                void operator()(void **arr, const size_t i, record_t &result) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto start = static_cast<member_type *>(arr[N]);
                    std::invoke(util::get_pointer_to_member<record_t, N>(), result) = start[i];
                }
            };

            //! Helper functor to fill Nth member of R::Ptr with a pointer to the appropriate value of record i
            template <size_t N>
            struct GetRefHelper {
                void operator()(void **arr, const size_t i, record_ptr_t &result) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto start = static_cast<member_type *>(arr[N]);
                    std::invoke(util::get_pointer_to_member<record_ptr_t, N>(), result) = start + i;
                }
            };

            //! Helper functor to fill Nth members of R::Ptr array elements with pointers to the appropriate values of
            //!     records under keys
            template <size_t N>
            struct GetRefsHelper {
                void operator()(void **arr, std::unordered_map<key_t, size_t> &m, const key_t *keys, record_ptr_t *dest, size_t count) {
                    typedef typename util::GetMemberType<record_t, N>::type member_type;
                    auto start = static_cast<member_type *>(arr[N]);
                    const key_t *k = keys;
                    record_ptr_t *d = dest;
                    for (size_t i = 0; i < count; i++, k++, d++) {
                        try {
                            std::invoke(util::get_pointer_to_member<record_ptr_t, N>(), *d) = start + m.at(*k);
                        } catch (std::out_of_range &e) {
                            std::invoke(util::get_pointer_to_member<record_ptr_t, N>(), *d) = nullptr;
                        }
                    }
                }
            };

            //! Helper functor to increment Nth member of R::Ptr
            template <size_t N>
            struct IncrementHelper {
                void operator()(record_ptr_t &ref) {
                    // Increment member by 1 (next element of its array)
                    auto member_p = util::get_pointer_to_member<record_ptr_t, N>();
                    std::invoke(member_p, ref)++;
                }
            };

            void allocate(size_t size);
    };

    /**
     * Allocate page-aligned space that fits at least the given amount of records.
     * Size of the allocated space is rounded up to the nearest 2^N pages.
     * If the table wasn't empty, the old data is copied over to the new space.
     *
     * \tparam K Key type
     * \tparam R Record type
     * \param size Number of records to allocate space for
     */
    template<typename K, typename R>
    void TableSoA<K, R>::allocate(size_t size) {
        // Make sure data will fit into new space
        assert(size > n);

        // Skip if already big enough or requested zero size (which any space fits)
        if (capacity >= size || size == 0) {
            return;
        }

        // Calculate target space size
        // For the rounded-up division see https://stackoverflow.com/q/2745074
        long pagesize = sysconf(_SC_PAGESIZE);
        unsigned long space_needed = size * sizeof(record_t);
        unsigned long pages_needed = space_needed / pagesize + (space_needed % pagesize != 0);
        size_t pages_target;
        if ((pages_needed & (pages_needed - 1)) == 0) {
            // Pages needed is a power of two -> use that
            // See http://www.graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2 for power of 2 test
            pages_target = pages_needed;
        } else {
            // Not a power of two -> find the nearest greater one
            // The nearest greater power of 2 has 1 in the position of the last leading 0 and 0s everywhere else
            int shift_by = ((sizeof(unsigned long) * 8) - __builtin_clzl(pages_needed));
            pages_target = 1 << shift_by;
        }
        size_t space_target = pages_target * pagesize;
        assert(space_target >= space_needed);

        // Allocate that much page-aligned space
        void *target = aligned_alloc(pagesize, space_target);
        assert(target != nullptr);

        // Compute array start pointers
        void *target_arrays[record_t::count];
        target_arrays[0] = target;
        util::invoke_n<record_t::count, AllocatePtrHelper>(target_arrays, space_target / sizeof(record_t));

        // Copy data into new space
        if (n > 0) {
            util::invoke_n<record_t::count, AllocateCopyHelper>(arrays, target_arrays, n);
        }

        // Deallocate old data
        if (data && capacity > 0) {
            free(data);
        }

        // Update state
        data = target;
        std::copy(std::begin(target_arrays), std::end(target_arrays), std::begin(arrays));
        capacity = space_target / sizeof(record_t);
        pages_alloc = pages_target;
    }

    template<typename K, typename R>
    opt_index TableSoA<K, R>::add(const key_t &key, const R &record) {
        // Check the key is not present yet
        try {
            map.at(key);
            return opt_index();
        } catch (std::out_of_range &e) {}

        // Check there is enough space
        if (capacity <= n) {
            // At capacity -> allocate enough space to add a record
            allocate(n + 1);
        }

        // Set row to the record
        util::invoke_n<record_t::count, AddHelper>(arrays, n, record);

        // Update map
        map[key] = n;

        // Return inserted index and increment size
        return opt_index(n++);
    }

    template<typename K, typename R>
    bool TableSoA<K, R>::add(const key_t *keys, const record_t *records, size_t count) {
        // Skip if count is zero
        if (count == 0) {
            return false;
        }

        // Check no key is present yet
        for (size_t i = 0; i < count; i++) {
            try {
                map.at(keys[i]);
                return false;
            } catch (std::out_of_range&) {}
        }

        // Check there is enough space
        if (capacity < (n + count)) {
            // Wouldn't fit -> allocate enough space to add all the records
            allocate(n + count);
        }

        // Append each record
        for (size_t i = 0; i < count; i++) {
            // Set row to the record
            util::invoke_n<record_t::count, AddHelper>(arrays, n, records[i]);

            // Update map
            map[keys[i]] = n++;
        }

        return true;
    }

    template<typename K, typename R>
    bool TableSoA<K, R>::add(const key_t *keys, const record_ptr_t &records, size_t count) {
        // Skip if count is zero
        if (count == 0) {
            return false;
        }

        // Check no key is present yet
        for (size_t i = 0; i < count; i++) {
            try {
                map.at(keys[i]);
                return false;
            } catch (std::out_of_range&) {}
        }

        // Check there is enough space
        if (capacity < (n + count)) {
            // Wouldn't fit -> allocate enough space to add all the records
            allocate(n + count);
        }

        // Copy records data to relevant arrays
        util::invoke_n<record_t::count, AddsPtrHelper>(arrays, n, records, count);

        // Update key map
        for (size_t i = 0; i < count; i++) {
            map[keys[i]] = n;
            n++;
        }

        return true;
    }

    template<typename K, typename R>
    opt_index TableSoA<K, R>::remove(opt_index idx) {
        // Find the index's key in the map and remove using the key overload
        if (idx.is_set() && idx.get() < n) {
            for (auto i = map.begin(); i != map.end(); i++) {
                if (i->second == idx.get()) {
                    return remove(i->first);
                }
            }

            // We got here -> none found
            return opt_index();
        } else {
            // Unset or out of range
            return opt_index();
        }
    }

    template<typename K, typename R>
    opt_index TableSoA<K, R>::remove(const key_t &key) {
        // Check the key is present
        try {
            // Get the index to delete and of last item
            size_t index = map.at(key);
            size_t last = n - 1;

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

            return opt_index(index);
        } catch (std::out_of_range &e) {
            // Not present -> nothing to remove
            return opt_index();
        }
    }

    template<typename K, typename R>
    opt_index TableSoA<K, R>::lookup(const key_t &key) {
        // Find the key
        try {
            // Return index when found
            return opt_index(map.at(key));
        } catch (std::out_of_range &e) {
            // Not present -> unset
            return opt_index();
        }
    }

    template<typename K, typename R>
    typename TableSoA<K, R>::key_t TableSoA<K, R>::lookup(const opt_index &idx) {
        // Check the index is valid
        if (!idx.is_set()) {
            // Unset -> error
            throw std::out_of_range("Index is unset.");
        }
        if (idx.get() > n) {
            // Outside -> error
            throw std::out_of_range("Index is outside the table.");
        }

        // Find the key
        for (auto i = map.begin(); i != map.end(); i++) {
            if (i->second == idx.get()) {
                return i->first;
            }
        }

        // Not found -> error
        throw std::out_of_range("No record found for the provided key.");
    }

    template<typename K, typename R>
    typename TableSoA<K, R>::record_t TableSoA<K, R>::get_copy(const key_t &key) {
        // Check the key is present
        try {
            // Get the index and prepare result
            size_t index = map.at(key);
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
    typename TableSoA<K, R>::record_t TableSoA<K, R>::get_copy(const opt_index &i) {
        // Check the index is set and within range
        if (!i.is_set()) {
            // Not set -> error
            throw std::invalid_argument("Index is not set.");
        } else if(i.get() < this->n) {
            // Get the index and prepare result
            size_t index = i.get();
            record_t result;

            // Move last into deleted
            util::invoke_n<record_t::count, GetCopyHelper>(arrays, index, result);

            return result;
        } else {
            // Not present -> error
            throw std::out_of_range("No record found for the provided index.");
        }
    }

    template<typename K, typename R>
    typename TableSoA<K, R>::record_ptr_t TableSoA<K, R>::get_reference(const key_t &key) {
        // Check the key is present
        try {
            // Get the index and prepare result
            size_t index = map.at(key);
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
    typename TableSoA<K, R>::record_ptr_t TableSoA<K, R>::get_reference(const opt_index &i) {
        // Check if the index is set and within range
        if (!i.is_set()) {
            // Not set -> error
            throw std::invalid_argument("Index is not set.");
        } else if (i.get() < this->n) {
            // Get the index and prepare result
            size_t index = i.get();
            record_ptr_t result;

            // Set result to point to the correct entries
            util::invoke_n<record_t::count, GetRefHelper>(arrays, index, result);

            return result;
        } else {
            // Not present -> error
            throw std::out_of_range("No record found for the provided index.");
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

    template<typename K, typename R>
    void TableSoA<K, R>::get_reference(const key_t *keys, record_ptr_t *dest, size_t count) {
        // Go through keys for each member and set references
        util::invoke_n<record_t::count, GetRefsHelper>(arrays, map, keys, dest, count);
    }

    template<typename K, typename R>
    std::vector<typename TableSoA<K, R>::key_t> TableSoA<K, R>::keys() {
        std::vector<key_t> result;
        for (auto kv : map) {
            result.push_back(kv.first);
        }
        return result;
    }

    template<typename K, typename R>
    TableSoA<K, R>::~TableSoA() {
        // Deallocate data
        if (data && capacity > 0) {
            free(data);
        }
    }

    /**
     * @}
     */
}

// Generate member type struct and member pointer get function for member N of type T and identifier I of struct S,
//  as well as the pointer variant for sub-struct S:Ptr
#define SOA_MEMBER(S, N, T, I) \
template<>  \
struct open_sea::util::GetMemberType<S, N> { typedef T type; }; \
\
template<>  \
inline typename open_sea::util::GetMemberPointerType<S, N>::type open_sea::util::get_pointer_to_member<S, N>() {   \
    return &S::I;   \
}   \
template<>  \
struct open_sea::util::GetMemberType<S::Ptr, N> { typedef T* type; }; \
\
template<>  \
inline typename open_sea::util::GetMemberPointerType<S::Ptr, N>::type open_sea::util::get_pointer_to_member<S::Ptr, N>() {   \
    return &S::Ptr::I;   \
}

/* Example usage:
 *
 * struct Data {
 *     static constexpr size_t count = 2;
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
