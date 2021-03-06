#include <octoon/ambient_light_component.h>
#include <octoon/video_feature.h>
#include <octoon/transform_component.h>

namespace octoon
{
	OctoonImplementSubInterface(AmbientLightComponent, LightComponent, "AmbientLight")

	AmbientLightComponent::AmbientLightComponent() noexcept
	{
	}

	AmbientLightComponent::~AmbientLightComponent() noexcept
	{
	}
	
	void
	AmbientLightComponent::setIntensity(float value) noexcept
	{
		if (ambientLight_)
			ambientLight_->setIntensity(value);
		LightComponent::setIntensity(value);
	}

	void
	AmbientLightComponent::setColor(const math::float3& value) noexcept
	{
		if (ambientLight_)
			ambientLight_->setColor(value);
		LightComponent::setColor(value);
	}

	GameComponentPtr
	AmbientLightComponent::clone() const noexcept
	{
		auto instance = std::make_shared<AmbientLightComponent>();
		instance->setName(this->getName());

		return instance;
	}

	void
	AmbientLightComponent::onActivate() noexcept
	{
		auto transform = this->getComponent<TransformComponent>();

		ambientLight_ = std::make_shared<light::AmbientLight>();
		ambientLight_->setActive(true);
		ambientLight_->setLayer(this->getGameObject()->getLayer());
		ambientLight_->setColor(this->getColor());
		ambientLight_->setIntensity(this->getIntensity());
		ambientLight_->setTransform(transform->getTransform(), transform->getTransformInverse());

		this->addComponentDispatch(GameDispatchType::MoveAfter);
	}

	void
	AmbientLightComponent::onDeactivate() noexcept
	{
		this->removeComponentDispatch(GameDispatchType::MoveAfter);

		ambientLight_->setActive(false);
	}

	void
	AmbientLightComponent::onMoveAfter() noexcept
	{
		if (this->ambientLight_)
		{
			auto transform = this->getComponent<TransformComponent>();
			this->ambientLight_->setTransform(transform->getTransform(), transform->getTransformInverse());
		}
	}

	void
	AmbientLightComponent::onLayerChangeAfter() noexcept
	{
		if (this->ambientLight_)
			this->ambientLight_->setLayer(this->getGameObject()->getLayer());
	}
}