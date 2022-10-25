#include <algorithm>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <CLI/CLI.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <tiny_obj_loader.h>

#include <NifFile.h>

#include "format.h"
#include "version.h"

namespace fs = std::filesystem;

#define WHITE "\033[0m"
#define RED   "\033[31m"
#define GREEN "\033[32m"

struct CLIOptions {
	fs::path input_file;
	fs::path output_file;
	bool skin;
	bool overwrite;
};

CLIOptions options = CLIOptions();

Vector3 transform_vertice(Vector3 v, MatTransform t, float w) {
	return t.rotation * (v * t.scale) + t.translation * w;
}

std::vector<Vector3> skin_vertices(NifFile &nifile, NiShape &shape, bool inverse = false) {
	auto vertices = *shape.get_vertices();

	std::vector<Vector3> transforms;
	transforms.resize(vertices.size());

	std::vector<int> bones;
	nifile.GetShapeBoneIDList(&shape, bones);

	for (int id : bones) {
		MatTransform transform;
		nifile.GetShapeTransformSkinToBone(&shape, id - 1, transform);

		std::unordered_map<ushort, float> weights;
		nifile.GetShapeBoneWeights(&shape, id - 1, weights);

		for (auto w : weights) {
			auto v = vertices[w.first];

			vertices[w.first] = transform_vertice(v, transform, w.second);
			transforms[w.first] += vertices[w.first] - v;
		}
	}

	return transforms;
}

// TODO: Optimize with type cast
void transfer_attributes(const tinyobj::attrib_t &attributes, NiShape &shape) {
	auto vertices = shape.get_vertices();

	if (vertices && vertices->size() == attributes.vertices.size() / 3)
	{
		std::vector<Vector3> vertices;
		vertices.reserve(attributes.vertices.size() / 3);

		for (size_t i = 0; i < attributes.vertices.size(); i += 3) {
			vertices.emplace_back(
					attributes.vertices[i],
					attributes.vertices[i + 1],
					attributes.vertices[i + 2]);
		}

		shape.set_vertices(vertices);
	}

	auto normals = shape.get_normals(false);

	if (normals && normals->size() == attributes.normals.size() / 3) {
		std::vector<Vector3> normals;
		normals.reserve(attributes.normals.size() / 3);

		for (size_t i = 0; i < attributes.normals.size(); i += 3) {
			normals.emplace_back(
					attributes.normals[i],
					attributes.normals[i + 1],
					attributes.normals[i + 2]);
		}

		shape.set_normals(normals);
	}

	auto uv = shape.get_uv();

	if (uv && uv->size() == attributes.texcoords.size() / 2) {
		std::vector<Vector2> uv;
		uv.reserve(attributes.texcoords.size() / 2);

		for (size_t i = 0; i < attributes.texcoords.size(); i += 2) {
			uv.emplace_back(
					attributes.texcoords[i],
					1.0 - attributes.texcoords[i + 1]);
		}

		shape.set_uv(uv);
	}
}

void export_shape(NiShape &shape, std::ostream &stream) {
	const std::vector<Vector3> vertices = *shape.get_vertices();
	const std::vector<Vector2>* uv = shape.get_uv();
	const std::vector<Vector3>* normals = shape.get_normals(false);

	std::vector<Triangle> faces;
	shape.GetTriangles(faces);

	fmt::print(stream,
			"# NifHacks 0.2\n\n"
			"# {} Vertices\n"
			"# {} Texture coordinates\n"
			"# {} Normals\n"
			"# {} Faces\n",
			vertices.size(),
			uv ? uv->size() : 0,
			normals ? normals->size() : 0,
			faces.size());

	if (auto name = shape.GetName(); !name.empty()) {
		fmt::print(stream, "\no {}\n\n", name);
	}

	for (auto v : vertices) {
		fmt::print(stream, "v {}\n", v);
	}

	fmt::print(stream, "\n");

	std::string f_zero = "{0}";

	if (uv) {
		for (auto &p : *uv) {
			fmt::print(stream, "vt {} {}\n", p.u, 1.0 - p.v);
		}

		fmt::print(stream, "\n");

		f_zero += "/{0}";
	}

	if (normals) {
		for (auto &p : *normals) {
			fmt::print(stream, "vn {}\n", p);
		}

		fmt::print(stream, "\n");

		f_zero += "/{0}";
	}

	auto f_one = f_zero;
	auto f_two = f_zero;

	std::replace(f_one.begin(), f_one.end(), '0', '1');
	std::replace(f_two.begin(), f_two.end(), '0', '2');

	auto f_format = fmt::format("f {} {} {}\n", f_zero, f_one, f_two);

	for (auto &f : faces) {
		fmt::print(stream, f_format, f.p1+1, f.p2+1, f.p3+1);
	}

	fmt::print(stream, "\n");
}

