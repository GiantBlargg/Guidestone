use std::{collections::HashMap, fs::File, io};

use binrw::{binread, BinRead};

#[allow(dead_code)]
#[binread]
#[br(little, magic = b"RBF1.23")]
struct BigTOC {
	#[br(temp)]
	num_files: i32,
	flags: i32,
	#[br(count = num_files)]
	file_entries: Vec<BigFileEntry>,
}

#[allow(dead_code)]
#[binread]
struct BigFileEntry {
	name_crc: u64,
	#[br(pad_size_to = 4)]
	name_length: u16,
	stored_length: u32,
	real_length: u32,
	offset: u32,
	timestamp: u32,
	#[br(pad_size_to = 4)]
	compression_type: u8,
}

struct FileEntry {
	stored_length: u32,
	real_length: u32,
	offset: u32,
	compressed: bool,
}

pub struct BigFile<R: io::Read + io::Seek> {
	read: R,
	toc: HashMap<String, FileEntry>,
}

impl<R: io::Read + io::Seek> BigFile<R> {
	pub fn load(mut read: R) -> Self {
		let big_toc = BigTOC::read(&mut read).unwrap();

		let toc: HashMap<String, FileEntry> = big_toc
			.file_entries
			.into_iter()
			.map(|f| -> io::Result<_> {
				read.seek(io::SeekFrom::Start(f.offset.into()))?;
				let mut name_encrypted = vec![0u8; f.name_length.into()];
				read.read_exact(&mut name_encrypted)?;

				let mut mask = 213u8;
				for letter in name_encrypted.iter_mut() {
					*letter ^= mask;
					mask = *letter;
				}

				let name = String::from_utf8(name_encrypted).unwrap();

				assert!(f.compression_type == 0 || f.compression_type == 1);
				assert!(f.compression_type == 1 || f.real_length == f.stored_length);

				Ok((
					name.to_lowercase().replace('\\', "/"),
					FileEntry {
						stored_length: f.stored_length,
						real_length: f.stored_length,
						offset: f.offset + u32::from(f.name_length) + 1,
						compressed: f.compression_type != 0,
					},
				))
			})
			.collect::<io::Result<_>>()
			.unwrap();

		Self { read, toc }
	}

	pub fn open(&mut self, filename: &str) -> Option<impl io::Read + io::Seek> {
		let filename = filename.to_lowercase();
		if let Some(f) = self.toc.get(&filename) {
			let data = {
				self.read
					.seek(io::SeekFrom::Start(f.offset.into()))
					.unwrap();
				if f.compressed {
					let mut buf = Vec::with_capacity(f.real_length as usize);
					lzss_decompress(&mut self.read, &mut buf);
					buf
				} else {
					let mut buf = vec![0u8; f.stored_length as usize];
					self.read.read_exact(&mut buf).unwrap();
					buf
				}
			};
			Some(io::Cursor::new(data))
		} else {
			None
		}
	}
}

pub struct HwFs(Vec<BigFile<File>>);

impl HwFs {
	pub fn load() -> Self {
		Self(
			std::env::var("HW_BIG")
				.unwrap()
				.split(';')
				.map(|f| BigFile::load(File::open(f).unwrap()))
				.collect(),
		)
	}

	pub fn open(&mut self, filename: &str) -> Option<impl io::Read + io::Seek> {
		self.0.iter_mut().filter_map(|b| b.open(filename)).next()
	}
}

fn lzss_decompress<R: io::Read, W: io::Write>(reader: R, mut writer: W) {
	const INDEX_SIZE: u8 = 12;
	const LENGTH_SIZE: u8 = 4;
	const BREAK_EVEN: u8 = (1 + INDEX_SIZE + LENGTH_SIZE) / 9;

	let mut reader = BitReader::new(reader);
	let mut window = [0u8; 1 << INDEX_SIZE];
	let mut current_position = 1;

	loop {
		if reader.read_bits(1) == 1 {
			let c = reader.read_bits(8) as u8;
			writer.write_all(&[c]).unwrap();
			window[current_position] = c;
			current_position = (current_position + 1) & (window.len() - 1);
		} else {
			let match_position = reader.read_bits(INDEX_SIZE);
			if match_position == 0 {
				break;
			}
			let match_length = (reader.read_bits(LENGTH_SIZE) as u8) + BREAK_EVEN;
			for i in 0..=match_length {
				let c = window[(usize::from(match_position) + usize::from(i)) & (window.len() - 1)];
				writer.write_all(&[c]).unwrap();
				window[current_position] = c;
				current_position = (current_position + 1) & (window.len() - 1);
			}
		}
	}
}

struct BitReader<R: io::Read> {
	reader: R,
	stage: u32,
	remaining: u8,
}

impl<R: io::Read> BitReader<R> {
	fn new(reader: R) -> Self {
		Self {
			reader,
			stage: 0,
			remaining: 0,
		}
	}

	fn read_bits(&mut self, n: u8) -> u16 {
		assert!(n > 0 && n <= 16);
		while n > self.remaining {
			let mut buf = [0u8];
			let result = self.reader.read(&mut buf);
			assert!(result.unwrap() == 1);
			let next_byte = buf[0];

			self.stage |= u32::from(next_byte) << (24 - self.remaining);
			self.remaining += 8;
		}
		let ret = (self.stage & ((!0) << (32 - n))) >> (32 - n);
		self.stage <<= n;
		self.remaining -= n;
		ret as u16
	}
}
