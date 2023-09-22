use ash::vk;
use guidestone_core::{math::Mat4, Camera, FrameInfo, Renderer};

use crate::{command, device::DeviceContext, framebuffer::Framebuffer, instance::InstanceContext};

pub struct Context {
	pub device: DeviceContext,
	pub instance: InstanceContext,
}

pub struct Render {
	render_cmd: command::Pool<3>,
	framebuffer: Framebuffer<3>,
	ctx: Context,
}

impl Render {
	pub fn new(instance: InstanceContext, surface: vk::SurfaceKHR) -> Self {
		let device = DeviceContext::new(&instance, surface);
		let framebuffer = Framebuffer::new(&device, surface);
		let render_cmd = command::Pool::<3>::new(&device.device, &device.queue);
		Self {
			render_cmd,
			framebuffer,
			ctx: Context { device, instance },
		}
	}
}

impl Renderer for Render {
	fn render_frame(&mut self, frame: FrameInfo) {
		if let Some(size) = frame.resize {
			self.framebuffer.resize(&self.ctx, size);
		}
		{
			let camera = Camera::default();
			let proj = Mat4::perspective(camera.fov, 1.0, camera.near);
			let view = Mat4::look_at(camera.eye, camera.target, camera.up);
			let _mat = proj * view;
		}
	}
}

impl Drop for Render {
	fn drop(&mut self) {
		unsafe {
			self.render_cmd.destroy(&self.ctx);
			self.framebuffer.destroy(&self.ctx);
		}
	}
}
