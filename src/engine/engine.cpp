#include "engine.hpp"

void Engine::init(Render* _render) { render = _render; }

void Engine::startGame() {}

void Engine::tickUpdate() {}

void Engine::update(double delta) { render->renderFrame(); }
