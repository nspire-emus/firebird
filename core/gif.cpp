#include <array>
#include <mutex>
#include <vector>

#include "emu.h"
#include "gif.h"
#include "lcd.h"

// We can't modify giflib.h ourselves, so #include that here.
#include "os/os.h"
#include "gif-h/gif.h"

struct RGB24 {
    uint8_t r, g, b, a;
};

static std::mutex gif_mutex;
static bool recording = false;
static GifWriter writer;
static std::vector<RGB24> buffer;
static unsigned int framenr = 0, framenrskip = 0, framedelay = 0;

bool gif_start_recording(const char *filename, unsigned int frameskip)
{
    std::lock_guard<std::mutex> lock(gif_mutex);

    framenr = framenrskip = frameskip;
    framedelay = 100 / (60/(frameskip+1));

    FILE *gif_file = fopen_utf8(filename, "wb");

    if(gif_file && GifBegin(&writer, gif_file, 320, 240, framedelay))
        recording = true;

    buffer.resize(320*240);

    return recording;
}

void gif_new_frame()
{
    if(!recording)
        return;

    std::lock_guard<std::mutex> gif_lock(gif_mutex);

    if(!recording || --framenr)
        return;

    framenr = framenrskip;

    static std::array<uint16_t, 320 * 240> framebuffer;

    lcd_cx_draw_frame(framebuffer.data());

    uint16_t *ptr16 = framebuffer.data();
    RGB24 *ptr24 = buffer.data();

    /* Convert RGB565 or RGB444 to RGBA8888 */
    if(emulate_cx)
    {
        for(unsigned int i = 0; i < 320*240; ++i)
        {
            ptr24->r = (*ptr16 & 0b1111100000000000) >> 8;
            ptr24->g = (*ptr16 & 0b0000011111100000) >> 3;
            ptr24->b = (*ptr16 & 0b0000000000011111) << 3;
            ++ptr24;
            ++ptr16;
        }
    }
    else
    {
        for(unsigned int i = 0; i < 320*240; ++i)
        {
            uint8_t pix = ~(*ptr16 & 0xF);
            ptr24->r = pix << 4;
            ptr24->g = pix << 4;
            ptr24->b = pix << 4;
            ++ptr24;
            ++ptr16;
        }
    }

    if(!GifWriteFrame(&writer, reinterpret_cast<const uint8_t*>(buffer.data()), 320, 240, framedelay))
        recording = false;
}

bool gif_stop_recording()
{
    std::lock_guard<std::mutex> lock(gif_mutex);

    bool ret = recording;

    recording = false;

    buffer.clear();
    GifEnd(&writer);
    return ret;
}
