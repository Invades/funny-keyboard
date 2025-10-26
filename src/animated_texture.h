#pragma once

#include <vector>
#include <string>
#include <raylib.h>
#include <webp/decode.h>
#include <webp/demux.h>

class AnimatedTexture {
public:
    AnimatedTexture() : current_frame(0), frame_time(0.0f), is_animated(false) {}
    
    AnimatedTexture(const AnimatedTexture&) = delete;
    AnimatedTexture& operator=(const AnimatedTexture&) = delete;
    
    AnimatedTexture(AnimatedTexture&& other) noexcept
        : filename(std::move(other.filename))
        , frames(std::move(other.frames))
        , frame_delays(std::move(other.frame_delays))
        , current_frame(other.current_frame)
        , frame_time(other.frame_time)
        , is_animated(other.is_animated)
    {
        other.frames.clear();
    }
    
    AnimatedTexture& operator=(AnimatedTexture&& other) noexcept {
        if (this != &other) {
            unload();
            filename = std::move(other.filename);
            frames = std::move(other.frames);
            frame_delays = std::move(other.frame_delays);
            current_frame = other.current_frame;
            frame_time = other.frame_time;
            is_animated = other.is_animated;
            other.frames.clear();
        }
        return *this;
    }
    
    ~AnimatedTexture() {
        unload();
    }
    
    bool load_from_file(const std::string& filepath) {
        filename = filepath;
        
        std::string ext = get_file_extension(filepath);
        
        if (ext == ".gif") {
            return load_gif(filepath);
        } else if (ext == ".webp") {
            return load_webp(filepath);
        } else {
            return load_static(filepath);
        }
    }
    
    void update(float delta_time) {
        if (!is_animated || frames.empty()) return;
        
        frame_time += delta_time;

        if (frame_time >= frame_delays[current_frame]) {
            frame_time = 0.0f;
            current_frame = (current_frame + 1) % frames.size();
        }
    }

    const Texture2D& get_current_texture() const {
        static Texture2D invalid_texture = {0};
        if (frames.empty() || current_frame >= frames.size()) {
            return invalid_texture;
        }
        return frames[current_frame];
    }
    
    bool is_loaded() const {
        return !frames.empty();
    }
    
    int get_width() const {
        return frames.empty() ? 0 : frames[0].width;
    }
    
    int get_height() const {
        return frames.empty() ? 0 : frames[0].height;
    }
    
private:
    void unload() {
        for (auto& tex : frames) {
            if (tex.id > 0) {
                UnloadTexture(tex);
            }
        }
        frames.clear();
        frame_delays.clear();
    }
    
    bool load_gif(const std::string& filepath) {
        int frame_count = 0;
        Image anim = LoadImageAnim(filepath.c_str(), &frame_count);
        
        if (anim.data == nullptr || frame_count <= 0) {
            return false;
        }

        int frame_width = anim.width;
        int frame_height = anim.height / frame_count;
        

        std::vector<Image> frame_images;
        for (int i = 0; i < frame_count; i++) {
            Rectangle source_rect = {0, (float)(i * frame_height), (float)frame_width, (float)frame_height};
            Image frame_img = ImageFromImage(anim, source_rect);
            frame_images.push_back(frame_img);
        }
        
        UnloadImage(anim);
        
        for (auto& frame_img : frame_images) {
            ImageResize(&frame_img, 64, 64);
            
            Texture2D frame_tex = LoadTextureFromImage(frame_img);
            UnloadImage(frame_img);
            
            if (frame_tex.id == 0) {
                for (auto& tex : frames) {
                    if (tex.id > 0) UnloadTexture(tex);
                }
                frames.clear();
                frame_delays.clear();
                return false;
            }
            
            frames.push_back(frame_tex);
            frame_delays.push_back(0.1f);
        }
        
        is_animated = (frame_count > 1);
        return true;
    }
    // webp demuxer
    bool load_webp(const std::string& filepath) {
        int file_size = 0;
        unsigned char* file_data = LoadFileData(filepath.c_str(), &file_size);
        
        if (!file_data) {
            return false;
        }
        
        WebPData webp_data = {file_data, static_cast<size_t>(file_size)};
        WebPDemuxer* demux = WebPDemux(&webp_data);
        
        if (!demux) {
            UnloadFileData(file_data);
            return false;
        }
        
        uint32_t flags = WebPDemuxGetI(demux, WEBP_FF_FORMAT_FLAGS);
        bool has_animation = (flags & ANIMATION_FLAG) != 0;
        
        if (has_animation) {
            int frame_count = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
            
            WebPIterator iter;
            if (WebPDemuxGetFrame(demux, 1, &iter)) {
                do {
                    int width, height;
                    uint8_t* rgba = WebPDecodeRGBA(iter.fragment.bytes, iter.fragment.size, &width, &height);
                    
                    if (rgba) {
                        Image img = {
                            .data = rgba,
                            .width = width,
                            .height = height,
                            .mipmaps = 1,
                            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
                        };

                        Image img_copy = ImageCopy(img);
                        WebPFree(rgba);
                        
                        ImageResize(&img_copy, 64, 64);
                        
                        Texture2D tex = LoadTextureFromImage(img_copy);
                        UnloadImage(img_copy);
                        
                        frames.push_back(tex);
                        
                        float delay_ms = iter.duration;
                        frame_delays.push_back(delay_ms / 1000.0f);
                    }
                } while (WebPDemuxNextFrame(&iter));
                
                WebPDemuxReleaseIterator(&iter);
            }
            
            is_animated = true;
        } else {
            // load static webp
            int width, height;
            uint8_t* rgba = WebPDecodeRGBA(file_data, file_size, &width, &height);
            
            if (rgba) {
                Image img = {
                    .data = rgba,
                    .width = width,
                    .height = height,
                    .mipmaps = 1,
                    .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
                };
                
                Image img_copy = ImageCopy(img);
                WebPFree(rgba);
                
                ImageResize(&img_copy, 64, 64);
                
                Texture2D tex = LoadTextureFromImage(img_copy);
                UnloadImage(img_copy);
                
                frames.push_back(tex);
                frame_delays.push_back(0.0f);
            }
            
            is_animated = false;
        }
        
        WebPDemuxDelete(demux);
        UnloadFileData(file_data);
        
        return !frames.empty();
    }
    
    bool load_static(const std::string& filepath) {
        Image img = LoadImage(filepath.c_str());
        
        if (img.data == nullptr) {
            return false;
        }
        
        ImageResize(&img, 64, 64);
        
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        
        if (tex.id == 0) {
            return false;
        }
        
        frames.push_back(tex);
        frame_delays.push_back(0.0f);
        is_animated = false;
        
        return true;
    }
    
    std::string get_file_extension(const std::string& filepath) {
        size_t dot_pos = filepath.find_last_of('.');
        if (dot_pos == std::string::npos) {
            return "";
        }
        
        std::string ext = filepath.substr(dot_pos);
        for (char& c : ext) {
            c = std::tolower(c);
        }
        return ext;
    }
    
    std::string filename;
    std::vector<Texture2D> frames;
    std::vector<float> frame_delays;
    size_t current_frame;
    float frame_time;
    bool is_animated;
};
