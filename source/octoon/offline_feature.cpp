#if defined(OCTOON_FEATURE_VIDEO_ENABLE)
#include <octoon/offline_feature.h>

#include <octoon/input_feature.h>
#include <octoon/input/input_event.h>

#include <octoon/game_app.h>
#include <octoon/game_server.h>

#include <octoon/hal_feature.h>
#include <octoon/hal/graphics.h>
#include <octoon/runtime/except.h>

#include <RadeonProRender.h>
#include <RadeonProRender_GL.h>
#include <Utils/config_manager.h>

#include <GL/GL.h>

namespace octoon
{
	OctoonImplementSubClass(OfflineFeature, GameFeature, "OfflineFeature")

	OfflineFeature::OfflineFeature() noexcept
		: framebuffer_w_(0)
		, framebuffer_h_(0)
		, rprContext_(nullptr)
		, rprScene_(nullptr)
		, rprMaterialSystem_(nullptr)
	{
	}

	OfflineFeature::OfflineFeature(std::uint32_t framebuffer_w, std::uint32_t framebuffer_h) noexcept
		: framebuffer_w_(framebuffer_w)
		, framebuffer_h_(framebuffer_h)
		, rprContext_(nullptr)
		, rprScene_(nullptr)
		, rprMaterialSystem_(nullptr)
	{
	}

	OfflineFeature::~OfflineFeature() noexcept
	{
	}

	void
	OfflineFeature::setFramebufferScale(std::uint32_t w, std::uint32_t h) noexcept
	{
		if (framebuffer_w_ != w || framebuffer_h_ != h)
		{
			framebuffer_w_ = w;
			framebuffer_h_ = h;
		}
	}

	void
	OfflineFeature::getFramebufferScale(std::uint32_t& w, std::uint32_t& h) noexcept
	{
		w = framebuffer_w_;
		h = framebuffer_h_;
	}

	void
	OfflineFeature::setFramebufferDirty(bool dirty) noexcept
	{
		this->dirty_ = dirty;
	}

	bool
	OfflineFeature::getFramebufferDirty() const noexcept
	{
		return this->dirty_;
	}

	void*
	OfflineFeature::getScene() const noexcept
	{
		return this->rprScene_;
	}

	void*
	OfflineFeature::getContext() const noexcept
	{
		return this->rprContext_;
	}

	void*
	OfflineFeature::getMaterialSystem() const noexcept
	{
		return this->rprMaterialSystem_;
	}

	void
	OfflineFeature::saveToFile(const char* path) noexcept
	{
		if (this->rprFramebuffer_)
			rprFrameBufferSaveToFile(this->rprFramebuffer_, path);
	}

