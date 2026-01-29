#pragma once

#include "library.h"


#include <SC_PlugIn.hpp>

#include <faust/dsp/llvm-dsp.h>

#include "sc_ui.h"

class ScFaust;

/*! @class ScFaust
 *  @brief Unit generator for evaluating faust dsp
 */
class ScFaust : public SCUnit {
public:
    ScFaust();
    ~ScFaust();

private:
    enum indices {
        hash = 0,
        numInputs = 1,
        numParams = 2,
        inputs = 3,
    };
    void next(int numSamples);

    llvm_dsp* mDsp = nullptr;
    SCRTUI* mScRtUi = nullptr;
    int mNumFaustInputs;
    int mNumParams;
    int* mParamOffsets = nullptr;
    bool mReady = false;
    Library::DSPFactory* mDspFactory = nullptr;
};
