#include <functional>
#include <vector>
#include <exception>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <cassert>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
protected:
    struct Data {
        std::pair<const KeyType, ValueType> *data = nullptr;
        size_t hash_key = 0;
        size_t psl = 0;
        bool free = true;

        Data() = default;
        Data(const KeyType& key, const ValueType& value, size_t hash_key_, size_t psl_ = 0, bool free_ = true) : hash_key(hash_key_), psl(psl_), free(free_) {
            data = new std::pair<const KeyType, ValueType>(key, value);
        }
        Data(const Data& other) : hash_key(other.hash_key), psl(other.psl), free(other.free) {
            delete data;
            data = new std::pair<const KeyType, ValueType>(other.data);
        }
        Data(Data&& other) {
            std::swap(data, other.data);
            std::swap(hash_key, other.hash_key);
            std::swap(psl, other.psl);
            std::swap(free, other.free);
        }

        ~Data() {
            delete data;
        }

        Data& operator=(const Data& other) {
            hash_key = other.hash_key;
            psl = other.psl; 
            free = other.free;

            if (other.data) {
                auto new_data = new std::pair<const KeyType, ValueType>(*other.data);
                delete data;
                data = new_data;
            } else {
                delete data;
                data = nullptr;
            }

            return *this;
        }
    };
    static constexpr double LOAD_FACTOR = 0.8;
    static constexpr size_t INITIAL_CAPACITY = 12;
    Data* data_ = nullptr;
    size_t capacity_ = 0;
    size_t size_ = 0;
    Hash hash_ = Hash();

    size_t find_(const KeyType& key) const {
        size_t hash_key = hash_(key);
        size_t curr_pos = hash_key % capacity_;
        while (!data_[curr_pos].free && (hash_key != data_[curr_pos].hash_key || !(key == data_[curr_pos].data->first))) {
            ++curr_pos;
            if (curr_pos == capacity_) {
                curr_pos = 0;
            }
        }
        return curr_pos;
    }

    void increase_capacity() {
        Data* new_data = new Data[2 * capacity_ + 1];
        std::swap(new_data, data_);
        size_t old_capacity = capacity_;
        capacity_ = 2 * capacity_ + 1;
        size_ = 0;
        for (size_t i = 0; i < old_capacity; ++i) {
            if (!new_data[i].free) {
                insert(*new_data[i].data);
            }
        }
        delete[] new_data;
    }

