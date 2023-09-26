#include "hydra/core.hxx"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

extern "C" {
    extern void se_load_rom(const char* path);
    extern void se_reset_core();
    extern void se_emulate_single_frame();
    extern void se_screenshot(uint8_t*, int*, int*);
    extern void se_emulate_single_frame();
    extern uint32_t se_get_width();
    extern uint32_t se_get_height();
    extern void se_touch(float, float);
    extern float* se_get_inputs();
    extern uint32_t se_add_cheat(uint8_t* data, uint32_t size);
    extern void se_remove_cheat(uint32_t id);
    extern void se_enable_cheat(uint32_t id);
    extern void se_disable_cheat(uint32_t id);
    extern void se_push_all_samples(void(*)(void*, size_t));
    extern uint32_t se_sample_count();
}

class HydraCore : public hydra::IBase, public hydra::ISoftwareRendered, public hydra::IFrontendDriven, public hydra::IInput, public hydra::IAudio, public hydra::ICheat
{
    HYDRA_CLASS
public:
    bool loadFile(const char* type, const char* path) override;
    void reset() override;
    hydra::Size getNativeSize() override;
    void setOutputSize(hydra::Size size) override;

    void setVideoCallback(void (*callback)(void* data, hydra::Size size)) override;

    void runFrame() override;
    uint16_t getFps() override;

    uint32_t getSampleRate() override { return 48000; }
    void setAudioCallback(void (*callback)(void* data, size_t size)) override;

    void setPollInputCallback(void(*callback)()) override;
    void setCheckButtonCallback(int32_t (*callback)(uint32_t, hydra::ButtonType)) override;

    uint32_t addCheat(const uint8_t* code, uint32_t size) override;
    void removeCheat(uint32_t id) override;
    void enableCheat(uint32_t id) override;
    void disableCheat(uint32_t id) override;

private:
    void (*video_callback)(void* data, hydra::Size size) = nullptr;
    void (*audio_callback)(void* data, size_t size) = nullptr;
    void (*poll_callback)() = nullptr;
    int32_t (*read_callback)(uint32_t, hydra::ButtonType) = nullptr;
};

bool HydraCore::loadFile(const char* type, const char* path)
{
    if (std::string(type) == std::string("rom"))
    {
        se_load_rom(path);
        return true;
    }
    return false;
}

void HydraCore::reset()
{
    se_reset_core();
}

hydra::Size HydraCore::getNativeSize()
{
    return { se_get_width(), se_get_height() };
}

void HydraCore::setOutputSize(hydra::Size size)
{
}

void HydraCore::setVideoCallback(void (*callback)(void* data, hydra::Size size))
{
    video_callback = callback;
}

void HydraCore::setAudioCallback(void (*callback)(void* data, size_t size))
{
    audio_callback = callback;
}

