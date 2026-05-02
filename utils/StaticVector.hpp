#ifndef STATIC_VECTOR_HPP
#define STATIC_VECTOR_HPP

//Example static vector class for use on 
//bare metal atmega328p
template <typename T, uint8_t N>
class StaticVector {
public:
    bool push_back(const T& value) {
        if (size_ >= N) return false;
        data_[size_++] = value;
        return true;
    }

    T& operator[](uint8_t i) {
        return data_[i];
    }

    uint8_t size() const {
        return size_;
    }

private:
    T data_[N]{};
    uint8_t size_ = 0;
};

#endif