public:
    class iterator {
        friend HashMap;
    protected:
        size_t index_;
        Data* data_;
        size_t capacity_;
        iterator(size_t ind, Data* data, size_t cap) : index_(ind), data_(data), capacity_(cap) {}

    public:
        iterator() : index_(0), data_(nullptr), capacity_(0) {}
        iterator(const iterator& it) : index_(it.index_), data_(it.data_), capacity_(it.capacity_) {}

        iterator& operator++() {
            ++index_;
            while (index_ < capacity_ && data_[index_].free) {
                ++index_;
            }
            return *this;
        }

        iterator operator++(int) {
            iterator prev = *this;
            ++(*this);
            return prev;
        }

        bool operator==(const iterator& other) const {
            return index_ == other.index_;
        }

        bool operator!=(const iterator& other) const {
            return index_ != other.index_;
        }

        std::pair<const KeyType, ValueType>& operator*() {
            return *data_[index_].data;
        }

        std::pair<const KeyType, ValueType>* operator->() {
            return data_[index_].data;
        }
    };

    class const_iterator {
        friend HashMap;
    protected:
        size_t index_;
        const Data* data_;
        size_t capacity_;
        const_iterator(size_t ind, Data* data, size_t cap) : index_(ind), data_(data), capacity_(cap) {}

    public:
        const_iterator() : index_(0), data_(nullptr), capacity_(0) {}
        const_iterator(const iterator& it) : index_(it.index_), data_(it.data_), capacity_(it.capacity_) {}
        const_iterator(const const_iterator& it) : index_(it.index_), data_(it.data_), capacity_(it.capacity_) {}

        const_iterator& operator++() {
            ++index_;
            while (index_ < capacity_ && data_[index_].free) {
                ++index_;
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator prev = *this;
            ++(*this);
            return prev;
        }

        bool operator==(const const_iterator& other) const {
            return index_ == other.index_;
        }

        bool operator!=(const const_iterator& other) const {
            return index_ != other.index_;
        }

        const std::pair<const KeyType, ValueType>& operator*() const {
            return *data_[index_].data;
        }

        const std::pair<const KeyType, ValueType>* operator->() const {
            return data_[index_].data;
        }
    };

    explicit HashMap(Hash hash = Hash()) : data_(nullptr), capacity_(INITIAL_CAPACITY), size_(0), hash_(hash) {
        data_ = new Data[capacity_];
    }

    HashMap(const HashMap& map) : data_(nullptr), capacity_(map.capacity_), size_(map.size_), hash_(map.hash_) {
        data_ = new Data[capacity_];
        for (size_t i = 0; i < capacity_; ++i) {
            data_[i] = map.data_[i];
        }
    }

    template<class Iterator>
    HashMap(Iterator begin_, Iterator end_, Hash hash = Hash()) : HashMap(hash) {
        for (auto it = begin_; it != end_; ++it) {
            insert(*it);
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> data, Hash hash = Hash()) : HashMap(hash) {
        for (auto curr : data) {
            insert({curr.first, curr.second});
        }
    }

    HashMap& operator=(const HashMap& map) {
        size_ = map.size_;
        capacity_ = map.capacity_;
        hash_ = map.hash_;

        Data* new_data = new Data[capacity_];
        for (size_t i = 0; i < capacity_; ++i) {
            new_data[i] = map.data_[i];
        }

        delete[] data_;
        data_ = new_data;

        return *this;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    Hash hash_function() const {
        return hash_;
    }

    iterator begin() {
        size_t min_el = 0;
        for (; min_el < capacity_ && data_[min_el].free; ++min_el) {}
        return iterator(min_el, data_, capacity_);
    }

    const_iterator begin() const {
        size_t min_el = 0;
        for (; min_el < capacity_ && data_[min_el].free; ++min_el) {}
        return const_iterator(min_el, data_, capacity_);
    }

    iterator end() {
        return iterator(capacity_, data_, capacity_);
    }

    const_iterator end() const {
        return const_iterator(capacity_, data_, capacity_);
    }

    std::pair<iterator, bool> insert(std::pair<KeyType, ValueType> data) {
        const KeyType& key = data.first;
        if (size_ + 1 > capacity_ * LOAD_FACTOR) {
            increase_capacity();
        }

        Data curr_el(data.first, data.second, hash_(key), 0, false);
        size_t curr_pos = curr_el.hash_key % capacity_;
        std::pair<iterator, bool> res = {iterator(), false};
        while (!data_[curr_pos].free) {
            if (!res.second && curr_el.hash_key == data_[curr_pos].hash_key && key == data_[curr_pos].data->first) {
                // key is already in the table
                return {iterator(curr_pos, data_, capacity_), false};
            }

            if (curr_el.psl > data_[curr_pos].psl) {
                std::swap(curr_el, data_[curr_pos]);
                if (!res.second) {
                    // find place for {key, value}
                    res.first = iterator(curr_pos, data_, capacity_);
                    res.second = true;
                }
            }

            ++curr_el.psl;
            ++curr_pos;
            if (curr_pos == capacity_) {
                curr_pos = 0;
            }
        }

        data_[curr_pos] = curr_el;
        if (!res.second) {
            // find place for {key, value}
            res.first = iterator(curr_pos, data_, capacity_);
            res.second = true;
        }
        ++size_;
        return res;
    }

    void erase(const KeyType& key) {
        size_t curr_pos = find_(key);
        if (!data_[curr_pos].free) {
            data_[curr_pos].free = true;
            data_[curr_pos].psl = 0;
            while (data_[(curr_pos + 1) %  capacity_].psl > 0) {
                std::swap(data_[curr_pos], data_[(curr_pos + 1) % capacity_]);
                --data_[curr_pos].psl;
                ++curr_pos;
                if (curr_pos == capacity_) {
                    curr_pos = 0;
                }
            }
            --size_;
        }
    }

    iterator find(const KeyType& key) {
        size_t curr_pos = find_(key);
        if (data_[curr_pos].free) {
            return end();
        } else {
            return iterator(curr_pos, data_, capacity_);
        }
    }

    const_iterator find(const KeyType& key) const {
        size_t curr_pos = find_(key);
        if (data_[curr_pos].free) {
            return end();
        } else {
            return const_iterator(curr_pos, data_, capacity_);
        }
    }

    ValueType& operator[](const KeyType& key) {
        size_t curr_pos = find_(key);
        if (data_[curr_pos].free) {
            return insert({key, ValueType()}).first->second;
        } else {
            return data_[curr_pos].data->second;
        }
    }

    const ValueType& at(const KeyType& key) const {
        const_iterator it = find(key);
        if (it == end()) {
            throw std::out_of_range("Not found key");
        }
        return it->second;
    }

    void clear() {
        delete[] data_;
        capacity_ = INITIAL_CAPACITY;
        size_ = 0;
        data_ = new Data[capacity_];
    }

    ~HashMap() {
        delete[] data_;
    }
};

