#include <resources/default_resource.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <fstream>

#include "app.h"

using json = nlohmann::json;

using namespace vecgui;

// 1. 定义与 JSON 对应的字幕数据结构
struct SubtitleWord {
    std::string word;
    double start;
    double end;
};

struct SubtitlePhrase {
    double start;
    double end;
    std::string targetText;
    std::vector<SubtitleWord> targetSrt;
};

// 2. 自定义字幕显示节点
class SubtitleNode : public Node {
public:
    void custom_ready() override {
        // 创建容器以居中显示字幕
        auto margin_container = std::make_shared<MarginContainer>();
        margin_container->set_margin_all(64);
        margin_container->set_anchor_flag(AnchorFlag::FullRect);
        add_child(margin_container);

        auto center_container = std::make_shared<Container>();
        margin_container->add_child(center_container);

        label = std::make_shared<Label>();
        label->set_font_size(64); // 设置一个较大的字号
        label->set_horizontal_alignment(Alignment::Center);
        label->set_vertical_alignment(Alignment::Center);
        center_container->add_child(label);

        // 尝试从文件加载，如果文件不存在则加载默认硬编码数据
        if (!load_from_file("subtitles.json")) {
            load_default_subtitles_data();
        }
    }

    bool load_from_file(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            return false;
        }

        try {
            json data = json::parse(f);
            subtitles.clear();
            for (auto& item : data) {
                SubtitlePhrase phrase;
                phrase.start = item["start"];
                phrase.end = item["end"];
                phrase.targetText = item["targetText"];
                for (auto& s : item["targetSrt"]) {
                    phrase.targetSrt.push_back({s["word"], s["start"], s["end"]});
                }
                subtitles.push_back(phrase);
            }
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }

    void custom_update(double dt) override {
        // 模拟时间线进度
        current_time += dt;

        // 简单的循环播放演示
        if (!subtitles.empty() && current_time > subtitles.back().end + 1.0) {
            current_time = 0;
            last_phrase_idx = -1;
            last_word_idx = -1;
        }

        update_subtitles_display();
    }

private:
    void update_subtitles_display() {
        // 查找当前时间点对应的“句子”
        int active_phrase_idx = -1;
        for (int i = 0; i < (int)subtitles.size(); ++i) {
            if (current_time >= subtitles[i].start && current_time <= subtitles[i].end) {
                active_phrase_idx = i;
                break;
            }
        }

        // 如果没有活动的句子，清除显示
        if (active_phrase_idx == -1) {
            if (last_phrase_idx != -1) {
                label->clear_spans();
                last_phrase_idx = -1;
                last_word_idx = -1;
            }
            return;
        }

        const auto& phrase = subtitles[active_phrase_idx];

        // 在当前句子中查找正在发音的“词”
        int active_word_idx = -1;
        for (int i = 0; i < (int)phrase.targetSrt.size(); ++i) {
            if (current_time >= phrase.targetSrt[i].start && current_time <= phrase.targetSrt[i].end) {
                active_word_idx = i;
                break;
            }
        }

        // 性能优化：只有在句子切换或高亮词切换时才重新构建富文本 Span
        if (active_phrase_idx != last_phrase_idx || active_word_idx != last_word_idx) {
            label->clear_spans();

            // 基础样式
            TextStyle normal_style;
            normal_style.color = ColorU::white();
            normal_style.shadow_color = ColorU::black();
            normal_style.shadow_radius = 8.0;

            // 高亮样式 (当前正在说的词)
            TextStyle highlight_style = normal_style;
            highlight_style.color = ColorU::yellow();
            highlight_style.bold = true;
            highlight_style.stroke_color = ColorU::black();
            highlight_style.stroke_width = 2.0f;

            // 根据 targetSrt 构建富文本 Span
            for (int i = 0; i < (int)phrase.targetSrt.size(); ++i) {
                const auto& word_data = phrase.targetSrt[i];
                if (i == active_word_idx) {
                    label->add_span({word_data.word, highlight_style});
                } else {
                    label->add_span({word_data.word, normal_style});
                }
            }

            last_phrase_idx = active_phrase_idx;
            last_word_idx = active_word_idx;
        }
    }

    void load_default_subtitles_data() {
        subtitles = {
            {0.034, 1.059, "大家好", {{"大家", 0.034, 0.717}, {"好", 0.717, 1.059}}},
            {1.1, 1.899, "认识 VecGui", {{"认识", 1.1, 1.26}, {" ", 1.26, 1.34}, {"VecGui", 1.34, 1.899}}},
            {2.399, 4.219, "立即生成字幕", {{"立即", 2.399, 3.006}, {"生成", 3.006, 3.612}, {"字幕", 3.612, 4.219}}},
            {4.259,
             6.44,
             "并将它们翻译成多种语言",
             {{"并", 4.259, 4.457},
              {"将", 4.457, 4.656},
              {"它们", 4.656, 5.052},
              {"翻译成", 5.052, 5.647},
              {"多种语言", 5.647, 6.44}}},
            {6.48, 7.499, "只需一键", {{"只", 6.48, 6.735}, {"需", 6.735, 6.989}, {"一键", 6.989, 7.499}}}};
    }

    std::shared_ptr<Label> label;
    std::vector<SubtitlePhrase> subtitles;
    double current_time = 0;
    int last_phrase_idx = -1;
    int last_word_idx = -1;
};

int main() {
    App app({1280, 720}, true);

    // 添加字幕显示节点到场景树
    app.get_tree_root()->add_child(std::make_shared<SubtitleNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
