#ifndef MUSIC_ANALYZER_H
#define MUSIC_ANALYZER_H

#include <vector>
#include <cstdint>
#include <mutex>
#include <functional>

/**
 * MusicAnalyzer - 音频能量分析器
 * 
 * 用于计算实时音频输入的能量 (RMS)，识别节奏感，并触发视觉反馈。
 */
class MusicAnalyzer {
public:
    static MusicAnalyzer& GetInstance() {
        static MusicAnalyzer instance;
        return instance;
    }

    void Feed(const std::vector<int16_t>& data);
    
    // 获取当前平滑后的能量值 (0.0 - 1.0)
    float GetEnergy() const;

    // 注册能量变化回调 (当检测到明显节奏/能量爆发时)
    void OnBeat(std::function<void(float strength)> callback);

    // 注册实时能量变化回调
    void OnEnergy(std::function<void(float energy)> callback);

private:
    MusicAnalyzer();
    ~MusicAnalyzer();

    float energy_ = 0.0f;
    float smooth_energy_ = 0.0f;
    uint32_t last_beat_time_ = 0;
    
    std::function<void(float)> beat_callback_;
    std::function<void(float)> energy_callback_;
    mutable std::mutex mutex_;

    // 平滑系数 (0.0 - 1.0, 越小越平滑)
    const float kSmoothingFactor = 0.15f;
    const float kBeatThreshold = 0.05f;
};

#endif // MUSIC_ANALYZER_H
