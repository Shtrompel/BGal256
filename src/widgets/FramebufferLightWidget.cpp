#define _FB_LIGHT_WIDGET_SRC
#ifdef _FB_LIGHT_WIDGET_SRC

#include <widgets/FramebufferLightWidget.hpp>
#include <context.hpp>
#include <random.hpp>


static int FramebufferLightWidget_totalPixels = 0;


struct FramebufferLightWidget::Internal2 {
	NVGLUframebuffer* fb = NULL;

	/** Pixel dimensions of the allocated framebuffer */
	math::Vec fbSize;
	/** Bounding box in world coordinates of where the framebuffer should be painted.
	Always has integer coordinates so that blitting framebuffers is pixel-perfect.
	*/
	math::Rect fbBox;
	/** Framebuffer's scale relative to the world */
	math::Vec fbScale;
	/** Framebuffer's subpixel offset relative to fbBox in world coordinates */
	math::Vec fbOffsetF;
	/** Local box where framebuffer content is valid.
	*/
	math::Rect fbClipBox = math::Rect::inf();
};


FramebufferLightWidget::FramebufferLightWidget() {
	internal2 = new Internal2;
}


FramebufferLightWidget::~FramebufferLightWidget() {
	deleteFramebuffer();
	delete internal2;
}


void FramebufferLightWidget::setDirty(bool dirty) {
	this->dirty = dirty;
}


int FramebufferLightWidget::getImageHandle() {
	if (!internal2->fb)
		return -1;
	return internal2->fb->image;
}


NVGLUframebuffer* FramebufferLightWidget::getFramebuffer() {
	return internal2->fb;
}


math::Vec FramebufferLightWidget::getFramebufferSize() {
	return internal2->fbSize;
}


void FramebufferLightWidget::deleteFramebuffer() {
	if (!internal2->fb)
		return;

	// If the framebuffer exists, the Window should exist.
	assert(APP->window);

	nvgluDeleteFramebuffer(internal2->fb);
	internal2->fb = NULL;

	FramebufferLightWidget_totalPixels -= internal2->fbSize.area();
}


void FramebufferLightWidget::step() {
	MultiLightWidget::step();
}


void FramebufferLightWidget::draw(const DrawArgs& args) {
	// Draw directly if bypassed or already drawing in a framebuffer
	if (bypassed || args.fb) {
		Widget::draw(args);
		return;
	}

	// Get world transform
	float xform[6];
	nvgCurrentTransform(args.vg, xform);
	// Skew and rotate is not supported
	if (!math::isNear(xform[1], 0.f) || !math::isNear(xform[2], 0.f)) {
		WARN("Skew and rotation detected but not supported in FramebufferLightWidget.");
		return;
	}

	// Extract scale and offset from world transform
	math::Vec scale = math::Vec(xform[0], xform[3]);
	math::Vec offset = math::Vec(xform[4], xform[5]);
	math::Vec offsetI = offset.floor();
	math::Vec offsetF = offset.minus(offsetI);

	// Re-render if drawing to a new subpixel location.
	// Anything less than 0.1 pixels isn't noticeable.
	math::Vec offsetFDelta = offsetF.minus(internal2->fbOffsetF);
	// 	if (dirtyOnSubpixelChange && dirtySubpixelChange && offsetFDelta.square() >= std::pow(0.1f, 2)) {
	if (dirtyOnSubpixelChange && offsetFDelta.square() >= std::pow(0.1f, 2)) {
		// DEBUG("%p dirty subpixel (%f, %f) (%f, %f)", this, VEC_ARGS(offsetF), VEC_ARGS(internal2->fbOffsetF));
		setDirty();
	}
	// Re-render if rescaled.
	else if (!scale.equals(internal2->fbScale)) {
		// DEBUG("%p dirty scale", this);
		setDirty();
	}
	// Re-render if viewport is outside framebuffer's clipbox when it was rendered.
	else if (!internal2->fbClipBox.contains(args.clipBox)) {
		setDirty();
	}

	if (dirty) {
		// Render only if there is frame time remaining 
		// (to avoid lagging frames significantly), or if 
		// it's one of the first framebuffers this frame 
		// (to avoid framebuffers from never rendering).
		const double minRemaining = -1 / 60.0;
		double remaining = APP->window->getFrameDurationRemaining();
		
		//int count = ++APP->window->fbCount();
		//if (count <= minCount || remaining > minRemaining) {
		if (remaining > minRemaining) {
			render(scale, offsetF, args.clipBox);
		}


	}

	if (!internal2->fb)
		return;

	// Draw framebuffer image, using world coordinates
	nvgSave(args.vg);
	nvgResetTransform(args.vg);

	math::Vec scaleRatio = scale.div(internal2->fbScale);
	// DEBUG("%f %f %f %f", scaleRatio.x, scaleRatio.y, offsetF.x, offsetF.y);

	// DEBUG("%f %f %f %f, %f %f", RECT_ARGS(internal2->fbBox), VEC_ARGS(internal2->fbSize));
	// DEBUG("offsetI (%f, %f) fbBox (%f, %f; %f, %f)", VEC_ARGS(offsetI), RECT_ARGS(internal2->fbBox));
	nvgBeginPath(args.vg);
	nvgRect(args.vg,
		offsetI.x + internal2->fbBox.pos.x * scaleRatio.x,
		offsetI.y + internal2->fbBox.pos.y * scaleRatio.y,
		internal2->fbBox.size.x * scaleRatio.x,
		internal2->fbBox.size.y * scaleRatio.y);
	NVGpaint paint = nvgImagePattern(args.vg,
		offsetI.x + internal2->fbBox.pos.x * scaleRatio.x,
		offsetI.y + internal2->fbBox.pos.y * scaleRatio.y,
		internal2->fbBox.size.x * scaleRatio.x,
		internal2->fbBox.size.y * scaleRatio.y,
		0.0, internal2->fb->image, 1.0);
	nvgFillPaint(args.vg, paint);
	nvgFill(args.vg);

	// For debugging the bounding box of the framebuffer
	// nvgStrokeWidth(args.vg, 2.0);
	// nvgStrokeColor(args.vg, nvgRGBAf(1, 1, 0, 0.5));
	// nvgStroke(args.vg);

	nvgRestore(args.vg);
}

