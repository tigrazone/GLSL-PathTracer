#include <miniply.h>
#include <stdint.h>

//
// Topology enum
//

enum class Topology {
  Soup,   // Every 3 indices specify a triangle.
  Strip,  // Triangle strip, triangle i uses indices i, i-1 and i-2
  Fan,    // Triangle fan, triangle i uses indices, i, i-1 and 0.
};


//
// TriMesh type
//

// This is what we populate to test & benchmark data extraction from the PLY
// file. It's a triangle mesh, so any faces with more than three verts will
// get triangulated.
//
// The structure can hold individual triangles, triangle strips or triangle
// fans (pick one). If it's strips or fans, you can use an optional
// terminator value to indicate where one strip/fan ends and a new one begins.
struct TriMesh {
	// Per-vertex data
	float* pos = nullptr; // has 3*numVerts elements.
	float* normal = nullptr; // if non-null, has 3 * numVerts elements.
	float* uv = nullptr; // if non-null, has 2 * numVerts elements.
	uint32_t numVerts = 0;

	// Per-index data
	int* indices = nullptr; // has numIndices elements.
	uint32_t numIndices = 0; // number of indices = 3 times the number of faces.

	Topology topology = Topology::Soup; // How to interpret the indices.
	bool hasTerminator = false;          // Only applies when topology != Soup.
	int terminator = -1;             // Value indicating the end of a strip or fan. Only applies when topology != Soup.

	~TriMesh();

	bool all_indices_valid() const;
};

extern TriMesh* parse_file_with_miniply(const char* filename, bool assumeTriangles);
