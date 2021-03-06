// GLTFParserWithTK.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/Serialize.h>
#include <GLTFSDK/GLTFResourceWriter.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "tclap\CmdLine.h"

using namespace TCLAP;
using namespace Microsoft::glTF;
using namespace std;

const string delimiter = "/";

class InStream : public IStreamReader
{
public:
	InStream() : 
		m_stream(make_shared<stringstream>(ios_base::app | ios_base::binary | ios_base::in | ios_base::out))
	{
	}

	shared_ptr<istream> GetInputStream(const  string&) const override
	{
		return static_pointer_cast<istream>(m_stream);
	}

private:
	 shared_ptr<stringstream> m_stream;
};

static ostream& operator<<(ostream &out, const Scene& scene)
{
	return out << "name: " 
			   << scene.name.c_str() 
			   << endl
		       << "id: " 
		       << scene.id 
			   << endl
			   << "num nodes: "
			   << scene.nodes.size() 
			   << endl;
}

class indent
{
public:
	int level = 1;
};

static indent indenter;

static ostream& operator<<(ostream &out, const indent& indenter)
{
	for (int i = 0; i < indenter.level; i++)
	{
		out << "----";
	}
	return out;
}

static ostream& operator<<(ostream &out, const Node& node)
{
	return out << indenter << "name: "
		<< node.name.c_str()
		<< endl
		<< indenter << "id: "
		<< node.id;
}

static ostream& operator<<(ostream &out, const Mesh& mesh)
{
	return out << indenter << "mesh: "
		<< mesh.name.c_str()
		<< endl
		<< indenter << "id: "
		<< mesh.id;
}

void PrintChildren(GLTFDocument& gltfDoc, vector<string>& children, int level)
{
	for (auto el: children)
	{
		auto nd = gltfDoc.nodes.Get(el);
		indenter.level = level;
		cout << nd << endl;
		if (nd.meshId.length() > 0)
		{
			auto mesh = gltfDoc.meshes.Get(nd.meshId);
			cout << mesh << endl;

			for (auto prim : mesh.primitives)
			{
				auto acc = gltfDoc.accessors[prim.positionsAccessorId];
				cout << indenter << "vertex count: " << acc.count << endl;
			}
		}
		PrintChildren(gltfDoc, nd.children, level + 1);
	}
}

int main(int argc, char **argv)
{
	CmdLine cmd("GLTF File Info", ' ', "0.1");

	// Define a value argument and add it to the command line.
	ValueArg<string> inputFileNameArg("f", "input", "input file", true, "Buggy.glb", "filename");
	cmd.add(inputFileNameArg);

	cmd.parse(argc, argv);

	auto input = inputFileNameArg.getValue();

	auto glbStream =  make_shared<ifstream>(input,  ios::binary);
	auto streamReader =  make_unique<InStream>();
	
	GLBResourceReader reader(*streamReader, glbStream);
	
	// get json from GLB and deserialize into GLTFDocument
	string json = reader.GetJson();
	GLTFDocument gltfDoc = DeserializeJson(json);

	int numScenes = gltfDoc.scenes.Elements().size();
	cout << numScenes << " scenes" << endl;

	int sceneIdx = 0;
	for (auto el : gltfDoc.scenes.Elements())
	{
		cout << "scene " << ++sceneIdx << endl;
		cout << "-----------------------" << endl;
		int level = 1;
		cout << el << endl;
		for (auto node : el.nodes)
		{
			auto nd = gltfDoc.nodes.Get(node);
			cout << node << endl;

			PrintChildren(gltfDoc, nd.children, level);
		}

		cout << "-----------------------" << endl;
	}

	int idx = 0;

	// Read an image out of the binary data...
	for (auto image : gltfDoc.images.Elements())
	{
		auto data = reader.ReadBinaryData(gltfDoc, image);
		auto extension = image.mimeType.substr(image.mimeType.find(delimiter) + 1, image.mimeType.length());;
		string name = "img" + std::to_string(idx++) + string(".") + extension;

		ofstream fs(name, ios::binary);
		fs.write((const char *)data.data(), data.size());
	}

    return 0;
}

