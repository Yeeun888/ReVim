#include <ostream>
#include <fstream>

template <typename T>
class gap_buffer {
    private: 
        struct buf {
            T* _buffer;
            T* _gap_start;
            T* _gap_end;
            size_t _size;
        };

        static size_t buffer_size;
        struct buf buf;

    public:
        /**
         * @brief Initializes buf with non-garbage values
         * 
         */
        gap_buffer();

        /**
         * @brief Accepts an fstream object of type T an initializes buffer
         * with 
         * 
         * @param stream 
         */
        void init_gap_buffer(std::fstream& stream, size_t start_buff_size);

        /**
         * @brief If buffer is empty adds more space in buffer and doubles 
         * buffer size. Else, adds element
         * 
         * @param element 
         * @return * void 
         */
        void insert(T element);

        /**
         * @brief Shifts buffer left
         * 
         */
        void left();

        /**
         * @brief Shifts buffer right
         * 
         */
        void right();

        /**
         * @brief Make buffer larger
         * 
         */
        void grow();

        /**
         * @brief DEBUG FUNCTION
         * 
         * @param dump_stream 
         */
        void dump_buffer(std::ostream& dump_stream);
};

// Implementation
template <typename T>
size_t gap_buffer<T>::buffer_size; // Definition of the static member

template <typename T>
gap_buffer<T>::gap_buffer() {
    buf._buffer = nullptr;
    buf._gap_start = nullptr;
    buf._gap_end = nullptr;
    buf._size = 0;
}

template <typename T>
void gap_buffer<T>::init_gap_buffer(std::fstream& stream, size_t start_buff_size) {
    buffer_size = start_buff_size;

    T t;
    int num_t = 0;
    while(stream.get(t)) {
        num_t++;
    }

    //Clear all flags and bring back
    stream.clear();
    stream.seekg(0, std::ios::beg);

    buf._buffer = (T *)calloc(sizeof(T) * (num_t + buffer_size), sizeof(T));

    //Start gap at the end
    buf._gap_start = &buf._buffer[num_t];
    buf._gap_end = &buf._buffer[num_t + buffer_size - 1];
    buf._size = num_t + buffer_size;

    return;
}

//--------------------------------- DEBUG  -------------------------------------
template <typename T>
void gap_buffer<T>::dump_buffer(std::ostream& dump_stream) {
    dump_stream << buf._buffer << '\n';
    dump_stream << buf._size << '\n';
}
