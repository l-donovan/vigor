#pragma once

#include <functional>
#include <fstream>
#include <string>
#include <optional>

struct TextBuffer : public std::filebuf {
private:
    using callable_t = std::function<bool(std::filebuf::int_type)>;
    callable_t callback = nullptr;
    std::fstream *stream = nullptr;

    unsigned int start_line = 0;
    unsigned int stop_line = 0;
public:
    TextBuffer() {}
    TextBuffer(callable_t cb) : callback(cb) {}

    void register_callback(callable_t cb);

    void load_file(std::string filepath);

    void inc_start_line();
    void inc_stop_line();
    void dec_start_line();
    void dec_stop_line();

    void append_text(std::string text);
    void insert_text(std::string text);

    std::optional<std::string> read_prev_line();
    std::optional<std::string> read_next_line();
};