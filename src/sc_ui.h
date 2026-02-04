#pragma once

#define FAUSTFLOAT float

#include <faust/gui/UI.h>
#include <SC_PlugIn.hpp>

extern InterfaceTable* ft;

struct FaustParam {
    std::string name;
    float initValue;
    float minValue;
    float maxValue;
    float stepValue;
};

inline FaustParam createFaustParam(const char* name, const float initValue, const float minValue, const float maxValue,
                                   const float stepValue) {
    FaustParam param;
    param.name = name;
    param.initValue = initValue;
    param.minValue = minValue;
    param.maxValue = maxValue;
    param.stepValue = stepValue;
    return param;
}


/*! @class SCUI
 *  @brief A wrapper between Faust UI and SuperCollider inputs
 */
class SCUI : public UI {
protected:
    std::vector<FaustParam> params;

public:
    size_t getNumParams() { return params.size(); }
    const std::vector<FaustParam>& getParams() { return params; }

    // UI callbacks
    void openTabBox(const char* label) override {}
    void openHorizontalBox(const char* label) override {};
    void openVerticalBox(const char* label) override {};
    void closeBox() override {};
    void addButton(const char* label, float* zone) override {
        auto param = createFaustParam(label, 0.0, 0.0, 1.0, 1.0);
        params.emplace_back(param);
    };
    void addCheckButton(const char* label, float* zone) override {
        auto param = createFaustParam(label, 0.0, 0.0, 1.0, 1.0);
        params.emplace_back(param);
    };
    void addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step) override {
        auto param = createFaustParam(label, init, min, max, step);
        params.emplace_back(param);
    };
    void addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step) override {
        auto param = createFaustParam(label, init, min, max, step);
        params.emplace_back(param);
    };
    void addNumEntry(const char* label, float* zone, float init, float min, float max, float step) override {
        auto param = createFaustParam(label, init, min, max, step);
        params.emplace_back(param);
    };
    void addHorizontalBargraph(const char* label, float* zone, float min, float max) override {};
    void addVerticalBargraph(const char* label, float* zone, float min, float max) override {};
    void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) override {
        Print("ERROR: Soundfile input is currently not supported - skipping parameter %s\n", label);
    };
};

/*! @class SCRTUI
 *  @brief Same as SCUI but does not allocate by redacting the name of the parameter
 *  and instead relying on the deterministic order obtained in the NRT version
 */
class SCRTUI : public UI {
protected:
    float** mParams = nullptr;
    World* mWorld = nullptr;

    int counter = 0;

    void add(float* zone) {
        mParams[counter] = zone;
        counter++;
    }

public:
    SCRTUI(World* world_, int numParams): mWorld(world_) {
        mParams = static_cast<float**>(RTAlloc(mWorld, sizeof(float*) * numParams));
        if (mParams == nullptr) {
            return;
        }
        mSuccess = true;
    }
    ~SCRTUI() { RTFree(mWorld, mParams); }

    bool mSuccess = false;
    float* getParam(const int num) { return mParams[num]; }
    // UI callbacks
    void openTabBox(const char* label) override {}
    void openHorizontalBox(const char* label) override {};
    void openVerticalBox(const char* label) override {};
    void closeBox() override {};
    void addButton(const char* label, float* zone) override { add(zone); };
    void addCheckButton(const char* label, float* zone) override { add(zone); };
    void addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step) override {
        add(zone);
    };
    void addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step) override {
        add(zone);
    };
    void addNumEntry(const char* label, float* zone, float init, float min, float max, float step) override {
        add(zone);
    };
    void addHorizontalBargraph(const char* label, float* zone, float min, float max) override { add(zone); };
    void addVerticalBargraph(const char* label, float* zone, float min, float max) override { add(zone); };
    void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) override {};
};