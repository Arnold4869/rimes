#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/testing.h>
#include <fcitx/addonmanager.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputmethodgroup.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <testfrontend_public.h>

namespace {

void Die(const std::string& message) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void Type(fcitx::AddonInstance* frontend, const fcitx::ICUUID& uuid, const char* keys) {
    for (const char* cursor = keys; *cursor != '\0'; ++cursor) {
        const char name[] = {*cursor, '\0'};
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key(name), false);
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key(name), true);
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: rimes-fcitx-e2e <build-dir> <addon-rel-dir> <data-rel-dir>\n";
        return EXIT_FAILURE;
    }

    fcitx::setupTestingEnvironment(argv[1], {argv[2]}, {argv[3]});

    char arg0[] = "rimes-fcitx-e2e";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testfrontend,testui,rimes";
    char* args[] = {arg0, arg1, arg2};
    fcitx::Instance instance(3, args);
    instance.addonManager().registerDefaultLoader(nullptr);

    fcitx::EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    dispatcher.schedule([&instance]() {
        auto& manager = instance.inputMethodManager();
        if (manager.entry("rimes") == nullptr) {
            Die("input method 'rimes' was not registered");
        }
        if (manager.groupCount() == 0) {
            manager.addEmptyGroup("Default");
            manager.setGroupOrder({"Default"});
        }
        fcitx::InputMethodGroup group("Default");
        group.setDefaultLayout("us");
        group.inputMethodList().emplace_back(fcitx::InputMethodGroupItem("rimes"));
        group.setDefaultInputMethod("rimes");
        manager.setGroup(group);
        manager.setCurrentGroup("Default");

        auto* frontend = instance.addonManager().addon("testfrontend", true);
        if (frontend == nullptr) {
            Die("testfrontend addon did not load");
        }
        const auto uuid = frontend->call<fcitx::ITestFrontend::createInputContext>("rimes-e2e");
        auto* ic = instance.inputContextManager().findByUUID(uuid);
        if (ic == nullptr) {
            Die("test input context was not created");
        }
        instance.setCurrentInputMethod(ic, "rimes", true);
        if (instance.inputMethod(ic) != "rimes") {
            Die("could not switch the test context to rimes");
        }

        frontend->call<fcitx::ITestFrontend::pushCommitExpectation>("你好");
        Type(frontend, uuid, "nihao");
        if (ic->inputPanel().clientPreedit().empty() && ic->inputPanel().preedit().empty()) {
            Die("nihao produced no preedit");
        }
        auto candidates = ic->inputPanel().candidateList();
        if (!candidates || candidates->size() < 1) {
            Die("nihao produced no candidate list");
        }
        bool saw_nihao = false;
        for (int index = 0; index < candidates->size(); ++index) {
            if (candidates->candidate(index).text().toStringForCommit() == "你好") {
                saw_nihao = true;
            }
        }
        if (!saw_nihao) {
            Die("candidate page after nihao did not include 你好");
        }
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("space"), false);
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("space"), true);
        std::cout << "ok: testfrontend nihao + Space\n";

        Type(frontend, uuid, "ni");
        candidates = ic->inputPanel().candidateList();
        if (!candidates || candidates->size() < 2) {
            Die("expected at least two candidates after typing ni");
        }
        const std::string second = candidates->candidate(1).text().toStringForCommit();
        frontend->call<fcitx::ITestFrontend::pushCommitExpectation>(second);
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("2"), false);
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("2"), true);
        std::cout << "ok: testfrontend number selection\n";

        Type(frontend, uuid, "a");
        candidates = ic->inputPanel().candidateList();
        if (!candidates || candidates->empty()) {
            Die("expected candidates before paging");
        }
        const std::string first_page = candidates->candidate(0).text().toStringForCommit();
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("Page_Down"), false);
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("Page_Down"), true);
        candidates = ic->inputPanel().candidateList();
        if (!candidates || candidates->empty()) {
            Die("page down cleared the candidate list");
        }
        const std::string paged = candidates->candidate(0).text().toStringForCommit();
        auto* pageable = candidates->toPageable();
        if (paged == first_page &&
            (pageable == nullptr || (!pageable->hasNext() && !pageable->hasPrev()))) {
            Die("page down left a single unpageable list");
        }
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("Escape"), false);
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("Escape"), true);
        std::cout << "ok: testfrontend paging\n";

        // Escape must clear composition. testfrontend fatals on any commit that
        // was not pushed, so this also asserts that Escape does not commit.
        Type(frontend, uuid, "nihao");
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("Escape"), false);
        frontend->call<fcitx::ITestFrontend::keyEvent>(uuid, fcitx::Key("Escape"), true);
        std::cout << "ok: testfrontend Escape\n";

        frontend->call<fcitx::ITestFrontend::destroyInputContext>(uuid);
        instance.exit();
    });

    try {
        return instance.exec();
    } catch (const fcitx::InstanceQuietQuit&) {
        return EXIT_SUCCESS;
    } catch (const std::exception& exception) {
        Die(exception.what());
    }
}