#define SE_KEY_A 0 
#define SE_KEY_B 1 
#define SE_KEY_X 2 
#define SE_KEY_Y 3 
#define SE_KEY_UP 4
#define SE_KEY_DOWN 5
#define SE_KEY_LEFT 6
#define SE_KEY_RIGHT 7
#define SE_KEY_L 8
#define SE_KEY_R 9
#define SE_KEY_START 10
#define SE_KEY_SELECT 11
#define SE_KEY_FOLD_SCREEN 12
#define SE_KEY_PEN_DOWN 13
#define SE_KEY_EMU_PAUSE 14
#define SE_KEY_EMU_REWIND 15
#define SE_KEY_EMU_FF_2X 16
#define SE_KEY_EMU_FF_MAX 17
#define SE_KEY_CAPTURE_STATE(A) (18+(A)*2)
#define SE_KEY_RESTORE_STATE(A) (18+(A)*2+1)
#define SE_KEY_RESET_GAME 26
#define SE_KEY_TURBO_A  27
#define SE_KEY_TURBO_B  28
#define SE_KEY_TURBO_X  29
#define SE_KEY_TURBO_Y  30
#define SE_KEY_TURBO_L  31
#define SE_KEY_TURBO_R  32
#define SE_KEY_SOLAR_P  33
#define SE_KEY_SOLAR_M  34
#define SE_KEY_TOGGLE_FULLSCREEN 35
#define SE_NUM_KEYBINDS 36
void HydraCore::runFrame()
{
    poll_callback();
    float* inputs = se_get_inputs();
    inputs[SE_KEY_LEFT] = !!read_callback(0, hydra::ButtonType::Keypad1Left);
    inputs[SE_KEY_RIGHT] = !!read_callback(0, hydra::ButtonType::Keypad1Right);
    inputs[SE_KEY_UP] = !!read_callback(0, hydra::ButtonType::Keypad1Up);
    inputs[SE_KEY_DOWN] = !!read_callback(0, hydra::ButtonType::Keypad1Down);
    inputs[SE_KEY_A] = !!read_callback(0, hydra::ButtonType::A);
    inputs[SE_KEY_B] = !!read_callback(0, hydra::ButtonType::B);
    inputs[SE_KEY_X] = !!read_callback(0, hydra::ButtonType::X);
    inputs[SE_KEY_Y] = !!read_callback(0, hydra::ButtonType::Y);
    inputs[SE_KEY_SELECT] = !!read_callback(0, hydra::ButtonType::Select);
    inputs[SE_KEY_START] = !!read_callback(0, hydra::ButtonType::Start);
    inputs[SE_KEY_L] = !!read_callback(0, hydra::ButtonType::L1);
    inputs[SE_KEY_R] = !!read_callback(0, hydra::ButtonType::R1);

    uint32_t touch = read_callback(0, hydra::ButtonType::Touch);
    uint16_t x = touch >> 16;
    uint16_t y = touch & 0xFFFF;
    if (touch != hydra::TOUCH_RELEASED && y > 192)
    {
        inputs[SE_KEY_PEN_DOWN] = 1;
        float fx = (float)x / 256.0f;
        float fy = (float)(y - 192) / 192.0f;
        se_touch(fx, fy);
    }
    else
    {
        inputs[SE_KEY_PEN_DOWN] = 0;
    }

    se_emulate_single_frame();
    int out_width = se_get_width(), out_height = se_get_height();
    uint8_t* imdata = (uint8_t*)malloc(out_width * out_height * 4);
    se_screenshot(imdata, &out_width, &out_height);
    video_callback(imdata, { static_cast<uint32_t>(out_width), static_cast<uint32_t>(out_height) });
    free(imdata);

    se_push_all_samples(audio_callback);
}

uint16_t HydraCore::getFps()
{
    return 60;
}

void HydraCore::setPollInputCallback(void(*callback)())
{
    poll_callback = callback;
}

void HydraCore::setCheckButtonCallback(int32_t (*callback)(uint32_t, hydra::ButtonType))
{
    read_callback = callback;
}

uint32_t HydraCore::addCheat(const uint8_t* code, uint32_t size)
{
    return se_add_cheat((uint8_t*)code, size);
}

void HydraCore::removeCheat(uint32_t id)
{
    se_remove_cheat(id);
}

void HydraCore::enableCheat(uint32_t id)
{
    se_enable_cheat(id);
}

void HydraCore::disableCheat(uint32_t id)
{
    se_disable_cheat(id);
}

HC_API hydra::IBase* createEmulator()
{
    return new HydraCore();
}

HC_API void destroyEmulator(hydra::IBase* emulator)
{
    delete emulator;
}

HC_API const char* getInfo(hydra::InfoType type)
{
    switch (type)
    {
        case hydra::InfoType::CoreName:
            return "SkyEmu";
        case hydra::InfoType::SystemName:
            return "Gameboy Color, Gameboy Advance, Nintendo DS";
        case hydra::InfoType::Description:
            return "Game Boy Advance, Game Boy, Game Boy Color, and Nintendo DS Emulator";
        case hydra::InfoType::Version:
            return "4.0";
        case hydra::InfoType::Author:
            return "Sky";
        case hydra::InfoType::Extensions:
            return "gb,gbc,gba,nds";
        case hydra::InfoType::License:
            return "MIT";
        case hydra::InfoType::Website:
            return "https://skyemu.app";
        case hydra::InfoType::Firmware:
            return "";
        default: return nullptr;
    }
}
