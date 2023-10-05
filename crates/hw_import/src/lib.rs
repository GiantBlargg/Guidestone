use big::HwFs;
use model::ImportModels;

mod big;
mod model;

pub struct HWImporter {
	models: ImportModels,
}

impl HWImporter {
	pub fn load() -> Self {
		let mut fs = HwFs::load();
		let models = ImportModels::new(&mut fs);
		Self { models }
	}
}
