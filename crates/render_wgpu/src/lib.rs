use std::{mem::size_of, num::NonZeroU64, slice};

use guidestone_core::{
	math::{Mat4, UVec2, Vec4},
	model::{CachedModel, ModelCache, Vertex},
	FrameInfo, RenderItem, Renderer,
};
use raw_window_handle::{HasRawDisplayHandle, HasRawWindowHandle};
use wgpu::{
	include_wgsl,
	util::{BufferInitDescriptor, DeviceExt},
	BindGroup, BindGroupDescriptor, BindGroupEntry, BindGroupLayout, Buffer, BufferDescriptor,
	BufferUsages, Device, Extent3d, PresentMode, Queue, RenderPipeline, ShaderStages, Surface,
	Texture, TextureDescriptor, TextureDimension, TextureFormat, TextureUsages,
};

struct Assets {
	vertex_buffer: Buffer,
	materials: Vec<BindGroup>,

	models: Vec<CachedModel>,
}

#[repr(C)]
struct ViewUniform {
	camera: Mat4,
}

struct SceneItem {
	model: u32,
	data_offset: u32,
}

pub struct Render {
	surface: Surface,
	device: Device,
	queue: Queue,

	surface_size: UVec2,
	surface_format: TextureFormat,
	present_mode: PresentMode,
	depth_buffer: Texture,

	assets: Option<Assets>,
	default_pipeline: RenderPipeline,
	material_bind_layout: BindGroupLayout,

	uniform_buffer: Buffer,
	uniform_bind: BindGroup,

	scene_bind_layout: BindGroupLayout,
	scene_bind: BindGroup,
	scene_buffer: Buffer,
	scene_items: Vec<SceneItem>,
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

		use wgpu::{BindGroupLayoutDescriptor, BindGroupLayoutEntry, BindingType};

		let global_bind = device.create_bind_group_layout(&BindGroupLayoutDescriptor {
			label: None,
			entries: &[
				BindGroupLayoutEntry {
					binding: 0,
					visibility: ShaderStages::VERTEX,
					ty: BindingType::Buffer {
						ty: wgpu::BufferBindingType::Uniform,
						has_dynamic_offset: false,
						min_binding_size: NonZeroU64::new(64),
					},
					count: None,
				},
				BindGroupLayoutEntry {
					binding: 1,
					visibility: ShaderStages::FRAGMENT,
					ty: BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
					count: None,
				},
			],
		});

		let material_bind_layout = device.create_bind_group_layout(&BindGroupLayoutDescriptor {
			label: None,
			entries: &[BindGroupLayoutEntry {
				binding: 0,
				visibility: ShaderStages::FRAGMENT,
				ty: BindingType::Texture {
					sample_type: wgpu::TextureSampleType::Float { filterable: true },
					view_dimension: wgpu::TextureViewDimension::D2,
					multisampled: false,
				},
				count: None,
			}],
		});

