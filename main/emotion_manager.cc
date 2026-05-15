#include "emotion_manager.h"
#include <esp_log.h>
#include <algorithm>

#define TAG "EmotionManager"

EmotionManager::EmotionManager() {}

EmotionManager::~EmotionManager() {
    if (decay_timer_handle_ != nullptr) {
        esp_timer_stop(decay_timer_handle_);
        esp_timer_delete(decay_timer_handle_);
    }
}

void EmotionManager::Initialize() {
    esp_timer_create_args_t timer_args = {
        .callback = &EmotionManager::TimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "emotion_decay_timer"
    };
    esp_timer_create(&timer_args, &decay_timer_handle_);
    
    // 每 1 分钟执行一次衰减 (60 * 1000,000 微秒)
    esp_timer_start_periodic(decay_timer_handle_, 60 * 1000000ULL);
    
    ESP_LOGI(TAG, "EmotionManager initialized with Mood=%.1f, Satiety=%.1f, Energy=%.1f", mood_, satiety_, energy_);
}

void EmotionManager::TimerCallback(void* arg) {
    auto manager = static_cast<EmotionManager*>(arg);
    manager->DecayValues();
}

void EmotionManager::DecayValues() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 基础衰减逻辑 (每分钟)
    mood_ = std::max(0.0f, mood_ - 0.1f);      // 约 6.0 / 小时
    satiety_ = std::max(0.0f, satiety_ - 0.2f);   // 约 12.0 / 小时
    energy_ = std::max(0.0f, energy_ - 0.15f);    // 约 9.0 / 小时

    // 状态互锁：饥饿或疲劳会加速心情变差
    if (satiety_ < 20.0f || energy_ < 20.0f) {
        mood_ = std::max(0.0f, mood_ - 0.2f);
    }

    ESP_LOGD(TAG, "Values decayed: Mood=%.1f, Satiety=%.1f, Energy=%.1f", mood_, satiety_, energy_);
}

float EmotionManager::GetMood() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mood_;
}

float EmotionManager::GetSatiety() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return satiety_;
}

float EmotionManager::GetEnergy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return energy_;
}

void EmotionManager::Feed(float amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    satiety_ = std::min(100.0f, satiety_ + amount);
    // 喂食通常也会提升一点心情
    mood_ = std::min(100.0f, mood_ + amount * 0.2f);
    ESP_LOGI(TAG, "Feed: Satiety=%.1f, Mood=%.1f", satiety_, mood_);
}

void EmotionManager::Play(float amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    mood_ = std::min(100.0f, mood_ + amount);
    energy_ = std::max(0.0f, energy_ - amount * 0.5f); // 玩耍消耗精力
    ESP_LOGI(TAG, "Play: Mood=%.1f, Energy=%.1f", mood_, energy_);
}

void EmotionManager::Rest(float amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    energy_ = std::min(100.0f, energy_ + amount);
    ESP_LOGI(TAG, "Rest: Energy=%.1f", energy_);
}

std::string EmotionManager::GetCurrentEmotion() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 优先级判断情绪状态映射
    if (energy_ < 20.0f) return "sleepy";
    if (satiety_ < 30.0f) return "hungry";
    if (mood_ < 40.0f) return "sad";
    if (mood_ > 80.0f) return "happy";
    
    return "neutral";
}
