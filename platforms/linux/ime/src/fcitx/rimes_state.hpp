#pragma once

#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>

#include "engine/rime_engine.hpp"
#include "rimes_ime.hpp"

namespace fcitx {

class RimesState : public InputContextProperty {
public:
    RimesState(RimesIme* ime, InputContext* ic);
    ~RimesState() override;

    void keyEvent(KeyEvent& event);
    void activate();
    void reset();
    void deactivate();
    void selectCandidate(int index);
    void page(bool next);
    void applySnapshot(const rimes::linuxime::EngineSnapshot& snapshot);
    std::string schemaId() const { return schema_id_; }

private:
    rimes::linuxime::RimeEngine::SessionId EnsureSession();
    std::int32_t ModifierMask(const KeyEvent& event) const;
    void UpdateUI(const rimes::linuxime::EngineSnapshot& snapshot);

    RimesIme* ime_;
    InputContext* ic_;
    rimes::linuxime::RimeEngine::SessionId session_ = 0;
    std::string schema_id_;
    bool composing_ = false;
};

}  // namespace fcitx
