use big::BigChain;
use model::make_model;

mod big;
mod model;

pub struct HWImporter {}

impl HWImporter {
	pub fn load() {
		let mut bigs = BigChain::load();

		let test = bigs
			.open("R1/ResourceCollector/RL0/LOD0/ResourceCollector.peo")
			.unwrap();

		make_model(test);
	}
}