	void
	OfflineFeature::onActivate() except
	{
		this->addMessageListener("feature:input:event", std::bind(&OfflineFeature::onInputEvent, this, std::placeholders::_1));

		auto context = this->getFeature<GraphicsFeature>()->getContext();

		hal::GraphicsFramebufferLayoutDesc framebufferLayoutDesc;
		framebufferLayoutDesc.addComponent(hal::GraphicsAttachmentLayout(0, hal::GraphicsImageLayout::ColorAttachmentOptimal, hal::GraphicsFormat::R32G32B32A32SFloat));
		framebufferLayoutDesc.addComponent(hal::GraphicsAttachmentLayout(1, hal::GraphicsImageLayout::DepthStencilAttachmentOptimal, hal::GraphicsFormat::D32_SFLOAT));

		hal::GraphicsTextureDesc colorTextureDesc;
		colorTextureDesc.setWidth(this->framebuffer_w_);
		colorTextureDesc.setHeight(this->framebuffer_h_);
		colorTextureDesc.setTexDim(hal::GraphicsTextureDim::Texture2D);
		colorTextureDesc.setTexFormat(hal::GraphicsFormat::R32G32B32A32SFloat);
		auto colorTexture_ = context->getDevice()->createTexture(colorTextureDesc);
		if (!colorTexture_)
			throw runtime::runtime_error::create("createTexture() failed");

		hal::GraphicsTextureDesc depthTextureDesc;
		depthTextureDesc.setWidth(this->framebuffer_w_);
		depthTextureDesc.setHeight(this->framebuffer_h_);
		depthTextureDesc.setTexDim(hal::GraphicsTextureDim::Texture2D);
		depthTextureDesc.setTexFormat(hal::GraphicsFormat::D32_SFLOAT);
		auto depthTexture_ = context->getDevice()->createTexture(depthTextureDesc);
		if (!depthTexture_)
			throw runtime::runtime_error::create("createTexture() failed");

		hal::GraphicsFramebufferDesc framebufferDesc;
		framebufferDesc.setWidth(this->framebuffer_w_);
		framebufferDesc.setHeight(this->framebuffer_h_);
		framebufferDesc.setGraphicsFramebufferLayout(context->getDevice()->createFramebufferLayout(framebufferLayoutDesc));
		framebufferDesc.setDepthStencilAttachment(hal::GraphicsAttachmentBinding(depthTexture_, 0, 0));
		framebufferDesc.addColorAttachment(hal::GraphicsAttachmentBinding(colorTexture_, 0, 0));

		this->framebuffer_ = context->getDevice()->createFramebuffer(framebufferDesc);
		if (!this->framebuffer_)
			throw runtime::runtime_error::create("createFramebuffer() failed");

		rpr_framebuffer_format format;
		format.type = RPR_COMPONENT_TYPE_FLOAT32;
		format.num_components = 4;

		rpr_framebuffer_desc desc;
		desc.fb_width = this->framebuffer_w_;
		desc.fb_height = this->framebuffer_h_;

		rprCreateContext(RPR_API_VERSION, 0, 0, RPR_CREATION_FLAGS_ENABLE_GPU0 | RPR_CREATION_FLAGS_ENABLE_GL_INTEROP, 0, 0, &this->rprContext_);
		rprContextCreateScene(rprContext_, &rprScene_);
		rprContextCreateMaterialSystem(rprContext_, 0, &this->rprMaterialSystem_);
		rprContextCreateFramebufferFromGLTexture2D(rprContext_, GL_TEXTURE_2D, 0, colorTexture_->handle(), &this->rprFramebuffer_);

		rprContextSetScene(rprContext_, rprScene_);
		rprContextSetAOV(rprContext_, RPR_AOV_COLOR, this->rprFramebuffer_);
	}

	void
	OfflineFeature::onDeactivate() noexcept
	{
		this->removeMessageListener("feature:input:event", std::bind(&OfflineFeature::onInputEvent, this, std::placeholders::_1));

		if (this->rprMaterialSystem_)
		{
			rprObjectDelete(this->rprMaterialSystem_);
			this->rprMaterialSystem_ = nullptr;
		}

		if (this->rprFramebuffer_)
		{
			rprObjectDelete(this->rprFramebuffer_);
			this->rprFramebuffer_ = nullptr;
		}

		if (this->rprScene_)
		{
			rprSceneClear(rprScene_);
			rprObjectDelete(this->rprScene_);
			this->rprScene_ = nullptr;
		}

		if (this->rprContext_)
		{
			rprContextClearMemory(rprContext_);
			rprObjectDelete(this->rprContext_);
			this->rprContext_ = nullptr;
		}
	}

	void
	OfflineFeature::onInputEvent(const runtime::any& data) noexcept
	{
		auto event = runtime::any_cast<input::InputEvent>(data);
		switch (event.event)
		{
		case input::InputEvent::SizeChange:
		case input::InputEvent::SizeChangeDPI:
			break;
		default:
			return;
		}
	}

	void
	OfflineFeature::onFrameBegin() noexcept
	{
	}

	void
	OfflineFeature::onFrame() noexcept
	{
		rpr_camera camera;
		rprSceneGetCamera(this->rprScene_, &camera);
		if (camera)
		{
			std::size_t count = 0;
			std::size_t type_size = 0;

			rprSceneGetInfo(this->rprScene_, RPR_SCENE_SHAPE_COUNT, 1, &count, &type_size);
			if (count == 0)
				return;

			rprSceneGetInfo(this->rprScene_, RPR_SCENE_LIGHT_COUNT, 1, &count, &type_size);
			if (count == 0)
				return;

			if (this->dirty_)
			{
				rprFrameBufferClear(this->rprFramebuffer_);
				this->dirty_ = false;
			}

			rprContextRender(rprContext_);

			auto graphics = this->getFeature<GraphicsFeature>();
			if (graphics)
			{
				auto context = graphics->getContext();
				math::float4 viewport(0, 0, this->framebuffer_w_, this->framebuffer_h_);
				context->blitFramebuffer(framebuffer_, viewport, nullptr, viewport);
			}
		}
	}

	void
	OfflineFeature::onFrameEnd() noexcept
	{
	}
}
#endif