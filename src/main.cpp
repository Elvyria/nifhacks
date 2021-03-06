#include <iostream>
#include <stdio.h>
#include <string>

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <fmt/core.h>
#include <tiny_obj_loader.h>
#include <fplus/fplus.hpp>

#include <NifFile.h>

#include <Windows.h>

using namespace boost::filesystem;

#define WHITE "\033[0m"
#define RED   "\033[31m"
#define GREEN "\033[32m"

int transfer_vertices(const std::string &obj_filename, const std::string &nif_filename) {
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;

	std::string warn, err;

	if (!tinyobj::LoadObj(&attributes, &shapes, nullptr, &warn, &err, obj_filename.c_str(), NULL, true)) {
		std::cerr << err << std::endl;

		return 1;
	}

	auto nifile = NifFile(nif_filename);
	auto nishapes = nifile.GetShapes();

	std::vector<NiShape*> identical_shapes;

	for (auto nishape : nishapes) {
		auto vertices = nishape->get_vertices();

		if (vertices && vertices->size() == attributes.vertices.size() / 3) {
			identical_shapes.emplace_back(nishape);
		}
	}

	NiShape* nishape;

	// TODO: Sideeffects
	if (identical_shapes.size() == 0)
	{
		fmt::print("Couldn't find a shape with the same ammount of vertices. Expected: {}", attributes.vertices.size());

		return 1;
	}
	else if (identical_shapes.size() > 1)
	{
		fmt::print("\n");

		for (size_t i = 0; i < identical_shapes.size(); i++) {
			fmt::print(GREEN "{}" WHITE ": {}\n", i, identical_shapes[i]->GetName());
		}

		fmt::print("\nMultiple shapes with the same amount of vertices where found.\nEnter the number of shape, which will acquire all data.\n");

		int index;
		std::cin >> index;

		if (index >= 0 && index < identical_shapes.size())
			nishape = identical_shapes[index];
		else
			return 1;
	}
	else if (identical_shapes.size() == 1)
	{
		nishape = identical_shapes[0];
	}

	// TODO: Separate

	auto vertices = nishape->get_vertices();

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

		nishape->set_vertices(vertices);
	}

	auto normals = nishape->get_normals(false);

	if (normals && normals->size() == attributes.normals.size() / 3) {
		std::vector<Vector3> normals;
		normals.reserve(attributes.normals.size() / 3);

		for (size_t i = 0; i < attributes.vertices.size(); i += 3) {
			normals.emplace_back(
					attributes.normals[i],
					attributes.normals[i + 1],
					attributes.normals[i + 2]);
		}

		nishape->set_normals(normals);
	}

	auto uv = nishape->get_uv();

	if (uv && uv->size() == attributes.texcoords.size() / 2) {
		std::vector<Vector2> uv;
		uv.reserve(attributes.texcoords.size() / 2);

		for (size_t i = 0; i < attributes.vertices.size(); i += 2) {
			uv.emplace_back(
					attributes.texcoords[i],
					1.0 - attributes.texcoords[i + 1]);
		}

		nishape->set_uv(uv);
	}

	nifile.Save(nif_filename + ".nif");

	return 0;
}

