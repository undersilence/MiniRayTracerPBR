#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H

#include "global.hpp"
#include "Vector.hpp"

enum MaterialType { DIFFUSE, TRANSPARENT, MICROFACET };

class Material {
private:

	// Compute reflection direction
	Vector3f reflect(const Vector3f& I, const Vector3f& N) const {
		return I - 2 * dotProduct(I, N) * N;
	}

	// Compute refraction direction using Snell's law
	//
	// We need to handle with care the two possible situations:
	//
	//    - When the ray is inside the object
	//
	//    - When the ray is outside.
	//
	// If the ray is outside, you need to make cosi positive cosi = -N.I
	//
	// If the ray is inside, you need to invert the refractive indices and negate the normal N
	Vector3f refract(const Vector3f& I, const Vector3f& N, const float& ior) const {
		float cosi = clamp(-1, 1, dotProduct(I, N));
		float etai = 1, etat = ior;
		Vector3f n = N;
		if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n = -N; }
		float eta = etai / etat;
		float k = 1 - eta * eta * (1 - cosi * cosi);
		return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
	}

	// Compute Fresnel equation
	//
	// \param I is the incident view direction
	//
	// \param N is the normal at the intersection point
	//
	// \param ior is the material refractive index
	//
	// \param[out] kr is the amount of light reflected
	void fresnel(const Vector3f& I, const Vector3f& N, const float& ior, float& kr) const {
		float cosi = clamp(-1, 1, dotProduct(I, N));
		float etai = 1, etat = ior;
		if (cosi > 0) { std::swap(etai, etat); }
		// Compute sini using Snell's law
		float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
		// Total internal reflection
		if (sint >= 1) {
			kr = 1;
		} else {
			float cost = sqrtf(std::max(0.f, 1 - sint * sint));
			cosi = fabsf(cosi);
			float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
			float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
			kr = (Rs * Rs + Rp * Rp) / 2;
		}
		// As a consequence of the conservation of energy, transmittance is given by:
		// kt = 1 - kr;
	}

	Vector3f toWorld(const Vector3f& a, const Vector3f& N) const {
		Vector3f B, C;
		if (std::fabs(N.x) > std::fabs(N.y)) {
			float invLen = 1.0f / std::sqrt(N.x * N.x + N.z * N.z);
			C = Vector3f(N.z * invLen, 0.0f, -N.x * invLen);
		} else {
			float invLen = 1.0f / std::sqrt(N.y * N.y + N.z * N.z);
			C = Vector3f(0.0f, N.z * invLen, -N.y * invLen);
		}
		B = crossProduct(C, N);
		return a.x * B + a.y * C + a.z * N;
	}

	float distributionGGX(const Vector3f& N, const Vector3f& H, const float& a) const {
		float a2 = a * a;
		float NdotH = std::max(dotProduct(N, H), 0.0f);
		float NdotH2 = NdotH * NdotH;
		float nom = a2;
		float denom = (NdotH2 * (a2 - 1.0) + 1.0);
		denom = M_PI * denom * denom;
		return nom / denom;
	}

	Vector3f fresnelSchlick(float cosTheta, const Vector3f& F0) const {
		return F0 + (Vector3f(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
	}

	float geometrySchlickGGX(float NdotV, float k) const {
		float nom = NdotV;
		float denom = NdotV * (1.0 - k) + k;

		return nom / denom;
	}

	float geometrySmith(const Vector3f& N, const Vector3f& V, const Vector3f& L, float k) const {
		float NdotV = std::max(dotProduct(N, V), 0.0f);
		float NdotL = std::max(dotProduct(N, L), 0.0f);
		float ggx1 = geometrySchlickGGX(NdotV, k);
		float ggx2 = geometrySchlickGGX(NdotL, k);

		return ggx1 * ggx2;
	}

	Vector3f sampleGGX(const Vector3f& wi, const Vector3f& N) const {
		float r0 = get_random_float();
		float r1 = get_random_float();
		float a2 = roughness * roughness;
		float theta = acosf(sqrtf((1 - r0) / ((a2 - 1) * r0 + 1)));
		float phi = 2 * M_PI * r1;
		Vector3f wm = SphericalToCartesian(theta, phi);
		Vector3f wm_w = toWorld(wm, N);
		Vector3f wo = 2 * wm_w * dotProduct(wm_w, wi) - wi;
		return wo;
	}

	float pdfGGX(const Vector3f& wi, const Vector3f& wo, const Vector3f& N) const {
		Vector3f H = normalize(wo + wi);
		float a2 = roughness * roughness;
		float costheta = dotProduct(N, H);
		float exp = (a2 - 1) * costheta * costheta + 1;
		float D = a2 / (M_PI * exp * exp);
		float pdf = (D * costheta) / (4 * dotProduct(wo, H));
		return pdf;
	}

public:
	MaterialType m_type;
	//Vector3f m_color;
	Vector3f m_emission;
	float ior;
	Vector3f albedo;
	float roughness;
	float metallic;
	float specularExponent;
	//Texture tex;

	inline Material(MaterialType t = DIFFUSE, Vector3f e = Vector3f(0, 0, 0), float roughness = 0.3, float metallic = 0.4);
	inline MaterialType getType();
	//inline Vector3f getColor();
	inline Vector3f getColorAt(double u, double v);
	inline Vector3f getEmission();
	inline bool hasEmission();

	// sample a ray by Material properties
	inline Vector3f sample(const Vector3f& wi, const Vector3f& N);
	// given a ray, calculate the PdF of this ray
	inline float pdf(const Vector3f& wi, const Vector3f& wo, const Vector3f& N);
	// given a ray, calculate the contribution of this ray
	inline Vector3f eval(const Vector3f& wi, const Vector3f& wo, const Vector3f& N);

};

Material::Material(MaterialType t, Vector3f e, float roughness, float metallic) {
	this->m_type = t;
	//m_color = c;
	this->m_emission = e;
	this->roughness = roughness;
	this->metallic = metallic;
}

MaterialType Material::getType() { return m_type; }
///Vector3f Material::getColor(){return m_color;}
Vector3f Material::getEmission() { return m_emission; }
bool Material::hasEmission() {
	if (m_emission.norm() > EPSILON) return true;
	else return false;
}

Vector3f Material::getColorAt(double u, double v) {
	return Vector3f();
}


Vector3f Material::sample(const Vector3f& wi, const Vector3f& N) {
	switch (m_type) {
	case DIFFUSE:
	{
		// uniform sample on the hemisphere
		float x_1 = get_random_float(), x_2 = get_random_float();
		float z = std::fabs(1.0f - 2.0f * x_1);
		float r = std::sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
		Vector3f localRay(r * std::cos(phi), r * std::sin(phi), z);
		return toWorld(localRay, N);
		break;
	}
	case MICROFACET:
	{
		return sampleGGX(wi, N);
		break;
	}
	}
}

float Material::pdf(const Vector3f& wi, const Vector3f& wo, const Vector3f& N) {

	switch (m_type) {
	case DIFFUSE:
	{
		if (dotProduct(wi, N) > 0.0f)
			return 0.5f / M_PI;
		else return 0.0f;
		break;
	}
	case MICROFACET:
		return pdfGGX(wi, wo, N);
		break;
	}
}

Vector3f Material::eval(const Vector3f& wi, const Vector3f& wo, const Vector3f& N) {
	switch (m_type) {
	case DIFFUSE:
	{
		// calculate the contribution of diffuse   model
		float cosalpha = dotProduct(N, wi);
		if (cosalpha > 0.0f) {
			Vector3f diffuse = albedo / M_PI;
			return diffuse;
		} else
			return Vector3f(0.0f);
		break;
	}
	case MICROFACET:
	{
		float cosalpha = dotProduct(N, wi);
		float costheta = dotProduct(N, wo);
		if (cosalpha > 0.0f) {
			Vector3f H = normalize(wi + wo);
			float NDF = distributionGGX(N, H, roughness);
			float G = geometrySmith(N, wo, wi, (roughness + 1) * (roughness + 1) / 8);
			Vector3f F0(0.04f);
			F0 = lerp(F0, albedo, metallic);
			Vector3f F = fresnelSchlick(costheta, F0);
			Vector3f Ks = F;
			Vector3f Kd = (Vector3f(1) - Ks) * (1.0f - metallic);
			return Kd * albedo / M_PI + NDF * F * G / std::max(0.0001f, (4 * cosalpha * std::max(costheta, 0.0f)));
		} else {
			return Vector3f(0.0f);
		}
		break;
	}
	}
}

#endif //RAYTRACING_MATERIAL_H