void FramebufferLightWidget::render(math::Vec scale, math::Vec offsetF, math::Rect clipBox) {
	// In case we fail drawing the framebuffer, don't try again the next frame, so reset `dirty` here.
	dirty = false;
	NVGcontext* vg = APP->window->vg;
	NVGcontext* fbVg = APP->window->fbVg;

	internal2->fbScale = scale;
	internal2->fbOffsetF = offsetF;

	math::Rect localBox;
	if (children.empty()) {
		localBox = box.zeroPos();
	}
	else {
		localBox = getVisibleChildrenBoundingBox();
	}

	// Intersect local box with viewport if viewportMargin is set
	internal2->fbClipBox = clipBox.grow(viewportMargin);
	if (internal2->fbClipBox.size.isFinite()) {
		localBox = localBox.intersect(internal2->fbClipBox);
	}

	// DEBUG("rendering FramebufferLightWidget localBox (%f, %f; %f, %f) fbOffset (%f, %f) fbScale (%f, %f)", RECT_ARGS(localBox), VEC_ARGS(internal2->fbOffsetF), VEC_ARGS(internal2->fbScale));
	// Transform to world coordinates, then expand to nearest integer coordinates
	math::Vec min = localBox.getTopLeft().mult(internal2->fbScale).plus(internal2->fbOffsetF).floor();
	math::Vec max = localBox.getBottomRight().mult(internal2->fbScale).plus(internal2->fbOffsetF).ceil();
	internal2->fbBox = math::Rect::fromMinMax(min, max);
	// DEBUG("%g %g %g %g", RECT_ARGS(internal2->fbBox));

	float pixelRatio = std::fmax(1.f, std::floor(APP->window->pixelRatio));
	math::Vec newFbSize = internal2->fbBox.size.mult(pixelRatio).ceil();

	// Create framebuffer if a new size is needed
	if (!internal2->fb || !newFbSize.equals(internal2->fbSize)) {
		// Delete old framebuffer
		deleteFramebuffer();

		// Create a framebuffer
		if (newFbSize.isFinite() && !newFbSize.isZero()) {
			// DEBUG("Creating framebuffer of size (%f, %f)", VEC_ARGS(newFbSize));
			internal2->fb = nvgluCreateFramebuffer(vg, newFbSize.x, newFbSize.y, 0);
			FramebufferLightWidget_totalPixels += newFbSize.area();
		}

		// DEBUG("Framebuffer total pixels: %.1f Mpx", FramebufferLightWidget_totalPixels / 1e6);
		internal2->fbSize = newFbSize;
	}
	if (!internal2->fb) {
		WARN("Framebuffer of size (%f, %f) could not be created for FramebufferLightWidget %p.", VEC_ARGS(internal2->fbSize), this);
		return;
	}

	// DEBUG("Drawing to framebuffer of size (%f, %f)", VEC_ARGS(internal2->fbSize));

	// Render to framebuffer
	if (oversample == 1.0) {
		// If not oversampling, render directly to framebuffer.
		nvgluBindFramebuffer(internal2->fb);
		drawFramebuffer();
		nvgluBindFramebuffer(NULL);
	}
	else {
		NVGLUframebuffer* fb = internal2->fb;
		// If oversampling, create another framebuffer and copy it to actual size.
		math::Vec oversampledFbSize = internal2->fbSize.mult(oversample).ceil();
		// DEBUG("Creating %0.fx oversampled framebuffer of size (%f, %f)", oversample, VEC_ARGS(internal2->fbSize));
		NVGLUframebuffer* oversampledFb = nvgluCreateFramebuffer(fbVg, oversampledFbSize.x, oversampledFbSize.y, 0);

		if (!oversampledFb) {
			WARN("Oversampled framebuffer of size (%f, %f) could not be created for FramebufferLightWidget %p.", VEC_ARGS(oversampledFbSize), this);
			return;
		}

		// Render to oversampled framebuffer.
		nvgluBindFramebuffer(oversampledFb);
		internal2->fb = oversampledFb;
		drawFramebuffer();
		internal2->fb = fb;
		nvgluBindFramebuffer(NULL);

		// Use NanoVG for copying oversampled framebuffer to normal framebuffer
		nvgluBindFramebuffer(internal2->fb);
		nvgBeginFrame(fbVg, internal2->fbBox.size.x, internal2->fbBox.size.y, 1.0);

		// Draw oversampled framebuffer
		nvgBeginPath(fbVg);
		nvgRect(fbVg, 0.0, 0.0, internal2->fbSize.x, internal2->fbSize.y);
		NVGpaint paint = nvgImagePattern(fbVg, 0.0, 0.0,
			internal2->fbSize.x, internal2->fbSize.y,
			0.0, oversampledFb->image, 1.0);
		nvgFillPaint(fbVg, paint);
		nvgFill(fbVg);

		glViewport(0.0, 0.0, internal2->fbSize.x, internal2->fbSize.y);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		nvgEndFrame(fbVg);
		nvgReset(fbVg);

		nvgluBindFramebuffer(NULL);
		nvgluDeleteFramebuffer(oversampledFb);
	}
};


