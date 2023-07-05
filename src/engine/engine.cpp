#include "engine.hpp"

#include "model.hpp"

void Engine::init(Render* _render) { render = _render; }

void Engine::startGame() {
	ModelCache model_cache;
	model_cache.loadClassicModel("r1/lightcorvette/rl0/lod0/lightcorvette.peo");
}

void Engine::tickUpdate() {}

void Engine::update(double delta) { render->renderFrame(); }
