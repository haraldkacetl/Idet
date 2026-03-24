#include "LlamaClient.hpp"

#include <thread>
#include <fstream>

LlamaClient::LlamaClient(LogCallback logger) : debug_logger(logger) {
    llama_backend_init();

    // hook llama.cpp logs into your logger
    llama_log_set([](ggml_log_level, const char* text, void* user_data) {
        auto* cb = static_cast<LogCallback*>(user_data);
        if (cb && *cb) {
            (*cb)(text);
        }
    }, &debug_logger);

    log("Backend initialized");
}

LlamaClient::~LlamaClient() {
    llama_backend_free();
}

void LlamaClient::log(const std::string& msg) {
    if (debug_logger) debug_logger("[LlamaClient] " + msg);
}

bool LlamaClient::load_model(const std::string& model_path, int n_ctx) {
    last_error.clear();

    if (model_path.empty()) {
        last_error = "Model path is empty";
        log(last_error);
        return false;
    }

    std::ifstream f(model_path);
    if (!f.good()) {
        last_error = "Model file does not exist: " + model_path;
        log(last_error);
        return false;
    }

    log("Loading model: " + model_path);

    llama_model_params m_params = llama_model_default_params();
    m_params.n_gpu_layers = 0;

    model.reset(llama_model_load_from_file(model_path.c_str(), m_params));

    if (!model) {
        last_error = "Failed to load model (check GGUF format)";
        log(last_error);
        return false;
    }

    llama_context_params c_params = llama_context_default_params();
    c_params.n_ctx = n_ctx;
    c_params.n_threads = std::thread::hardware_concurrency();

    ctx.reset(llama_init_from_model(model.get(), c_params));

    if (!ctx) {
        last_error = "Failed to create context";
        log(last_error);
        model.reset();
        return false;
    }

    log("Model loaded successfully");
    return true;
}

std::vector<llama_token> LlamaClient::tokenize(const std::string& text, bool add_bos) {
    const llama_vocab* vocab = llama_model_get_vocab(model.get());

    int n_tokens = -llama_tokenize(
        vocab,
        text.c_str(),
        text.length(),
        nullptr,
        0,
        add_bos,
        false
    );

    std::vector<llama_token> tokens(n_tokens);

    llama_tokenize(
        vocab,
        text.c_str(),
        text.length(),
        tokens.data(),
        tokens.size(),
        add_bos,
        false
    );

    return tokens;
}

std::string LlamaClient::token_to_piece(llama_token token) {
    const llama_vocab* vocab = llama_model_get_vocab(model.get());

    std::vector<char> buf(32);
    int n = llama_token_to_piece(vocab, token, buf.data(), buf.size(), 0, false);

    if (n < 0) {
        buf.resize(-n);
        llama_token_to_piece(vocab, token, buf.data(), buf.size(), 0, false);
    } else {
        buf.resize(n);
    }

    return std::string(buf.begin(), buf.end());
}

std::string LlamaClient::complete_text(const std::string& prompt, const GenerationConfig& config) {
    if (!is_ready()) return "";

    auto tokens = tokenize(prompt, true);

    // Minimal placeholder (safe, won’t crash)
    return "Prompt tokens: " + std::to_string(tokens.size());
}