use std::ffi::{c_char, c_void, CStr};

use ash::{
	extensions::{ext::DebugUtils, khr::Surface},
	vk, Entry, Instance, LoadingError,
};

pub struct InstanceContext {
	pub entry: Entry,
	pub instance: Instance,
	debug: Option<(DebugUtils, vk::DebugUtilsMessengerEXT)>,
	pub surface_fn: Surface,
}

impl InstanceContext {
	pub fn load(extensions: Vec<*const c_char>) -> Result<Self, LoadingError> {
		Ok(Self::new(unsafe { Entry::load()? }, extensions))
	}
	pub fn new(entry: Entry, mut extensions: Vec<*const c_char>) -> Self {
		let debug_messenger_create_info = vk::DebugUtilsMessengerCreateInfoEXT {
			message_severity: vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE
				| vk::DebugUtilsMessageSeverityFlagsEXT::INFO
				| vk::DebugUtilsMessageSeverityFlagsEXT::WARNING
				| vk::DebugUtilsMessageSeverityFlagsEXT::ERROR,
			message_type: vk::DebugUtilsMessageTypeFlagsEXT::GENERAL
				| vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION
				| vk::DebugUtilsMessageTypeFlagsEXT::PERFORMANCE,
			pfn_user_callback: Some(Self::debug_callback),
			..Default::default()
		};

		let debug = {
			if cfg!(debug_assertions) {
				let can_debug = unsafe { entry.enumerate_instance_extension_properties(None) }
					.unwrap()
					.into_iter()
					.any(|ext| {
						(unsafe { CStr::from_ptr(ext.extension_name.as_ptr()) } == DebugUtils::NAME)
					});
				if !can_debug {
					log::warn!("Vulkan logging not available");
				}
				can_debug
			} else {
				false
			}
		};

		if debug {
			extensions.push(DebugUtils::NAME.as_ptr());
		}

		let instance = {
			let app_info = vk::ApplicationInfo::default().api_version(vk::API_VERSION_1_3);
			let mut debug_messenger_info = debug_messenger_create_info;
			let create_info = vk::InstanceCreateInfo::default()
				.application_info(&app_info)
				.enabled_extension_names(&extensions)
				.push_next(&mut debug_messenger_info);

			unsafe { entry.create_instance(&create_info, None) }.unwrap()
		};

		let debug = if debug {
			let debug_utils = DebugUtils::new(&entry, &instance);
			let debug_messenger = unsafe {
				debug_utils
					.create_debug_utils_messenger(&debug_messenger_create_info, None)
					.unwrap()
			};
			Some((debug_utils, debug_messenger))
		} else {
			None
		};

		let surface_fn = Surface::new(&entry, &instance);

		Self {
			entry,
			instance,
			debug,
			surface_fn,
		}
	}

	extern "system" fn debug_callback(
		message_severity: vk::DebugUtilsMessageSeverityFlagsEXT,
		message_types: vk::DebugUtilsMessageTypeFlagsEXT,
		p_callback_data: *const vk::DebugUtilsMessengerCallbackDataEXT,
		_: *mut c_void,
	) -> u32 {
		let callback_data = unsafe { *p_callback_data };
		// let message_id_name = if callback_data.p_message_id_name.is_null() {
		// 	None
		// } else {
		// 	Some(unsafe { CStr::from_ptr(callback_data.p_message_id_name) })
		// };
		let message = unsafe { CStr::from_ptr(callback_data.p_message) };

		match callback_data.message_id_number as u32 {
			// UNASSIGNED-BestPractices-CreateImage-Depth32Format
			// AMD is fine with 32-bit Depth Buffers
			0x53bb41ae => return 0,

			// UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension-debugging
			// Debugging is disabled in Release
			0x822806fa => return 0,

			// UNASSIGNED-BestPractices-ImageBarrierAccessLayout
			// This warning incorrectly reports usage of new VkAccessFlagBits2
			0x849fcec7 => return 0,

			_ => (),
		}

		use log::{log, Level};

		let severity = match message_severity {
			vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE => Level::Debug,
			vk::DebugUtilsMessageSeverityFlagsEXT::INFO => Level::Info,
			vk::DebugUtilsMessageSeverityFlagsEXT::WARNING => Level::Warn,
			vk::DebugUtilsMessageSeverityFlagsEXT::ERROR => Level::Error,
			_ => Level::Error,
		};
		let message_type = match message_types {
			vk::DebugUtilsMessageTypeFlagsEXT::GENERAL => "vk::General",
			vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION => "vk::Validation",
			vk::DebugUtilsMessageTypeFlagsEXT::PERFORMANCE => "vk::Performance",
			vk::DebugUtilsMessageTypeFlagsEXT::DEVICE_ADDRESS_BINDING => "vk::DeviceAddressBinding",
			_ => "vk::Unknown",
		};

		log!(target: message_type, severity, "{}", message.to_string_lossy());

		0
	}
}

impl Drop for InstanceContext {
	fn drop(&mut self) {
		if let Some((debug_utils, debug_messenger)) = self.debug.take() {
			unsafe { debug_utils.destroy_debug_utils_messenger(debug_messenger, None) };
		}

		unsafe { self.instance.destroy_instance(None) };
	}
}
