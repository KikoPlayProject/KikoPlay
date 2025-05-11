#include "vad.h"


void VadIterator::init_engine_threads(int inter_threads, int intra_threads)
{
    // The method should be called in each thread/proc in multi-thread/proc work
    session_options.SetIntraOpNumThreads(intra_threads);
    session_options.SetInterOpNumThreads(inter_threads);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}

void VadIterator::init_onnx_model(const QString &model_path)
{
    // Init threads = 1 for
    init_engine_threads(1, 1);
    // Load model
    session = std::make_shared<Ort::Session>(env, model_path.toStdWString().c_str(), session_options);
}

void VadIterator::reset_states()
{
    // Call reset before each audio start
    std::memset(_state.data(), 0.0f, _state.size() * sizeof(float));
    triggered = false;
    temp_end = 0;
    current_sample = 0;

    prev_end = next_start = 0;

    speeches.clear();
    current_speech = timestamp_t();
    QList<float> tmp(64, 0);
    context.swap(tmp);
}

void VadIterator::predict(const QList<float> &data)
{
    // Infer
    // Create ort tensors
    input.assign(data.begin(), data.end());
    Ort::Value input_ort = Ort::Value::CreateTensor<float>(
        memory_info, input.data(), input.size(), input_node_dims, 2);
    Ort::Value state_ort = Ort::Value::CreateTensor<float>(
        memory_info, _state.data(), _state.size(), state_node_dims, 3);
    Ort::Value sr_ort = Ort::Value::CreateTensor<int64_t>(
        memory_info, sr.data(), sr.size(), sr_node_dims, 1);

    // Clear and add inputs
    ort_inputs.clear();
    ort_inputs.emplace_back(std::move(input_ort));
    ort_inputs.emplace_back(std::move(state_ort));
    ort_inputs.emplace_back(std::move(sr_ort));

    // Infer
    ort_outputs = session->Run(
        Ort::RunOptions{nullptr},
        input_node_names.data(), ort_inputs.data(), ort_inputs.size(),
        output_node_names.data(), output_node_names.size());

    // Output probability & update h,c recursively
    float speech_prob = ort_outputs[0].GetTensorMutableData<float>()[0];
    float *stateN = ort_outputs[1].GetTensorMutableData<float>();
    std::memcpy(_state.data(), stateN, size_state * sizeof(float));

    // Push forward sample index
    current_sample += window_size_samples;

    // Reset temp_end when > threshold
    if ((speech_prob >= threshold))
    {
#ifdef __DEBUG_SPEECH_PROB___
        float speech = current_sample - window_size_samples; // minus window_size_samples to get precise start time point.
        printf("{    start: %.3f s (%.3f) %08d}\n", 1.0 * speech / sample_rate, speech_prob, current_sample- window_size_samples);
#endif //__DEBUG_SPEECH_PROB___
        if (temp_end != 0)
        {
            temp_end = 0;
            if (next_start < prev_end)
                next_start = current_sample - window_size_samples;
        }
        if (triggered == false)
        {
            triggered = true;

            current_speech.start = current_sample - window_size_samples;
        }
        return;
    }

    if ((triggered == true) && ((current_sample - current_speech.start) > max_speech_samples)) {
        if (prev_end > 0) {
            current_speech.end = prev_end;
            speeches.push_back(current_speech);
            current_speech = timestamp_t();

            // previously reached silence(< neg_thres) and is still not speech(< thres)
            if (next_start < prev_end)
                triggered = false;
            else{
                current_speech.start = next_start;
            }
            prev_end = 0;
            next_start = 0;
            temp_end = 0;

        } else {
            current_speech.end = current_sample;
            speeches.push_back(current_speech);
            current_speech = timestamp_t();
            prev_end = 0;
            next_start = 0;
            temp_end = 0;
            triggered = false;
        }
        return;

    }
    if ((speech_prob >= (threshold - 0.15)) && (speech_prob < threshold))
    {
        if (triggered) {
#ifdef __DEBUG_SPEECH_PROB___
            float speech = current_sample - window_size_samples; // minus window_size_samples to get precise start time point.
            printf("{ speeking: %.3f s (%.3f) %08d}\n", 1.0 * speech / sample_rate, speech_prob, current_sample - window_size_samples);
#endif //__DEBUG_SPEECH_PROB___
        }
        else {
#ifdef __DEBUG_SPEECH_PROB___
            float speech = current_sample - window_size_samples; // minus window_size_samples to get precise start time point.
            printf("{  silence: %.3f s (%.3f) %08d}\n", 1.0 * speech / sample_rate, speech_prob, current_sample - window_size_samples);
#endif //__DEBUG_SPEECH_PROB___
        }
        return;
    }


    // 4) End
    if ((speech_prob < (threshold - 0.15)))
    {
#ifdef __DEBUG_SPEECH_PROB___
        float speech = current_sample - window_size_samples - speech_pad_samples; // minus window_size_samples to get precise start time point.
        printf("{      end: %.3f s (%.3f) %08d}\n", 1.0 * speech / sample_rate, speech_prob, current_sample - window_size_samples);
#endif //__DEBUG_SPEECH_PROB___
        if (triggered == true)
        {
            if (temp_end == 0)
            {
                temp_end = current_sample;
            }
            if (current_sample - temp_end > min_silence_samples_at_max_speech)
                prev_end = temp_end;
            // a. silence < min_slience_samples, continue speaking
            if ((current_sample - temp_end) < min_silence_samples)
            {

            }
            // b. silence >= min_slience_samples, end speaking
            else
            {
                current_speech.end = temp_end;
                if (current_speech.end - current_speech.start > min_speech_samples)
                {
                    speeches.push_back(current_speech);
                    current_speech = timestamp_t();
                    prev_end = 0;
                    next_start = 0;
                    temp_end = 0;
                    triggered = false;
                }
                else
                {
                    current_speech = timestamp_t();
                    prev_end = 0;
                    next_start = 0;
                    temp_end = 0;
                    triggered = false;
                }
            }
        }
        else {
            // may first windows see end state.
        }
        return;
    }
}

