#include <Game.hpp>
#include <../Engine/include/Globals.hpp>
#include <GameObject.hpp>
#include <CompilingOptions.hpp>
#include <MathsUtils.hpp>
#include <Audio.hpp>

#include <thread>
#include <fstream>

#include <Helpers.hpp>

Game::Game(GLFWwindow *window) : App(window) {}

void Game::init(int paramSample)
{
    App::init();

    // activateMainSceneBindlessTextures();

    setIcon("ressources/icon.png");

    setController(&spectator);

    ambientLight = vec3(0.1);

    finalProcessingStage = ShaderProgram(
        "shader/post-process/final composing.frag",
        "shader/post-process/basic.vert",
        "",
        globals.standartShaderUniform2D());

    finalProcessingStage.addUniform(ShaderUniform(Bloom.getIsEnableAddr(), 10));

    camera.init(radians(70.0f), globals.windowWidth(), globals.windowHeight(), 0.1f, 1E4f);
    // camera.setMouseFollow(false);
    // camera.setPosition(vec3(0, 1, 0));
    // camera.setDirection(vec3(1, 0, 0));
    auto myfile = std::fstream("saves/cameraState.bin", std::ios::in | std::ios::binary);
    if(myfile)
    {
        CameraState buff;
        myfile.read((char*)&buff, sizeof(CameraState));
        myfile.close();
        camera.setState(buff);
    }


    /* Loading 3D Materials */
    depthOnlyMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/depthOnly.frag",
            "shader/foward/basic.vert",
            ""));

    depthOnlyStencilMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/depthOnlyStencil.frag",
            "shader/foward/basic.vert",
            ""));

    depthOnlyHeightMapMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/depthOnlyStencil.frag",
            "shader/special/heightMaps.vert",
            ""));

    GameGlobals::PBR = MeshMaterial(
        new ShaderProgram(
            "shader/foward/PBR.frag",
            // "shader/clustered/clusterDebug.frag",
            "shader/foward/basic.vert",
            "",
            globals.standartShaderUniform3D()));

    GameGlobals::PBRstencil = MeshMaterial(
        new ShaderProgram(
            "shader/foward/PBR.frag",
            "shader/foward/basic.vert",
            "",
            globals.standartShaderUniform3D()));

    GameGlobals::PBRHeightMap = MeshMaterial(
        new ShaderProgram(
            "shader/foward/PBR.frag",
            "shader/special/heightMaps.vert",
            "",
            globals.standartShaderUniform3D()));

    skyboxMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/foward/Skybox.frag",
            "shader/foward/basic.vert",
            "",
            globals.standartShaderUniform3D()));

    GameGlobals::PBRstencil.depthOnly = depthOnlyStencilMaterial;
    GameGlobals::PBRHeightMap.depthOnly = depthOnlyHeightMapMaterial;
    scene.depthOnlyMaterial = depthOnlyMaterial;

    /* UI */
    FUIfont = FontRef(new FontUFT8);
    FUIfont->readCSV("ressources/fonts/Roboto/out.csv");
    FUIfont->setAtlas(Texture2D().loadFromFileKTX("ressources/fonts/Roboto/out.ktx"));
    defaultFontMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/2D/sprite.frag",
            "shader/2D/sprite.vert",
            "",
            globals.standartShaderUniform2D()));

    defaultSUIMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/2D/fastui.frag",
            "shader/2D/fastui.vert",
            "",
            globals.standartShaderUniform2D()));

    fuiBatch = SimpleUiTileBatchRef(new SimpleUiTileBatch);
    fuiBatch->setMaterial(defaultSUIMaterial);
    fuiBatch->state.position.z = 0.0;
    fuiBatch->state.forceUpdate();

    /* VSYNC and fps limit */
    globals.fpsLimiter.activate();
    globals.fpsLimiter.freq = 144.f;
    glfwSwapInterval(0);
}

bool Game::userInput(GLFWKeyInfo input)
{
    if (baseInput(input))
        return true;

    if (input.action == GLFW_PRESS)
    {
        switch (input.key)
        {
        case GLFW_KEY_ESCAPE:
            state = quit;
            break;

        case GLFW_KEY_F2:
            globals.currentCamera->toggleMouseFollow();
            break;

        case GLFW_KEY_1:
            Bloom.toggle();
            break;

        case GLFW_KEY_2:
            SSAO.toggle();
            break;

        case GLFW_KEY_F5:
            #ifdef _WIN32
            system("cls");
            #else
            system("clear");
            #endif

            finalProcessingStage.reset();
            Bloom.getShader().reset();
            SSAO.getShader().reset();
            depthOnlyMaterial->reset();
            GameGlobals::PBR->reset();
            GameGlobals::PBRstencil->reset();
            GameGlobals::PBRHeightMap->reset();
            GameGlobals::PBRHeightMap.depthOnly->reset();
            skyboxMaterial->reset();


            break;
        
        case GLFW_KEY_F6:
            if(helpers->state.hide == ModelStateHideStatus::SHOW)
                helpers->state.hide = ModelStateHideStatus::HIDE;
            else
                helpers->state.hide = ModelStateHideStatus::SHOW;
            break;

        case GLFW_KEY_F8:
            {
                auto myfile = std::fstream("saves/cameraState.bin", std::ios::out | std::ios::binary);
                myfile.write((char*)&camera.getState(), sizeof(CameraState));
                myfile.close();
            }
                break;

        default:
            break;
        }
    }

    return true;
};

