use std::io;

use common::model::{Model, Texture};

use big::HwFs;
use model::ImportModels;

mod big;
mod model;

pub struct HWImporter {
	fs: HwFs,
	models: ImportModels,
}

impl HWImporter {
	pub fn load() -> Self {
		let mut fs = HwFs::load();
		let models = ImportModels::new(&mut fs);
		Self { fs, models }
	}

	pub fn load_model(&mut self, path: &str) -> io::Result<Model> {
		self.models.load_model(&mut self.fs, path)
	}

	pub fn load_texture(&mut self, path: &str) -> io::Result<Texture> {
		self.models.load_texture(&mut self.fs, path)
	}
}
