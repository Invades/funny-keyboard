#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <raylib.h>
#include "animated_texture.h"

#if defined(_WIN32)
    #undef NOGDI
    #undef NOUSER
#endif

struct KeyEffect {
    std::string key_text;
    float x, y;
    float alpha;
    float scale;
    std::chrono::steady_clock::time_point start_time;
    int texture_index;
    bool active;
};

class KeyRenderer {
public:
    KeyRenderer(int screen_width, int screen_height)
        : width(screen_width), height(screen_height), tint_color({255, 255, 255, 255}) {}
    
    ~KeyRenderer() {
        textures.clear();
        
        if (custom_font_loaded) {
            UnloadFont(font);
        }
    }
    
    bool init(const std::vector<std::string>& image_paths, const std::string& font_path = "", Color tint = {255, 255, 255, 255}) {
        tint_color = tint;
        
        if (!font_path.empty()) {
            font = LoadFont(font_path.c_str());
            if (font.texture.id > 0) {
                custom_font_loaded = true;
                std::cout << "Loaded custom font: " << font_path << "\n";
            } else {
                std::cout << "Warning: Failed to load font: " << font_path << ", using default\n";
                font = GetFontDefault();
                custom_font_loaded = false;
            }
        } else {
            font = GetFontDefault();
            custom_font_loaded = false;
        }
        
        textures.reserve(image_paths.size());
        
        for (const auto& image_path : image_paths) {
            AnimatedTexture anim_tex;
            if (anim_tex.load_from_file(image_path)) {
                std::cout << "Loaded image: " << image_path << " - texture id: " << anim_tex.get_current_texture().id << "\n";
                textures.push_back(std::move(anim_tex));
            } else {
                std::cout << "Warning: Failed to load image: " << image_path << "\n";
            }
        }
        
        if (textures.empty()) {
            std::cout << "No images loaded, will use default circle rendering\n";
        }
        
        return true;
    }
    
    void add_key_effect(const std::string& key_text, int vk_code) {
        KeyEffect effect;
        effect.key_text = key_text;
        
        effect.x = GetRandomValue(100, width - 100);
        effect.y = GetRandomValue(100, height - 100);
        
        effect.alpha = 1.0f;
        effect.scale = 1.0f;
        effect.start_time = std::chrono::steady_clock::now();
        
        if (!textures.empty()) {
            effect.texture_index = GetRandomValue(0, textures.size() - 1);
        } else {
            effect.texture_index = -1;
        }
        
        effect.active = true;
        
        active_effects.push_back(effect);
    }
    
    void update_and_render() {
        auto now = std::chrono::steady_clock::now();
        static auto last_frame = now;
        float delta_time = std::chrono::duration<float>(now - last_frame).count();
        last_frame = now;
        
        for (auto& anim_tex : textures) {
            anim_tex.update(delta_time);
        }
        
        for (auto it = active_effects.begin(); it != active_effects.end();) {
            KeyEffect& effect = *it;
            
            float elapsed = std::chrono::duration<float>(now - effect.start_time).count();
            
            float fade_duration = 1.0f;
            effect.alpha = 1.0f - (elapsed / fade_duration);
            
            effect.scale = 1.0f + (elapsed * 0.5f);
            
            if (effect.alpha <= 0.0f) {
                it = active_effects.erase(it);
                continue;
            }
            
            render_effect(effect);
            
            ++it;
        }
    }
    
private:
    void render_effect(const KeyEffect& effect) {
        Color text_color = {255, 255, 255, (unsigned char)(effect.alpha * 255)};
        
        int font_size = 48 * effect.scale;
        Vector2 text_size = MeasureTextEx(font, effect.key_text.c_str(), font_size, 2);
        
        if (effect.texture_index >= 0 && effect.texture_index < textures.size()) {
            const Texture2D& texture = textures[effect.texture_index].get_current_texture();
            
            if (texture.id > 0) {
                float base_scale = effect.scale;
                float scale_x = base_scale;
                float scale_y = base_scale;
                
                // stretch horizontally for modifiers
                if (text_size.x > texture.width * base_scale) {
                    scale_x = (text_size.x / texture.width) * 1.2f;
                }
                
                float scaled_width = texture.width * scale_x;
                float scaled_height = texture.height * scale_y;
                
                Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
                Rectangle dest = {
                    effect.x - scaled_width / 2,
                    effect.y - scaled_height / 2,
                    scaled_width,
                    scaled_height
                };
                Vector2 origin = {0, 0};
                
                BeginBlendMode(BLEND_ADDITIVE);
                
                Color vibrant_color = {tint_color.r, tint_color.g, tint_color.b, 
                                      (unsigned char)(effect.alpha * 180)};
                DrawTexturePro(texture, source, dest, origin, 0.0f, vibrant_color);
                
                EndBlendMode();
                
                BeginBlendMode(BLEND_ALPHA);
                Color solid_color = {tint_color.r, tint_color.g, tint_color.b, 
                                    (unsigned char)(effect.alpha * 255)};
                DrawTexturePro(texture, source, dest, origin, 0.0f, solid_color);
                EndBlendMode();
            }
        } else {
            Color circle_color = {tint_color.r, tint_color.g, tint_color.b, (unsigned char)(effect.alpha * 200)};
            DrawCircle(effect.x, effect.y, 30 * effect.scale, circle_color);
        }
        
        DrawTextEx(font, effect.key_text.c_str(), 
                  {effect.x - text_size.x / 2, effect.y - text_size.y / 2},
                  font_size, 2, text_color);
    }
    
    int width, height;
    Font font;
    bool custom_font_loaded = false;
    Color tint_color;
    std::vector<AnimatedTexture> textures;
    std::vector<KeyEffect> active_effects;
};
