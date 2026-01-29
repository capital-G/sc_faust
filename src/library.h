#pragma once
#include <SC_PlugIn.hpp>
#include <faust/dsp/llvm-dsp.h>

#include "sc_ui.h"

extern InterfaceTable* ft;

namespace Library {

/*! @brief a ref counter wrapper for llvm_dsp_factory */
struct DSPFactory {
    llvm_dsp_factory* factory = nullptr;
    // indicates if this DSP fatory is ready to be deleted
    bool shouldDelete = false;
    // number of instances using the factory. only delete
    // the dsp factory if this count is 0!
    uint instanceCount = 0;
};

/*! @brief a linked list of available DSP factories */
struct CodeLibrary {
    CodeLibrary* next;
    int hash;
    // this is wrapped as a pointer such that we can delete the code library
    // while there are still units using the dsp factory.
    DSPFactory* dspFactory;
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

/*! @brief sets gFaustLibPath which is the location of the faust library.
 *  This is NOT RT safe but this should not be expected to be set regularly and while the sounds run.
 */
void setFaustLibPath(World* world, void* inUserData, sc_msg_iter* args, void* replyAddr);

/*! @brief removes a faust factory from the server. This may be deferred
 *  if there are still running instances. */
void freeNodeCallback(World* world, void* inUserData, sc_msg_iter* args, void* replyAddr);

/*! @brief remove all faust factories from the server by using `freeNodeCallback` */
void freeAllCallback(World* world, void* inUserData, sc_msg_iter* args, void* replyAddr);

/*! @brief deletes a DSP factory in the NRT thread. Call this only in RT context! */
void deleteDspFactory(World* world, DSPFactory* factory);

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
