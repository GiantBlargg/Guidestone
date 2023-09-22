use ash::{extensions::khr::Swapchain, vk, Device, Instance};

use crate::instance::InstanceContext;

#[derive(Debug)]
struct PhysicalDevice {
	physical_device: vk::PhysicalDevice,
	properties: vk::PhysicalDeviceProperties,
	features13: vk::PhysicalDeviceVulkan13Features<'static>,
}

impl PhysicalDevice {
	fn get_physical_devices(instance: &Instance) -> Vec<PhysicalDevice> {
		unsafe {
			instance
				.enumerate_physical_devices()
				.unwrap()
				.into_iter()
				.map(|physical_device| {
					let mut features13 = vk::PhysicalDeviceVulkan13Features::default();
					let mut features =
						vk::PhysicalDeviceFeatures2::default().push_next(&mut features13);
					instance.get_physical_device_features2(physical_device, &mut features);

					PhysicalDevice {
						physical_device,
						properties: instance.get_physical_device_properties(physical_device),
						features13,
					}
				})
		}
		.collect()
	}
}

struct DeviceConfig {
	physical_device: vk::PhysicalDevice,
	queue_family: u32,
	formats: FormatConfig,
}
pub struct FormatConfig {
	pub present_mode: vk::PresentModeKHR,
	pub surface_format: vk::SurfaceFormatKHR,
	pub depth_format: vk::Format,
}
impl DeviceConfig {
	#[rustfmt::skip]
	fn is_srgb(sf: &vk::SurfaceFormatKHR) -> bool {
		matches!(sf, vk::SurfaceFormatKHR {
			format:
				vk::Format::R8G8B8_SRGB |
				vk::Format::B8G8R8_SRGB |
				vk::Format::R8G8B8A8_SRGB |
				vk::Format::B8G8R8A8_SRGB |
				vk::Format::A8B8G8R8_SRGB_PACK32,
			color_space: vk::ColorSpaceKHR::SRGB_NONLINEAR,
		})
	}

	fn config(
		context: &InstanceContext,
		surface: vk::SurfaceKHR,
		physical_device: vk::PhysicalDevice,
	) -> Option<DeviceConfig> {
		let queue_family: u32 = {
			(0u32..)
				.zip(unsafe {
					context
						.instance
						.get_physical_device_queue_family_properties(physical_device)
				})
				.filter(|(_, qf)| {
					qf.queue_flags.contains(
						vk::QueueFlags::GRAPHICS
							| vk::QueueFlags::COMPUTE | vk::QueueFlags::TRANSFER,
					)
				})
				.find(|(i, _)| unsafe {
					context
						.surface_fn
						.get_physical_device_surface_support(physical_device, *i, surface)
						.unwrap()
				})?
				.0
		};

		let present_mode = {
			unsafe {
				context
					.surface_fn
					.get_physical_device_surface_present_modes(physical_device, surface)
					.unwrap()
			}
			.into_iter()
			.max_by_key(|mode| match *mode {
				vk::PresentModeKHR::IMMEDIATE => 1,
				vk::PresentModeKHR::MAILBOX => 2, // TODO: Increase to 5 later
				vk::PresentModeKHR::FIFO => 3,
				vk::PresentModeKHR::FIFO_RELAXED => 4,
				_ => 0,
			})?
		};

		let surface_format = {
			unsafe {
				context
					.surface_fn
					.get_physical_device_surface_formats(physical_device, surface)
					.unwrap()
			}
			.into_iter()
			.find(Self::is_srgb)?
		};

		let depth_format = {
			vec![vk::Format::D32_SFLOAT, vk::Format::X8_D24_UNORM_PACK32]
				.into_iter()
				.find(|f| {
					unsafe {
						context
							.instance
							.get_physical_device_format_properties(physical_device, *f)
					}
					.optimal_tiling_features
					.contains(vk::FormatFeatureFlags::DEPTH_STENCIL_ATTACHMENT)
				})?
		};

		Some(Self {
			physical_device,
			queue_family,
			formats: FormatConfig {
				present_mode,
				surface_format,
				depth_format,
			},
		})
	}

	fn build(self, context: &InstanceContext) -> DeviceContext {
		let device = {
			let queue_create = [vk::DeviceQueueCreateInfo::default()
				.queue_family_index(self.queue_family)
				.queue_priorities(&[1.0])];

			let extensions = [Swapchain::NAME.as_ptr()];
			let mut features13 = vk::PhysicalDeviceVulkan13Features {
				dynamic_rendering: 1,
				synchronization2: 1,
				..Default::default()
			};
			let device_create_info = vk::DeviceCreateInfo::default()
				.queue_create_infos(&queue_create)
				.enabled_extension_names(&extensions)
				.push_next(&mut features13);

			unsafe {
				context
					.instance
					.create_device(self.physical_device, &device_create_info, None)
			}
			.unwrap()
		};

		let swapchain = Swapchain::new(&context.instance, &device);

		let queue = Queue {
			family: self.queue_family,
			queue: unsafe { device.get_device_queue(self.queue_family, 0) },
		};

		DeviceContext {
			device,
			swapchain_fn: swapchain,
			physical_device: self.physical_device,
			queue,
			format: self.formats,
		}
	}
}

pub struct Queue {
	pub family: u32,
	pub queue: vk::Queue,
}
pub struct DeviceContext {
	pub device: Device,
	pub swapchain_fn: Swapchain,
	pub physical_device: vk::PhysicalDevice,
	pub queue: Queue,
	pub format: FormatConfig,
}

impl DeviceContext {
	pub(crate) fn new(instance: &InstanceContext, surface: vk::SurfaceKHR) -> Self {
		let physical_devices = PhysicalDevice::get_physical_devices(&instance.instance);

		let configs: Vec<_> = physical_devices
			.into_iter()
			.filter(|pd| pd.properties.api_version >= vk::API_VERSION_1_3)
			.filter(|pd| pd.features13.dynamic_rendering != 0)
			.filter(|pd| pd.features13.synchronization2 != 0)
			.filter_map(|pd| {
				DeviceConfig::config(instance, surface, pd.physical_device).map(|c| (c, pd))
			})
			.collect();

		let config = configs.into_iter().next().unwrap().0;

		config.build(instance)
	}
}

impl Drop for DeviceContext {
	fn drop(&mut self) {
		unsafe { self.device.destroy_device(None) };
	}
}
