#include <Controllers.hpp>
#include <Globals.hpp>
#include <MathsUtils.hpp>

void DroneController::update()
{
    updateDirectionStateWASD();

    const float delta = globals.appTime.getDelta();
    GLFWwindow *window = globals.getWindow();
    const vec3 cpos = globals.currentCamera->getPosition();
    const vec3 cfront = globals.currentCamera->getDirection();
    const vec3 cup = cross(cfront, cross(cfront, vec3(0, 1, 0)));
    const vec3 cright = cross(cfront, cup);

    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        globals.currentCamera->setDirection(rotateVec(cfront, cright, delta));

    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        globals.currentCamera->setDirection(rotateVec(cfront, cright, -delta));

    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        globals.currentCamera->setDirection(rotateVec(cfront, cup, delta));

    if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        globals.currentCamera->setDirection(rotateVec(cfront, cup, -delta));

    const float dspeed = speed * delta * (sprintActivated ? sprintFactor : 1.f);

    vec3 depUp = vec3(0, 1, 0);
    vec3 depFront = globals.currentCamera->getDirection();
    

    /* Projecting front vector on the plan with the up normal */
    depFront = normalize(cross(depUp, cross(depFront, depUp)));

    vec3 depRight = cross(depUp, depFront);
    deplacementDir = depFront*(float)frontFactor + depUp*(float)upFactor + depRight*(float)rightFactor;

    globals.currentCamera->setPosition(cpos + dspeed*deplacementDir);
}