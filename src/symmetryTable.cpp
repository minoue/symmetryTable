#include <algorithm>
#include <cstddef>
#include <deque>
#include <system_error>
#include <tuple>
#include <chrono>

#include "symmetryTable.hpp"

#include <maya/MStatus.h>
#include <maya/MApiNamespace.h>
#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

static const char* vFlag = "-v";
static const char* vFlagLong = "-vertex";
static const char* eFlag = "-e";
static const char* eFlagLong = "-edge";
static const char* fFlag = "-f";
static const char* fFlagLong = "-face";
static const char* verboseFlag = "-vb";
static const char* verboseFlagLong = "-verbose";
static const char* halfFlag = "-hf";
static const char* halfFlagLong = "-half";

void timeIt(
    std::chrono::system_clock::time_point begin,
    std::chrono::system_clock::time_point end,
    std::string message)
{
    auto dur = end - begin;
    auto elapsedTime = std::chrono::duration<double>(dur).count();
    std::cout << message << elapsedTime << " seconds." << std::endl;
}

bool isCCW(std::vector<int> faceVertices, int edgeVertices[2])
{
    size_t numItems = faceVertices.size();
    size_t counter = 0;
    for (size_t i = 0; i < numItems; i++) {
        if (counter == numItems - 1) {
            // last
            if (edgeVertices[0] == faceVertices[i] && edgeVertices[1] == faceVertices[0]) {
                return true;
            }
        } else {
            if (edgeVertices[0] == faceVertices[i] && edgeVertices[1] == faceVertices[counter + 1]) {
                return true;
            }
        }
        counter += 1;
    }
    return false;
}

MSyntax SymmetryTable::newSyntax()
{
    MSyntax syntax;
    syntax.addFlag(vFlag, vFlagLong, MSyntax::kBoolean);
    syntax.addFlag(eFlag, eFlagLong, MSyntax::kBoolean);
    syntax.addFlag(fFlag, fFlagLong, MSyntax::kBoolean);
    syntax.addFlag(verboseFlag, verboseFlagLong, MSyntax::kBoolean);
    syntax.addFlag(halfFlag, halfFlagLong, MSyntax::kBoolean);
    return syntax;
}

SymmetryTable::SymmetryTable()
{
}

SymmetryTable::~SymmetryTable()
{
}

MStatus SymmetryTable::doIt(const MArgList& args)
{
    MStatus status;
    MArgDatabase argData(newSyntax(), args, &status);

    if (argData.isFlagSet(vFlag)) {
        status = argData.getFlagArgument(vFlag, 0, outVertex);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        outVertex = true;
    }
    
    if (argData.isFlagSet(eFlag)) {
        status = argData.getFlagArgument(eFlag, 0, outEdge);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        outEdge = false;
    }
    
    if (argData.isFlagSet(fFlag)) {
        status = argData.getFlagArgument(fFlag, 0, outFace);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        outFace = false;
    }

    if (argData.isFlagSet(verboseFlag)) {
        status = argData.getFlagArgument(verboseFlag, 0, verbose);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        verbose = false;
    }
    
    if (argData.isFlagSet(halfFlag)) {
        status = argData.getFlagArgument(halfFlag, 0, halfOnly);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        halfOnly = false;
    }

    return redoIt();
}