void FramebufferLightWidget::drawFramebuffer() {
	NVGcontext* vg = APP->window->fbVg;
	nvgSave(vg);

	float pixelRatio = internal2->fbSize.x * oversample / internal2->fbBox.size.x;
	nvgBeginFrame(vg, internal2->fbBox.size.x, internal2->fbBox.size.y, pixelRatio);

	// Use local scaling
	nvgTranslate(vg, -internal2->fbBox.pos.x, -internal2->fbBox.pos.y);
	nvgTranslate(vg, internal2->fbOffsetF.x, internal2->fbOffsetF.y);
	nvgScale(vg, internal2->fbScale.x, internal2->fbScale.y);

	// Draw children
	DrawArgs args;
	args.vg = vg;
	args.clipBox = box.zeroPos();
	args.fb = internal2->fb;
	Widget::draw(args);

	glViewport(0.0, 0.0, internal2->fbSize.x * oversample, internal2->fbSize.y * oversample);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	// glClearColor(0.0, 1.0, 1.0, 0.5);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	nvgEndFrame(vg);

	// Clean up the NanoVG state so that calls to nvgTextBounds() etc during step() don't use a dirty state.
	nvgReset(vg);
	nvgRestore(vg);
}


void FramebufferLightWidget::onDirty(const DirtyEvent& e) {
	setDirty();
	MultiLightWidget::onDirty(e);
}


void FramebufferLightWidget::onContextCreate(const ContextCreateEvent& e) {
	setDirty();
	MultiLightWidget::onContextCreate(e);
}


void FramebufferLightWidget::onContextDestroy(const ContextDestroyEvent& e) {
	deleteFramebuffer();
	setDirty();
	MultiLightWidget::onContextDestroy(e);
}

#endif // _FB_LIGHT_WIDGET