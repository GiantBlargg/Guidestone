use std::{mem::size_of, num::NonZeroU64, slice};

use guidestone_core::{
	math::{Mat4, UVec2},
	model::{ModelCache, Vertex},
	FrameInfo, Renderer,
};
use raw_window_handle::{HasRawDisplayHandle, HasRawWindowHandle};
use wgpu::{
	util::DeviceExt, BindGroup, Buffer, BufferUsages, Device, PresentMode, Queue, RenderPipeline,
	ShaderStages, Surface, TextureFormat, TextureUsages, TextureViewDescriptor,
};

struct Assets {
	vertex_buffer: Buffer,

	model_cache: ModelCache,
}

#[repr(C)]
struct ViewUniform {
	camera: Mat4,
}

pub struct Render {
	surface: Surface,
	device: Device,
	queue: Queue,

	surface_size: UVec2,
	surface_format: TextureFormat,
	present_mode: PresentMode,

	assets: Option<Assets>,
	default_pipeline: RenderPipeline,

	uniform_buffer: Buffer,
	uniform_bind: BindGroup,
}

impl Render {
	pub async fn new<W: HasRawWindowHandle + HasRawDisplayHandle>(window: &W) -> Self {
		let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
			backends: wgpu::Backends::PRIMARY,
			..Default::default()
		});

		let surface = unsafe { instance.create_surface(window).unwrap() };

		let adapter = instance
			.request_adapter(&wgpu::RequestAdapterOptions {
				compatible_surface: Some(&surface),
				..Default::default()
			})
			.await
			.unwrap();

		let (device, queue) = adapter
			.request_device(&Default::default(), None)
			.await
			.unwrap();

		let caps = surface.get_capabilities(&adapter);
		let surface_format = caps
			.formats
			.into_iter()
			.find(|format| {
				matches!(
					format,
					TextureFormat::Rgba8UnormSrgb | TextureFormat::Bgra8UnormSrgb
				)
			})
			.unwrap();
		let present_mode = caps
			.present_modes
			.into_iter()
			.max_by_key(|present_mode| match present_mode {
				PresentMode::AutoVsync => 1,
				PresentMode::Mailbox => 0,
				_ => 0,
			})
			.unwrap();

		let camera_bind = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
			label: None,
			entries: &[wgpu::BindGroupLayoutEntry {
				binding: 0,
				visibility: ShaderStages::VERTEX,
				ty: wgpu::BindingType::Buffer {
					ty: wgpu::BufferBindingType::Uniform,
					has_dynamic_offset: false,
					min_binding_size: NonZeroU64::new(64),
				},
				count: None,
			}],
		});
		let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
			label: None,
			bind_group_layouts: &[&camera_bind],
			push_constant_ranges: &[],
		});

		let default_pipeline = {
			let shaders = device.create_shader_module(wgpu::ShaderModuleDescriptor {
				label: None,
				source: wgpu::ShaderSource::Wgsl(include_str!("../shaders.wgsl").into()),
			});

			use wgpu::VertexAttribute;

			let vertex_attrib = [
				VertexAttribute {
					format: wgpu::VertexFormat::Float32x3,
					offset: 0,
					shader_location: 0,
				},
				VertexAttribute {
					format: wgpu::VertexFormat::Float32x3,
					offset: 12,
					shader_location: 1,
				},
				VertexAttribute {
					format: wgpu::VertexFormat::Float32x2,
					offset: 24,
					shader_location: 2,
				},
			];
			let vertex_buffer_layout = [wgpu::VertexBufferLayout {
				array_stride: size_of::<Vertex>() as u64,
				step_mode: wgpu::VertexStepMode::Vertex,
				attributes: &vertex_attrib,
			}];
			let vertex = wgpu::VertexState {
				module: &shaders,
				entry_point: "vert",
				buffers: &vertex_buffer_layout,
			};

			let targets = [Some(wgpu::ColorTargetState {
				format: surface_format,
				blend: None,
				write_mask: wgpu::ColorWrites::all(),
			})];
			let fragment = wgpu::FragmentState {
				module: &shaders,
				entry_point: "frag",
				targets: &targets,
			};

			let primitive = wgpu::PrimitiveState {
				topology: wgpu::PrimitiveTopology::TriangleList,
				front_face: wgpu::FrontFace::Ccw,
				cull_mode: None,
				polygon_mode: wgpu::PolygonMode::Fill,
				..Default::default()
			};
			device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
				label: None,
				layout: Some(&pipeline_layout),
				vertex,
				primitive,
				depth_stencil: None,
				multisample: Default::default(),
				fragment: Some(fragment),
				multiview: None,
			})
		};

		let uniform_buffer = device.create_buffer(&wgpu::BufferDescriptor {
			label: None,
			size: size_of::<ViewUniform>() as u64,
			usage: BufferUsages::COPY_DST | BufferUsages::UNIFORM,
			mapped_at_creation: false,
		});
		let uniform_bind = device.create_bind_group(&wgpu::BindGroupDescriptor {
			label: None,
			layout: &camera_bind,
			entries: &[wgpu::BindGroupEntry {
				binding: 0,
				resource: uniform_buffer.as_entire_binding(),
			}],
		});

		Self {
			surface,
			device,
			queue,
			surface_size: UVec2::default(),
			surface_format,
			present_mode,
			assets: None,
			default_pipeline,
			uniform_buffer,
			uniform_bind,
		}
	}
}