MStatus SymmetryTable::redoIt()
{
    MStatus status;

    MSelectionList sel;
    MGlobal::getActiveSelectionList(sel);

    MDagPath dagPath;
    MObject components;

    // Get first edge index
    sel.getDagPath(0, dagPath, components);
    MFnSingleIndexedComponent fnComp(components);
    if (fnComp.componentType() != MFn::kMeshEdgeComponent) {
        MGlobal::displayError("Selected component is not EdgeComponent. Select a middle edge for symmetry");
        return MS::kFailure;
    }
    MIntArray ids;
    fnComp.getElements(ids);

    int selectedIndex = ids[0];

    initMesh(dagPath);

    int faceAIndex = edges[static_cast<size_t>(selectedIndex)].faces[0];
    int faceBIndex = edges[static_cast<size_t>(selectedIndex)].faces[1];

    // Calc time for queue process
    auto start = std::chrono::system_clock::now();
    
    std::deque<Task> dq;

    Task initialTask = { faceAIndex, faceBIndex, selectedIndex, selectedIndex };

    dq.push_front(initialTask);

    for (Task t = dq.front(); !dq.empty(); dq.pop_front(), t = dq.front()) {

        int leftFaceIndex = t.faceAIndex;
        int rightFaceIndex = t.faceBIndex;
        int leftStartEdgeIndex = t.edgeAIndex;
        int rightStartEdgeIndex = t.edgeBIndex;

        Face& leftFace = faces[static_cast<size_t>(leftFaceIndex)];
        Face& rightFace = faces[static_cast<size_t>(rightFaceIndex)];
        Edge& leftStartingEdge = edges[static_cast<size_t>(leftStartEdgeIndex)];
        Edge& rightStartingEdge = edges[static_cast<size_t>(rightStartEdgeIndex)];

        if (leftFace.isDone == true)
            continue;

        if (rightFace.isDone == true)
            continue;

        leftFace.isDone = true;
        rightFace.isDone = true;
        
        // Set pair of faces
        leftFace.opposite = rightFaceIndex;
        rightFace.opposite = leftFaceIndex;

        // Search pairs of vertices
        int leftStartingVert = leftStartingEdge.vertices[0];
        int leftStartingVertIndex = findIndex(leftFace.vertices, leftStartingVert);
        if (leftStartingVertIndex == -1) {
            MGlobal::displayError("Mesh may not be symmetrical.");
            return MS::kFailure;
        }
        std::rotate(leftFace.vertices.begin(), leftFace.vertices.begin() + leftStartingVertIndex, leftFace.vertices.end());

        int rightStartingVert = rightStartingEdge.vertices[0];
        int rightStartingVertIndex = findIndex(rightFace.vertices, rightStartingVert);
        if (rightStartingVertIndex == -1) {
            MGlobal::displayError("Mesh may not be symmetrical.");
            return MS::kFailure;
        }
        std::rotate(rightFace.vertices.begin(), rightFace.vertices.begin() + rightStartingVertIndex, rightFace.vertices.end());

        bool isLeftEdgeCCW = isCCW(leftFace.vertices, leftStartingEdge.vertices);
        bool isRightEdgeCCW = isCCW(rightFace.vertices, rightStartingEdge.vertices);

        std::tuple<bool, bool> cond(isLeftEdgeCCW, isRightEdgeCCW);
        std::tuple<bool, bool> allTrue(true, true);
        std::tuple<bool, bool> allFalse(false, false);

        if (cond == allTrue || cond == allFalse) {
            // If both edges are facing same direction (ccw or cw) they need to be mirrored, not reversed
            int rightStartingVert2 = rightStartingEdge.vertices[1];
            int rightStartingVertIndex2 = findIndex(rightFace.vertices, rightStartingVert2);
            if (rightStartingVertIndex2 == -1) {
                MGlobal::displayError("Mesh may not be symmetrical.");
                return MS::kFailure;
            }
            std::rotate(rightFace.vertices.begin(), rightFace.vertices.begin() + rightStartingVertIndex2, rightFace.vertices.end());
            std::reverse(rightFace.vertices.begin(), rightFace.vertices.end());
            std::rotate(rightFace.vertices.begin(), rightFace.vertices.end() - 1, rightFace.vertices.end());
        } else {
            std::reverse(rightFace.vertices.begin(), rightFace.vertices.end());
            std::rotate(rightFace.vertices.begin(), rightFace.vertices.end()-1, rightFace.vertices.end());
        }

        size_t numFaceVertices = leftFace.vertices.size();
        for (size_t i = 0; i < numFaceVertices; i++) {
            int leftId = leftFace.vertices[i];
            int rightId = rightFace.vertices[i];

            // store corresponding vertices
            vertices[static_cast<size_t>(rightId)] = leftId;
            if ( halfOnly ) {
                // If half flag is on, keep values one side -1, otherwise store corresponding vertices
                vertices[static_cast<size_t>(leftId)] = -1;
            } else {
                vertices[static_cast<size_t>(leftId)] = rightId;
            }
        }
        // Search pairs of edges
        int leftStartingEdgeLocalIndex = findIndex(leftFace.edges, leftStartEdgeIndex);
        if (leftStartingEdgeLocalIndex == -1) {
            MGlobal::displayError("Mesh may not be symmetrical.");
            return MS::kFailure;
        }
        std::rotate(leftFace.edges.begin(), leftFace.edges.begin() + leftStartingEdgeLocalIndex, leftFace.edges.end());

        int rightStartingEdgeLocalIndex = findIndex(rightFace.edges, rightStartEdgeIndex);
        if (rightStartingEdgeLocalIndex == -1) {
            MGlobal::displayError("Mesh may not be symmetrical.");
            return MS::kFailure;
        }
        std::rotate(rightFace.edges.begin(), rightFace.edges.begin() + rightStartingEdgeLocalIndex, rightFace.edges.end());
        std::reverse(rightFace.edges.begin(), rightFace.edges.end());
        std::rotate(rightFace.edges.begin(), rightFace.edges.end()-1, rightFace.edges.end());

        size_t numFaceEdges = leftFace.edges.size();
        for (size_t i = 0; i < numFaceEdges; i++) {
            int leftId = leftFace.edges[i];
            int rightId = rightFace.edges[i];

            Edge& le = edges[static_cast<size_t>(leftId)];
            Edge& re = edges[static_cast<size_t>(rightId)];

            if (le.opposite != -1) {
                continue;
            }
            if (re.opposite != -1) {
                continue;
            }

            le.opposite = rightId;
            re.opposite = leftId;

            if (leftId == rightId)
                continue;

            int next_left_face;
            int next_right_face;

            if (le.onBoundary == true) {
                // only 1 face
                next_left_face = le.faces[0];
            } else {
                // two faces
                for (int j = 0; j < 2; j++) {
                    if (le.faces[j] != leftFaceIndex) {
                        next_left_face = le.faces[j];
                    }
                }
            }

            if (re.onBoundary == true) {
                next_right_face = re.faces[0];
            } else {
                for (int k = 0; k < 2; k++) {
                    if (re.faces[k] != rightFaceIndex) {
                        next_right_face = re.faces[k];
                    }
                }
            }

            Task newTask = { next_left_face, next_right_face, leftId, rightId };
            dq.push_back(newTask);
        }
    }

    // Set result return
    MIntArray result;
    
    if (outEdge == true) {
        for (Edge& e : edges) {
            result.append(e.opposite);
        }
    } else if (outFace == true) {
        for (Face& f : faces) {
            result.append(f.opposite);
        }
    } else if (outVertex == true) {
        for (int x : vertices) {
            result.append(x);
        }
    }
    
    auto end = std::chrono::system_clock::now();

    if (verbose)
        timeIt(start, end, "Topological symmetry calculated in : ");

    MPxCommand::setResult(result);
    return status;
}

