#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/range3f.h"

#include <iostream>
#include <math.h>
#define PI 3.14159265
PXR_NAMESPACE_USING_DIRECTIVE

void smooth(std::vector<GfVec3f> *, int, int, int);

int main(int argc, char *argv[])
{
    // Create the layer to populate.
    SdfLayerRefPtr layer = SdfLayer::CreateNew("soap-ring.usda");

    // Create a UsdStage with that root layer.
    UsdStageRefPtr stage = UsdStage::Open(layer);

	double r = 0.8;
	double height = 2;
	int numRows = 20;
	int numColumns = 20;

    // Now we'll populate the stage with content from the objStream.
    std::vector<GfVec3f> objVerts;
	for (int h = 0; h <= numRows; h++) {
		for (int t = 0; t < numColumns; t++) {
			double rad = t * PI * 2 / numColumns;
			objVerts.emplace_back(
				(float) r * sin(rad), 
				(float) height * h / numRows, 
				(float) r * cos(rad));
		}
	}
	
			

    std::cout << "objVerts.size() = " << objVerts.size() << std::endl;

	if (objVerts.empty()) {
		throw std::logic_error("Empty array of input vertices");
	}
      

    // Copy objVerts into VtVec3fArray for Usd.
    VtVec3fArray usdPoints;
    usdPoints.assign(objVerts.begin(), objVerts.end());

	std::vector<std::array<int, 4>> meshElements;
	for (int h = 0; h < numRows; h++) {
		for (int t = 0; t < numColumns; t++) {
			int indOne = numRows * h + t%numColumns;
			int indTwo = numRows * h + (t + 1) % numColumns;
			int indThr = numRows * (h + 1) + (t + 1) % numColumns;
			int indFou = numRows * (h + 1) + t % numColumns;
			meshElements.emplace_back(std::array<int, 4>{indOne, indTwo, indThr, indFou});
		}
	}

   
    // Usd currently requires an extent, somewhat unfortunately.
    const int nFrames = 100;
    GfRange3f extent;
    for (const auto& pt : usdPoints) {
        extent.UnionWith(pt);
    }
    VtVec3fArray extentArray(2);
    extentArray[0] = extent.GetMin();
    extentArray[1] = extent.GetMax();

    // Create a mesh for this surface
    UsdGeomMesh mesh = UsdGeomMesh::Define(stage, SdfPath("/TriangulatedSurface0"));

    // Set up the timecode
    stage->SetStartTimeCode(0.);
    stage->SetEndTimeCode((double) nFrames);    

    // Populate the mesh vertex data
    UsdAttribute pointsAttribute = mesh.GetPointsAttr();
    pointsAttribute.SetVariability(SdfVariabilityVarying);

    std::vector<double> timeSamples;
    timeSamples.push_back(0.);
	for (int frame = 1; frame <= nFrames; frame++) {
		timeSamples.push_back((double)frame);
	}
    for(const auto sample: timeSamples){
		for (int j = 0; j < numColumns; j++) {
			int index = numRows * numColumns + j;
			auto& vertex = objVerts.at(index);
			//vertex.data()[0] += 0.05;
			//vertex.data()[1] += 0.05;
		}
		smooth(&objVerts, 100, numRows, numColumns);
		pointsAttribute.Set(usdPoints, sample);
		
        usdPoints.assign(objVerts.begin(), objVerts.end());
    }

    VtIntArray faceVertexCounts, faceVertexIndices;
    for (const auto& element : meshElements) {
        faceVertexCounts.push_back(element.size());
		for (const auto& vertex : element) {
			faceVertexIndices.push_back(vertex);
		}
    }

    // Now set the attributes.
    mesh.GetFaceVertexCountsAttr().Set(faceVertexCounts);
    mesh.GetFaceVertexIndicesAttr().Set(faceVertexIndices);

    // Set extent.
    mesh.GetExtentAttr().Set(extentArray);

    stage->GetRootLayer()->Save();

    std::cout << "USD file saved!" << std::endl;

    return 0;
}
void smooth(std::vector<GfVec3f> * objVerts, int reps, int rows, int cols) {
	std::vector<GfVec3f> clone;
	for (auto& vertex : *objVerts) {
		clone.emplace_back(vertex);
	}
	for (int n = 0; n < reps; n++) {
		for (int i = 1; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int index = i * cols + j;
				int up = (i + 1) * cols + j;
				int dn = (i - 1) * cols + j;
				int lf = i * cols + (j - 1 + cols) % cols;
				int rt = i * cols + (j + 1) % cols;
				auto& vertex = clone.at(index);
				auto& uV = clone.at(up);
				auto& dV = clone.at(dn);
				auto& lV = clone.at(lf);
				auto& rV = clone.at(rt);
				(*objVerts).at(index) = (uV + dV + lV + rV) / 4;
			}
		}
	}
}