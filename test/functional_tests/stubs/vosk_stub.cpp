/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// A stand-in for libvosk, so the paths below VoskRecognizer's library guard can
// be reached by a test on a machine with no speech engine installed - which is
// every CI runner. It resolves the same symbols the real library exports and
// hands back opaque tokens rather than decoding anything.
//
// It exists for one property in particular: a real libvosk dereferences the
// recognizer handle it is given, so passing it a null one is a crash that a
// test cannot survive to report. This records the null instead, which is what
// lets a test assert that the ordering bug in VoskRecognizer::initialize()
// (#10759) stays fixed rather than inferring it from a stack trace.
//
// Loaded by path, never linked, so nothing here may depend on Qt or on Mudlet.

#include <cstring>
#include <string>

namespace {
// Recorded rather than asserted: the library has no way to fail a test, so it
// keeps the evidence and the test reads it back through voskStubNullHandleCalls().
int gNullHandleCalls = 0;
int gModelsAlive = 0;
int gRecognizersAlive = 0;
// Anything a caller might keep a pointer into has to outlive the call.
const char* const kEmptyResult = "{\"text\": \"\"}";

// The real library reads the model directory; this only has to answer whether a
// path was offered at all, so that a caller handing it nothing still gets null.
struct StubModel
{
    std::string path;
};

struct StubRecognizer
{
    StubModel* model = nullptr;
    float sampleRate = 0.0f;
    int wordsRequested = 0;
    int endpointerMode = 0;
};
} // namespace

extern "C" {

// --- the test's own window into what the stub saw -------------------------

// How many times a null recognizer handle reached one of the entry points that
// a real libvosk would have dereferenced.
int voskStubNullHandleCalls()
{
    return gNullHandleCalls;
}

void voskStubReset()
{
    gNullHandleCalls = 0;
    gModelsAlive = 0;
    gRecognizersAlive = 0;
}

// Both should be zero once a recognizer has been closed; a leak here is a leak
// in VoskRecognizer's own teardown.
int voskStubModelsAlive()
{
    return gModelsAlive;
}

int voskStubRecognizersAlive()
{
    return gRecognizersAlive;
}

// --- the libvosk surface VoskRecognizer resolves --------------------------

void* vosk_model_new(const char* path)
{
    if (!path || !*path) {
        return nullptr;
    }
    ++gModelsAlive;
    auto* model = new StubModel;
    model->path = path;
    return model;
}

void vosk_model_free(void* model)
{
    if (!model) {
        ++gNullHandleCalls;
        return;
    }
    --gModelsAlive;
    delete static_cast<StubModel*>(model);
}

void* vosk_recognizer_new(void* model, float sampleRate)
{
    if (!model) {
        ++gNullHandleCalls;
        return nullptr;
    }
    ++gRecognizersAlive;
    auto* recognizer = new StubRecognizer;
    recognizer->model = static_cast<StubModel*>(model);
    recognizer->sampleRate = sampleRate;
    return recognizer;
}

void vosk_recognizer_free(void* recognizer)
{
    if (!recognizer) {
        ++gNullHandleCalls;
        return;
    }
    --gRecognizersAlive;
    delete static_cast<StubRecognizer*>(recognizer);
}

int vosk_recognizer_accept_waveform(void* recognizer, const char*, int)
{
    if (!recognizer) {
        ++gNullHandleCalls;
        return 0;
    }
    return 0;
}

const char* vosk_recognizer_result(void* recognizer)
{
    if (!recognizer) {
        ++gNullHandleCalls;
    }
    return kEmptyResult;
}

const char* vosk_recognizer_partial_result(void* recognizer)
{
    if (!recognizer) {
        ++gNullHandleCalls;
    }
    return kEmptyResult;
}

const char* vosk_recognizer_final_result(void* recognizer)
{
    if (!recognizer) {
        ++gNullHandleCalls;
    }
    return kEmptyResult;
}

void vosk_recognizer_reset(void* recognizer)
{
    if (!recognizer) {
        ++gNullHandleCalls;
    }
}

void vosk_set_log_level(int) {}

void vosk_recognizer_set_endpointer_mode(void* recognizer, int mode)
{
    // The call #10759 is about. A real libvosk reads through the handle here.
    if (!recognizer) {
        ++gNullHandleCalls;
        return;
    }
    static_cast<StubRecognizer*>(recognizer)->endpointerMode = mode;
}

void vosk_recognizer_set_words(void* recognizer, int words)
{
    // The other one, and the one the QA report actually caught.
    if (!recognizer) {
        ++gNullHandleCalls;
        return;
    }
    static_cast<StubRecognizer*>(recognizer)->wordsRequested = words;
}

} // extern "C"