MStatus SymmetryTable::initMesh(MDagPath& dagPath)
{
    auto start = std::chrono::system_clock::now();

    MFnMesh fnMesh(dagPath);
    size_t numVerts = static_cast<size_t>(fnMesh.numVertices());
    size_t numFaces = static_cast<size_t>(fnMesh.numPolygons());
    size_t numEdges = static_cast<size_t>(fnMesh.numEdges());

    vertices.resize(numVerts);
    edges.resize(numEdges);
    faces.resize(numFaces);

    // Fill all with -1. This is to determine if vertex is left side or right side
    std::fill(vertices.begin(), vertices.end(), -1);

    MIntArray ids;

    for (MItMeshPolygon itPoly(dagPath); !itPoly.isDone(); itPoly.next()) {
        unsigned int index = itPoly.index();

        // Insert contained edges to edge list of the face
        itPoly.getEdges(ids);
        unsigned int numIDs = ids.length();
        for (unsigned int i = 0; i < numIDs; i++) {
            faces[index].edges.push_back(ids[i]);
        }

        // for vertices
        itPoly.getVertices(ids);
        numIDs = ids.length();
        for (unsigned int i = 0; i < numIDs; i++) {
            faces[index].vertices.push_back(ids[i]);
        }
    }
    for (MItMeshEdge itEdge(dagPath); !itEdge.isDone(); itEdge.next()) {
        size_t index = static_cast<size_t>(itEdge.index());
        bool onBoundary = itEdge.onBoundary();
        edges[index].onBoundary = onBoundary;

        itEdge.getConnectedFaces(ids);
        unsigned int numIDs = ids.length();
        if (numIDs > 2) {
            MGlobal::displayError("Non manifold edges found");
            return MS::kFailure;
        }

        // if border edge, only 1 face available
        if (onBoundary == true) {
            edges[index].faces[0] = ids[0];
            edges[index].faces[1] = -1;
        } else {
            edges[index].faces[0] = ids[0];
            edges[index].faces[1] = ids[1];
        }

        // Get two vertices of the edge
        int v1 = itEdge.index(0);
        int v2 = itEdge.index(1);
        edges[index].vertices[0] = v1;
        edges[index].vertices[1] = v2;
    }

    auto end = std::chrono::system_clock::now();

    if (verbose)
        timeIt(start, end, "Mesh init time : ");

    return MS::kSuccess;
}

MStatus SymmetryTable::undoIt()
{
    return MS::kSuccess;
}

bool SymmetryTable::isUndoable() const
{
    return false;
}

void* SymmetryTable::creator()

{
    return new SymmetryTable;
}