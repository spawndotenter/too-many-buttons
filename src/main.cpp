#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <set>

using namespace geode::prelude;

struct VanillaButton {
    const char* setting;
    const char* nodeID;
};

static const VanillaButton VANILLA[] = {
    {"hide-play", "play-button"},
    {"hide-icon-kit", "icon-kit-button"},
    {"hide-editor", "editor-button"},
    {"hide-achievements", "achievements-button"},
    {"hide-settings", "settings-button"},
    {"hide-stats", "stats-button"},
    {"hide-newgrounds", "newgrounds-button"},
    {"hide-daily-chests", "daily-chest-button"},
    {"hide-profile", "profile-button"},
    {"hide-more-games", "more-games-button"},
    {"hide-robtop-logo", "robtop-logo-button"},
    {"hide-facebook", "facebook-button"},
    {"hide-twitter", "twitter-button"},
    {"hide-youtube", "youtube-button"},
    {"hide-twitch", "twitch-button"},
    {"hide-discord", "discord-button"},
};

static std::set<std::string> parseList(std::string const& text) {
    std::set<std::string> out;
    std::string cur;
    auto flush = [&] {
        size_t a = cur.find_first_not_of(" \t");
        size_t b = cur.find_last_not_of(" \t");
        if (a != std::string::npos) out.insert(cur.substr(a, b - a + 1));
        cur.clear();
    };
    for (char c : text) {
        if (c == ',') flush();
        else cur += c;
    }
    flush();
    return out;
}

static void moveGeodeButton(MenuLayer* layer) {
    auto geodeBtn = layer->getChildByIDRecursive("geode.loader/geode-button");
    auto chest = layer->getChildByIDRecursive("daily-chest-button");
    if (!geodeBtn || !chest) return;

    auto target = chest->getParent();
    auto oldParent = geodeBtn->getParent();
    if (!target || !oldParent || target == oldParent) return;

    Ref<CCNode> keep = geodeBtn;
    geodeBtn->removeFromParentAndCleanup(false);
    target->addChild(geodeBtn);
    oldParent->updateLayout();
    target->updateLayout();
}

static void applyButtons(MenuLayer* layer) {
    auto mod = Mod::get();
    std::set<CCNode*> touched;

    // Game buttons
    for (auto const& b : VANILLA) {
        if (auto node = layer->getChildByIDRecursive(b.nodeID)) {
            node->setVisible(!mod->getSettingValue<bool>(b.setting));
            if (auto p = node->getParent()) touched.insert(p);
        }
    }

    // Buttons from other mods (automatic)
    bool hideAllMods = mod->getSettingValue<bool>("hide-mod-buttons");
    auto keepMods = parseList(mod->getSettingValue<std::string>("keep-visible-mods"));
    auto hiddenIDs = parseList(mod->getSettingValue<std::string>("hidden-ids"));

    if (auto kids = layer->getChildren()) {
        for (auto child : CCArrayExt<CCNode*>(kids)) {
            auto menu = typeinfo_cast<CCMenu*>(child);
            if (!menu || !menu->getChildren()) continue;

            for (auto btn : CCArrayExt<CCNode*>(menu->getChildren())) {
                std::string id = btn->getID();
                if (id.empty()) continue;

                log::info("[Too Many Buttons] {} -> {}", menu->getID(), id);

                auto slash = id.find('/');
                std::string modID = slash == std::string::npos ? "" : id.substr(0, slash);
                if (modID == "geode.loader") continue;

                bool isModButton = !modID.empty();
                bool hide = hiddenIDs.contains(id) || (isModButton && hiddenIDs.contains(modID));
                if (isModButton && hideAllMods && !keepMods.contains(modID)) hide = true;

                if (hide) {
                    btn->setVisible(false);
                    touched.insert(menu);
                }
            }
        }
    }

    // Geode button position
    if (mod->getSettingValue<std::string>("geode-button-position") == "Under Daily Chests") {
        moveGeodeButton(layer);
    }

    for (auto node : touched) node->updateLayout();
}

class $modify(TMBMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        // Wait one frame so buttons from every other mod already exist.
        this->scheduleOnce(schedule_selector(TMBMenu::onApplyButtons), 0.f);
        return true;
    }

    void onApplyButtons(float) {
        applyButtons(this);
    }
};
