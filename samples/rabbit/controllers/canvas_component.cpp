#include "canvas_component.h"
#include "rabbit_behaviour.h"
#include <octoon/camera_component.h>
#include <octoon/image/image.h>
#include <octoon/hal/graphics.h>

namespace rabbit
{
	CanvasComponent::CanvasComponent() noexcept
		: active_(false)
	{
	}

	CanvasComponent::~CanvasComponent() noexcept
	{
	}

	void
	CanvasComponent::setActive(bool active) noexcept
	{
		active_ = active;
	}

	bool
	CanvasComponent::getActive() const noexcept
	{
		return active_;
	}

	void
	CanvasComponent::onEnable() noexcept
	{
	}

	void
	CanvasComponent::onDisable() noexcept
	{
	}

	void
	CanvasComponent::onPostProcess() noexcept
	{
		auto& context = this->getContext();

		auto camera = context->profile->entitiesModule->camera->getComponent<octoon::CameraComponent>();
		auto colorTexture = camera->getFramebuffer()->getFramebufferDesc().getColorAttachments().front().getBindingTexture();
		if (colorTexture)
		{
			auto& desc = colorTexture->getTextureDesc();

			void* data = nullptr;
			if (colorTexture->map(0, 0, desc.getWidth(), desc.getHeight(), 0, &data))
			{
				std::memcpy(this->getModel()->outputBuffer.data(), data, desc.getWidth() * desc.getHeight() * 3 * sizeof(float));
				colorTexture->unmap();
			}
		}
	}

	void
	CanvasComponent::save(std::string_view filepath) noexcept
	{
		auto canvas = this->getContext()->profile->canvasModule;
		auto width = canvas->width;
		auto height = canvas->height;
		auto output = canvas->outputBuffer.data();

		octoon::image::Image image;

		if (image.create(octoon::image::Format::R8G8B8SRGB, width, height))
		{
			auto data = (char*)image.data();

#			pragma omp parallel for num_threads(4)
			for (std::int32_t y = 0; y < height; y++)
			{
				for (std::uint32_t x = 0; x < width; x++)
				{
					auto src = y * width + x;
					auto dst = ((height - y - 1) * width + x) * 3;

					data[dst] = (std::uint8_t)(output[src].x * 255);
					data[dst + 1] = (std::uint8_t)(output[src].y * 255);
					data[dst + 2] = (std::uint8_t)(output[src].z * 255);
				}
			}

			image.save(std::string(filepath), "png");
		}
	}
}