int obj_to_nif(const std::string &obj_filename, const std::string &nif_filename) {
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> obj_shapes;

	std::string err;

	if (!tinyobj::LoadObj(&attributes, &obj_shapes, nullptr, &err, obj_filename.c_str())) {
		std::cerr << err << std::endl;

		return 1;
	}

	auto nifile = NifFile(nif_filename);
	auto nif_shapes = nifile.GetShapes();

	std::vector<NiShape*> identical_shapes;

	for (auto shape : nif_shapes) {
		auto vertices = shape->get_vertices();

		if (vertices && vertices->size() == attributes.vertices.size() / 3) {
			identical_shapes.emplace_back(shape);
		}
	}

	if (identical_shapes.size() == 0)
	{
		fmt::print("Couldn't find a shape with the same ammount of vertices.\nExpected: {}\n", attributes.vertices.size());

		return 1;
	}

	NiShape* shape = identical_shapes[0];

	if (identical_shapes.size() > 1)
	{
		fmt::print("\n");

		for (size_t i = 0; i < identical_shapes.size(); i++) {
			fmt::print(GREEN "{}" WHITE ": {}\n", i, identical_shapes[i]->GetName());
		}

		fmt::print("\nMultiple shapes with the same amount of vertices where found.\nEnter the number of shape, which will acquire all data.\n");

		int index;
		std::cin >> index;

		if (index >= 0 && index < identical_shapes.size())
			shape = identical_shapes[index];
		else
			return 1;
	}

	auto transforms = skin_vertices(nifile, *shape);
	transfer_attributes(attributes, *shape);

	// TODO: Optimize with SIMD
	if (options.skin) {
		auto vertices = *shape->get_vertices();

		for (size_t i = 0; i < transforms.size(); i++) {
			vertices[i] -= transforms[i];
		}

		shape->set_vertices(vertices);
	}

	auto export_name = nif_filename;

	if (!options.overwrite) {
		export_name += ".nif";
	}

	nifile.Save(export_name);

	fmt::print("Successfully transfered coordinates to " GREEN "{}\n" WHITE, export_name);

	return 0;
}

int nif_to_obj(const std::string &nif_filename, const std::string &obj_filename) {
	if (fs::exists(obj_filename)) {
		fmt::print(RED "Warning! Target already exists and will be overwritten.\n" WHITE);
	}

	auto nifile = NifFile(nif_filename);

	auto shapes = nifile.GetShapes();

	if (shapes.size() == 0) {
		fmt::print("No shapes to export.\n");
		return 1;
	}

	fmt::print("\n");

	int index = 0;
	if (shapes.size() > 1) {
		for (size_t i = 0; i < shapes.size(); i++) {
			fmt::print(GREEN "{}" WHITE ": {}\n", i, shapes[i]->GetName());
		}

		fmt::print("\nEnter the number of shape you want to export.\n");

		std::cin >> index;

		if (index < 0 || index >= shapes.size()) {
			return 1;
		}
	}

	NiShape* shape = shapes[index];

	if (!shape) {
		fmt::print("Couldn't find shape in source file.\n");

		return 1;
	}

	// TODO: Optimize with SIMD
	if (options.skin) {
		auto transforms = skin_vertices(nifile, *shape);
		auto vertices = *shape->get_vertices();

		for (size_t i = 0; i < transforms.size(); i++) {
			vertices[i] += transforms[i];
		}

		shape->set_vertices(vertices);
	}

	std::ofstream obj_stream(obj_filename, std::ios::out);
	export_shape(*shape, obj_stream);

	fmt::print("Successfully exported " GREEN "{}" WHITE " to " GREEN "{}\n" WHITE, shape->GetName(), obj_filename);

	return 0;
}

int main(int argc, char *argv[]) {

	CLI::App app { "Janky tool that (sometimes) get things done for your nif-needs\n" };

	app.add_option("INPUT", options.input_file)
		->option_text("    *.nif *.obj")
		->required(true)
		->check(CLI::ExistingFile);

	app.add_option("OUTPUT", options.output_file)
		->option_text("   *.obj *.nif")
		->required(true);

	app.add_flag("-s,--skin", options.skin, "Apply skin transforms to shape");
	app.add_flag("-i,--in-place", options.overwrite, "Modify file in place");
	app.set_version_flag("-v,--version", fmt::format("nifhacks {:d}.{:d}.{:d}", NIFHACKS_VERSION_MAJOR, NIFHACKS_VERSION_MINOR, NIFHACKS_VERSION_PATCH));

	CLI11_PARSE(app, argc, argv);

	auto input_ext = options.input_file.extension();
	auto output_ext = options.output_file.extension();

	if (input_ext == ".obj" && output_ext == ".nif")
	{
		return obj_to_nif(options.input_file.string(), options.output_file.string());
	}

	if (input_ext == ".nif" && output_ext == ".obj")
	{
		return nif_to_obj(options.input_file.string(), options.output_file.string());
	}

	std::cerr << app.help() << std::flush;
}
