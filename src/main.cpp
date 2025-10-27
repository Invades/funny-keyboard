#include <iostream>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>
#include <set>
#include <fstream>
#include <sstream>
#include <map>

#include <nlohmann/json.hpp>
#include "renderer.h"
#include "definitions.h"
using json = nlohmann::json;

// logging macros based on build configuration
#ifdef DEBUG
    #define LOG_INFO(msg) do { \
        std::cout << msg << "\n"; \
        std::ostringstream oss; \
        oss << msg << "\n"; \
        OutputDebugStringA(oss.str().c_str()); \
    } while(0)
    
    #define LOG_ERROR(msg) do { \
        std::cerr << msg << "\n"; \
        std::ostringstream oss; \
        oss << "ERROR: " << msg << "\n"; \
        OutputDebugStringA(oss.str().c_str()); \
    } while(0)
    
    #define LOG_WARNING(msg) do { \
        std::cout << "Warning: " << msg << "\n"; \
        std::ostringstream oss; \
        oss << "Warning: " << msg << "\n"; \
        OutputDebugStringA(oss.str().c_str()); \
    } while(0)
#else
    #define LOG_INFO(msg) ((void)0)
    #define LOG_ERROR(msg) ((void)0)
    #define LOG_WARNING(msg) ((void)0)
#endif

static HHOOK g_keyboard_hook = nullptr;
static std::atomic<bool> g_running{true};
static std::set<DWORD> g_pressed_keys;
static std::mutex g_keys_mutex;
static float g_volume = 1.0f;

struct Config {
    float volume = 0.5f;
    std::string main_sound = "assets/main.wav";
    std::map<std::string, std::string> per_key_overrides;
    std::vector<std::string> images;
    std::string font = "";
    std::string colorize = "";
    
    Config() {
        per_key_overrides["enter"] = "assets/enter.wav";
        per_key_overrides["backspace"] = "assets/backspace.wav";
        images.push_back("assets/fire.webp");
        images.push_back("assets/fire2.webp");
        images.push_back("assets/fire3.webp");
        colorize = "#2AD317";
        font = "C:\\Windows\\Fonts\\MTCORSVA.TTF";
    }
};

static Sound g_main_sound;
static std::map<std::string, Sound> g_key_sounds;

static KeyRenderer* g_renderer = nullptr;

Color parse_hex_color(const std::string& hex_str) {
    if (hex_str == "false" || hex_str == "False" || hex_str == "FALSE") {
        return Color{255, 255, 255, 255};
    }
    
    std::string hex = hex_str;
    if (!hex.empty() && hex[0] == '#') {
        hex = hex.substr(1);
    }
    
    if (hex.length() != 6) {
        return Color{50, 122, 56, 255}; // #327a38
    }
    
    try {
        unsigned int hex_value = std::stoul(hex, nullptr, 16);
        unsigned char r = (hex_value >> 16) & 0xFF;
        unsigned char g = (hex_value >> 8) & 0xFF;
        unsigned char b = hex_value & 0xFF;
        return Color{r, g, b, 255};
    } catch (...) {
        return Color{50, 122, 56, 255};  // #327a38
    }
}

void save_default_config(const std::string& filename)
{
    Config default_config;
    
    json j;
    j["volume"] = default_config.volume * 100.0f;
    j["main_sound"] = default_config.main_sound;
    j["per_key_overrides"] = default_config.per_key_overrides;
    j["images"] = default_config.images;
    j["font"] = default_config.font;
    j["colorize"] = default_config.colorize;
    j["font"] = default_config.font;
    
    std::ofstream config_file(filename);
    if (config_file.is_open()) {
        config_file << j.dump(4);
        config_file.close();
        LOG_INFO("Created default config file");
    } else {
        LOG_ERROR("Failed to create config file");
    }
}

