#include <RayTracing/ImgWindow.h>
#include <RayTracing/RayCamera.h>

#include <Utility/Image.h>
#include <Utility/LambdaOp.h>
#include <Utility/ImgPixelSet.h>

#include <RayTracing/Sphere.h>
#include <RayTracing/Group.h>
#include <RayTracing/OpMaterial.h>
#include <RayTracing/Sky.h>

#include "Defines.h"

using namespace CppUtility::Other;
using namespace RayTracing;
using namespace Define;
using namespace glm;
using namespace std;
typedef vec3 rgb;

Hitable::Ptr CreateScene();
rgb RayTracer(Ptr<Hitable> scene, Ray::Ptr & ray);
rgb Background(const Ray::Ptr & ray);

int main(int argc, char ** argv){
	ImgWindow imgWindow(str_WindowTitle);
	if (!imgWindow.IsValid()) {
		printf("ERROR: Image Window Create Fail.\n");
		return 1;
	}

	Image & img = imgWindow.GetImg();
	const size_t val_ImgWidth = img.GetWidth();
	const size_t val_ImgHeight = img.GetHeight();
	const size_t val_ImgChannel = img.GetChannel();

	ImgPixelSet pixelSet(val_ImgWidth, val_ImgHeight);

	vec3 origin(0, 0, 0);
	vec3 viewPoint(0, 0, -1);
	float ratioWH = (float)val_ImgWidth / (float)val_ImgHeight;

	RayCamera::Ptr camera = ToPtr(new RayCamera(origin, viewPoint, ratioWH, 90.0f));

	auto scene = CreateScene();

	LambdaOp::Ptr imgUpdate = ToPtr(new LambdaOp([&]() {
		static double loopMax = 100;
		static uniform_real_distribution<> randMap(0.0f,1.0f);
		static default_random_engine engine;
		loopMax = glm::max(100 * imgWindow.GetScale(), 1.0);
		int cnt = 0;
		while (pixelSet.Size() > 0) {
			auto pixel = pixelSet.RandPick();
			size_t i = pixel.x;
			size_t j = pixel.y;
			rgb color(0);
			const size_t sampleNum = 4;
			for (size_t k = 0; k < sampleNum; k++) {
				float u = (i + randMap(engine)) / (float)val_ImgWidth;
				float v = (j + randMap(engine)) / (float)val_ImgHeight;
				Ray::Ptr ray = camera->GenRay(u, v);
				color += RayTracer(scene, ray);
			}
			color /= sampleNum;
			float r = color.r;
			float g = color.g;
			float b = color.b;
			img.SetPixel(i, val_ImgHeight - 1 - j, Image::Pixel<float>(r, g, b));
			if (++cnt > loopMax)
				return;
		}
		imgUpdate->SetIsHold(false);
	}, true));

	imgWindow.Run(imgUpdate);

	return 0;
}

Hitable::Ptr CreateScene() {
	auto normalMaterial = ToPtr(new OpMaterial([](HitRecord & rec)->bool{
		vec3 lightColor = 0.5f * (rec.normal + 1.0f);
		rec.ray->SetLightColor(lightColor);
		return false;
	}));

	auto skyMat = ToPtr(new OpMaterial([](HitRecord & rec)->bool {
		float t = 0.5 * (rec.pos.y + 1.0f);
		rgb white = rgb(1.0f, 1.0f, 1.0f);
		rgb blue = rgb(0.5f, 0.7f, 1.0f);
		rgb lightColor = (1 - t) * white + t * blue;
		rec.ray->SetLightColor(lightColor);
		return false;
	}));
	auto sky = ToPtr(new Sky(skyMat));

	auto sphere = ToPtr(new Sphere(vec3(0, 0, -1), 0.5f, normalMaterial));
	auto sphereBottom = ToPtr(new Sphere(vec3(0, -100.5f, -1), 100.0f, normalMaterial));
	auto group = ToPtr(new Group);
	(*group) << sphere << sphereBottom << sky;

	return group;
}

rgb RayTracer(Ptr<Hitable> scene, Ray::Ptr & ray) {
	auto hitRst = scene->RayIn(ray);
	if (hitRst.hit) {
		if (hitRst.hitable->RayOut(hitRst.record))
			return RayTracer(scene, ray);
		else
			return ray->GetColor();
	}
	else
		return rgb(0);
}

rgb Background(const Ray::Ptr & ray) {
	float t = 0.5*(normalize(ray->GetDir()).y + 1.0f);
	rgb white = rgb(1.0f, 1.0f, 1.0f);
	rgb blue = rgb(0.5f, 0.7f, 1.0f);
	return (1 - t)*white + t * blue;
}