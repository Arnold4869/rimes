#pragma once

#include <memory>

#include <fcitx-utils/event.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include "engine/rime_engine.hpp"

namespace fcitx {

class RimesState;

class RimesIme final : public InputMethodEngineV2 {
public:
    explicit RimesIme(Instance* instance);
    ~RimesIme() override;

    void keyEvent(const InputMethodEntry& entry, KeyEvent& keyEvent) override;
    void activate(const InputMethodEntry& entry, InputContextEvent& event) override;
    void deactivate(const InputMethodEntry& entry, InputContextEvent& event) override;
    void reset(const InputMethodEntry& entry, InputContextEvent& event) override;
    std::string subMode(const InputMethodEntry& entry, InputContext& inputContext) override;

    Instance* instance() { return instance_; }
    rimes::linuxime::RimeEngine& engine() { return engine_; }
    FactoryFor<RimesState>& factory() { return factory_; }

    // Single commit path. Later Linux Buffer hooks here (see rime_hooks.hpp).
    void commitText(InputContext* ic, std::string_view text);

    void applySnapshot(InputContext* ic, const rimes::linuxime::EngineSnapshot& snapshot);

private:
    void StartDeployWatch();
    void OnDeployReady();

    Instance* instance_;
    rimes::linuxime::RimeEngine engine_;
    FactoryFor<RimesState> factory_;
    std::unique_ptr<EventSourceTime> deploy_timer_;
};

class RimesImeFactory : public AddonFactory {
public:
    AddonInstance* create(AddonManager* manager) override;
};

}  // namespace fcitx
