#include "library.h"

#include <fstream>
#include <iostream>


using namespace Library;

CodeLibrary* gLibrary = nullptr;
FaustMemoryManager* gFaustMemoryManager = nullptr;

/*! @brief writes params to file, separated by $ */
bool writeParamsToFile(const std::vector<ParamPair>& params, const std::string& filename) {
    std::ofstream outFile(filename);

    if (!outFile) {
        Print("Error opening file %s\n", filename.c_str());
        return false;
    }

    for (const auto& [param, _] : params) {
        outFile << param << "$";
    }
    outFile.close();
    // Print("Writing to file %s\n", filename.c_str());
    return true;
}

/*! @brief Runs in stage 2 (NRT) and compiles the DSP code */
bool compileScript(World*, void* cmdData) {
    auto payload = static_cast<CompileCodeCallbackPayload*>(cmdData);

    auto errorMessage = std::string();
    const auto target = std::string("");
    const char* argv[2] = { "-I", FAUST_LIBRARY_PATH };
    payload->factory = createDSPFactoryFromString("sc_faust", payload->code, 2, argv, target, errorMessage, -1);
    payload->factory->setMemoryManager(gFaustMemoryManager);

    if (!errorMessage.empty()) {
        Print("ERROR: %s\n", errorMessage.c_str());
        // do not continue to stage 3
        return true;
    }
    // Print("Compiled %d successfully\n", payload->hash);

    // create an instance so we can obtain the parameters
    payload->dspInstance = payload->factory->createDSPInstance();
    payload->scUi = new SCUI();
    payload->dspInstance->buildUserInterface(payload->scUi);
    payload->numOutputs = payload->dspInstance->getNumOutputs();

    // Print("Faust script has %d params\n", payload->scUi->getNumParams());
    writeParamsToFile(payload->scUi->getParams(), payload->paramExchangePath);

    return true;
};

/*! @brief runs in stage 3 (RT). Insert the factory into our library */
bool swapCode(World* world, void* cmdData) {
    auto payload = static_cast<Library::CompileCodeCallbackPayload*>(cmdData);

    // in case stage 2 failed we do not swap
    if (payload->factory == nullptr) {
        return true;
    }

    auto* newNode = static_cast<Library::CodeLibrary*>(RTAlloc(world, sizeof(CodeLibrary)));
    if (!newNode) {
        Print("ERROR: RTAlloc failed to add item to the code library\n");
        return false;
    }
    newNode->hash = payload->hash;
    newNode->factory = payload->factory;
    newNode->next = gLibrary;
    newNode->numOutputs = payload->numOutputs;

    gLibrary = newNode;

    return true;
};

namespace Library {
void faustCompileScript(World* world, void*, sc_msg_iter* args, void* replyAddr) {
    // set the world for the faust memory manager once
    if (gFaustMemoryManager == nullptr) {
        gFaustMemoryManager = new FaustMemoryManager();
        gFaustMemoryManager->world = world;
    }
    auto payload = static_cast<CompileCodeCallbackPayload*>(RTAlloc(world, sizeof(CompileCodeCallbackPayload)));
    if (!payload) {
        Print("ERROR: Failed to allocate memory for compile code callback.\n");
        return;
    }
    // init payload such that we do nat have dangling pointers
    payload->dspInstance = nullptr;
    payload->factory = nullptr;
    payload->scUi = nullptr;
    payload->code = nullptr;
    payload->paramExchangePath = nullptr;
    payload->numOutputs = 0;
    payload->hash = args->geti();

    const char* exchangePath = args->gets();
    auto exchangeLength = strlen(exchangePath) + 1;
    payload->paramExchangePath = static_cast<char*>(RTAlloc(world, sizeof(char) * exchangeLength));
    if (!payload->paramExchangePath) {
        Print("ERROR: Failed to allocate memory for exchange path.\n");
        RTFree(world, payload);
        return;
    }
    std::copy_n(exchangePath, exchangeLength, payload->paramExchangePath);

    const char* code = args->gets();
    auto codeLength = strlen(code) + 1;
    payload->code = static_cast<char*>(RTAlloc(world, codeLength));
    if (!payload->code) {
        Print("ERROR: Failed to allocate memory for compile code.\n");
        RTFree(world, payload->paramExchangePath);
        RTFree(world, payload);
        return;
    }
    // copy such that we can take ownership of our own copy of the code
    std::copy_n(code, codeLength, payload->code);

    payload->sampleRate = static_cast<int>(world->mSampleRate);

    // @todo construct /done message based on given hash
    // faustgen(8) + 10 chars for 24bit int as string
    auto* cmdName = static_cast<char*>(RTAlloc(world, sizeof(char) * 18));
    if (!cmdName) {
        Print("ERROR: Failed to allocate memory for script command.\n");
        RTFree(world, payload->code);
        RTFree(world, payload->paramExchangePath);
        RTFree(world, payload);
    }
    snprintf(cmdName, 18, "faust%d", payload->hash);

    ft->fDoAsynchronousCommand(
        world, replyAddr,
        // @todo does cmdName leak? will it get freed by the server on its own?
        cmdName, static_cast<void*>(payload), compileScript, swapCode,
        [](World* world, void* cmdData) {
            auto payload = static_cast<CompileCodeCallbackPayload*>(cmdData);
            delete payload->scUi;
            delete payload->dspInstance;
            return true;
        },
        [](World* world, void* cmdData) {
            auto payload = static_cast<Library::CompileCodeCallbackPayload*>(cmdData);
            RTFree(world, payload->code);
            RTFree(world, payload->paramExchangePath);
            RTFree(world, payload);
        },
        0, nullptr);
}

CodeLibrary* findEntry(const int hash) {
    for (auto node = gLibrary; node != nullptr; node = node->next) {
        if (node->hash == hash) {
            return node;
        }
    }
    return nullptr;
}
}