#include "music_analyzer.h"
#include <cmath>
#include <esp_timer.h>
#include <esp_log.h>

#define TAG "MusicAnalyzer"

MusicAnalyzer::MusicAnalyzer() {}
MusicAnalyzer::~MusicAnalyzer() {}

void MusicAnalyzer::Feed(const std::vector<int16_t>& data) {
    if (data.empty()) return;

    // 计算均方根 (RMS)
    double sum_sq = 0;
    for (int16_t sample : data) {
        sum_sq += (double)sample * sample;
    }
    float rms = std::sqrt(sum_sq / data.size()) / 32768.0f;

    std::lock_guard<std::mutex> lock(mutex_);
    
    // 简单的指数平滑 (LPF)
    smooth_energy_ = smooth_energy_ * (1.0f - kSmoothingFactor) + rms * kSmoothingFactor;
    
    // 节奏/能量爆发检测 (Beat Detection)
    // 当瞬间能量显著高于近期平均能量时，视为一个“节拍”
    if (rms > smooth_energy_ * 1.5f + 0.01f) {
        uint32_t now = esp_timer_get_time() / 1000;
        // 限制触发频率，避免高频噪声误触发
        if (now - last_beat_time_ > 250) { 
            last_beat_time_ = now;
            if (beat_callback_) {
                beat_callback_(rms);
            }
        }
    }

    if (energy_callback_) {
        energy_callback_(smooth_energy_);
    }
}

float MusicAnalyzer::GetEnergy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return smooth_energy_;
}

void MusicAnalyzer::OnBeat(std::function<void(float)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    beat_callback_ = callback;
}

void MusicAnalyzer::OnEnergy(std::function<void(float)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    energy_callback_ = callback;
}
