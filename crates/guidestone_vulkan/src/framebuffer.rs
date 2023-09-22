use ash::vk;
use guidestone_core::math::UVec2;

use crate::{command::Recorder, device::DeviceContext, render::Context};

struct Swapchain {
	surface: vk::SurfaceKHR,
	swapchain: vk::SwapchainKHR,
	target_size: UVec2,
	actual_size: UVec2,
}

impl Swapchain {
	fn new(surface: vk::SurfaceKHR) -> Self {
		Self {
			surface,
			swapchain: Default::default(),
			target_size: UVec2::new(0, 0),
			actual_size: UVec2::new(0, 0),
		}
	}

	fn resize(&mut self, size: UVec2) {
		self.target_size = size
	}

	fn recreate(&mut self, ctx: &Context) {
		unsafe {
			ctx.device.device.device_wait_idle().unwrap();
			let caps = ctx
				.instance
				.surface_fn
				.get_physical_device_surface_capabilities(ctx.device.physical_device, self.surface);
		}
	}

	fn acquire_image() {}
}

pub struct Framebuffer<const COUNT: usize> {
	swapchain: Swapchain,
	acquire_semaphores: [vk::Semaphore; COUNT],
	present_semaphores: [vk::Semaphore; COUNT],
}

impl<const COUNT: usize> Framebuffer<COUNT> {
	pub fn new(device: &DeviceContext, surface: vk::SurfaceKHR) -> Self {
		let fill = |_| unsafe {
			device
				.device
				.create_semaphore(&Default::default(), None)
				.unwrap()
		};
		Self {
			swapchain: Swapchain::new(surface),
			acquire_semaphores: std::array::from_fn(fill),
			present_semaphores: std::array::from_fn(fill),
		}
	}

	pub fn resize(&mut self, ctx: &Context, size: UVec2) {}
	pub fn start_rendering(&mut self, ctx: &Context, recorder: &mut Recorder<COUNT>) {}
	pub fn present(&mut self, ctx: &Context, recorder: Recorder<COUNT>) {}

	pub unsafe fn destroy(&mut self, ctx: &Context) {
		for sem in self.acquire_semaphores {
			ctx.device.device.destroy_semaphore(sem, None);
		}
		for sem in self.present_semaphores {
			ctx.device.device.destroy_semaphore(sem, None);
		}
	}
}
