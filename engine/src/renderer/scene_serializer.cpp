#include "renderer/scene_serializer.h"

#include "core/assets/asset_manager.h"
#include "core/assets/types/model.h"
#include "core/ecs/entity.h"
#include "core/ecs/entity_impl.h"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
 
// -------------------------------------------------------
// YAML helpers for GLM types
// -------------------------------------------------------
namespace YAML {
 
template<>
struct convert<glm::vec3> {
	static Node encode(const glm::vec3& v) {
		Node n(NodeType::Sequence);
		n.push_back(v.x); n.push_back(v.y); n.push_back(v.z);
		n.SetStyle(EmitterStyle::Flow);
		return n;
	}
	static bool decode(const Node& n, glm::vec3& v) {
		if (!n.IsSequence() || n.size() < 3) return false;
		v.x = n[0].as<float>();
		v.y = n[1].as<float>();
		v.z = n[2].as<float>();
		return true;
	}
};
	
template<>
struct convert<glm::quat> {
	// stored as [w, x, y, z]
	static Node encode(const glm::quat& q) {
		Node n(NodeType::Sequence);
		n.push_back(q.w); n.push_back(q.x); n.push_back(q.y); n.push_back(q.z);
		n.SetStyle(EmitterStyle::Flow);
		return n;
	}
	static bool decode(const Node& n, glm::quat& q) {
		if (!n.IsSequence() || n.size() < 4) return false;
		q.w = n[0].as<float>();
		q.x = n[1].as<float>();
		q.y = n[2].as<float>();
		q.z = n[3].as<float>();
		return true;
	}
};
 
} // namespace YAML

namespace ic
{
static void SerializeEntity(YAML::Emitter& out, ic::Entity e, ic::RenderScene* scene)
{
	out << YAML::BeginMap;
	out << YAML::Key << "id"   << YAML::Value << (uint64_t)e.GetUUID();
	out << YAML::Key << "name" << YAML::Value << e.GetName();

	out << YAML::Key << "components" << YAML::Value << YAML::BeginMap;

	// TransformComponent
	if (e.HasComponent<TransformComponent>())
	{
		auto& t = e.GetComponent<TransformComponent>();
		out << YAML::Key << "TransformComponent" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "position" << YAML::Value << YAML::convert<glm::vec3>::encode(t.position);
		out << YAML::Key << "rotation" << YAML::Value << YAML::convert<glm::quat>::encode(t.rotation);
		out << YAML::Key << "scale"    << YAML::Value << YAML::convert<glm::vec3>::encode(t.scale);
		out << YAML::EndMap;
	}

	// MeshComponent
	if (e.HasComponent<MeshComponent>())
	{
		auto& m = e.GetComponent<MeshComponent>();
		std::string modelName = AssetManager::Get().GetRegistry()->GetAssetName(m.modelID);
		if (!modelName.empty())
		{
		out << YAML::Key << "MeshComponent" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "model" << YAML::Value << modelName;
		out << YAML::EndMap;
		}
	}

	// HierarchyComponent — store parent UUID, not raw entt handle
	// raw handles are meaningless across save/load
	if (e.HasComponent<HierarchyComponent>())
	{
		auto& h = e.GetComponent<HierarchyComponent>();
		out << YAML::Key << "HierarchyComponent" << YAML::Value << YAML::BeginMap;

		// parent — store as UUID so deserialization can re-link by identity
		if (h.parent != entt::null)
		{
		ic::Entity parentEntity(h.parent, scene);
		out << YAML::Key << "parent" << YAML::Value << (uint64_t)parentEntity.GetUUID();
		}
		else
		{
		out << YAML::Key << "parent" << YAML::Value << 0;  // 0 = no parent
		}

		// children list — also as UUIDs
		out << YAML::Key << "children" << YAML::Value << YAML::BeginSeq;
		for (auto childHandle : h.children)
		{
		ic::Entity child(childHandle, scene);
		out << (uint64_t)child.GetUUID();
		}
		out << YAML::EndSeq;

		out << YAML::EndMap;
	}

	// CameraComponent
	if (e.HasComponent<CameraComponent>())
	{
		auto& cam = e.GetComponent<CameraComponent>().camera;
		out << YAML::Key << "CameraComponent" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "fov"  << YAML::Value << cam.fovY;
		out << YAML::Key << "aspect" << YAML::Value << cam.aspectRatio;
		out << YAML::Key << "near" << YAML::Value << cam.znear;
		out << YAML::Key << "far"  << YAML::Value << cam.zfar;
		out << YAML::EndMap;
	}

	out << YAML::EndMap;  // components
	out << YAML::EndMap;  // entity
}

void SceneSerializer::Serialize(const char* path, ic::RenderScene* scene)
{
	YAML::Emitter out;
	out << YAML::BeginMap;

	out << YAML::Key << "scene" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "name" << YAML::Value << "Unnamed Scene";
	out << YAML::EndMap;

	out << YAML::Key << "entities" << YAML::Value << YAML::BeginSeq;

	for (ic::Entity e : scene->GetAllEntities())
	{
		SerializeEntity(out, e, scene);
	}

	out << YAML::EndSeq;
	out << YAML::EndMap;

	// Replace by my own file open
	std::ofstream file(path);
	if (!file.is_open())
	{
		IC_CORE_ERROR("SceneSerializer: could not open {} for writing", path);
		return;
	}
	file << out.c_str();
}

ic::RenderScene* SceneSerializer::Deserialize(const char* path)
{
	YAML::Node root;
	try {
		root = YAML::LoadFile(path);
	} catch (const YAML::Exception& e) {
		IC_CORE_ERROR("SceneSerializer: YAML parse error in {}: {}", path, e.what());
		return new ic::RenderScene(); // return blank scene on failure
	}
 
	auto* scene = new ic::RenderScene();
	
	auto entitiesNode = root["entities"];
	if (!entitiesNode) return scene; // empty scene, valid
 
	for (auto node : entitiesNode) {
		std::string name = node["name"] ? node["name"].as<std::string>() : "Entity";
	
		ic::Entity e = scene->CreateEntityWithName(name);
	
		auto components = node["components"];
		if (!components) continue;
	
		// TransformComponent
		if (components["TransformComponent"]) {
			auto tc  = components["TransformComponent"];
			auto& t  = e.GetComponent<ic::TransformComponent>();
		
			if (tc["position"]) t.SetPosition(tc["position"].as<glm::vec3>());
			if (tc["rotation"]) t.SetRotation(tc["rotation"].as<glm::quat>());
			if (tc["scale"])    t.SetScale   (tc["scale"].as<glm::vec3>());
		}
	
		// MeshComponent
		if (components["MeshComponent"]) {
			std::string modelName = components["MeshComponent"]["model"].as<std::string>();
			IC_GUID id = ic::AssetManager::Get().GetRegistry()->GetAssetId(modelName.c_str());
			ic::AssetManager::Get().LoadAs<Model>(id);
			if (id != IC_GUID{})
				e.AddComponent<ic::MeshComponent>().SetMesh(id);
			else
				IC_CORE_WARN("SceneSerializer: model '{}' not found in registry", modelName);
		}
	}
 
    	return scene;
}


} // namespace ic
