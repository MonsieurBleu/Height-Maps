#pragma once

#include <App.hpp>
#include <Fonts.hpp>
#include <FastUI.hpp>

#include <GameGlobals.hpp>

class Game final : public App
{
private:

    /* Fast-UI */
    FontRef FUIfont;    
    MeshMaterial defaultFontMaterial;
    MeshMaterial defaultSUIMaterial;
    SimpleUiTileBatchRef fuiBatch;

    SpectatorController spectator;

    ObjectGroupRef helpers;

    bool wireframe = false;

public:
    Game(GLFWwindow *window);
    void init(int paramSample);
    bool userInput(GLFWKeyInfo input);
    void mainloop();
};