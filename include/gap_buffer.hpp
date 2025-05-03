#include <ostream>
#include <fstream>
#include <stdlib.h>

template <typename T>
class gap_buffer {
    private: 
        struct buf {
            T* _buffer;                 // Buffer containing text
            T* _gap_start;              // Gap start address
            T* _gap_end;                // Gap end address
            size_t _buf_size;           // Buffer size
        };

        static size_t gap_size;         // Gap starting size (double on resize)
        struct buf buf;                 // the actual buf

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
         * buffer size. Else, adds element. 
         * 
         * @param element 
         * @param pos Position in the text without gap buffer
         * @return 1 on success, 0 on failure
         */
        int insert(T element);

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
         * @return 1 on success, 0 on filaure
         */ 
        int grow();

        // ------------------------ DEBUG FUNCTIONS ----------------------------
        void dump_buffer(std::ostream& dump_stream);
        void fill_gap(T t);
};

// --------------------- TEMPLATE IMPLEMENTATION -------------------------------

template <typename T>
size_t gap_buffer<T>::gap_size; // Definition of the static member

template <typename T>
gap_buffer<T>::gap_buffer() {
    buf._buffer = nullptr;
    buf._gap_start = nullptr;
    buf._gap_end = nullptr;
    buf._buf_size = 0;
    gap_size = 0;
}

template <typename T>
void gap_buffer<T>::init_gap_buffer(std::fstream& stream, size_t start_gap_size) {
    //Save stream pos
    std::streampos init_pos = stream.tellg();

    gap_size = start_gap_size;

    T t;
    int num_t = 0;
    while(stream.get(t)) {
        num_t++;
    }

    //Clear all flags and bring back
    stream.clear();
    stream.seekg(0, std::ios::beg);

    buf._buffer = (T *)calloc(sizeof(T) * (num_t + gap_size), sizeof(T));

    //Start gap at the end
    buf._gap_start = &buf._buffer[num_t];
    buf._gap_end = &buf._buffer[num_t + gap_size];
    buf._buf_size = num_t + gap_size;

    for (int i = 0; i < num_t; ++i) {
        stream.get(t);
        buf._buffer[i] = t;
    }

    //Clear all flags and return stream to the beginning
    stream.clear();
    stream.seekg(init_pos, std::ios::beg);

    return;
}

template <typename T>
int gap_buffer<T>::insert(T t) {
    if (buf._gap_start == buf._gap_end) {
        if(!grow()) {
            return 0;
        };
    }

    *buf._gap_start = t;
    ++buf._gap_start;

    return 1;
}

template <typename T>
int gap_buffer<T>::grow() {
    // Alternative to limit gap growth
    // gap_size = (gap_size * 2 > 10000) 10000 : gap_size * 2;

    // This should be the same. However to confirm safety, it is recalculated
    size_t prev_buf_start_index = buf._gap_start - buf._buffer;
    size_t elements_to_move = buf._buf_size - (buf._gap_start - buf._buffer);

    T* newPtr = (T *)realloc(buf._buffer, sizeof(T) * (buf._buf_size + 2*gap_size));
    if (newPtr == nullptr)  {
        return 0;
    } else {
        buf._buffer = newPtr;
    }

    buf._gap_start = &buf._buffer[prev_buf_start_index];
    buf._gap_end = &buf._buffer[prev_buf_start_index + 2*gap_size];
    
    memmove(buf._gap_end, buf._gap_start, elements_to_move); 
    
    buf._buf_size += 2*gap_size;
    gap_size *= 2;
    //Move buffer items right

    return 1;
}

//--------------------------------- DEBUG  -------------------------------------
template <typename T>
void gap_buffer<T>::dump_buffer(std::ostream& dump_stream) {
    dump_stream << buf._buffer << '\n';
    dump_stream << buf._buf_size << '\n';
}

template <typename T>
void gap_buffer<T>::fill_gap(T t) {

}