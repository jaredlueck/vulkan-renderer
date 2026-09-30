#pragma once
#include <glm/glm.hpp>

namespace matrix {
	static glm::mat4x4 translate(float x, float y, float z) {
		return glm::mat4x4(
			glm::vec4(1.0, 0.0, 0.0, 0.0),
			glm::vec4(0.0, 1.0, 0.0, 0.0),
			glm::vec4(0.0, 0.0, 1.0, 0.0),
			glm::vec4(x, y, z, 1.0)
		);
	}

	static glm::mat4x4 rotate(float radians, glm::vec3 axis) {
		// orthonormal basis with axis as +z
		axis = glm::normalize(axis);

		// account for when the axis we want to rotate around is the y axis
		glm::vec3 reference =
			glm::abs(axis.y) < 0.999f
			? glm::vec3(0.0f, 1.0f, 0.0f)
			: glm::vec3(1.0f, 0.0f, 0.0f);

		glm::vec3 right = glm::normalize(glm::cross(axis, reference));
		glm::vec3 up = glm::normalize(glm::cross(right, axis));
		
		glm::mat4 basis(
			glm::vec4(right, 0.0),
			glm::vec4(up, 0.0),
			glm::vec4(axis, 0.0),
			glm::vec4(0.0, 0.0, 0.0, 1.0)
		);

		glm::mat4 changeOfbasis = glm::transpose(basis);

		glm::mat4 rotateZ(
			glm::vec4(glm::cos(radians), -1 * glm::sin(radians), 0.0, 0.0),
			glm::vec4(glm::sin(radians), glm::cos(radians), 0.0, 0.0),
			glm::vec4(0.0, 0.0, 1.0, 0.0),
			glm::vec4(0.0, 0.0, 0.0, 1.0)
		);
		// perform roatation around z in new basis and then convert back to standard basis
		return basis * rotateZ * changeOfbasis;
	}

	static glm::mat4x4 scale(float x, float y, float z);

	// lookAt transforms the given coordinate to one from the camera's frame of reference
	static glm::mat4x4 lookAt(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
		glm::vec3 forward = glm::normalize(target - position);
		glm::vec3 right = glm::normalize(glm::cross(forward, up));
		up = glm::normalize(glm::cross(right, forward));

		glm::mat4x4 translate = matrix::translate(-position[0], -position[1], -position[2]);
		glm::mat4x4 basis = glm::mat4x4(glm::vec4(right, 0.0), glm::vec4(up, 0.0), glm::vec4(forward, 0.0), glm::vec4(0.0, 0.0, 0.0, 1.0));
		return glm::inverse(basis) * translate;
	}

	// vertical fov and aspect w/h
	// vulkan uses right handed coordinate system with +Y pointing down
	// z is between 0 and 1 in NDC
	static glm::mat4x4 perspective(float near, float far, float fov, float aspect) {
		float c = 1/glm::tan(fov / 2);
		float a = aspect;
		return glm::mat4x4(
			glm::vec4(c/a, 0.0, 0.0, 0.0),
			glm::vec4(0.0, -c, 0.0, 0.0),
			glm::vec4(0.0, 0.0, far / (far - near), 1.0),
			glm::vec4(0.0, 0.0, -far * near / (far - near), 0.0)
		);
	}
}