void export_shape(std::FILE* stream, NiShape &shape, int v_offset = 1, int vt_offset = 1, int vn_offset = 1) {
	const std::vector<Vector3>* vertices = shape.get_vertices();
	const std::vector<Vector2>* uv = shape.get_uv();
	const std::vector<Vector3>* normals = shape.get_normals(false);

	std::vector<Triangle> faces;
	shape.GetTriangles(faces);

	const auto v_size = vertices->size();
	const auto vn_size = normals ? normals->size() : 0;
	const auto vt_size = uv ? uv->size() : 0;

	fmt::print(stream,
			"# NifHacks 0.1\n\n"
			"# {} Vertices\n"
			"# {} Texture coordinates\n"
			"# {} Normals\n"
			"# {} Faces\n",
			v_size, vt_size, vn_size, faces.size());

	const std::string name = shape.GetName();

	if (!name.empty()) {
		fmt::print(stream, "o {}\n", name.c_str());
	}

	std::string f_format = "{0}";

	for (auto &v : *vertices) {
		fmt::print(stream, "v {} {} {}\n", v.x, v.y, v.z);
	}

	fmt::print(stream, "\n");

	if (uv) {
		for (auto &p : *uv) {
			fmt::print(stream, "vt {} {}\n", p.u, 1.0 - p.v);
		}

		fmt::print(stream, "\n");

		f_format += "/{0}";
	}

	if (normals) {
		for (auto &p : *normals) {
			fmt::print(stream, "vn {} {} {}\n", p.x, p.y, p.z);
		}

		fmt::print(stream, "\n");

		f_format += "/{0}";
	}

	f_format = fmt::format("f {} {} {}\n",
			f_format,
			boost::replace_all_copy(f_format, "0", "1"),
			boost::replace_all_copy(f_format, "0", "2"));

	for (auto &f : faces) {
		fmt::print(stream, f_format,
				f.p1 + v_offset,
				f.p2 + vt_offset,
				f.p3 + vn_offset);
	}

	fmt::print(stream, "\n");
}

int export_shape(const std::string &nif_filename, const std::string &obj_filename, const std::string &shape_name) {
	auto nifile = NifFile(nif_filename);

	auto shape = nifile.FindBlockByName<NiShape>(shape_name);

	if (shape) {
		FILE* obj_file;
		fopen_s(&obj_file, obj_filename.c_str(), "w");

		export_shape(obj_file, *shape, 1, 1, 1);

		fclose(obj_file);

		return 0;
	}

	return 1;
}

#ifdef _WIN32

void enable_virtual_terminal() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) return;

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode)) return;

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode)) return;
}

#endif

int main(int argc, char *argv[]) {

#ifdef _WIN32
	enable_virtual_terminal();
#endif

	if (argc < 3) {
		fmt::print("USAGE:\n	nifhack [source] [target]\n");

		return 1;
	}

	auto source = argv[1];
	auto target = argv[2];

	if (!exists(source)) {
		fmt::print("File doesn't exist\n");

		return 1;
	}

	path source_path (source);
	path target_path (target);

	auto source_extension = source_path.extension().string();
	auto target_extension = target_path.extension().string();
	
	if (strcmp(source_extension.c_str(), ".obj") == 0 && strcmp(target_extension.c_str(), ".nif") == 0)
	{
		transfer_vertices(source, target);
	}
	else if (strcmp(source_extension.c_str(), ".nif") == 0 && strcmp(target_extension.c_str(), ".obj") == 0)
	{
		if (exists(target)) {
			fmt::print(RED "Warning! Target already exists and will be overwritten.\n" WHITE);
		}

		auto nifile = NifFile(source);

		NiShape* shape;

		auto shapes = nifile.GetShapes();

		if (shapes.size() == 0) {
			fmt::print("No shapes to export.\n");
			return 1;
		}

		fmt::print("\n");

		for (size_t i = 0; i < shapes.size(); i++) {
			fmt::print(GREEN "{}" WHITE ": {}\n", i, shapes[i]->GetName());
		}

		fmt::print("\nEnter the number of shape you want to export.\n");

		int index;
		std::cin >> index;

		if (index < 0 || index >= shapes.size()) {
			return 1;
		}

		shape = shapes[index];

		// TODO: Add cli argument
		// shape = nifile.FindBlockByName<NiShape>(argv[3]);

		if (!shape) {
			fmt::print("Couldn't find shape in source file.\n");

			return 1;
		}

		FILE* obj_file;
		fopen_s(&obj_file, target, "w");

		export_shape(obj_file, *shape);

		fclose(obj_file);
	}


}
