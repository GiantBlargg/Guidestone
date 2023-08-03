#include "engine.hpp"

#include "model.hpp"

Engine::Engine(Platform& p) : platform(p), active(*this) {}

void Engine::init() {
	render = platform.init_video();

	active.start();
};

void Engine::startGame() {
	ModelCache model_cache;
	model_cache.loadClassicModel("r1/resourcecollector/rl0/lod0/resourcecollector.peo");
	render->setModelCache(model_cache);
}
