#ifndef BRR_IMAGE_H
#define BRR_IMAGE_H

#include <filesystem>

namespace brr::vis
{
    class Image
    {
    public:

        // Construct image loaded from file
        Image(std::filesystem::path image_path);

        // Construct image loaded from buffer
        Image(uint32_t width, uint32_t height, uint8_t* data);

        // Destructor
        ~Image();

    private:
        uint8_t* m_buffer = nullptr;
        uint32_t m_totalSize = 0;
        uint32_t m_width = 0, m_height = 0;
    };

}

#endif