#include "sc_faust.h"

#include "library.h"

#include <fstream>
#include <iostream>

#ifndef FAUST_LIBRARY_PATH
error "FAUST_LIBRARY_PATH must be defined by CMake"
#endif

    InterfaceTable* ft;

extern Library::CodeLibrary* gLibrary;

ScFaust::ScFaust() {
    auto hash = static_cast<int>(in0(indices::hash));
    mNumInputs = static_cast<int>(in0(indices::numInputs));
    mNumParams = static_cast<int>(in0(indices::numParams));

    set_calc_function<ScFaust, &ScFaust::next>();

    auto node = Library::findEntry(hash);
    if (node == nullptr) {
        Print("ERROR: Could not find faust factory with hash %d\n", hash);
        return;
    }
    // we could handle too few outputs, but excessive outputs of a faust script
    // would result in a buffer overflow which can lead to a crash.
    // At some point we may support this, but for now we simply bail out.
    if (mNumOutputs != node->numOutputs) {
        Print("ERROR: Requested %d output channels, but Faust script only outputs %d channels\n", mNumOutputs,
              node->numOutputs);
        return;
    }

    mScRtUi = new SCRTUI(mWorld, node->numParams);
    mDsp = node->factory->createDSPInstance();
    mDsp->init(static_cast<int>(mWorld->mSampleRate));
    mDsp->buildUserInterface(mScRtUi);
}

ScFaust::~ScFaust() {
    delete mScRtUi;
    delete mDsp;
}


void ScFaust::next(int numSamples) {
    if (mDsp != nullptr) {
        for (int i = 0; i < mNumParams; i++) {
            auto paramOffset = **(mInBuf + indices::inputs + mNumInputs + (i * 2));
            auto param = mScRtUi->getParam(paramOffset);
            *param = **(mInBuf + indices::inputs + mNumInputs + (i * 2) + 1);
        };
        mDsp->compute(numSamples, mInBuf + indices::inputs, mOutBuf);
    }
}

PluginLoad("ScFaust") {
    ft = inTable;

    registerUnit<ScFaust>(inTable, "Faust", false);

    ft->fDefinePlugInCmd("faustscript", Library::faustCompileScript, nullptr);
}
