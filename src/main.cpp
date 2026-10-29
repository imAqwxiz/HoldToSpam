
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <algorithm>
#include <cstdint>

using namespace geode::prelude;

namespace {
    constexpr float ACTIVATION_DELAY = 15.0f;

    bool g_holding = false;
    bool g_syntheticInput = false;
    bool g_spamButtonDown = true;

    float g_holdTime = 0.0f;
    float g_tickTimer = 0.0f;

    int g_button = 0;
    bool g_player1 = true;
}

class $modify(HoldToSpamGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        // Synthetic events must not reset the physical hold timer.
        if (g_syntheticInput) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        if (down) {
            // Start tracking the first held gameplay input.
            if (!g_holding) {
                g_holding = true;
                g_holdTime = 0.0f;
                g_tickTimer = 0.0f;

                g_button = button;
                g_player1 = isPlayer1;
                g_spamButtonDown = true;
            }
        } else if (
            g_holding &&
            button == g_button &&
            isPlayer1 == g_player1
        ) {
            // Releasing the tracked input immediately resets everything.
            g_holding = false;
            g_holdTime = 0.0f;
            g_tickTimer = 0.0f;
            g_spamButtonDown = true;
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void update(float dt) {
        GJBaseGameLayer::update(dt);

        if (!g_holding) {
            return;
        }

        g_holdTime += dt;

        // Do not automate anything until the full 15 seconds have passed.
        if (g_holdTime < ACTIVATION_DELAY) {
            return;
        }

        auto cps = std::clamp(
            static_cast<int>(
                Mod::get()->getSettingValue<int64_t>("cps")
            ),
            1,
            60
        );

        // One press/release cycle contains two transitions.
        const float halfInterval = 0.5f / static_cast<float>(cps);

        g_tickTimer += dt;

        if (g_tickTimer < halfInterval) {
            return;
        }

        g_tickTimer = 0.0f;
        g_spamButtonDown = !g_spamButtonDown;

        // Send the synthetic transition without changing the physical hold.
        g_syntheticInput = true;
        GJBaseGameLayer::handleButton(
            g_spamButtonDown,
            g_button,
            g_player1
        );
        g_syntheticInput = false;
    }
};

