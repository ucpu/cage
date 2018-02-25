#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>

namespace cage
{
	void initializeComponents(componentsStruct &components, entityManagerClass *manager)
	{
		components.parent = manager->defineComponent(parentComponent(), false);
		components.text = manager->defineComponent(textComponent(), true);
		components.image = manager->defineComponent(imageComponent(), true);
		components.control = manager->defineComponent(controlComponent(), true);
		components.layout = manager->defineComponent(layoutComponent(), true);
		components.position = manager->defineComponent(positionComponent(), false);
		components.format = manager->defineComponent(formatComponent(), false);
		components.selection = manager->defineComponent(selectionComponent(), true);
	}

	parentComponent::parentComponent() : parent(0), ordering(0) {}
	textComponent::textComponent() : assetName(0), textName(0) {}
	imageComponent::imageComponent() : animationStart(0), tw(1), th(1), animationSpeed(1), animationOffset(0), textureName(0) {}
	controlComponent::controlComponent() : controlType(controlTypeEnum::Empty), ival(0), state(controlStateEnum::Normal) {}
	layoutComponent::layoutComponent() : layoutMode(layoutModeEnum::Manual), hScrollbarMode(scrollbarModeEnum::Automatic), vScrollbarMode(scrollbarModeEnum::Automatic) {}
	positionComponent::positionComponent() : xUnit(unitsModeEnum::Automatic), yUnit(unitsModeEnum::Automatic), wUnit(unitsModeEnum::Automatic), hUnit(unitsModeEnum::Automatic) {}
	formatComponent::formatComponent() : color(vec3::Nan), fontName(0), align((textAlignEnum)-1), size(real::Nan) {}
	selectionComponent::selectionComponent() : selectionStart(0), selectionLength(0) {}
}