#include "vigor/text_buffer.h"

void TextBuffer::register_callback(callable_t cb) {
    this->callback = cb;
}

void TextBuffer::load_file(std::string filepath) {
    this->stream->open(filepath);
}

void TextBuffer::inc_start_line() {
    this->start_line++;
}

void TextBuffer::inc_stop_line() {
    this->stop_line++;
}

void TextBuffer::dec_start_line() {
    this->start_line--;
}

void TextBuffer::dec_stop_line() {
    this->stop_line--;
}

void TextBuffer::append_text(std::string text) {
    long pos = this->stream->tellp();
    this->stream->seekp(0, this->stream->end);
    this->stream->write(text.c_str(), text.size());
    this->stream->seekp(pos);
}

void TextBuffer::insert_text(std::string text) {
    this->stream->write(text.c_str(), text.size());
}

std::optional<std::string> TextBuffer::read_prev_line() {
    return {};
}

std::optional<std::string> TextBuffer::read_next_line() {
    unsigned int buffer_height = this->stop_line - this->start_line;

    if (buffer_height >= this->max_buffer_height)
        return {};

    this->inc_stop_line();

    char line[1024];
    this->stream->getline(line, 1024);

    if (this->stream->rdstate() & std::fstream::failbit) {
        // TODO: Handle
    }

    return line;
}