impl Renderer for Render {
	fn render_frame(&mut self, frame: FrameInfo) {
		if let Some(size) = frame.resize {
			if size != self.surface_size {
				self.surface.configure(
					&self.device,
					&wgpu::SurfaceConfiguration {
						usage: TextureUsages::RENDER_ATTACHMENT,
						format: self.surface_format,
						width: size.x,
						height: size.y,
						present_mode: self.present_mode,
						alpha_mode: wgpu::CompositeAlphaMode::Opaque,
						view_formats: Vec::new(),
					},
				);
				self.surface_size = size;
			}
		}

		{
			let aspect = (self.surface_size.x as f32) / (self.surface_size.y as f32);
			let proj = Mat4::perspective(frame.camera.fov, aspect, frame.camera.near);
			let view = Mat4::look_at(frame.camera.eye, frame.camera.target, frame.camera.up);
			let view_uniform = ViewUniform {
				camera: proj * view,
			};
			self.queue.write_buffer(&self.uniform_buffer, 0, unsafe {
				struct_to_bytes(&view_uniform)
			});
		}

		let mut command_encoder = self.device.create_command_encoder(&Default::default());

		let surface_texture = self.surface.get_current_texture().unwrap();
		let texture = &surface_texture.texture;
		let view = texture.create_view(&TextureViewDescriptor {
			..Default::default()
		});
		{
			let mut render_pass = command_encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
				color_attachments: &[Some(wgpu::RenderPassColorAttachment {
					view: &view,
					resolve_target: None,
					ops: wgpu::Operations {
						load: wgpu::LoadOp::Clear(wgpu::Color::BLACK),
						store: true,
					},
				})],
				depth_stencil_attachment: None,
				..Default::default()
			});

			render_pass.set_bind_group(0, &self.uniform_bind, &[]);

			if let Some(assets) = &self.assets {
				render_pass.set_vertex_buffer(0, assets.vertex_buffer.slice(..));

				render_pass.set_pipeline(&self.default_pipeline);

				for mesh in &assets.model_cache.models.first().unwrap().meshes {
					render_pass.draw(
						mesh.first_vertex..(mesh.first_vertex + mesh.num_vertices),
						0..1,
					)
				}
			}
		}

		self.queue.submit([command_encoder.finish()]);

		surface_texture.present();
	}

	fn set_model_cache(&mut self, model_cache: ModelCache) {
		let vertex_bytes = unsafe { vec_to_bytes(&model_cache.vertices) };
		let vertex_buffer = self
			.device
			.create_buffer_init(&wgpu::util::BufferInitDescriptor {
				label: None,
				contents: vertex_bytes,
				usage: BufferUsages::VERTEX,
			});

		self.assets = Some(Assets {
			vertex_buffer,
			model_cache,
		});
	}
}

unsafe fn vec_to_bytes<T>(vec: &Vec<T>) -> &[u8] {
	slice::from_raw_parts(vec.as_ptr().cast(), vec.len() * size_of::<T>())
}

unsafe fn struct_to_bytes<T>(value: &T) -> &[u8] {
	slice::from_raw_parts((value as *const T).cast(), size_of::<T>())
}
