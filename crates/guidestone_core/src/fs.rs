use std::{
	ffi::CString,
	fs::File,
	io::{self, ErrorKind, Read, Seek, SeekFrom},
	marker::PhantomData,
	path::{Path, PathBuf},
};

pub use guidestone_proc_macro::FromRead;

pub trait FromRead: Sized {
	fn from_read<R: Read>(r: &mut R) -> io::Result<Self>;
}

impl FromRead for u8 {
	fn from_read<R: Read>(r: &mut R) -> io::Result<Self> {
		let mut buf = [0];
		r.read_exact(&mut buf)?;
		Ok(buf[0])
	}
}
macro_rules! impl_from_read_le_bytes {
	($($t:ty)+) => {$(
		impl FromRead for $t {
			fn from_read<R: Read>(r: &mut R) -> io::Result<Self> {
				Ok(<$t>::from_le_bytes(FromRead::from_read(r)?))
			}
		}
	)+};
}

impl_from_read_le_bytes!(u16 i16 u32 i32 f32);

// impl<const N: usize> FromRead for [u8; N] {
// 	fn from_read<R: Read>(r: &mut R) -> io::Result<Self> {
// 		let mut buf = [0; N];
// 		r.read_exact(&mut buf)?;
// 		Ok(buf)
// 	}
// }

impl<T: FromRead, const N: usize> FromRead for [T; N] {
	fn from_read<R: Read>(r: &mut R) -> io::Result<Self> {
		(0..N)
			.map(|_| T::from_read(r))
			.collect::<io::Result<Vec<T>>>()
			.map(|vec| vec.try_into().unwrap_or_else(|_| panic!()))
	}
}

impl FromRead for CString {
	fn from_read<R: Read>(r: &mut R) -> io::Result<Self> {
		let bytes: Vec<u8> = r
			.bytes()
			.take_while(|b| b.as_ref().map_or(0, |b| *b) != 0)
			.collect::<io::Result<_>>()?;
		Ok(CString::new(bytes).unwrap())
	}
}

pub struct GetFromReadIter<'a, T: FromRead, R: Read + Seek> {
	reader: &'a mut R,
	start_err: Option<io::Error>,
	_data: PhantomData<T>,
}

impl<T: FromRead, R: Read + Seek> Iterator for GetFromReadIter<'_, T, R> {
	type Item = io::Result<T>;

	fn next(&mut self) -> Option<Self::Item> {
		if self.start_err.is_some() {
			return Some(Err(self.start_err.take().unwrap()));
		}

		let mut probe = [0u8];
		let result = self.reader.read(&mut probe);
		match result {
			Ok(0) => return None,
			Ok(n) => {
				self.reader
					.seek(SeekFrom::Current(-(TryInto::<i64>::try_into(n).unwrap())))
					.unwrap();
			}
			Err(ref e) if e.kind() == ErrorKind::Interrupted => {}
			Err(e) => return Some(Err(e)),
		}

		Some(T::from_read(&mut self.reader))
	}
}

pub trait GetFromRead: Read + Seek + Sized {
	fn get<T: FromRead>(&mut self) -> io::Result<T> {
		FromRead::from_read(self)
	}
	fn get_at<T: FromRead, I: Into<u64>>(&mut self, pos: I) -> io::Result<T> {
		self.seek(SeekFrom::Start(pos.into()))?;
		FromRead::from_read(self)
	}
	fn get_iter<T: FromRead>(&mut self) -> GetFromReadIter<T, Self> {
		GetFromReadIter {
			reader: self,
			start_err: None,
			_data: PhantomData,
		}
	}
	fn get_iter_at<T: FromRead, I: Into<u64>>(&mut self, pos: I) -> GetFromReadIter<T, Self> {
		let start_err = self.seek(SeekFrom::Start(pos.into()));
		GetFromReadIter {
			reader: self,
			start_err: start_err.err(),
			_data: PhantomData,
		}
	}
	fn get_vec<T: FromRead, L: Into<u64>>(&mut self, len: L) -> io::Result<Vec<T>> {
		self.get_iter()
			.take(Into::<u64>::into(len).try_into().unwrap())
			.collect()
	}
	fn get_vec_at<T: FromRead, I: Into<u64>, L: Into<u64>>(
		&mut self,
		pos: I,
		len: L,
	) -> io::Result<Vec<T>> {
		self.get_iter_at(pos)
			.take(Into::<u64>::into(len).try_into().unwrap())
			.collect()
	}
}

impl<T: Read + Seek> GetFromRead for T {}

pub struct ClassicFS {
	hwc_data: PathBuf,
}

impl ClassicFS {
	pub fn load<P: AsRef<Path>>(&self, path: P) -> io::Result<impl GetFromRead> {
		File::open(self.hwc_data.join(path))
	}
}

impl Default for ClassicFS {
	fn default() -> Self {
		Self {
			hwc_data: std::env::var_os("HWC_DATA").unwrap().into(),
		}
	}
}
