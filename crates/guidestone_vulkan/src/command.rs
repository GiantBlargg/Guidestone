use ash::{vk, Device};

use crate::{device::Queue, render::Context};

#[must_use]
pub struct Recorder<'a, const COUNT: usize> {
	index: usize,
	buffer: vk::CommandBuffer,
	pool: &'a mut Pool<COUNT>,
	device: &'a Device,
}

impl<'a, const COUNT: usize> Recorder<'a, COUNT> {
	pub fn end(self) -> &'a mut Pool<COUNT> {
		let pool = self.pool as *mut Pool<COUNT>;
		drop(self);
		unsafe { &mut *pool }
	}

	pub fn wait(&mut self, sem: vk::SemaphoreSubmitInfo<'static>) {
		self.pool.buffers[self.index & COUNT]
			.wait_semaphores
			.push(sem);
	}

	pub fn signal(&mut self, sem: vk::SemaphoreSubmitInfo<'static>) {
		self.pool.buffers[self.index & COUNT]
			.signal_semaphores
			.push(sem);
	}
}

impl<'a, const COUNT: usize> Drop for Recorder<'a, COUNT> {
	fn drop(&mut self) {
		self.pool.queue_submit(self.device, self.index);
	}
}

struct BufferData {
	pool: vk::CommandPool,
	buffer: vk::CommandBuffer,
	fence: vk::Fence,
	wait_semaphores: Vec<vk::SemaphoreSubmitInfo<'static>>,
	signal_semaphores: Vec<vk::SemaphoreSubmitInfo<'static>>,
}
pub struct Pool<const COUNT: usize> {
	next_record: usize,
	next_queue: usize,
	next_submit: usize,
	buffers: [BufferData; COUNT],
}

impl<const COUNT: usize> Pool<COUNT> {
	pub fn new(device: &Device, queue: &Queue) -> Self {
		let buffers = std::array::from_fn(|_| unsafe {
			let pool = device
				.create_command_pool(
					&vk::CommandPoolCreateInfo {
						queue_family_index: queue.family,
						..Default::default()
					},
					None,
				)
				.unwrap();
			let buffer = device
				.allocate_command_buffers(&vk::CommandBufferAllocateInfo {
					command_pool: pool,
					level: vk::CommandBufferLevel::PRIMARY,
					command_buffer_count: 1,
					..Default::default()
				})
				.unwrap()[0];
			let fence = device
				.create_fence(
					&vk::FenceCreateInfo {
						flags: vk::FenceCreateFlags::SIGNALED,
						..Default::default()
					},
					None,
				)
				.unwrap();
			BufferData {
				pool,
				buffer,
				fence,
				wait_semaphores: Vec::with_capacity(1),
				signal_semaphores: Vec::with_capacity(1),
			}
		});
		Self {
			next_record: 0,
			next_queue: 0,
			next_submit: 0,
			buffers,
		}
	}

	pub fn record<'a>(&'a mut self, device: &'a Device) -> Recorder<COUNT> {
		let index = self.next_record;
		self.next_record += 1;

		// Make sure that the prev recorder has queued
		assert!(index == self.next_queue);
		// Make sure that this buffer has been submitted
		assert!(index - COUNT < self.next_submit);

		let BufferData {
			pool,
			buffer,
			fence,
			wait_semaphores: _,
			signal_semaphores: _,
		} = self.buffers[index & COUNT];
		unsafe {
			device.wait_for_fences(&[fence], false, u64::MAX).unwrap();
			device.reset_fences(&[fence]).unwrap();
			device
				.reset_command_pool(pool, vk::CommandPoolResetFlags::empty())
				.unwrap();
			device
				.begin_command_buffer(
					buffer,
					&vk::CommandBufferBeginInfo {
						flags: vk::CommandBufferUsageFlags::ONE_TIME_SUBMIT,
						..Default::default()
					},
				)
				.unwrap();
		}

		Recorder {
			index,
			buffer,
			pool: self,
			device,
		}
	}

	// Called by Recorder
	pub fn queue_submit(&mut self, device: &Device, index: usize) {
		assert!(index == self.next_queue);

		unsafe {
			device
				.end_command_buffer(self.buffers[self.next_queue % COUNT].buffer)
				.unwrap();
		}
	}

	pub fn submit(&mut self, device: &Device, queue: &Queue) {
		while self.next_submit < self.next_queue {
			let BufferData {
				pool: _,
				buffer,
				fence,
				wait_semaphores,
				signal_semaphores,
			} = &self.buffers[self.next_submit & COUNT];
			self.next_submit += 1;

			let cmd_info = [vk::CommandBufferSubmitInfo {
				command_buffer: *buffer,
				..Default::default()
			}];

			let submit = [vk::SubmitInfo2::default()
				.command_buffer_infos(&cmd_info)
				.wait_semaphore_infos(wait_semaphores)
				.signal_semaphore_infos(signal_semaphores)];

			unsafe { device.queue_submit2(queue.queue, &submit, *fence).unwrap() };
		}
	}

	pub unsafe fn destroy(&mut self, ctx: &Context) {
		for BufferData { pool, fence, .. } in &self.buffers {
			ctx.device.device.destroy_command_pool(*pool, None);
			ctx.device.device.destroy_fence(*fence, None);
		}
	}
}