void VadIterator::process(const QList<float> &input_wav, bool *cancelFlag)
{
    reset_states();

    audio_length_samples = input_wav.size();

    for (int j = 0; j < audio_length_samples; j += window_size_samples)
    {
        if (j + window_size_samples > audio_length_samples)
            break;
        QList<float> r{ &input_wav[0] + j, &input_wav[0] + j + window_size_samples };
        std::for_each(r.begin(), r.end(), [](float& v) {v /= 32768; });
        QList<float> r_with_context = context;
        std::copy(r.begin(), r.end(), std::back_inserter(r_with_context));
        predict(r_with_context);
        std::copy(r.end() - context.size(), r.end(), context.begin());

        if (cancelFlag && *cancelFlag) return;
    }

    if (current_speech.start >= 0) {
        current_speech.end = audio_length_samples;
        speeches.push_back(current_speech);
        current_speech = timestamp_t();
        prev_end = 0;
        next_start = 0;
        temp_end = 0;
        triggered = false;
    }
}

void VadIterator::process(const QList<float> &input_wav, QList<float> &output_wav, bool *cancelFlag)
{
    process(input_wav, cancelFlag);
    if (cancelFlag && *cancelFlag) return;
    collect_chunks(input_wav, output_wav);
}

void VadIterator::collect_chunks(const QList<float> &input_wav, QList<float> &output_wav)
{
    output_wav.clear();
    for (int i = 0; i < speeches.size(); i++) {
        int start = std::min<int>(speeches[i].start, input_wav.size() - 1);
        int end = std::min<int>(speeches[i].end, input_wav.size() - 1);
        if (start >= end) continue;
        QList<float> slice(&input_wav[start], &input_wav[end]);
        output_wav += slice;
    }
}

void VadIterator::drop_chunks(const QList<float> &input_wav, QList<float> &output_wav)
{
    output_wav.clear();
    int current_start = 0;
    for (int i = 0; i < speeches.size(); i++) {
        QList<float> slice(&input_wav[current_start], &input_wav[speeches[i].start]);
        output_wav += slice;
        current_start = speeches[i].end;
    }

    QList<float> slice(&input_wav[current_start], &input_wav[input_wav.size()]);
    output_wav += slice;
}

VadIterator::VadIterator(const QString &model, int Sample_rate, int windows_frame_size,
                         float Threshold, int min_silence_duration_ms, int speech_pad_ms, int min_speech_duration_ms,
                         float max_speech_duration_s)
{
    init_onnx_model(model);
    threshold = Threshold;
    sample_rate = Sample_rate;
    sr_per_ms = sample_rate / 1000;

    window_size_samples = windows_frame_size * sr_per_ms;

    min_speech_samples = sr_per_ms * min_speech_duration_ms;
    speech_pad_samples = sr_per_ms * speech_pad_ms;

    max_speech_samples = (
        sample_rate * max_speech_duration_s
        - window_size_samples
        - 2 * speech_pad_samples
        );

    min_silence_samples = sr_per_ms * min_silence_duration_ms;
    min_silence_samples_at_max_speech = sr_per_ms * 98;

    QList<float> tmp(64, 0);
    context.swap(tmp);

    input.resize(window_size_samples);
    input_node_dims[0] = 1;
    input_node_dims[1] = window_size_samples + context.size();

    _state.resize(size_state);
    sr.resize(1);
    sr[0] = sample_rate;
}
