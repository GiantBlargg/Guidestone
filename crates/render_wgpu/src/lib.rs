use guidestone_core::{FrameInfo, Renderer};
use raw_window_handle::{HasRawDisplayHandle, HasRawWindowHandle};
use wgpu::{
	Adapter, Backends, Color, CompositeAlphaMode, Device, Instance, InstanceDescriptor,
	PresentMode, Queue, RenderPassColorAttachment, RenderPassDescriptor, RequestAdapterOptions,
	Surface, SurfaceConfiguration, TextureFormat, TextureUsages, TextureViewDescriptor,
};

pub struct Render {
	surface: Surface,
	adapter: Adapter,
	device: Device,
	queue: Queue,
}

impl Render {
	pub async fn new<W: HasRawWindowHandle + HasRawDisplayHandle>(window: &W) -> Self {
		let instance = Instance::new(InstanceDescriptor {
			backends: Backends::PRIMARY,
			..Default::default()
		});

		let surface = unsafe { instance.create_surface(window).unwrap() };

		let adapter = instance
			.request_adapter(&RequestAdapterOptions {
				compatible_surface: Some(&surface),
				..Default::default()
			})
			.await
			.unwrap();

		let (device, queue) = adapter
			.request_device(&Default::default(), None)
			.await
			.unwrap();

		Self {
			surface,
			adapter,
			device,
			queue,
		}
	}
}

impl Renderer for Render {
	fn render_frame(&mut self, frame: FrameInfo) {
		if let Some(size) = frame.resize {
			let caps = self.surface.get_capabilities(&self.adapter);
			let format = caps
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
			self.surface.configure(
				&self.device,
				&SurfaceConfiguration {
					usage: TextureUsages::RENDER_ATTACHMENT,
					format,
					width: size.x,
					height: size.y,
					present_mode,
					alpha_mode: CompositeAlphaMode::Opaque,
					view_formats: Vec::new(),
				},
			);
		}

		let mut command_encoder = self.device.create_command_encoder(&Default::default());

		let surface_texture = self.surface.get_current_texture().unwrap();
		let texture = &surface_texture.texture;
		let view = texture.create_view(&TextureViewDescriptor {
			..Default::default()
		});
		{
			let render_pass = command_encoder.begin_render_pass(&RenderPassDescriptor {
				color_attachments: &[Some(RenderPassColorAttachment {
					view: &view,
					resolve_target: None,
					ops: wgpu::Operations {
						load: wgpu::LoadOp::Clear(Color::BLACK),
						store: true,
					},
				})],
				depth_stencil_attachment: None,
				..Default::default()
			});
		}

		self.queue.submit([command_encoder.finish()]);

		surface_texture.present();
	}
}