Config load_config(const std::string& filename)
{
    Config config;
    
    std::ifstream config_file(filename);
    if (!config_file.is_open()) {
        LOG_INFO("Config file not found, creating default: " << filename);
        save_default_config(filename);
        return config;
    }
    
    try {
        json j;
        config_file >> j;
        
        if (j.contains("volume")) {
            float volume_percent = j["volume"].get<float>();
            if (volume_percent < 0.0f) volume_percent = 0.0f;
            if (volume_percent > 500.0f) volume_percent = 500.0f;
            config.volume = volume_percent / 100.0f;
            LOG_INFO("Loaded volume: " << volume_percent << "%");
        }
        
        if (j.contains("main_sound")) {
            config.main_sound = j["main_sound"].get<std::string>();
            LOG_INFO("Loaded main_sound: " << config.main_sound);
        }
        
        if (j.contains("per_key_overrides") && j["per_key_overrides"].is_object()) {
            config.per_key_overrides = j["per_key_overrides"].get<std::map<std::string, std::string>>();
            LOG_INFO("Loaded " << config.per_key_overrides.size() << " per-key overrides");
        }
        
        if (j.contains("images") && j["images"].is_array()) {
            config.images = j["images"].get<std::vector<std::string>>();
            LOG_INFO("Loaded " << config.images.size() << " image paths");
        }
        
        if (j.contains("font")) {
            config.font = j["font"].get<std::string>();
            if (!config.font.empty()) {
                LOG_INFO("Loaded font: " << config.font);
            }
        }

        if (j.contains("colorize")) {
            config.colorize = j["colorize"].get<std::string>();
            LOG_INFO("Loaded colorize: " << config.colorize);
        }
        
    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse config file: " << e.what());
        LOG_INFO("Using default configuration");
    }
    
    return config;
}

std::string vk_code_to_key_name(int vk_code)
{
    switch (vk_code) {
        case VK_BACK: return "backspace";
        case VK_RETURN: return "enter";
        case VK_SPACE: return "space";
        case VK_LMENU: case VK_RMENU: return "alt";
        case VK_LSHIFT: case VK_RSHIFT: return "shift";
        case VK_LCONTROL: case VK_RCONTROL: return "control";
        case VK_LWIN: case VK_RWIN: return "win";
        case VK_DELETE: return "delete";
        case VK_INSERT: return "insert";
        case VK_HOME: return "home";
        case VK_END: return "end";
        case VK_PRIOR: return "pageup";
        case VK_NEXT: return "pagedown";
        case VK_SNAPSHOT: return "printscreen";
        case VK_SCROLL: return "scrolllock";
        case VK_PAUSE: return "pause";
        case VK_NUMLOCK: return "numlock";
        case VK_CAPITAL: return "capslock";
        case VK_TAB: return "tab";
        case VK_ESCAPE: return "escape";
        case VK_VOLUME_MUTE: return "volmute";
        case VK_VOLUME_DOWN: return "voldown";
        case VK_VOLUME_UP: return "volup";
        case VK_MEDIA_NEXT_TRACK: return "nexttrack";
        case VK_MEDIA_PREV_TRACK: return "prevtrack";
        case VK_MEDIA_STOP: return "stop";
        case VK_MEDIA_PLAY_PAUSE: return "playpause";

        default: {
            char key_char = MapVirtualKeyA(vk_code, MAPVK_VK_TO_CHAR);
            if (key_char >= 32 && key_char <= 126) {
                return std::string(1, std::tolower(key_char));
            }
            return "";
        }
    }
}

static void enqueue_tone_for_key(int key)
{
    // check if there's per key override
    std::string key_name = vk_code_to_key_name(key);
    Sound* sound_to_play = &g_main_sound;
    
    if (!key_name.empty()) {
        auto it = g_key_sounds.find(key_name);
        if (it != g_key_sounds.end() && it->second.frameCount > 0) {
            sound_to_play = &it->second;
        }
    }
    
    if (sound_to_play->frameCount > 0) {
        SetSoundVolume(*sound_to_play, g_volume);
        PlaySound(*sound_to_play);
    }
}

