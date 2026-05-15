#ifndef EMOTION_MANAGER_H
#define EMOTION_MANAGER_H

#include <esp_timer.h>
#include <mutex>
#include <string>

/**
 * EmotionManager - 桌面宠物的核心情绪与状态引擎
 * 
 * 负责维护宠物的心情 (Mood)、饱食度 (Satiety) 和精力 (Energy) 指标。
 * 提供指标衰减、恢复逻辑以及基于指标的情绪表现状态。
 */
class EmotionManager {
public:
    static EmotionManager& GetInstance() {
        static EmotionManager instance;
        return instance;
    }

    void Initialize();
    
    // 获取当前状态
    float GetMood() const;
    float GetSatiety() const;
    float GetEnergy() const;

    // 交互行为
    void Feed(float amount);    // 喂食，增加饱食度
    void Play(float amount);    // 玩耍，增加心情，消耗精力
    void Rest(float amount);    // 休息，增加精力

    // 获取当前的情绪字符串 (用于 UI 动画匹配)
    std::string GetCurrentEmotion() const;

private:
    EmotionManager();
    ~EmotionManager();

    // 禁用拷贝
    EmotionManager(const EmotionManager&) = delete;
    EmotionManager& operator=(const EmotionManager&) = delete;

    void DecayValues();
    static void TimerCallback(void* arg);

    float mood_ = 100.0f;     // 0-100
    float satiety_ = 100.0f;  // 0-100
    float energy_ = 100.0f;   // 0-100

    mutable std::mutex mutex_;
    esp_timer_handle_t decay_timer_handle_ = nullptr;
};

#endif // EMOTION_MANAGER_H