		let scene_bind_layout = device.create_bind_group_layout(&BindGroupLayoutDescriptor {
			label: None,
			entries: &[BindGroupLayoutEntry {
				binding: 0,
				visibility: ShaderStages::VERTEX,
				ty: BindingType::Buffer {
					ty: wgpu::BufferBindingType::Uniform,
					has_dynamic_offset: true,
					min_binding_size: NonZeroU64::new(64),
				},
				count: None,
			}],
		});
		let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
			label: None,
			bind_group_layouts: &[&global_bind, &material_bind_layout, &scene_bind_layout],
			push_constant_ranges: &[],
		});

		let default_pipeline = {
			let shaders = device.create_shader_module(include_wgsl!("../shaders.wgsl"));

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

			let depth_stencil = wgpu::DepthStencilState {
				format: TextureFormat::Depth24Plus,
				depth_write_enabled: true,
				depth_compare: wgpu::CompareFunction::Greater,
				stencil: Default::default(),
				bias: Default::default(),
			};

			device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
				label: None,
				layout: Some(&pipeline_layout),
				vertex,
				primitive,
				depth_stencil: Some(depth_stencil),
				multisample: Default::default(),
				fragment: Some(fragment),
				multiview: None,
			})
		};

		let uniform_buffer = device.create_buffer(&BufferDescriptor {
			label: None,
			size: size_of::<ViewUniform>() as u64,
			usage: BufferUsages::COPY_DST | BufferUsages::UNIFORM,
			mapped_at_creation: false,
		});
		let address_mode = wgpu::AddressMode::Repeat;
		let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
			address_mode_u: address_mode,
			address_mode_v: address_mode,
			mag_filter: wgpu::FilterMode::Nearest,
			min_filter: wgpu::FilterMode::Nearest,
			..Default::default()
		});
		let uniform_bind = device.create_bind_group(&BindGroupDescriptor {
			label: None,
			layout: &global_bind,
			entries: &[
				BindGroupEntry {
					binding: 0,
					resource: uniform_buffer.as_entire_binding(),
				},
				BindGroupEntry {
					binding: 1,
					resource: wgpu::BindingResource::Sampler(&sampler),
				},
			],
		});

		// Place holder depth buffer, will get replaced before it's used
		let depth_buffer = device.create_texture(&TextureDescriptor {
			label: None,
			size: Extent3d {
				width: 1,
				height: 1,
				depth_or_array_layers: 1,
			},
			mip_level_count: 1,
			sample_count: 1,
			dimension: TextureDimension::D2,
			format: TextureFormat::Depth24Plus,
			usage: TextureUsages::RENDER_ATTACHMENT,
			view_formats: &[],
		});
		let scene_buffer = device.create_buffer(&BufferDescriptor {
			label: None,
			size: 1000 * size_of::<Mat4>() as u64,
			usage: BufferUsages::UNIFORM,
			mapped_at_creation: false,
		});
		let scene_bind = device.create_bind_group(&BindGroupDescriptor {
			label: None,
			layout: &scene_bind_layout,
			entries: &[BindGroupEntry {
				binding: 0,
				resource: scene_buffer.as_entire_binding(),
			}],
		});

		Self {
			surface,
			device,
			queue,
			surface_size: UVec2::default(),
			surface_format,
			present_mode,
			depth_buffer,
			assets: None,
			default_pipeline,
			material_bind_layout,
			uniform_buffer,
			uniform_bind,
			scene_bind_layout,
			scene_bind,
			scene_buffer,
			scene_items: Vec::new(),
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
				self.depth_buffer = self.device.create_texture(&TextureDescriptor {
					label: None,
					size: Extent3d {
						width: size.x,
						height: size.y,
						depth_or_array_layers: 1,
					},
					mip_level_count: 1,
					sample_count: 1,
					dimension: TextureDimension::D2,
					format: TextureFormat::Depth24Plus,
					usage: TextureUsages::RENDER_ATTACHMENT,
					view_formats: &[],
				});
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
		let surface_view = texture.create_view(&Default::default());
		let depth_view = self.depth_buffer.create_view(&Default::default());
		{
			use wgpu::{LoadOp, Operations, StoreOp};
			let mut render_pass = command_encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
				color_attachments: &[Some(wgpu::RenderPassColorAttachment {
					view: &surface_view,
					resolve_target: None,
					ops: Operations {
						load: LoadOp::Clear(wgpu::Color::BLACK),
						store: StoreOp::Store,
					},
				})],
				depth_stencil_attachment: Some(wgpu::RenderPassDepthStencilAttachment {
					view: &depth_view,
					depth_ops: Some(Operations {
						load: LoadOp::Clear(0.0),
						store: StoreOp::Store,
					}),
					stencil_ops: None,
				}),
				..Default::default()
			});

			if let Some(assets) = &self.assets {
				render_pass.set_pipeline(&self.default_pipeline);
				render_pass.set_bind_group(0, &self.uniform_bind, &[]);

				render_pass.set_vertex_buffer(0, assets.vertex_buffer.slice(..));

				for item in &self.scene_items {
					render_pass.set_bind_group(2, &self.scene_bind, &[item.data_offset]);
					for mesh in &assets.models[item.model as usize].meshes {
						render_pass.set_bind_group(
							1,
							&assets.materials[mesh.material as usize],
							&[],
						);
						render_pass.draw(
							mesh.first_vertex..(mesh.first_vertex + mesh.num_vertices),
							0..1,
						)
					}
				}
			}
		}

		self.queue.submit([command_encoder.finish()]);

		surface_texture.present();
	}

	fn set_model_cache(&mut self, model_cache: ModelCache) {
		let vertex_bytes = unsafe { vec_to_bytes(&model_cache.vertices) };
		let vertex_buffer = self.device.create_buffer_init(&BufferInitDescriptor {
			label: None,
			contents: vertex_bytes,
			usage: BufferUsages::VERTEX,
		});

		let textures: Vec<Texture> = model_cache
			.textures
			.into_iter()
			.map(|texture| {
				let texture_bytes = unsafe { vec_to_bytes(&texture.rgba) };
				self.device.create_texture_with_data(
					&self.queue,
					&TextureDescriptor {
						label: None,
						size: Extent3d {
							width: texture.size.x,
							height: texture.size.y,
							depth_or_array_layers: 1,
						},
						mip_level_count: 1,
						sample_count: 1,
						dimension: TextureDimension::D2,
						format: TextureFormat::Rgba8UnormSrgb,
						usage: TextureUsages::TEXTURE_BINDING,
						view_formats: &[],
					},
					texture_bytes,
				)
			})
			.collect();

		let materials = model_cache
			.materials
			.into_iter()
			.map(|material| {
				self.device.create_bind_group(&BindGroupDescriptor {
					label: None,
					layout: &self.material_bind_layout,
					entries: &[BindGroupEntry {
						binding: 0,
						resource: wgpu::BindingResource::TextureView(
							&textures[material.texture as usize].create_view(&Default::default()),
						),
					}],
				})
			})
			.collect();

		self.assets = Some(Assets {
			vertex_buffer,
			materials,
			models: model_cache.models,
		});
	}

	fn set_render_list(&mut self, render_list: Vec<RenderItem>) {
		let mut scene_buffer = Vec::new();
		let mut items = Vec::new();

		for item in render_list {
			let offset = scene_buffer.len() as u32;
			let mat = Mat4::from([
				Vec4::from((item.rotation[0], 0.0)),
				Vec4::from((item.rotation[1], 0.0)),
				Vec4::from((item.rotation[2], 0.0)),
				Vec4::from((item.position, 1.0)),
			]);
			items.push(SceneItem {
				model: item.model,
				data_offset: offset,
			});
			scene_buffer.extend_from_slice(unsafe { struct_to_bytes(&mat) });
		}

		if scene_buffer.len() as u64 > self.scene_buffer.size() {
			self.scene_buffer = self.device.create_buffer_init(&BufferInitDescriptor {
				label: None,
				contents: &scene_buffer,
				usage: self.scene_buffer.usage(),
			});
			self.scene_bind = self.device.create_bind_group(&BindGroupDescriptor {
				label: None,
				layout: &self.scene_bind_layout,
				entries: &[BindGroupEntry {
					binding: 0,
					resource: self.scene_buffer.as_entire_binding(),
				}],
			});
		} else {
			self.queue
				.write_buffer(&self.scene_buffer, 0, &scene_buffer);
		}

		self.scene_items = items;
	}
}

unsafe fn vec_to_bytes<T>(vec: &Vec<T>) -> &[u8] {
	slice::from_raw_parts(vec.as_ptr().cast(), vec.len() * size_of::<T>())
}

unsafe fn struct_to_bytes<T>(value: &T) -> &[u8] {
	slice::from_raw_parts((value as *const T).cast(), size_of::<T>())
}