LRESULT __stdcall keyboard_hook_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        DWORD vkCode = kbd->vkCode;
        
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (kbd->flags & LLKHF_UP) {
                return CallNextHookEx(g_keyboard_hook, nCode, wParam, lParam);
            }
            
            std::lock_guard<std::mutex> lk(g_keys_mutex);
            
            // exit key combo (ctrl+alt+f)
            if (vkCode == 'F') {
                bool ctrl_pressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                bool alt_pressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                
                if (ctrl_pressed && alt_pressed) {
                    g_running = false;
                    return CallNextHookEx(g_keyboard_hook, nCode, wParam, lParam);
                }
            }
            
            // play once per key press
            if (g_pressed_keys.find(vkCode) == g_pressed_keys.end()) {
                g_pressed_keys.insert(vkCode);
                enqueue_tone_for_key(vkCode);
                
                LOG_INFO("Key pressed: vkCode=" << vkCode << " (first press)");
                
                if (g_renderer) {
                    std::string key_text;
                    
                    if (vkCode == VK_RETURN) {
                        key_text = "RET";
                    } else if (vkCode == VK_BACK) {
                        key_text = "BKSP";
                    } else if (vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU) {
                        key_text = "ALT";
                    } else if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) {
                        key_text = "CTRL";
                    } else if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) {
                        key_text = "SHFT";
                    } else if (vkCode == VK_LWIN || vkCode == VK_RWIN) {
                        key_text = "WIN";
                    } else if (vkCode == VK_DELETE) {
                        key_text = "DEL";
                    } else if (vkCode == VK_INSERT) {
                        key_text = "INS";
                    } else if (vkCode == VK_HOME) {
                        key_text = "HOME";
                    } else if (vkCode == VK_END) {
                        key_text = "END";
                    } else if (vkCode == VK_PRIOR) {
                        key_text = "PGUP";
                    } else if (vkCode == VK_NEXT) {
                        key_text = "PGDN";
                    } else if (vkCode == VK_SNAPSHOT) {
                        key_text = "PRSTC";
                    } else if (vkCode == VK_SCROLL) {
                        key_text = "SCRL";
                    } else if (vkCode == VK_PAUSE) {
                        key_text = "PAUSE";
                    } else if (vkCode == VK_NUMLOCK) {
                        key_text = "NUM";
                    } else if (vkCode == VK_CAPITAL) {
                        key_text = "CAPS";
                    } else if (vkCode == VK_TAB) {
                        key_text = "TAB";
                    } else if (vkCode == VK_ESCAPE) {
                        key_text = "ESC";
                    } else if (vkCode == VK_SPACE) {
                        key_text = " ";
                    } else if (vkCode == VK_VOLUME_MUTE) {
                        key_text = "MUTE";
                    } else if (vkCode == VK_VOLUME_DOWN) {
                        key_text = "VOL-";
                    } else if (vkCode == VK_VOLUME_UP) {
                        key_text = "VOL+";
                    } else if (vkCode == VK_MEDIA_NEXT_TRACK) {
                        key_text = "NEXT";
                    } else if (vkCode == VK_MEDIA_PREV_TRACK) {
                        key_text = "PREV";
                    } else if (vkCode == VK_MEDIA_STOP) {
                        key_text = "STOP";
                    } else if (vkCode == VK_MEDIA_PLAY_PAUSE) {
                        key_text = "PLAY";
                    } else if (vkCode >= VK_F1 && vkCode <= VK_F12) {
                        key_text = "F" + std::to_string(vkCode - VK_F1 + 1);
                    } else {
                        char key_char = MapVirtualKeyA(vkCode, MAPVK_VK_TO_CHAR);
                        if (key_char != 0) {
                            key_text = std::string(1, key_char);
                        } else {
                            key_text = "?";
                        }
                    }
                    
                    g_renderer->add_key_effect(key_text, vkCode);
                    LOG_INFO("Added visual effect for key: " << key_text);
                }
            } else {
                LOG_INFO("Key held (ignored): vkCode=" << vkCode);
            }
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            std::lock_guard<std::mutex> lk(g_keys_mutex);
            g_pressed_keys.erase(vkCode);
        }
    }
    return CallNextHookEx(g_keyboard_hook, nCode, wParam, lParam);
}

