#ifndef VAD_H
#define VAD_H
#include "onnxruntime_cxx_api.h"
#include <QString>
#include <QList>

class timestamp_t
{
public:
    int start;
    int end;

    // default + parameterized constructor
    timestamp_t(int start = -1, int end = -1)
        : start(start), end(end)
        {
        };

    // assignment operator modifies object, therefore non-const
    timestamp_t& operator=(const timestamp_t& a)
    {
        start = a.start;
        end = a.end;
        return *this;
    };

    // equality comparison. doesn't modify object. therefore const.
    bool operator==(const timestamp_t& a) const
    {
        return (start == a.start && end == a.end);
    };
};

class VadIterator
{
public:
    // Construction
    VadIterator(const QString &model,
                int Sample_rate = 16000, int windows_frame_size = 32,
                float Threshold = 0.5, int min_silence_duration_ms = 0,
                int speech_pad_ms = 30, int min_speech_duration_ms = 32,
                float max_speech_duration_s = std::numeric_limits<float>::infinity());

public:
    void process(const QList<float>& input_wav, bool *cancelFlag = nullptr);
    void process(const QList<float>& input_wav, QList<float>& output_wav, bool *cancelFlag = nullptr);
    void collect_chunks(const QList<float>& input_wav, QList<float>& output_wav);
    void drop_chunks(const QList<float>& input_wav, QList<float>& output_wav);
    const QList<timestamp_t> &get_speech_timestamps() const { return speeches; }

private:
    // OnnxRuntime resources
    Ort::Env env;
    Ort::SessionOptions session_options;
    std::shared_ptr<Ort::Session> session = nullptr;
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU);

private:
    void init_engine_threads(int inter_threads, int intra_threads);
    void init_onnx_model(const QString &model_path);
    void reset_states();
    void predict(const QList<float> &data);

private:
    // model config
    int64_t window_size_samples;  // Assign when init, support 256 512 768 for 8k; 512 1024 1536 for 16k.
    int sample_rate;  //Assign when init support 16000 or 8000
    int sr_per_ms;   // Assign when init, support 8 or 16
    float threshold;
    int min_silence_samples; // sr_per_ms * #ms
    int min_silence_samples_at_max_speech; // sr_per_ms * #98
    int min_speech_samples; // sr_per_ms * #ms
    float max_speech_samples;
    int speech_pad_samples; // usually a
    int audio_length_samples;

    // model states
    bool triggered = false;
    unsigned int temp_end = 0;
    unsigned int current_sample = 0;
    // MAX 4294967295 samples / 8sample per ms / 1000 / 60 = 8947 minutes
    int prev_end;
    int next_start = 0;

    //Output timestamp
    QList<timestamp_t> speeches;
    timestamp_t current_speech;

    // Onnx model
    // Inputs
    std::vector<Ort::Value> ort_inputs;

    std::vector<const char *> input_node_names = {"input", "state", "sr"};
    QList<float> input;
    QList<float> context;
    unsigned int size_state = 2 * 1 * 128; // It's FIXED.
    std::vector<float> _state;
    std::vector<int64_t> sr;

    int64_t input_node_dims[2] = {};
    const int64_t state_node_dims[3] = {2, 1, 128};
    const int64_t sr_node_dims[1] = {1};

    // Outputs
    std::vector<Ort::Value> ort_outputs;
    std::vector<const char *> output_node_names = {"output", "stateN"};

};

#endif // VAD_H
