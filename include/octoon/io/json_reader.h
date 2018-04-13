#ifndef OCTOON_JSON_READER_H
#define OCTOON_JSON_READER_H

#include <string>
#include <cstdint>
#include <octoon/io/text_reader.h>
#include <octoon/io/istream.h>

namespace octoon
{
    namespace io
    {
        class JsonReader
        {
        public:
            JsonReader(istream &stream);

            virtual void close();
            virtual int peek();
            virtual int read();
            virtual void read(char *str, std::int32_t begin, std::int32_t end);
            virtual std::string readLine();
            virtual std::string readToEnd();
        private:
            istream& base_stream;
        };
    }
}

#endif