void Game::physicsLoop()
{
    physicsTicks.freq = 45.f;
    physicsTicks.activate();

    while (state != quit)
    {
        physicsTicks.start();

        physicsMutex.lock();
        physicsEngine.update(1.f / physicsTicks.freq);
        physicsMutex.unlock();

        physicsTicks.waitForEnd();
    }
}

void Game::mainloop()
{
    /* Loading Models and setting up the scene */
    ModelRef skybox = newModel(skyboxMaterial);
    skybox->loadFromFolder("ressources/models/skybox/", true, false);

    // skybox->invertFaces = true;
    skybox->depthWrite = true;
    skybox->state.frustumCulled = false;
    skybox->state.scaleScalar(1E6);
    scene.add(skybox);

    ModelRef floor = newModel(GameGlobals::PBRHeightMap);
    floor->loadFromFolder("ressources/models/ground/");
    floor->state.scaleScalar(16.0);
    scene.add(floor);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(3.0);

    /* Setting up the UI */
    FastUI_context ui(fuiBatch, FUIfont, scene2D, defaultFontMaterial);
    FastUI_valueMenu menu(ui, {});

    menu->state.setPosition(vec3(-0.9, 0.5, 0)).scaleScalar(0.8);
    globals.appTime.setMenuConst(menu);
    globals.cpuTime.setMenu(menu);
    globals.gpuTime.setMenu(menu);
    globals.fpsLimiter.setMenu(menu);


    SceneDirectionalLight sun = newDirectionLight(
        DirectionLight()
            .setColor(vec3(143, 107, 71) / vec3(255))
            .setDirection(normalize(vec3(-1.0, -1.0, 0.0)))
            .setIntensity(1.0));

    sun->cameraResolution = vec2(2048);
    sun->shadowCameraSize = vec2(50, 50);
    sun->activateShadows();
    scene.add(sun);

    sun->setMenu(menu, U"Sun");


    menu.batch();
    scene2D.updateAllObjects();
    fuiBatch->batch();

   
    // Texture2D HeightMaps = Texture2D().loadFromFileHDR("ressources/maps/RuggedTerrain.png")
    //     .setFormat(GL_RGB)
    //     .setInternalFormat(GL_RGB8)
    //     .setPixelType(GL_UNSIGNED_BYTE)
    //     .setWrapMode(GL_CLAMP_TO_BORDER)
    //     .generate()
    //     ; 
    Texture2D HeightMaps = Texture2D().loadFromFileHDR("ressources/maps/RuggedTerrain.hdr")
        .setFormat(GL_RGB)
        .setInternalFormat(GL_RGB32F)
        .setPixelType(GL_FLOAT)
        .setWrapMode(GL_REPEAT) 
        // .setWrapMode(GL_CLAMP_TO_BORDER)
        .generate()
    ;
    floor->setMap(HeightMaps, 2);
    Texture2D DispMaps = Texture2D().loadFromFileHDR("ressources/models/ground/disp.hdr")
        .setFormat(GL_RGB)
        .setInternalFormat(GL_RGB32F)
        .setPixelType(GL_FLOAT)
        .setWrapMode(GL_REPEAT)
        .generate()
    ;
    floor->setMap(DispMaps, 3);


    state = AppState::run;
    // std::thread physicsThreads(&Game::physicsLoop, this);

    glLineWidth(1.0);

    /* Main Loop */
    while (state != AppState::quit)
    {
        mainloopStartRoutine();

        for (GLFWKeyInfo input; inputs.pull(input); userInput(input));

        menu.trackCursor();
        menu.updateText();

        mainloopPreRenderRoutine();

        /* UI & 2D Render */
        glEnable(GL_BLEND);
        glEnable(GL_FRAMEBUFFER_SRGB);

        scene2D.updateAllObjects();
        fuiBatch->batch();
        screenBuffer2D.activate();
        fuiBatch->draw();
        scene2D.cull();
        scene2D.draw();
        screenBuffer2D.deactivate();

        /* 3D Pre-Render */
        glDisable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_BLEND);
        glDepthFunc(GL_GREATER);
        glEnable(GL_DEPTH_TEST);

        scene.updateAllObjects();
        scene.generateShadowMaps();
        renderBuffer.activate();

        scene.cull();

        /* 3D Early Depth Testing */
        scene.depthOnlyDraw(*globals.currentCamera, true);
        glDepthFunc(GL_EQUAL);

        /* 3D Render */
        skybox->bindMap(0, 4);
        scene.genLightBuffer();
        scene.draw();
        renderBuffer.deactivate();

        /* Post Processing */
        renderBuffer.bindTextures();
        SSAO.render(*globals.currentCamera);
        Bloom.render(*globals.currentCamera);

        /* Final Screen Composition */
        glViewport(0, 0, globals.windowWidth(), globals.windowHeight());
        finalProcessingStage.activate();
        // sun->shadowMap.bindTexture(0, 6);
        screenBuffer2D.bindTexture(0, 7);
        globals.drawFullscreenQuad();

        /* Main loop End */
        mainloopEndRoutine();
    }

    // physicsThreads.join();
}