int main()
{
    Config config = load_config("config.json");
    g_volume = config.volume;
    int monitor_width = GetScreenWidth();
    int monitor_height = GetScreenHeight();
    
    if (monitor_width == 0 || monitor_height == 0) {
        monitor_width = GetSystemMetrics(SM_CXSCREEN);
        monitor_height = GetSystemMetrics(SM_CYSCREEN);
    }
    
    LOG_INFO("Primary monitor resolution: " << monitor_width << "x" << monitor_height);

    SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED | 
                    FLAG_WINDOW_TOPMOST | FLAG_WINDOW_MOUSE_PASSTHROUGH);
    
    InitWindow(monitor_width, monitor_height, "funny-keyboard");
    SetWindowPosition(0, 0);
    SetTargetFPS(60);
    
    HWND hwnd = (HWND)GetWindowHandle();
    
    ShowWindow(hwnd, SW_HIDE);
    
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE;
    exStyle &= ~WS_EX_APPWINDOW;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    
    typedef struct {
        int cxLeftWidth;
        int cxRightWidth;
        int cyTopHeight;
        int cyBottomHeight;
    } MARGINS;
    
    typedef LONG (__stdcall *DwmExtendFrameIntoClientAreaProc)(HWND, const MARGINS*);
    HINSTANCE dwmapi = GetModuleHandle("dwmapi.dll");
    if (dwmapi) {
        DwmExtendFrameIntoClientAreaProc DwmExtendFrameIntoClientArea = 
            (DwmExtendFrameIntoClientAreaProc)GetProcAddress((HMODULE)dwmapi, "DwmExtendFrameIntoClientArea");
        if (DwmExtendFrameIntoClientArea) {
            MARGINS margins = {-1, -1, -1, -1};
            DwmExtendFrameIntoClientArea(hwnd, &margins);
        }
    }
    
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    
    BeginDrawing();
    ClearBackground((Color){0, 0, 0, 0});
    EndDrawing();
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    
    g_renderer = new KeyRenderer(monitor_width, monitor_height);
    Color tint_color = parse_hex_color(config.colorize);
    if (!g_renderer->init(config.images, config.font, tint_color)) {
        LOG_ERROR("Failed to initialize renderer");
        CloseWindow();
        return 1;
    }

    InitAudioDevice();
    
    if (!IsAudioDeviceReady()) {
        LOG_ERROR("Failed to initialize audio device");
#ifdef RELEASE
        MessageBoxA(NULL, "Failed to initialize audio device", 
                    "funny keyboard", MB_OK | MB_ICONERROR);
#endif
        delete g_renderer;
        CloseWindow();
        return 1;
    }

    LOG_INFO("Loading audio files from config...");
    
    g_main_sound = LoadSound(config.main_sound.c_str());
    if (g_main_sound.frameCount == 0) {
        LOG_ERROR("'" << config.main_sound << "' is required but could not be loaded");
        CloseAudioDevice();
        delete g_renderer;
        CloseWindow();
        return 1;
    }
    LOG_INFO("Loaded main sound: " << config.main_sound);
    
    for (const auto& [key_name, sound_file] : config.per_key_overrides) {
        Sound sound = LoadSound(sound_file.c_str());
        if (sound.frameCount > 0) {
            g_key_sounds[key_name] = sound;
            LOG_INFO("Loaded override sound for '" << key_name << "': " << sound_file);
        } else {
            LOG_WARNING("Failed to load override sound for '" << key_name << "': " << sound_file << " (will use main sound)");
        }
    }
    
    LOG_INFO("Audio files loaded successfully");

    g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook_proc, GetModuleHandle(NULL), 0);
    if (!g_keyboard_hook) {
        LOG_ERROR("Failed to install keyboard hook");
#ifdef RELEASE
        MessageBoxA(NULL, "Failed to install keyboard hook", 
                    "funny keyboard", MB_OK | MB_ICONERROR);
#endif
        delete g_renderer;
        CloseWindow();
        return 1;
    }

    LOG_INFO("Global keyboard hook active");

    while (!WindowShouldClose() && g_running) {
        BeginDrawing();
        
        ClearBackground((Color){0, 0, 0, 0});
        
        if (g_renderer) {
            g_renderer->update_and_render();
        }
        
        EndDrawing();
    }

    if (g_keyboard_hook) {
        UnhookWindowsHookEx(g_keyboard_hook);
        g_keyboard_hook = nullptr;
    }

    if (g_main_sound.frameCount > 0) {
        UnloadSound(g_main_sound);
    }
    
    for (auto& [key_name, sound] : g_key_sounds) {
        if (sound.frameCount > 0) {
            UnloadSound(sound);
        }
    }
    g_key_sounds.clear();
    
    CloseAudioDevice();
    
    if (g_renderer) {
        delete g_renderer;
        g_renderer = nullptr;
    }
    
    CloseWindow();
    return 0;
}
