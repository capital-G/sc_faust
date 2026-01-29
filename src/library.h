#pragma once
#include <SC_PlugIn.hpp>
#include <faust/dsp/llvm-dsp.h>

#include "sc_ui.h"

extern InterfaceTable* ft;

namespace Library {
/*! @brief a linked list of available DSP factories */
struct CodeLibrary {
    CodeLibrary* next;
    int hash;
    llvm_dsp_factory* factory;
    int numParams;
    int numOutputs;
};

/*! @brief payload for async callback to compile faust scripts */
struct CompileCodeCallbackPayload {
    int hash;
    char* code;
    int sampleRate;
    char* paramExchangePath;

    // gets created in stage 2 and moved in stage 3
    // to the CodeLibrary.
    llvm_dsp_factory* factory;
    // necessary to create such we can create a mapping of
    // paramName -> position for the client via an OSC message
    // which will get created in stage 3.
    // This gets filled in stage 2 which is NRT, and freed again in stage 4.
    llvm_dsp* dspInstance;
    SCUI* scUi;
    int numOutputs;
    int numParams;
};

/*! @brief takes an OSC message and compiles triggers an async compilation of the faust code */
void faustCompileScript(World* world, void* inUserData, sc_msg_iter* args, void* replyAddr);

/*! @brief looks up the entry within the global code library. If the entry does not exist,
 *  it will return a nullptr.
 */
CodeLibrary* findEntry(int hash);
}

/*! @brief faust memory manager which uses SC's RTalloc */
struct FaustMemoryManager : public dsp_memory_manager {
    World* world;

    void* allocate(size_t size) override { return RTAlloc(world, size); }

    void destroy(void* ptr) override { RTFree(world, ptr); }
};
