#include "sc_faust.h"

#include "library.h"

#include <fstream>
#include <iostream>

InterfaceTable* ft;

extern Library::CodeLibrary* gLibrary;

ScFaust::ScFaust() {
    auto hash = static_cast<int>(in0(indices::hash));
    mNumFaustInputs = static_cast<int>(in0(indices::numInputs));
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

    void* mScRtUiLocation = RTAlloc(mWorld, sizeof(SCRTUI));
    if (mScRtUiLocation == nullptr) {
        Print("ERROR: Could not allocate memory for SCRTUI UI location\n");
        return;
    }
    mScRtUi = new (mScRtUiLocation) SCRTUI(mWorld, node->numParams);
    if (!mScRtUi->mSuccess) {
        Print("ERROR: Could not allocate memory for SCRTUI parameters\n");
        return;
    }
    mDspFactory = node->dspFactory;
    mDsp = mDspFactory->factory->createDSPInstance();
    mDsp->init(static_cast<int>(mWorld->mSampleRate));
    mDsp->buildUserInterface(mScRtUi);
    mDspFactory->instanceCount += 1;

    mParamOffsets = static_cast<int*>(RTAlloc(mWorld, sizeof(int) * mNumParams));
    if (mParamOffsets == nullptr) {
        Print("ERROR: Could not allocate memory for param offsets\n");
        return;
    }
    for (int i = 0; i < mNumParams; i++) {
        mParamOffsets[i] = static_cast<int>(in0(indices::inputs + mNumFaustInputs + (2 * i)));
    }
    mReady = true;
}

ScFaust::~ScFaust() {
    if (mScRtUi != nullptr) {
        mScRtUi->~SCRTUI();
    }
    RTFree(mWorld, mParamOffsets);
    RTFree(mWorld, mScRtUi);
    delete mDsp;
    if (mDspFactory != nullptr) {
        mDspFactory->instanceCount -= 1;
        if (mDspFactory->instanceCount == 0 && mDspFactory->shouldDelete) {
            Library::deleteDspFactory(mWorld, mDspFactory);
        }
    }
}


void ScFaust::next(int numSamples) {
    if (mDsp != nullptr && mReady) {
        for (int i = 0; i < mNumParams; i++) {
            auto param = mScRtUi->getParam(mParamOffsets[i]);
            *param = **(mInBuf + indices::inputs + mNumFaustInputs + (2 * i) + 1);
        };
        mDsp->compute(numSamples, mInBuf + indices::inputs, mOutBuf);
    } else {
        ClearUnitOutputs(this, mNumOutputs);
    }
}

PluginLoad("ScFaust") {
    ft = inTable;

    registerUnit<ScFaust>(inTable, "Faust", false);

    ft->fDefinePlugInCmd("faustscript", Library::faustCompileScript, nullptr);
    ft->fDefinePlugInCmd("faustlibpath", Library::setFaustLibPath, nullptr);
    ft->fDefinePlugInCmd("faustfree", Library::freeNodeCallback, nullptr);
    ft->fDefinePlugInCmd("faustfreeall", Library::freeAllCallback, nullptr);
}
