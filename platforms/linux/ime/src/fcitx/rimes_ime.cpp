#include "rimes_ime.hpp"

#include <filesystem>

#include <fcitx-utils/event.h>
#include <fcitx-utils/log.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/userinterface.h>

#include "engine/rime_hooks.hpp"
#include "engine/rime_paths.hpp"
#include "rimes_state.hpp"

#ifndef RIMES_INSTALL_SHARED_DIR
#define RIMES_INSTALL_SHARED_DIR ""
#endif

namespace fcitx {
namespace {

FCITX_DEFINE_LOG_CATEGORY(rimes_log, "rimes");

}  // namespace

RimesIme::RimesIme(Instance* instance)
    : instance_(instance),
      factory_([this](InputContext& ic) { return new RimesState(this, &ic); }) {
    instance_->inputContextManager().registerProperty("rimesState", &factory_);

    auto paths = rimes::linuxime::ResolveEnginePaths(
        std::filesystem::path(RIMES_INSTALL_SHARED_DIR));
    rimes::linuxime::RimeEngineOptions options;
    options.shared_data_dir = paths.shared_data_dir;
    options.user_data_dir = paths.user_data_dir;
    options.log_dir = paths.log_dir;
    options.wait_for_maintenance = false;

    std::string error;
    if (!engine_.Start(options, &error)) {
        FCITX_LOGC(rimes_log, Error) << "librime start failed: " << error;
    } else if (engine_.IsDeploying()) {
        FCITX_LOGC(rimes_log, Info) << "librime first-run deploy started in the background";
        StartDeployWatch();
    }
}

RimesIme::~RimesIme() { deploy_timer_.reset(); }

void RimesIme::StartDeployWatch() {
    deploy_timer_ = instance_->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 200000, 200000,
        [this](EventSourceTime* source, uint64_t) {
            if (engine_.PollMaintenance()) {
                source->setEnabled(false);
                OnDeployReady();
                return true;
            }
            source->setTime(now(CLOCK_MONOTONIC) + 200000);
            return true;
        });
}

void RimesIme::OnDeployReady() {
    FCITX_LOGC(rimes_log, Info) << "librime deploy finished";
    instance_->inputContextManager().foreachFocused([this](InputContext* ic) {
        if (instance_->inputMethod(ic) == "rimes") {
            instance_->showInputMethodInformation(ic);
            ic->updateUserInterface(UserInterfaceComponent::StatusArea);
        }
        return true;
    });
}

void RimesIme::keyEvent(const InputMethodEntry& entry, KeyEvent& keyEvent) {
    FCITX_UNUSED(entry);
    auto* state = keyEvent.inputContext()->propertyFor(&factory_);
    if (state != nullptr) {
        state->keyEvent(keyEvent);
    }
}

void RimesIme::activate(const InputMethodEntry& entry, InputContextEvent& event) {
    FCITX_UNUSED(entry);
    auto* state = event.inputContext()->propertyFor(&factory_);
    if (state != nullptr) {
        state->activate();
    }
}

void RimesIme::deactivate(const InputMethodEntry& entry, InputContextEvent& event) {
    FCITX_UNUSED(entry);
    auto* state = event.inputContext()->propertyFor(&factory_);
    if (state != nullptr) {
        state->deactivate(event);
    }
}

void RimesIme::reset(const InputMethodEntry& entry, InputContextEvent& event) {
    FCITX_UNUSED(entry);
    auto* state = event.inputContext()->propertyFor(&factory_);
    if (state != nullptr) {
        state->reset();
    }
}

std::string RimesIme::subMode(const InputMethodEntry& entry, InputContext& inputContext) {
    FCITX_UNUSED(entry);
    if (engine_.IsDeploying()) {
        return "Deploying";
    }
    auto* state = inputContext.propertyFor(&factory_);
    if (state == nullptr) {
        return {};
    }
    return state->schemaId();
}

void RimesIme::commitText(InputContext* ic, std::string_view text) {
    // Single commit path. Buffer/Capsule/Mailbox must hook here later.
    static_cast<void>(rimes::linuxime::kCommitHookNote);
    if (ic == nullptr || text.empty()) {
        return;
    }
    ic->commitString(std::string(text));
}

void RimesIme::applySnapshot(InputContext* ic,
                             const rimes::linuxime::EngineSnapshot& snapshot) {
    if (ic == nullptr) {
        return;
    }
    if (!snapshot.commit_text.empty()) {
        commitText(ic, snapshot.commit_text);
    }
}

AddonInstance* RimesImeFactory::create(AddonManager* manager) {
    return new RimesIme(manager->instance());
}

}  // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::RimesImeFactory);
