//
// Created by Luca Warmenhoven on 17/05/2024.
//

#ifndef GRAPHICS_TEST_RENDERING_H
#define GRAPHICS_TEST_RENDERING_H

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <OpenGL/gl3.h>
#include "GLFW/glfw3.h"
#include <vector>

using namespace glm;

#define RENDER_MODE_2D 0
#define RENDER_MODE_3D 1

#define VBO_POSITION_INDEX 0
#define VBO_NORMAL_INDEX 1
#define VBO_UV_INDEX 2

class Transformation {
public:
    vec3 position;
    vec3 scale;
    union {
        vec3 rotation;
        struct
        {
            float pitch;
            float yaw;
            float roll;
        };
    };
};

/**
 * Class representing a drawable object.
 * The draw method must be overridden when extending this class.
 */
class Drawable : public Transformation
{
public:
    Drawable(vec3 position, vec3 scale, vec3 rotation);
    virtual void draw(float deltaTime) = 0;
};

/**
 * Class representing an updatable object.
 */
class Updatable
{
public:
    virtual void update(float deltaTime) = 0;
};

/**
 * Struct representing a frustum edge plane.
 * This contains all the points that define the plane.
 */
typedef struct
{
    vec3 normal;
} Plane;

/**
 * Struct representing a
 */
typedef struct
{
    Plane top;
    Plane bottom;
    Plane left;
    Plane right;
    Plane near;
    Plane far;
    float zFar;
    float zNear;
    float aspect;
    float fov;
} Frustum;

/**
 * Function for creating a new frustum.
 *
 * @param zNear The near clipping plane
 * @param zFar  The far clipping plane
 * @param fov   The field of view
 * @param aspect The aspect ratio of the frustum
 * @return A new frustum
 */
Frustum createFrustum(float zNear, float zFar, float fov, float aspect);

/**
 * Function for checking whether the provided point lies within the view frustum.
 * This function checks whether the provided point lies within all planes of the frustum.
 *
 * @param reference The camera to check the frustum for
 * @param frustum The reference view frustum to check against
 * @param referencePosition The position to check
 * @return Whether the point lies within the frustum
 */
bool isWithinFrustum(Transformation *reference, Frustum frustum, vec3 referencePosition);


class Renderer : public Transformation
{
    /** The model, view and projection matrices. */
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    mat4 modelViewProjectionMatrix;

    unsigned char renderMode;

public:

    Renderer();

    /**
     * Function that prepares all the matrices for further use.
     * These matrices can then be pushed to the provided shader by calling `pushMatrices(programId)`.
     */
    void computeMatrices(float fov, float zNear, float zFar, float width, float height);

    /**
     * Function that prepares the object for rendering.
     */
    void pushMatrices(GLuint programId);

    /**
     * Function that translates the object.
     * @param pos The pos to translate to.
     */
    void translate(vec3 pos);

    /**
     * Function that translates the object.
     * @param x The x position to translate to.
     * @param y The y position to translate to.
     * @param z The z position to translate to.
     */
    void translate(float x, float y, float z);

    /**
     * Function that translates the object on the x axis.
     * @param x The x position to translate to.
     */
    void translateX(float x);

    /**
     * Function that translates the object on the y axis.
     * @param y The y position to translate to.
     */
    void translateY(float y);

    /**
     * Function that translates the object on the z axis.
     * @param z The z position to translate to.
     */
    void translateZ(float z);

    /**
     * Function that rotates the object.
     * @param rotation The rotation to rotate to.
     */
    void rotate(vec4 rotation);

    /**
     * Function that rotates the object.
     * @param x The x rotation to rotate to.
     * @param y The y rotation to rotate to.
     * @param z The z rotation to rotate to.
     */
    void rotate(float x, float y, float z);

    /**
     * Function that rotates the object on the x axis.
     * @param radians The x rotation to rotate to.
     */
    void rotateX(float radians);

    /**
     * Function that rotates the object on the y axis.
     * @param radians The y rotation to rotate to.
     */
    void rotateY(float radians);

    /**
     * Function that rotates the object on the z axis.
     * @param radians The z rotation to rotate to.
     */
    void rotateZ(float radians);

    /**
     * Function that scales the object.
     *
     * @param scalingFactor The scalingFactor to scalingFactor to.
     */
    void scale(vec3 scalingFactor);

    /**
     * Function that scales the object.
     *
     * @param x The x scale to scale to.
     * @param y The y scale to scale to.
     * @param z The z scale to scale to.
     */
    void scale(float x, float y, float z);

    /**
     * Function that scales the object.
     *
     * @param scalar The scalar to scale to.
     */
    void scale(float scalar);

    /**
     * Function that scales the object on the x axis.
     * @param x The x scale to scale to.
     */
    void scaleX(float x);

    /**
     * Function that scales the object on the y axis.
     * @param y The y scale to scale to.
     */
    void scaleY(float y);

    /**
     * Function that scales the object on the z axis.
     * @param z The z scale to scale to.
     */
    void scaleZ(float z);

    /**
     * Function that sets the model matrix.
     * @param model The model matrix to set.
     */
    void setModelMatrix(mat4 model);

    /**
     * Function that sets the view matrix.
     * @param view The view matrix to set.
     */
    void setViewMatrix(mat4 view);

    /**
     * Function that sets the projection matrix.
     * @param projection The projection matrix to set.
     */
    void setProjectionMatrix(mat4 projection);

    /**
     * Function that sets the model, view and projection matrices.
     * @param model The model matrix to set.
     * @param view The view matrix to set.
     * @param projection The projection matrix to set.
     */
    void setModelViewProjectionMatrix(mat4 model, mat4 view, mat4 projection);

    /**
     * Function that sets the draw mode.
     * @param renderMode The draw mode to set.
     */
    void setRenderMode(unsigned char renderMode);

    /**
     * Function that resets the matrices.
     */
    void resetMatrices();

    static const mat4 IDENTITY;

};

/**
 * A struct representing a vertex.
 */
typedef struct
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
} Vertex;


#endif //GRAPHICS_TEST_RENDERING_H