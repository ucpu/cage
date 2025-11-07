#include <cage-engine/virtualReality.h>

namespace cage
{
	bool VirtualRealityController::tracking() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	Transform VirtualRealityController::aimPose() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	Transform VirtualRealityController::gripPose() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	PointerRange<const Real> VirtualRealityController::axes() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	PointerRange<const bool> VirtualRealityController::buttons() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	void VirtualRealityGraphicsFrame::updateProjections()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	Transform VirtualRealityGraphicsFrame::pose() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	uint64 VirtualRealityGraphicsFrame::displayTime() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	void VirtualRealityGraphicsFrame::renderBegin()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	void VirtualRealityGraphicsFrame::acquireTextures()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	void VirtualRealityGraphicsFrame::renderCommit()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	void VirtualRealityGraphicsFrame::renderCancel()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	void VirtualReality::processEvents()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	bool VirtualReality::tracking() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	Transform VirtualReality::pose() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	VirtualRealityController &VirtualReality::leftController()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	VirtualRealityController &VirtualReality::rightController()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	Holder<VirtualRealityGraphicsFrame> VirtualReality::graphicsFrame()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	uint64 VirtualReality::targetFrameTiming() const
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}

	Holder<VirtualReality> newVirtualReality()
	{
		CAGE_THROW_CRITICAL(Exception, "VR currently not implemented");
	}
}
