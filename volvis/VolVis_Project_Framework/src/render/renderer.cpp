#include "renderer.h"
#include <algorithm>
#include <algorithm> // std::fill
#include <cmath>
#include <functional>
#include <glm/common.hpp>
#include <glm/gtx/component_wise.hpp>
#include <iostream>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tuple>

namespace render {

// The renderer is passed a pointer to the volume, gradinet volume, camera and an initial renderConfig.
// The camera being pointed to may change each frame (when the user interacts). When the renderConfig
// changes the setConfig function is called with the updated render config. This gives the Renderer an
// opportunity to resize the framebuffer.
Renderer::Renderer(
    const volume::Volume* pVolume,
    const volume::GradientVolume* pGradientVolume,
    const render::RayTraceCamera* pCamera,
    const RenderConfig& initialConfig)
    : m_pVolume(pVolume)
    , m_pGradientVolume(pGradientVolume)
    , m_pCamera(pCamera)
    , m_config(initialConfig)
{
    resizeImage(initialConfig.renderResolution);
}

// Set a new render config if the user changed the settings.
void Renderer::setConfig(const RenderConfig& config)
{
    if (config.renderResolution != m_config.renderResolution)
        resizeImage(config.renderResolution);

    m_config = config;
}

// Resize the framebuffer and fill it with black pixels.
void Renderer::resizeImage(const glm::ivec2& resolution)
{
    m_frameBuffer.resize(size_t(resolution.x) * size_t(resolution.y), glm::vec4(0.0f));
}

// Clear the framebuffer by setting all pixels to black.
void Renderer::resetImage()
{
    std::fill(std::begin(m_frameBuffer), std::end(m_frameBuffer), glm::vec4(0.0f));
}

// Return a VIEW into the framebuffer. This view is merely a reference to the m_frameBuffer member variable.
// This does NOT make a copy of the framebuffer.
gsl::span<const glm::vec4> Renderer::frameBuffer() const
{
    return m_frameBuffer;
}

// Main render function. It computes an image according to the current renderMode.
// Multithreading is enabled in Release/RelWithDebInfo modes. In Debug mode multithreading is disabled to make debugging easier.
void Renderer::render()
{
    resetImage();

    static constexpr float sampleStep = 1.0f;
    const glm::vec3 planeNormal = -glm::normalize(m_pCamera->forward());
    const glm::vec3 volumeCenter = glm::vec3(m_pVolume->dims()) / 2.0f;
    const Bounds bounds { glm::vec3(0.0f), glm::vec3(m_pVolume->dims() - glm::ivec3(1)) };

    // 0 = sequential (single-core), 1 = TBB (multi-core)
#ifdef NDEBUG
    // If NOT in debug mode then enable parallelism using the TBB library (Intel Threaded Building Blocks).
#define PARALLELISM 1
#else
    // Disable multi threading in debug mode.
#define PARALLELISM 0
#endif

#if PARALLELISM == 0
    // Regular (single threaded) for loops.
    for (int x = 0; x < m_config.renderResolution.x; x++) {
        for (int y = 0; y < m_config.renderResolution.y; y++) {
#else
    // Parallel for loop (in 2 dimensions) that subdivides the screen into tiles.
    const tbb::blocked_range2d<int> screenRange { 0, m_config.renderResolution.y, 0, m_config.renderResolution.x };
        tbb::parallel_for(screenRange, [&](tbb::blocked_range2d<int> localRange) {
        // Loop over the pixels in a tile. This function is called on multiple threads at the same time.
        for (int y = std::begin(localRange.rows()); y != std::end(localRange.rows()); y++) {
            for (int x = std::begin(localRange.cols()); x != std::end(localRange.cols()); x++) {
#endif
            // Compute a ray for the current pixel.
            const glm::vec2 pixelPos = glm::vec2(x, y) / glm::vec2(m_config.renderResolution);
            Ray ray = m_pCamera->generateRay(pixelPos * 2.0f - 1.0f);

            // Compute where the ray enters and exists the volume.
            // If the ray misses the volume then we continue to the next pixel.
            if (!instersectRayVolumeBounds(ray, bounds))
                continue;

            // Get a color for the current pixel according to the current render mode.
            glm::vec4 color {};
            switch (m_config.renderMode) {
            case RenderMode::RenderSlicer: {
                color = traceRaySlice(ray, volumeCenter, planeNormal);
                break;
            }
            case RenderMode::RenderMIP: {
                color = traceRayMIP(ray, sampleStep);
                break;
            }
            case RenderMode::RenderComposite: {
                color = traceRayComposite(ray, sampleStep);
                break;
            }
            case RenderMode::RenderIso: {
                color = traceRayISO(ray, sampleStep);
                break;
            }
            case RenderMode::RenderTF2D: {
                color = traceRayTF2D(ray, sampleStep);
                break;
            }
            };
            // Write the resulting color to the screen.
            fillColor(x, y, color);

#if PARALLELISM == 1
        }
    }
});
#else
            }
        }
#endif
}

// ======= DO NOT MODIFY THIS FUNCTION ========
// This function generates a view alongside a plane perpendicular to the camera through the center of the volume
//  using the slicing technique.
glm::vec4 Renderer::traceRaySlice(const Ray& ray, const glm::vec3& volumeCenter, const glm::vec3& planeNormal) const
{
    const float t = glm::dot(volumeCenter - ray.origin, planeNormal) / glm::dot(ray.direction, planeNormal);
    const glm::vec3 samplePos = ray.origin + ray.direction * t;
    const float val = m_pVolume->getSampleInterpolate(samplePos);
    return glm::vec4(glm::vec3(std::max(val / m_pVolume->maximum(), 0.0f)), 1.f);
}

// ======= DO NOT MODIFY THIS FUNCTION ========
// Function that implements maximum-intensity-projection (MIP) raycasting.
// It returns the color assigned to a ray/pixel given it's origin, direction and the distances
// at which it enters/exits the volume (ray.tmin & ray.tmax respectively).
// The ray must be sampled with a distance defined by the sampleStep
glm::vec4 Renderer::traceRayMIP(const Ray& ray, float sampleStep) const
{
    float maxVal = 0.0f;

    // Incrementing samplePos directly instead of recomputing it each frame gives a measureable speed-up.
    glm::vec3 samplePos = ray.origin + ray.tmin * ray.direction;
    const glm::vec3 increment = sampleStep * ray.direction;
    for (float t = ray.tmin; t <= ray.tmax; t += sampleStep, samplePos += increment) {
        const float val = m_pVolume->getSampleInterpolate(samplePos);
        maxVal = std::max(val, maxVal);
    }

    // Normalize the result to a range of [0 to mpVolume->maximum()].
    return glm::vec4(glm::vec3(maxVal) / m_pVolume->maximum(), 1.0f);
}

// ======= TODO: IMPLEMENT ========
// This function should find the position where the ray intersects with the volume's isosurface.
// If volume shading is DISABLED then simply return the isoColor.
// If volume shading is ENABLED then return the phong-shaded color at that location using the local gradient (from m_pGradientVolume).
//   Use the camera position (m_pCamera->position()) as the light position.
// Use the bisectionAccuracy function (to be implemented) to get a more precise isosurface location between two steps.
glm::vec4 Renderer::traceRayISO(const Ray& ray, float sampleStep) const
{
    static constexpr glm::vec3 isoColor { 0.8f, 0.8f, 0.2f };
    static constexpr glm::vec3 noColor { 0.0f, 0.0f, 0.0f };

    glm::vec3 color = noColor;

    glm::vec3 samplePos = ray.origin + ray.tmin * ray.direction;
    glm::vec3 accuratePos = samplePos;
    const glm::vec3 increment = sampleStep * ray.direction;
    // for each sample across the ray...
    float prev = ray.tmin;
    for (float t = ray.tmin; t <= ray.tmax; t += sampleStep, samplePos += increment) {
        const float val = m_pVolume->getSampleInterpolate(samplePos);
        // if the sample has a value greater or equal to the isoValue
        if (val >= m_config.isoValue) {
            // find a better approximation of the iso surface using binary search
            float betterT = bisectionAccuracy(ray, prev, t, m_config.isoValue);
            // save this approximation and set the pixel color to the iso surface color
            accuratePos = ray.origin + betterT * ray.direction;
            color = isoColor;
            // and leave the sampling loop
            break;
        }
        prev = t;
    }

    volume::GradientVoxel grad = m_pGradientVolume->getGradient(accuratePos.x, accuratePos.y, accuratePos.z);

    if (m_config.volumeShading) {         // if shading is enabled from the config
        if (m_config.toneBasedShading) {  // use toneBasedShading to determine whether to use cool-warm or Phong shading
            color = computeToneBasedShading(color, grad);
        } else {
            color = computePhongShading(
                color,                    // sample color
                grad,                     // sample gradient
                m_pCamera->position(),    // camera as light vector
                m_pCamera->position()     // camera is the view vector
            );
        }
    }
    return glm::vec4(color, 1.0f);    // return the color with 100% opacity
}

// ======= TODO: IMPLEMENT ========
// Given that the iso value lies somewhere between t0 and t1, find a t for which the value
// closely matches the iso value (less than 0.01 difference). Add a limit to the number of
// iterations such that it does not get stuck in degenerate cases.
float Renderer::bisectionAccuracy(const Ray& ray, float t0, float t1, float isoValue) const
{
    // binary search to find a better t
    // assuming t0 <= t1
    float mid = 0.0f;
    float tMid = 0.0f;
    int maxIter = 100;
    int iter = 0;
    while (iter < maxIter) {
        tMid = (t0 + t1) / 2;
        mid = m_pVolume->getSampleInterpolate(ray.origin + tMid * ray.direction);
        if (glm::abs(isoValue - mid) < 0.01f) {
            break;
        }
        if (mid < isoValue)
            t0 = tMid;
        if (mid > isoValue)
            t1 = tMid;
        iter++;
    }
    return tMid;
}

// ======= TODO: IMPLEMENT ========
// Compute Phong Shading given the voxel color (material color), the gradient, the light vector and view vector.
// You can find out more about the Phong shading model at:
// https://en.wikipedia.org/wiki/Phong_reflection_model
//
// Use the given color for the ambient/specular/diffuse (you are allowed to scale these constants by a scalar value).
// You are free to choose any specular power that you'd like.
glm::vec3 Renderer::computePhongShading(const glm::vec3& color, const volume::GradientVoxel& gradient, const glm::vec3& L, const glm::vec3& V)
{
    const float ka = 0.1;
    const float ks = 0.7;
    const float kd = 0.2;
    const int a = 4;

    glm::vec3 LNorm = glm::normalize(L);
    glm::vec3 VNorm = glm::normalize(V);
    glm::vec3 GNorm = glm::normalize(gradient.dir);

    glm::vec3 R = 2 * glm::dot(LNorm, GNorm) * GNorm - LNorm;
    glm::vec3 RNorm = glm::normalize(R);

    // Phone Shading equation from wikipedia
    glm::vec3 ret = ka * color + kd * glm::dot(LNorm, VNorm) * color + ks * (float) glm::pow(glm::dot(RNorm, VNorm), a) * color;
    return ret;
}

// Implementation of cool to warm shading
// Gooch, Amy, et al. "A non-photorealistic lighting model for automatic technical illustration." 1998.
glm::vec3 Renderer::computeToneBasedShading(const glm::vec3& color, const volume::GradientVoxel& gradient)
{
    float b = 0.4f;
    float y = 0.4f;
    float alpha = 0.2f;
    float beta = 0.6f;

    glm::vec3 kBlue = glm::vec3(0.0f, 0.0f, b);
    glm::vec3 kYellow = glm::vec3(y, y, 0.0f);

    glm::vec3 kCool = kBlue + alpha * color;
    glm::vec3 kWarm = kYellow + beta * color;

    float f = (1 + glm::dot(color, gradient.dir)) / 2;
    return f * kCool + (1 - f) * kWarm;
}

// ======= TODO: IMPLEMENT ========
// In this function, implement 1D transfer function raycasting.
// Use getTFValue to compute the color for a given volume value according to the 1D transfer function.
glm::vec4 Renderer::traceRayComposite(const Ray& ray, float sampleStep) const
{
    glm::vec4 aggregatedColor = glm::vec4(0.0f);

    glm::vec3 samplePos = ray.origin + ray.tmin * ray.direction;
    const glm::vec3 increment = sampleStep * ray.direction;

    // sample across the ray
    for (float t = ray.tmin; t <= ray.tmax; t += sampleStep, samplePos += increment) {
        float val = m_pVolume->getSampleInterpolate(samplePos);
        volume::GradientVoxel grad = m_pGradientVolume->getGradient(samplePos.x, samplePos.y, samplePos.z);

        // map the value to the transfer function
        glm::vec4 tf = getTFValue(val);

        // apply shading if in config
        if (m_config.volumeShading) {
            // get the shading value and aggregate it across the ray
            // there's a bug here but I can't find its cause..
            glm::vec3 rgb = glm::vec3(tf.r, tf.g, tf.b);
            tf = glm::vec4(computePhongShading(rgb, grad, m_pCamera->position(), m_pCamera->position()), tf.a);
        }

        // aggregate using front-to-back compositing
        aggregatedColor.r += (1 - aggregatedColor.a) * tf.a * tf.r;
        aggregatedColor.g += (1 - aggregatedColor.a) * tf.a * tf.g;
        aggregatedColor.b += (1 - aggregatedColor.a) * tf.a * tf.b;
        aggregatedColor.a += (1 - aggregatedColor.a) * tf.a;
    }
    
    return aggregatedColor;
}

// ======= DO NOT MODIFY THIS FUNCTION ========
// Looks up the color+opacity corresponding to the given volume value from the 1D tranfer function LUT (m_config.tfColorMap).
// The value will initially range from (m_config.tfColorMapIndexStart) to (m_config.tfColorMapIndexStart + m_config.tfColorMapIndexRange) .
glm::vec4 Renderer::getTFValue(float val) const
{
    // Map value from [m_config.tfColorMapIndexStart, m_config.tfColorMapIndexStart + m_config.tfColorMapIndexRange) to [0, 1) .
    const float range01 = (val - m_config.tfColorMapIndexStart) / m_config.tfColorMapIndexRange;
    const size_t i = std::min(static_cast<size_t>(range01 * static_cast<float>(m_config.tfColorMap.size())), m_config.tfColorMap.size() - 1);
    return m_config.tfColorMap[i];
}

// ======= TODO: IMPLEMENT ========
// In this function, implement 2D transfer function raycasting.
// Use the getTF2DOpacity function that you implemented to compute the opacity according to the 2D transfer function.
glm::vec4 Renderer::traceRayTF2D(const Ray& ray, float sampleStep) const
{
    glm::vec4 aggregatedColor = glm::vec4(0.0f);

    glm::vec3 samplePos = ray.origin + ray.tmin * ray.direction;
    const glm::vec3 increment = sampleStep * ray.direction;
    // we sample the ray as usual
    for (float t = ray.tmin; t <= ray.tmax; t += sampleStep, samplePos += increment) {
        float val = m_pVolume->getSampleInterpolate(samplePos);
        volume::GradientVoxel grad = m_pGradientVolume->getGradient(samplePos.x, samplePos.y, samplePos.z);

        // get the color from the UI
        glm::vec4 color = m_config.TF2DColor;
        // we calculate the opacity through the 2D transfer function
        glm::vec4 colorWithOpacity = glm::vec4(color.r, color.g, color.b, getTF2DOpacity(val, grad.magnitude));

        // aggregate using front-to-back compositing
        aggregatedColor.r += (1 - aggregatedColor.a) * colorWithOpacity.a * colorWithOpacity.r;
        aggregatedColor.g += (1 - aggregatedColor.a) * colorWithOpacity.a * colorWithOpacity.g;
        aggregatedColor.b += (1 - aggregatedColor.a) * colorWithOpacity.a * colorWithOpacity.b;
        aggregatedColor.a += (1 - aggregatedColor.a) * colorWithOpacity.a;
    }

    return aggregatedColor;
}

// ======= TODO: IMPLEMENT ========
// This function should return an opacity value for the given intensity and gradient according to the 2D transfer function.
// Calculate whether the values are within the radius/intensity triangle defined in the 2D transfer function widget.
// If so: return a tent weighting as described in the assignment
// Otherwise: return 0.0f
//
// The 2D transfer function settings can be accessed through m_config.TF2DIntensity and m_config.TF2DRadius.
float Renderer::getTF2DOpacity(float intensity, float gradientMagnitude) const
{
    float maxGradient = m_pGradientVolume->maxMagnitude();
    float radius = m_config.TF2DRadius;
    float intensityApex = m_config.TF2DIntensity;

    float opacity = 0.0f;

    // check if the given point (intensity, gradientMagnitude) is inside the triangle of the 2DTF function
    if (gradientMagnitude >= (maxGradient / radius) * (intensity - intensityApex) &&
        gradientMagnitude >= (maxGradient / radius) * (intensityApex - intensity)) {
            // do a linear interpolation between the edge of the triangle and the vertical line passing from its apex
            // in the vertical -> opacity = max
            // in the edges -> opacity = 0
            float f = (intensity - intensityApex) / (radius * gradientMagnitude);
            opacity = m_config.TF2DColor.a * (1 - f) + 0 * f;  // for clarity
        }
    
    return opacity;
}

// This function computes if a ray intersects with the axis-aligned bounding box around the volume.
// If the ray intersects then tmin/tmax are set to the distance at which the ray hits/exists the
// volume and true is returned. If the ray misses the volume the the function returns false.
//
// If you are interested you can learn about it at.
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool Renderer::instersectRayVolumeBounds(Ray& ray, const Bounds& bounds) const
{
    const glm::vec3 invDir = 1.0f / ray.direction;
    const glm::bvec3 sign = glm::lessThan(invDir, glm::vec3(0.0f));

    float tmin = (bounds.lowerUpper[sign[0]].x - ray.origin.x) * invDir.x;
    float tmax = (bounds.lowerUpper[!sign[0]].x - ray.origin.x) * invDir.x;
    const float tymin = (bounds.lowerUpper[sign[1]].y - ray.origin.y) * invDir.y;
    const float tymax = (bounds.lowerUpper[!sign[1]].y - ray.origin.y) * invDir.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    tmin = std::max(tmin, tymin);
    tmax = std::min(tmax, tymax);

    const float tzmin = (bounds.lowerUpper[sign[2]].z - ray.origin.z) * invDir.z;
    const float tzmax = (bounds.lowerUpper[!sign[2]].z - ray.origin.z) * invDir.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    ray.tmin = std::max(tmin, tzmin);
    ray.tmax = std::min(tmax, tzmax);
    return true;
}

// This function inserts a color into the framebuffer at position x,y
void Renderer::fillColor(int x, int y, const glm::vec4& color)
{
    const size_t index = static_cast<size_t>(m_config.renderResolution.x * y + x);
    m_frameBuffer[index] = color;
}
}